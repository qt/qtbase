// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qthread.h"
#include "qthread_p.h"

#include <private/qcoreapplication_p.h>
#include <private/qcore_unix_p.h>
#include "qdebug.h"
#include "qloggingcategory.h"
#include "qthreadstorage.h"
#include <private/qtools_p.h>

#if defined(Q_OS_WASM)
#  include <private/qeventdispatcher_wasm_p.h>
#else
#  include <private/qeventdispatcher_unix_p.h>
#  if defined(Q_OS_DARWIN)
#    include <private/qeventdispatcher_cf_p.h>
#  elif !defined(QT_NO_GLIB)
#    include <private/qeventdispatcher_glib_p.h>
#  endif
#endif

#ifdef __GLIBCXX__
#include <cxxabi.h>
#endif

#include <sched.h>
#include <errno.h>
#if __has_include(<pthread_np.h>)
#  include <pthread_np.h>
#endif

#if defined(Q_OS_FREEBSD)
#  include <sys/cpuset.h>
#elif defined(Q_OS_BSD4)
#  include <sys/sysctl.h>
#endif
#ifdef Q_OS_VXWORKS
#  include <vxCpuLib.h>
#  include <cpuset.h>
#endif

#ifdef Q_OS_HPUX
#include <sys/pstat.h>
#endif

#if defined(Q_OS_LINUX) && !defined(QT_LINUXBASE)
#include <sys/prctl.h>
#endif

#if defined(Q_OS_LINUX) && !defined(SCHED_IDLE)
// from linux/sched.h
# define SCHED_IDLE    5
#endif

#if defined(Q_OS_DARWIN) || !defined(Q_OS_ANDROID) && !defined(Q_OS_OPENBSD) && defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && (_POSIX_THREAD_PRIORITY_SCHEDULING-0 >= 0)
#define QT_HAS_THREAD_PRIORITY_SCHEDULING
#endif

#if defined(Q_OS_QNX)
#include <sys/neutrino.h>
#endif

QT_BEGIN_NAMESPACE

[[maybe_unused]]
Q_STATIC_LOGGING_CATEGORY(lcQThread, "qt.core.thread", QtWarningMsg)

using namespace QtMiscUtils;

#if QT_CONFIG(thread)

static_assert(sizeof(pthread_t) <= sizeof(Qt::HANDLE));

enum { ThreadPriorityResetFlag = 0x80000000 };

// If we have a way to perform a timed pthread_join(), we will do it if its
// clock is not worse than the one QWaitCondition is using. This ensures that
// QThread::wait() only returns after pthread_join() or equivalent has
// returned, ensuring that the thread has definitely exited.
//
// Because only one thread can call this family of functions at a time, we
// count how many threads are waiting and all but one of them wait on a
// QWaitCondition, with the joining thread having the responsibility for waking
// up all others when the joining concludes. If the joining times out, the
// thread in charge wakes up one of the other waiters (if there's any) to
// assume responsibility for joining.
//
// If we don't have a way to perform timed pthread_join(), then we don't try
// joining a all. All waiting threads will wait for the launched thread to
// call QWaitCondition::wakeAll(). Note in this case it is possible for the
// waiting threads to conclude the launched thread has exited before it has.
//
// To support this scenario, we start the thread in detached state.
static constexpr bool UsingPThreadTimedJoin = QT_CONFIG(pthread_clockjoin)
        || (QT_CONFIG(pthread_timedjoin) && QWaitConditionClockId == CLOCK_REALTIME);
#if !QT_CONFIG(pthread_clockjoin)
int pthread_clockjoin_np(...) { return ENOSYS; }    // pretend
#endif
#if !QT_CONFIG(pthread_timedjoin)
int pthread_timedjoin_np(...) { return ENOSYS; }    // pretend
#endif

