// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qthread.h"

#include "qplatformdefs.h"

#include <private/qcoreapplication_p.h>
#include <private/qcore_unix_p.h>
#include "qloggingcategory.h"
#include <private/qtools_p.h>

#if defined(Q_OS_DARWIN)
#  include <private/qeventdispatcher_cf_p.h>
#elif defined(Q_OS_WASM)
#    include <private/qeventdispatcher_wasm_p.h>
#else
#  if !defined(QT_NO_GLIB)
#    include "../kernel/qeventdispatcher_glib_p.h"
#  endif
#endif

#if !defined(Q_OS_WASM)
#  include <private/qeventdispatcher_unix_p.h>
#endif

#include "qthreadstorage.h"

#include "qthread_p.h"

#include "qdebug.h"

#ifdef __GLIBCXX__
#include <cxxabi.h>
#endif

#include <sched.h>
#include <errno.h>

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

using namespace QtMiscUtils;

#if QT_CONFIG(thread)

static_assert(sizeof(pthread_t) <= sizeof(Qt::HANDLE));

enum { ThreadPriorityResetFlag = 0x80000000 };

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
// However, we can't destroy the QThreadData for the thread that called
// ::exit() that early, because a lot of existing content (including in Qt)
// runs when the static destructors are run and they do depend on QThreadData
// being extant. Likewise, we can't destroy it at global static destructor time
// because it's too late: the event dispatcher is usually a class found in a
// plugin and the plugin's destructors (as well as QtGui's) will have run. So
// we strike a middle-ground and destroy at function-local static destructor
// time (see set_thread_data()), because those run after the thread_local ones,
// before the global ones, and in reverse order of creation.

// Always access this through the {get,set,clear}_thread_data() functions.
Q_CONSTINIT static thread_local QThreadData *currentThreadData = nullptr;

static void destroy_current_thread_data(void *p)
{
    QThreadData *data = static_cast<QThreadData *>(p);
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

    // the QThread object may still have a reference, so this may not delete
    data->deref();

    // ... but we must reset it to zero before returning so we aren't
    // leaving a dangling pointer.
    currentThreadData = nullptr;
}

// Utility functions for getting, setting and clearing thread specific data.
static QThreadData *get_thread_data()
{
    return currentThreadData;
}

namespace {
struct PThreadTlsKey
{
    pthread_key_t key;
    PThreadTlsKey() noexcept { pthread_key_create(&key, destroy_current_thread_data); }
    ~PThreadTlsKey() { pthread_key_delete(key); }
};
}
#if QT_SUPPORTS_INIT_PRIORITY
Q_DECL_INIT_PRIORITY(10)
#endif
static PThreadTlsKey pthreadTlsKey; // intentional non-trivial init & destruction

static void set_thread_data(QThreadData *data) noexcept
{
    if (data) {
        // As noted above: one global static for the thread that called
        // ::exit() (which may not be a Qt thread) and the pthread_key_t for
        // all others.
        static struct Cleanup {
            ~Cleanup() {
                if (QThreadData *data = get_thread_data())
                    destroy_current_thread_data(data);
            }
        } currentThreadCleanup;
        pthread_setspecific(pthreadTlsKey.key, data);
    }
    currentThreadData = data;
}

static void clear_thread_data()
{
    set_thread_data(nullptr);
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
    clear_thread_data();
}

QThreadData *QThreadData::current(bool createIfNecessary)
{
    QThreadData *data = get_thread_data();
    if (!data && createIfNecessary) {
        data = new QThreadData;
        QT_TRY {
            set_thread_data(data);
            data->thread.storeRelease(new QAdoptedThread(data));
        } QT_CATCH(...) {
            clear_thread_data();
            data->deref();
            data = nullptr;
            QT_RETHROW;
        }
        data->deref();
        data->isAdopted = true;
        data->threadId.storeRelaxed(QThread::currentThreadId());
        if (!QCoreApplicationPrivate::theMainThreadId.loadAcquire()) {
            auto *mainThread = data->thread.loadRelaxed();
            mainThread->setObjectName("Qt mainThread");
            QCoreApplicationPrivate::theMainThread.storeRelease(mainThread);
            QCoreApplicationPrivate::theMainThreadId.storeRelaxed(data->threadId.loadRelaxed());
        }
    }
    return data;
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
        qTerminate();
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

        d->isInFinish = true;
        d->priority = QThread::InheritPriority;
        locker.unlock();
        emit thr->finished(QThread::QPrivateSignal());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        void *data = &d->data->tls;
        QThreadStorageData::finish((void **)data);
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

        d->running = false;
        d->finished = true;
        d->interruptionRequested.store(false, std::memory_order_relaxed);

        d->isInFinish = false;

        d->thread_done.wakeAll();
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
#  if defined(Q_OS_FREEBSD) && !defined(CPU_COUNT_S)
#    define CPU_COUNT_S(setsize, cpusetp)   ((int)BIT_COUNT(setsize, cpusetp))
    // match the Linux API for simplicity
    using cpu_set_t = cpuset_t;
    auto sched_getaffinity = [](pid_t, size_t cpusetsize, cpu_set_t *mask) {
        return cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, cpusetsize, mask);
    };
#  endif

    // get the number of threads we're assigned, not the total in the system
    QVarLengthArray<cpu_set_t, 1> cpuset(1);
    int size = 1;
    if (Q_UNLIKELY(sched_getaffinity(0, sizeof(cpu_set_t), cpuset.data()) < 0)) {
        for (size = 2; size <= 4; size *= 2) {
            cpuset.resize(size);
            if (sched_getaffinity(0, sizeof(cpu_set_t) * size, cpuset.data()) == 0)
                break;
        }
        if (size > 4)
            return 1;
    }
    cores = CPU_COUNT_S(sizeof(cpu_set_t) * size, cpuset.data());
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
#if defined(Q_OS_VXWORKS) && defined(VXWORKS_DKM)
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

    if (d->isInFinish)
        d->thread_done.wait(locker.mutex());

    if (d->running)
        return;

    d->running = true;
    d->finished = false;
    d->returnCode = 0;
    d->exited = false;
    d->interruptionRequested.store(false, std::memory_order_relaxed);
    d->terminated = false;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

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
            d->running = false;
            d->finished = false;
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

        d->running = false;
        d->finished = false;
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

bool QThread::wait(QDeadlineTimer deadline)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->finished || !d->running)
        return true;

    if (isCurrentThread()) {
        qWarning("QThread::wait: Thread tried to wait on itself");
        return false;
    }

    return d->wait(locker, deadline);
}

bool QThreadPrivate::wait(QMutexLocker<QMutex> &locker, QDeadlineTimer deadline)
{
    Q_ASSERT(locker.isLocked());
    QThreadPrivate *d = this;

    while (d->running) {
        if (!d->thread_done.wait(locker.mutex(), deadline))
            return false;
    }

    return true;
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

#endif // QT_CONFIG(thread)

QT_END_NAMESPACE