#if QT_CONFIG(broken_threadlocal_dtors)
// On most modern platforms, the C runtime has a helper function that helps the
// C++ runtime run the thread_local non-trivial destructors when threads exit
// and that code ensures that they are run in the correct order on program exit
// too ([basic.start.term]/2: "The destruction of all constructed objects with
// thread storage duration within that thread strongly happens before
// destroying any object with static storage duration."). In the absence of
// this function, the ordering can be wrong depending on when the first
// non-trivial thread_local object was created relative to other statics.
// Moreover, this can be racy and having our own thread_local early in
// QThreadPrivate::start() made it even more so. See QTBUG-129846 for analysis.
//
// There's a good correlation between this C++11 feature and our ability to
// call QThreadPrivate::cleanup() from destroy_thread_data().
//
// https://gcc.gnu.org/git/?p=gcc.git;a=blob;f=libstdc%2B%2B-v3/libsupc%2B%2B/atexit_thread.cc;hb=releases/gcc-14.2.0#l133
// https://github.com/llvm/llvm-project/blob/llvmorg-19.1.0/libcxxabi/src/cxa_thread_atexit.cpp#L118-L120
#endif
//
// Thus, the destruction of QThreadData is split into 3 steps:
// - finish()
// - cleanup()
// - deref & delete
//
// The reason for the first split is that user content may run as a result of
// the finished() signal, in thread_local destructors or similar, so we don't
// want to destroy the event dispatcher too soon. If we did, the event
// dispatcher could get recreated.
//
// For auxiliary threads started with QThread, finish() is run as soon as run()
// returns, while cleanup() and the deref happen at pthread_set_specific
// destruction time (except for broken_threadlocal_dtors, see above).
//
// For auxiliary threads started with something else and adopted as a
// QAdoptedThread, there's only one choice: all three steps happen at at
// pthread_set_specific destruction time.
//
// Finally, for the thread that called ::exit() (which in most cases happens by
// returning from the main() function), finish() and cleanup() happen at
// function-local static destructor time, and the deref & delete happens later,
// at global static destruction time. That way, we delete the event dispatcher
// before QLibraryStore's clean up runs and unloads remaining plugins. This
// strategy was chosen because of crashes observed while running the event
// dispatcher's destructor, and though the cause of the crash was something
// else (QFactoryLoader always loads with PreventUnloadHint set), other plugins
// may still attempt to access QThreadData in their global destructors.

Q_CONSTINIT static thread_local QThreadData *currentThreadData = nullptr;

static void destroy_current_thread_data(QThreadData *data)
{
    QThread *thread = data->thread.loadAcquire();

#ifdef Q_OS_APPLE
    // apparent runtime bug: the trivial has been cleared and we end up
    // recreating the QThreadData
    currentThreadData = data;
#endif

    if (data->isAdopted) {
        // If this is an adopted thread, then QThreadData owns the QThread and
        // this is very likely the last reference. These pointers cannot be
        // null and there is no race.
        QThreadPrivate *thread_p = static_cast<QThreadPrivate *>(QObjectPrivate::get(thread));
        thread_p->finish();
        if constexpr (!QT_CONFIG(broken_threadlocal_dtors))
            thread_p->cleanup();
    } else if constexpr (!QT_CONFIG(broken_threadlocal_dtors)) {
        // We may be racing the QThread destructor in another thread. With
        // two-phase clean-up enabled, there's also no race because it will
        // stop in a call to QThread::wait() until we call cleanup().
        QThreadPrivate *thread_p = static_cast<QThreadPrivate *>(QObjectPrivate::get(thread));
        thread_p->cleanup();
    } else {
        // We may be racing the QThread destructor in another thread and it may
        // have begun destruction; we must not dereference the QThread pointer.
    }
}

static void deref_current_thread_data(QThreadData *data)
{
    // the QThread object may still have a reference, so this may not delete
    data->deref();

    // ... but we must reset it to zero before returning so we aren't
    // leaving a dangling pointer.
    currentThreadData = nullptr;
}

static void destroy_auxiliary_thread_data(void *p)
{
    auto data = static_cast<QThreadData *>(p);
    destroy_current_thread_data(data);
    deref_current_thread_data(data);
}

// Utility functions for getting, setting and clearing thread specific data.
static QThreadData *get_thread_data()
{
    return currentThreadData;
}

namespace {
struct QThreadDataDestroyer
{
    pthread_key_t key;
    QThreadDataDestroyer() noexcept
    {
        pthread_key_create(&key, &destroy_auxiliary_thread_data);
    }
    ~QThreadDataDestroyer()
    {
        // running global static destructors upon ::exit()
        if (QThreadData *data = get_thread_data())
            deref_current_thread_data(data);
        pthread_key_delete(key);
    }

    struct EarlyMainThread {
        EarlyMainThread() { QThreadStoragePrivate::init(); }
        ~EarlyMainThread()
        {
            // running function-local destructors upon ::exit()
            if (QThreadData *data = get_thread_data())
                destroy_current_thread_data(data);
        }
    };
};
}
#if QT_SUPPORTS_INIT_PRIORITY
Q_DECL_INIT_PRIORITY(10)
#endif
static QThreadDataDestroyer threadDataDestroyer; // intentional non-trivial init & destruction

static void set_thread_data(QThreadData *data) noexcept
{
    if (data) {
        // As noted above: one global static for the thread that called
        // ::exit() (which may not be a Qt thread) and the pthread_key_t for
        // all others.
        static QThreadDataDestroyer::EarlyMainThread currentThreadCleanup;
        pthread_setspecific(threadDataDestroyer.key, data);
    }
    currentThreadData = data;
}

template <typename T>
static typename std::enable_if<std::is_integral_v<T>, Qt::HANDLE>::type to_HANDLE(T id)
{
    return reinterpret_cast<Qt::HANDLE>(static_cast<intptr_t>(id));
}

template <typename T>
static typename std::enable_if<std::is_integral_v<T>, T>::type from_HANDLE(Qt::HANDLE id)
{
    return static_cast<T>(reinterpret_cast<intptr_t>(id));
}

template <typename T>
static typename std::enable_if<std::is_pointer_v<T>, Qt::HANDLE>::type to_HANDLE(T id)
{
    return id;
}

template <typename T>
static typename std::enable_if<std::is_pointer_v<T>, T>::type from_HANDLE(Qt::HANDLE id)
{
    return static_cast<T>(id);
}

void QThreadData::clearCurrentThreadData()
{
    set_thread_data(nullptr);
}

QThreadData *QThreadData::currentThreadData() noexcept
{
    return get_thread_data();
}

QThreadData *QThreadData::createCurrentThreadData()
{
    Q_ASSERT(!currentThreadData());
    std::unique_ptr data = std::make_unique<QThreadData>();

    // This needs to be called prior to new QAdoptedThread() to avoid
    // recursion (see qobject.cpp).
    set_thread_data(data.get());

    QT_TRY {
        data->thread.storeRelease(new QAdoptedThread(data.get()));
    } QT_CATCH(...) {
        clearCurrentThreadData();
        QT_RETHROW;
    }
    return data.release();
}

void QAdoptedThread::init()
{
}

/*
   QThreadPrivate
*/

extern "C" {
typedef void *(*QtThreadCallback)(void *);
}

#endif // QT_CONFIG(thread)

QAbstractEventDispatcher *QThreadPrivate::createEventDispatcher(QThreadData *data)
{
    Q_UNUSED(data);
#if defined(Q_OS_DARWIN)
    bool ok = false;
    int value = qEnvironmentVariableIntValue("QT_EVENT_DISPATCHER_CORE_FOUNDATION", &ok);
    if (ok && value > 0)
        return new QEventDispatcherCoreFoundation;
    else
        return new QEventDispatcherUNIX;
#elif defined(Q_OS_WASM)
    return new QEventDispatcherWasm();
#elif !defined(QT_NO_GLIB)
    const bool isQtMainThread = data->thread.loadAcquire() == QCoreApplicationPrivate::mainThread();
    if (qEnvironmentVariableIsEmpty("QT_NO_GLIB")
        && (isQtMainThread || qEnvironmentVariableIsEmpty("QT_NO_THREADED_GLIB"))
        && QEventDispatcherGlib::versionSupported())
        return new QEventDispatcherGlib;
    else
        return new QEventDispatcherUNIX;
#else
    return new QEventDispatcherUNIX;
#endif
}

#if QT_CONFIG(thread)

#if (defined(Q_OS_LINUX) || defined(Q_OS_DARWIN) || defined(Q_OS_QNX))
static void setCurrentThreadName(const char *name)
{
#  if defined(Q_OS_LINUX) && !defined(QT_LINUXBASE)
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
#  elif defined(Q_OS_DARWIN)
    pthread_setname_np(name);
#  elif defined(Q_OS_QNX)
    pthread_setname_np(pthread_self(), name);
#  endif
}
#endif

namespace {
template <typename T>
void terminate_on_exception(T &&t)
{
#ifndef QT_NO_EXCEPTIONS
    try {
#endif
        std::forward<T>(t)();
#ifndef QT_NO_EXCEPTIONS
#ifdef __GLIBCXX__
    // POSIX thread cancellation under glibc is implemented by throwing an exception
    // of this type. Do what libstdc++ is doing and handle it specially in order not to
    // abort the application if user's code calls a cancellation function.
    } catch (abi::__forced_unwind &) {
        throw;
#endif // __GLIBCXX__
    } catch (...) {
        std::terminate();
    }
#endif // QT_NO_EXCEPTIONS
}
} // unnamed namespace

void *QThreadPrivate::start(void *arg)
{
#ifdef PTHREAD_CANCEL_DISABLE
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
#endif
    QThread *thr = reinterpret_cast<QThread *>(arg);
    QThreadData *data = QThreadData::get2(thr);
    // If a QThread is restarted, reuse the QBindingStatus, too
    data->reuseBindingStatusForNewNativeThread();

    // this ensures the thread-local is created as early as possible
    set_thread_data(data);

    pthread_cleanup_push([](void *arg) { static_cast<QThread *>(arg)->d_func()->finish(); }, arg);
    terminate_on_exception([&] {
        {
            QMutexLocker locker(&thr->d_func()->mutex);

            // do we need to reset the thread priority?
            if (thr->d_func()->priority & ThreadPriorityResetFlag) {
                thr->d_func()->setPriority(QThread::Priority(thr->d_func()->priority & ~ThreadPriorityResetFlag));
            }
#ifndef Q_OS_DARWIN // For Darwin we set it as an attribute when starting the thread
            if (thr->d_func()->serviceLevel != QThread::QualityOfService::Auto)
                thr->d_func()->setQualityOfServiceLevel(thr->d_func()->serviceLevel);
#endif

            // threadId is set in QThread::start()
            Q_ASSERT(data->threadId.loadRelaxed() == QThread::currentThreadId());

            data->ref();
            data->quitNow = thr->d_func()->exited;
        }

        data->ensureEventDispatcher();
        data->eventDispatcher.loadRelaxed()->startingUp();

#if (defined(Q_OS_LINUX) || defined(Q_OS_DARWIN) || defined(Q_OS_QNX))
        {
            // Sets the name of the current thread. We can only do this
            // when the thread is starting, as we don't have a cross
            // platform way of setting the name of an arbitrary thread.
            if (Q_LIKELY(thr->d_func()->objectName.isEmpty()))
                setCurrentThreadName(thr->metaObject()->className());
            else
                setCurrentThreadName(std::exchange(thr->d_func()->objectName, {}).toLocal8Bit());
        }
#endif

        emit thr->started(QThread::QPrivateSignal());
#ifdef PTHREAD_CANCEL_DISABLE
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
        pthread_testcancel();
#endif
        thr->run();
    });

    // This calls finish(); later, the currentThreadCleanup thread-local
    // destructor will call cleanup().
    pthread_cleanup_pop(1);
    return nullptr;
}

void QThreadPrivate::finish()
{
    terminate_on_exception([&] {
        QThreadPrivate *d = this;
        QThread *thr = q_func();

        // Disable cancellation; we're already in the finishing touches of this
        // thread, and we don't want cleanup to be disturbed by
        // abi::__forced_unwind being thrown from all kinds of functions.
#ifdef PTHREAD_CANCEL_DISABLE
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
#endif

        QMutexLocker locker(&d->mutex);

        d->threadState = QThreadPrivate::Finishing;
        locker.unlock();
        emit thr->finished(QThread::QPrivateSignal());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QThreadStoragePrivate::finish(&d->data->tls);
    });

    if constexpr (QT_CONFIG(broken_threadlocal_dtors))
        cleanup();
}

void QThreadPrivate::cleanup()
{
    terminate_on_exception([&] {
        QThreadPrivate *d = this;

        // Disable cancellation again: we did it above, but some user code
        // running between finish() and cleanup() may have turned them back on.
#ifdef PTHREAD_CANCEL_DISABLE
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
#endif

        QMutexLocker locker(&d->mutex);
        d->priority = QThread::InheritPriority;

        QAbstractEventDispatcher *eventDispatcher = d->data->eventDispatcher.loadRelaxed();
        if (eventDispatcher) {
            d->data->eventDispatcher = nullptr;
            locker.unlock();
            eventDispatcher->closingDown();
            delete eventDispatcher;
            locker.relock();
        }

        d->interruptionRequested.store(false, std::memory_order_relaxed);

        d->wakeAll();
    });
}


/**************************************************************************
 ** QThread
 *************************************************************************/

/*
    CI tests fails on ARM architectures if we try to use the assembler, so
    stick to the pthread version there. The assembler would be

    // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0344k/Babeihid.html
    asm volatile ("mrc p15, 0, %0, c13, c0, 3" : "=r" (tid));

    and

    // see glibc/sysdeps/aarch64/nptl/tls.h
    asm volatile ("mrs %0, tpidr_el0" : "=r" (tid));

    for 32 and 64bit versions, respectively.
*/
Qt::HANDLE QThread::currentThreadIdImpl() noexcept
{
    return to_HANDLE(pthread_self());
}

#if defined(QT_LINUXBASE) && !defined(_SC_NPROCESSORS_ONLN)
// LSB doesn't define _SC_NPROCESSORS_ONLN.
#  define _SC_NPROCESSORS_ONLN 84
#endif

#ifdef Q_OS_WASM
int QThreadPrivate::idealThreadCount = 1;
#endif

#if QT_CONFIG(trivial_auto_var_init_pattern) && defined(Q_CC_GNU_ONLY)
// Don't pre-fill the automatic-storage arrays used in this function
// (important for the FreeBSD & Linux code using a VLA).
__attribute__((optimize("trivial-auto-var-init=uninitialized")))
#endif
int QThread::idealThreadCount() noexcept
{
    int cores = 1;

#if defined(Q_OS_HPUX)
    // HP-UX
    struct pst_dynamic psd;
    if (pstat_getdynamic(&psd, sizeof(psd), 1, 0) == -1) {
        perror("pstat_getdynamic");
    } else {
        cores = (int)psd.psd_proc_cnt;
    }
#elif (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_FREEBSD)
    QT_WARNING_PUSH
#  if defined(Q_CC_CLANG) && Q_CC_CLANG >= 1800
    QT_WARNING_DISABLE_CLANG("-Wvla-cxx-extension")
#  endif

    // get the number of threads we're assigned, not the total in the system
    constexpr qsizetype MaxCpuCount = 1024 * 1024;
    constexpr qsizetype MaxCpuSetArraySize = MaxCpuCount / sizeof(cpu_set_t) / 8;
    qsizetype size = 1;
    do {
        cpu_set_t cpuset[size];
        if (sched_getaffinity(0, sizeof(cpu_set_t) * size, cpuset) == 0) {
            cores = CPU_COUNT_S(sizeof(cpu_set_t) * size, cpuset);
            break;
        }
        size *= 4;
    } while (size < MaxCpuSetArraySize);
    QT_WARNING_POP
#elif defined(Q_OS_BSD4)
    // OpenBSD, NetBSD, BSD/OS, Darwin (macOS, iOS, etc.)
    size_t len = sizeof(cores);
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    if (sysctl(mib, 2, &cores, &len, NULL, 0) != 0) {
        perror("sysctl");
    }
#elif defined(Q_OS_INTEGRITY)
#if (__INTEGRITY_MAJOR_VERSION >= 10)
    // Integrity V10+ does support multicore CPUs
    Value processorCount;
    if (GetProcessorCount(CurrentTask(), &processorCount) == 0)
        cores = processorCount;
    else
#endif
    // as of aug 2008 Integrity only supports one single core CPU
    cores = 1;
#elif defined(Q_OS_VXWORKS)
    cpuset_t cpus = vxCpuEnabledGet();
    cores = 0;

    // 128 cores should be enough for everyone ;)
    for (int i = 0; i < 128 && !CPUSET_ISZERO(cpus); ++i) {
        if (CPUSET_ISSET(cpus, i)) {
            CPUSET_CLR(cpus, i);
            cores++;
        }
    }
#elif defined(Q_OS_WASM)
    cores = QThreadPrivate::idealThreadCount;
#else
    // the rest: Solaris, AIX, Tru64
    cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (cores == -1)
        return 1;
#endif
    return cores;
}

void QThread::yieldCurrentThread()
{
    sched_yield();
}

#endif // QT_CONFIG(thread)

static void qt_nanosleep(timespec amount)
{
    // We'd like to use clock_nanosleep.
    //
    // But clock_nanosleep is from POSIX.1-2001 and both are *not*
    // affected by clock changes when using relative sleeps, even for
    // CLOCK_REALTIME.
    //
    // nanosleep is POSIX.1-1993

    int r;
    QT_EINTR_LOOP(r, nanosleep(&amount, &amount));
}

void QThread::sleep(unsigned long secs)
{
    sleep(std::chrono::seconds{secs});
}

void QThread::msleep(unsigned long msecs)
{
    sleep(std::chrono::milliseconds{msecs});
}

void QThread::usleep(unsigned long usecs)
{
    sleep(std::chrono::microseconds{usecs});
}

void QThread::sleep(std::chrono::nanoseconds nsec)
{
    qt_nanosleep(durationToTimespec(nsec));
}

#if QT_CONFIG(thread)

#ifdef QT_HAS_THREAD_PRIORITY_SCHEDULING
#if defined(Q_OS_QNX)
static bool calculateUnixPriority(int priority, int *sched_policy, int *sched_priority)
{
    // On QNX, NormalPriority is mapped to 10.  A QNX system could use a value different
    // than 10 for the "normal" priority but it's difficult to achieve this so we'll
    // assume that no one has ever created such a system.  This makes the mapping from
    // Qt priorities to QNX priorities lopsided.   There's usually more space available
    // to map into above the "normal" priority than below it.  QNX also has a privileged
    // priority range (for threads that assist the kernel).  We'll assume that no Qt
    // thread needs to use priorities in that range.
    int priority_norm = 10;
    // _sched_info::priority_priv isn't documented.  You'd think that it's the start of the
    // privileged priority range but it's actually the end of the unpriviledged range.
    struct _sched_info info;
    if (SchedInfo_r(0, *sched_policy, &info) != EOK)
        return false;

    if (priority == QThread::IdlePriority) {
        *sched_priority = info.priority_min;
        return true;
    }

    if (priority_norm < info.priority_min)
        priority_norm = info.priority_min;
    if (priority_norm > info.priority_priv)
        priority_norm = info.priority_priv;

    int to_min, to_max;
    int from_min, from_max;
    int prio;
    if (priority < QThread::NormalPriority) {
        to_min = info.priority_min;
        to_max = priority_norm;
        from_min = QThread::LowestPriority;
        from_max = QThread::NormalPriority;
    } else {
        to_min = priority_norm;
        to_max = info.priority_priv;
        from_min = QThread::NormalPriority;
        from_max = QThread::TimeCriticalPriority;
    }

    prio = ((priority - from_min) * (to_max - to_min)) / (from_max - from_min) + to_min;
    prio = qBound(to_min, prio, to_max);

    *sched_priority = prio;
    return true;
}
#else
// Does some magic and calculate the Unix scheduler priorities
// sched_policy is IN/OUT: it must be set to a valid policy before calling this function
// sched_priority is OUT only
static bool calculateUnixPriority(int priority, int *sched_policy, int *sched_priority)
{
#ifdef SCHED_IDLE
    if (priority == QThread::IdlePriority) {
        *sched_policy = SCHED_IDLE;
        *sched_priority = 0;
        return true;
    }
    const int lowestPriority = QThread::LowestPriority;
#else
    const int lowestPriority = QThread::IdlePriority;
#endif
    const int highestPriority = QThread::TimeCriticalPriority;

    int prio_min;
    int prio_max;
#if defined(Q_OS_VXWORKS)
    // for other scheduling policies than SCHED_RR or SCHED_FIFO
    prio_min = SCHED_FIFO_LOW_PRI;
    prio_max = SCHED_FIFO_HIGH_PRI;

    if ((*sched_policy == SCHED_RR) || (*sched_policy == SCHED_FIFO))
#endif
    {
    prio_min = sched_get_priority_min(*sched_policy);
    prio_max = sched_get_priority_max(*sched_policy);
    }

    if (prio_min == -1 || prio_max == -1)
        return false;

    int prio;
    // crudely scale our priority enum values to the prio_min/prio_max
    prio = ((priority - lowestPriority) * (prio_max - prio_min) / highestPriority) + prio_min;
    prio = qMax(prio_min, qMin(prio_max, prio));

    *sched_priority = prio;
    return true;
}
#endif
#endif

void QThread::start(Priority priority)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->threadState == QThreadPrivate::Finishing)
        d->wait(locker, QDeadlineTimer::Forever);

    if (d->threadState == QThreadPrivate::Running)
        return;

    d->threadState = QThreadPrivate::Running;
    d->returnCode = 0;
    d->exited = false;
    d->interruptionRequested.store(false, std::memory_order_relaxed);
    d->terminated = false;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if constexpr (!UsingPThreadTimedJoin)
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef Q_OS_DARWIN
    if (d->serviceLevel != QThread::QualityOfService::Auto)
        pthread_attr_set_qos_class_np(&attr, d->nativeQualityOfServiceClass(), 0);
#endif

    d->priority = priority;

#if defined(QT_HAS_THREAD_PRIORITY_SCHEDULING)
    switch (priority) {
    case InheritPriority:
        {
            pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
            break;
        }

    default:
        {
            int sched_policy;
            if (pthread_attr_getschedpolicy(&attr, &sched_policy) != 0) {
                // failed to get the scheduling policy, don't bother
                // setting the priority
                qWarning("QThread::start: Cannot determine default scheduler policy");
                break;
            }

            int prio;
            if (!calculateUnixPriority(priority, &sched_policy, &prio)) {
                // failed to get the scheduling parameters, don't
                // bother setting the priority
                qWarning("QThread::start: Cannot determine scheduler priority range");
                break;
            }

            sched_param sp;
            sp.sched_priority = prio;

            if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0
                || pthread_attr_setschedpolicy(&attr, sched_policy) != 0
                || pthread_attr_setschedparam(&attr, &sp) != 0) {
                // could not set scheduling hints, fallback to inheriting them
                // we'll try again from inside the thread
                pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
                d->priority = qToUnderlying(priority) | ThreadPriorityResetFlag;
            }
            break;
        }
    }
#endif // QT_HAS_THREAD_PRIORITY_SCHEDULING


    if (d->stackSize > 0) {
#if defined(_POSIX_THREAD_ATTR_STACKSIZE) && (_POSIX_THREAD_ATTR_STACKSIZE-0 > 0)
        int code = pthread_attr_setstacksize(&attr, d->stackSize);
#else
        int code = ENOSYS; // stack size not supported, automatically fail
#endif // _POSIX_THREAD_ATTR_STACKSIZE

        if (code) {
            qErrnoWarning(code, "QThread::start: Thread stack size error");

            // we failed to set the stacksize, and as the documentation states,
            // the thread will fail to run...
            d->threadState = QThreadPrivate::NotStarted;
            return;
        }
    }

#ifdef Q_OS_INTEGRITY
    if (Q_LIKELY(objectName().isEmpty()))
        pthread_attr_setthreadname(&attr, metaObject()->className());
    else
        pthread_attr_setthreadname(&attr, objectName().toLocal8Bit());
#else
    // avoid interacting with the binding system
    d->objectName = d->extraData ? d->extraData->objectName.valueBypassingBindings()
                                 : QString();
#endif

    pthread_t threadId;
    int code = pthread_create(&threadId, &attr, QThreadPrivate::start, this);
    if (code == EPERM) {
        // caller does not have permission to set the scheduling
        // parameters/policy
#if defined(QT_HAS_THREAD_PRIORITY_SCHEDULING)
        pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
#endif
        code = pthread_create(&threadId, &attr, QThreadPrivate::start, this);
    }
    d->data->threadId.storeRelaxed(to_HANDLE(threadId));

    pthread_attr_destroy(&attr);

    if (code) {
        qErrnoWarning(code, "QThread::start: Thread creation error");

        d->threadState = QThreadPrivate::NotStarted;
        d->data->threadId.storeRelaxed(nullptr);
    }
}

void QThread::terminate()
{
#if !defined(Q_OS_ANDROID)
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    const auto id = d->data->threadId.loadRelaxed();
    if (!id)
        return;

    if (d->terminated) // don't try again, avoids killing the wrong thread on threadId reuse (ABA)
        return;

    d->terminated = true;

    const bool selfCancelling = d->data == get_thread_data();
    if (selfCancelling) {
        // Posix doesn't seem to specify whether the stack of cancelled threads
        // is unwound, and there's nothing preventing a QThread from
        // terminate()ing itself, so drop the mutex before calling
        // pthread_cancel():
        locker.unlock();
    }

    if (int code = pthread_cancel(from_HANDLE<pthread_t>(id))) {
        if (selfCancelling)
            locker.relock();
        d->terminated = false; // allow to try again
        qErrnoWarning(code, "QThread::start: Thread termination error");
    }
#endif
}

static void wakeAllInternal(QThreadPrivate *d)
{
    d->threadState = QThreadPrivate::Finished;
    if (d->waiters)
        d->thread_done.wakeAll();
}

inline void QThreadPrivate::wakeAll()
{
    if (data->isAdopted || !UsingPThreadTimedJoin)
        wakeAllInternal(this);
}

bool QThreadPrivate::wait(QMutexLocker<QMutex> &locker, QDeadlineTimer deadline)
{
    constexpr int HasJoinerBit = int(0x8000'0000);  // a.k.a. sign bit
    struct timespec ts, *pts = nullptr;
    if (!deadline.isForever()) {
        ts = deadlineToAbstime(deadline);
        pts = &ts;
    }

    auto doJoin = [&] {
        // pthread_join() & family are cancellation points
        struct CancelState {
            QThreadPrivate *d;
            QMutexLocker<QMutex> *locker;
            int joinResult = ETIMEDOUT;
            static void run(void *arg) { static_cast<CancelState *>(arg)->run(); }
            void run()
            {
                locker->relock();
                if (joinResult == ETIMEDOUT && d->waiters)
                    d->thread_done.wakeOne();
                else if (joinResult == 0)
                    wakeAllInternal(d);
                d->waiters &= ~HasJoinerBit;
            }
        } nocancel = { this, &locker };
        int &r = nocancel.joinResult;

        // we're going to perform the join, so don't let other threads do it
        waiters |= HasJoinerBit;
        locker.unlock();

        pthread_cleanup_push(&CancelState::run, &nocancel);
        pthread_t thrId = from_HANDLE<pthread_t>(data->threadId.loadRelaxed());
        if constexpr (QT_CONFIG(pthread_clockjoin))
            r = pthread_clockjoin_np(thrId, nullptr, SteadyClockClockId, pts);
        else
            r = pthread_timedjoin_np(thrId, nullptr, pts);
        Q_ASSERT(r == 0 || r == ETIMEDOUT);
        pthread_cleanup_pop(1);

        Q_ASSERT(waiters >= 0);
        return r != ETIMEDOUT;
    };
    Q_ASSERT(threadState != QThreadPrivate::Finished);
    Q_ASSERT(locker.isLocked());

    bool result = false;

    // both branches call cancellation points
    ++waiters;
    bool mustJoin = (waiters & HasJoinerBit) == 0;
    pthread_cleanup_push([](void *ptr) {
        --(*static_cast<decltype(waiters) *>(ptr));
    }, &waiters);
    for (;;) {
        if (UsingPThreadTimedJoin && mustJoin && !data->isAdopted) {
            result = doJoin();
            break;
        }
        if (!thread_done.wait(locker.mutex(), deadline))
            break;      // timed out
        result = threadState == QThreadPrivate::Finished;
        if (result)
            break;      // success
        mustJoin = (waiters & HasJoinerBit) == 0;
    }
    pthread_cleanup_pop(1);

    return result;
}

void QThread::setTerminationEnabled(bool enabled)
{
    QThread *thr = currentThread();
    Q_ASSERT_X(thr != nullptr, "QThread::setTerminationEnabled()",
               "Current thread was not started with QThread.");

    Q_UNUSED(thr);
#if defined(Q_OS_ANDROID)
    Q_UNUSED(enabled);
#else
    pthread_setcancelstate(enabled ? PTHREAD_CANCEL_ENABLE : PTHREAD_CANCEL_DISABLE, nullptr);
    if (enabled)
        pthread_testcancel();
#endif
}

// Caller must lock the mutex
void QThreadPrivate::setPriority(QThread::Priority threadPriority)
{
    priority = threadPriority;

    // copied from start() with a few modifications:

#ifdef QT_HAS_THREAD_PRIORITY_SCHEDULING
    int sched_policy;
    sched_param param;

    if (pthread_getschedparam(from_HANDLE<pthread_t>(data->threadId.loadRelaxed()), &sched_policy, &param) != 0) {
        // failed to get the scheduling policy, don't bother setting
        // the priority
        qWarning("QThread::setPriority: Cannot get scheduler parameters");
        return;
    }

    int prio;
    if (!calculateUnixPriority(priority, &sched_policy, &prio)) {
        // failed to get the scheduling parameters, don't
        // bother setting the priority
        qWarning("QThread::setPriority: Cannot determine scheduler priority range");
        return;
    }

    param.sched_priority = prio;
    int status = pthread_setschedparam(from_HANDLE<pthread_t>(data->threadId.loadRelaxed()), sched_policy, &param);

# ifdef SCHED_IDLE
    // were we trying to set to idle priority and failed?
    if (status == -1 && sched_policy == SCHED_IDLE && errno == EINVAL) {
        // reset to lowest priority possible
        pthread_getschedparam(from_HANDLE<pthread_t>(data->threadId.loadRelaxed()), &sched_policy, &param);
        param.sched_priority = sched_get_priority_min(sched_policy);
        pthread_setschedparam(from_HANDLE<pthread_t>(data->threadId.loadRelaxed()), sched_policy, &param);
    }
# else
    Q_UNUSED(status);
# endif // SCHED_IDLE
#endif
}

void QThreadPrivate::setQualityOfServiceLevel(QThread::QualityOfService qosLevel)
{
    [[maybe_unused]]
    Q_Q(QThread);
    serviceLevel = qosLevel;
#ifdef Q_OS_DARWIN
    qCDebug(lcQThread) << "Setting thread QoS class to" << serviceLevel << "for thread" << q;
    pthread_set_qos_class_self_np(nativeQualityOfServiceClass(), 0);
#endif
}

#ifdef Q_OS_DARWIN
qos_class_t QThreadPrivate::nativeQualityOfServiceClass() const
{
    // @note Consult table[0] to see what the levels mean
    // [0] https://developer.apple.com/library/archive/documentation/Performance/Conceptual/power_efficiency_guidelines_osx/PrioritizeWorkAtTheTaskLevel.html#//apple_ref/doc/uid/TP40013929-CH35-SW5
    // There are more levels but they have two other documented ones,
    // QOS_CLASS_BACKGROUND, which is below UTILITY, but has no guarantees
    // for scheduling (ie. the OS could choose to never give it CPU time),
    // and QOS_CLASS_USER_INITIATED, documented as being intended for
    // user-initiated actions, such as loading a text document.
    switch (serviceLevel) {
    case QThread::QualityOfService::Auto:
        return QOS_CLASS_DEFAULT;
    case QThread::QualityOfService::High:
        return QOS_CLASS_USER_INTERACTIVE;
    case QThread::QualityOfService::Eco:
        return QOS_CLASS_UTILITY;
    }
    Q_UNREACHABLE_RETURN(QOS_CLASS_DEFAULT);
}
#endif

#endif // QT_CONFIG(thread)

QT_END_NAMESPACE

