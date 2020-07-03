/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qthread.h"

#include "qplatformdefs.h"

#include <private/qcoreapplication_p.h>
#include <private/qcore_unix_p.h>

#if defined(Q_OS_DARWIN)
#  include <private/qeventdispatcher_cf_p.h>
#else
#  if !defined(QT_NO_GLIB)
#    include "../kernel/qeventdispatcher_glib_p.h"
#  endif
#endif

#include <private/qeventdispatcher_unix_p.h>

#include "qthreadstorage.h"

#include "qthread_p.h"

#include "qdebug.h"

#ifdef __GLIBCXX__
#include <cxxabi.h>
#endif

#include <sched.h>
#include <errno.h>

#ifdef Q_OS_BSD4
#include <sys/sysctl.h>
#endif
#ifdef Q_OS_VXWORKS
#  if (_WRS_VXWORKS_MAJOR > 6) || ((_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR >= 6))
#    include <vxCpuLib.h>
#    include <cpuset.h>
#    define QT_VXWORKS_HAS_CPUSET
#  endif
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

#if QT_CONFIG(thread)

Q_STATIC_ASSERT(sizeof(pthread_t) <= sizeof(Qt::HANDLE));

enum { ThreadPriorityResetFlag = 0x80000000 };


static thread_local QThreadData *currentThreadData = nullptr;

static pthread_once_t current_thread_data_once = PTHREAD_ONCE_INIT;
static pthread_key_t current_thread_data_key;

static void destroy_current_thread_data(void *p)
{
    QThreadData *data = static_cast<QThreadData *>(p);
    // thread_local variables are set to zero before calling this destructor function,
    // if they are internally using pthread-specific data management,
    // so we need to set it back to the right value...
    currentThreadData = data;
    if (data->isAdopted) {
        QThread *thread = data->thread.loadAcquire();
        Q_ASSERT(thread);
        QThreadPrivate *thread_p = static_cast<QThreadPrivate *>(QObjectPrivate::get(thread));
        Q_ASSERT(!thread_p->finished);
        thread_p->finish(thread);
    }
    data->deref();

    // ... but we must reset it to zero before returning so we aren't
    // leaving a dangling pointer.
    currentThreadData = nullptr;
}

static void create_current_thread_data_key()
{
    pthread_key_create(&current_thread_data_key, destroy_current_thread_data);
}

static void destroy_current_thread_data_key()
{
    pthread_once(&current_thread_data_once, create_current_thread_data_key);
    pthread_key_delete(current_thread_data_key);

    // Reset current_thread_data_once in case we end up recreating
    // the thread-data in the rare case of QObject construction
    // after destroying the QThreadData.
    pthread_once_t pthread_once_init = PTHREAD_ONCE_INIT;
    current_thread_data_once = pthread_once_init;
}
Q_DESTRUCTOR_FUNCTION(destroy_current_thread_data_key)


// Utility functions for getting, setting and clearing thread specific data.
static QThreadData *get_thread_data()
{
    return currentThreadData;
}

static void set_thread_data(QThreadData *data)
{
    currentThreadData = data;
    pthread_once(&current_thread_data_once, create_current_thread_data_key);
    pthread_setspecific(current_thread_data_key, data);
}

static void clear_thread_data()
{
    currentThreadData = nullptr;
    pthread_setspecific(current_thread_data_key, nullptr);
}

template <typename T>
static typename std::enable_if<QTypeInfo<T>::isIntegral, Qt::HANDLE>::type to_HANDLE(T id)
{
    return reinterpret_cast<Qt::HANDLE>(static_cast<intptr_t>(id));
}

template <typename T>
static typename std::enable_if<QTypeInfo<T>::isIntegral, T>::type from_HANDLE(Qt::HANDLE id)
{
    return static_cast<T>(reinterpret_cast<intptr_t>(id));
}

template <typename T>
static typename std::enable_if<QTypeInfo<T>::isPointer, Qt::HANDLE>::type to_HANDLE(T id)
{
    return id;
}

template <typename T>
static typename std::enable_if<QTypeInfo<T>::isPointer, T>::type from_HANDLE(Qt::HANDLE id)
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
            data->thread = new QAdoptedThread(data);
        } QT_CATCH(...) {
            clear_thread_data();
            data->deref();
            data = nullptr;
            QT_RETHROW;
        }
        data->deref();
        data->isAdopted = true;
        data->threadId.storeRelaxed(to_HANDLE(pthread_self()));
        if (!QCoreApplicationPrivate::theMainThread.loadAcquire())
            QCoreApplicationPrivate::theMainThread.storeRelease(data->thread.loadRelaxed());
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
typedef void*(*QtThreadCallback)(void*);
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

#if (defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_QNX))
static void setCurrentThreadName(const char *name)
{
#  if defined(Q_OS_LINUX) && !defined(QT_LINUXBASE)
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
#  elif defined(Q_OS_MAC)
    pthread_setname_np(name);
#  elif defined(Q_OS_QNX)
    pthread_setname_np(pthread_self(), name);
#  endif
}
#endif

void *QThreadPrivate::start(void *arg)
{
#if !defined(Q_OS_ANDROID)
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
#endif
    pthread_cleanup_push(QThreadPrivate::finish, arg);

#ifndef QT_NO_EXCEPTIONS
    try
#endif
    {
        QThread *thr = reinterpret_cast<QThread *>(arg);
        QThreadData *data = QThreadData::get2(thr);

        {
            QMutexLocker locker(&thr->d_func()->mutex);

            // do we need to reset the thread priority?
            if (int(thr->d_func()->priority) & ThreadPriorityResetFlag) {
                thr->d_func()->setPriority(QThread::Priority(thr->d_func()->priority & ~ThreadPriorityResetFlag));
            }

            data->threadId.storeRelaxed(to_HANDLE(pthread_self()));
            set_thread_data(data);

            data->ref();
            data->quitNow = thr->d_func()->exited;
        }

        data->ensureEventDispatcher();

#if (defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_QNX))
        {
            // Sets the name of the current thread. We can only do this
            // when the thread is starting, as we don't have a cross
            // platform way of setting the name of an arbitrary thread.
            if (Q_LIKELY(thr->objectName().isEmpty()))
                setCurrentThreadName(thr->metaObject()->className());
            else
                setCurrentThreadName(thr->objectName().toLocal8Bit());
        }
#endif

        emit thr->started(QThread::QPrivateSignal());
#if !defined(Q_OS_ANDROID)
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
        pthread_testcancel();
#endif
        thr->run();
    }
#ifndef QT_NO_EXCEPTIONS
#ifdef __GLIBCXX__
    // POSIX thread cancellation under glibc is implemented by throwing an exception
    // of this type. Do what libstdc++ is doing and handle it specially in order not to
    // abort the application if user's code calls a cancellation function.
    catch (const abi::__forced_unwind &) {
        throw;
    }
#endif // __GLIBCXX__
    catch (...) {
        qTerminate();
    }
#endif // QT_NO_EXCEPTIONS

    // This pop runs finish() below. It's outside the try/catch (and has its
    // own try/catch) to prevent finish() to be run in case an exception is
    // thrown.
    pthread_cleanup_pop(1);

    return nullptr;
}

void QThreadPrivate::finish(void *arg)
{
#ifndef QT_NO_EXCEPTIONS
    try
#endif
    {
        QThread *thr = reinterpret_cast<QThread *>(arg);
        QThreadPrivate *d = thr->d_func();

        QMutexLocker locker(&d->mutex);

        d->isInFinish = true;
        d->priority = QThread::InheritPriority;
        void *data = &d->data->tls;
        locker.unlock();
        emit thr->finished(QThread::QPrivateSignal());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QThreadStorageData::finish((void **)data);
        locker.relock();

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
        d->interruptionRequested = false;

        d->isInFinish = false;
        d->thread_done.wakeAll();
    }
#ifndef QT_NO_EXCEPTIONS
#ifdef __GLIBCXX__
    // POSIX thread cancellation under glibc is implemented by throwing an exception
    // of this type. Do what libstdc++ is doing and handle it specially in order not to
    // abort the application if user's code calls a cancellation function.
    catch (const abi::__forced_unwind &) {
        throw;
    }
#endif // __GLIBCXX__
    catch (...) {
        qTerminate();
    }
#endif // QT_NO_EXCEPTIONS
}




/**************************************************************************
 ** QThread
 *************************************************************************/

Qt::HANDLE QThread::currentThreadId() noexcept
{
    // requires a C cast here otherwise we run into trouble on AIX
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
#elif defined(Q_OS_BSD4)
    // FreeBSD, OpenBSD, NetBSD, BSD/OS, OS X, iOS
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
    // VxWorks
#  if defined(QT_VXWORKS_HAS_CPUSET)
    cpuset_t cpus = vxCpuEnabledGet();
    cores = 0;

    // 128 cores should be enough for everyone ;)
    for (int i = 0; i < 128 && !CPUSET_ISZERO(cpus); ++i) {
        if (CPUSET_ISSET(cpus, i)) {
            CPUSET_CLR(cpus, i);
            cores++;
        }
    }
#  else
    // as of aug 2008 VxWorks < 6.6 only supports one single core CPU
    cores = 1;
#  endif
#elif defined(Q_OS_WASM)
    cores = QThreadPrivate::idealThreadCount;
#else
    // the rest: Linux, Solaris, AIX, Tru64
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

static timespec makeTimespec(time_t secs, long nsecs)
{
    struct timespec ts;
    ts.tv_sec = secs;
    ts.tv_nsec = nsecs;
    return ts;
}

void QThread::sleep(unsigned long secs)
{
    qt_nanosleep(makeTimespec(secs, 0));
}

void QThread::msleep(unsigned long msecs)
{
    qt_nanosleep(makeTimespec(msecs / 1000, msecs % 1000 * 1000 * 1000));
}

void QThread::usleep(unsigned long usecs)
{
    qt_nanosleep(makeTimespec(usecs / 1000 / 1000, usecs % (1000*1000) * 1000));
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
    d->interruptionRequested = false;

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
                d->priority = Priority(priority | ThreadPriorityResetFlag);
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

    if (!d->data->threadId.loadRelaxed())
        return;

    int code = pthread_cancel(from_HANDLE<pthread_t>(d->data->threadId.loadRelaxed()));
    if (code) {
        qErrnoWarning(code, "QThread::start: Thread termination error");
    }
#endif
}

bool QThread::wait(QDeadlineTimer deadline)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (from_HANDLE<pthread_t>(d->data->threadId.loadRelaxed()) == pthread_self()) {
        qWarning("QThread::wait: Thread tried to wait on itself");
        return false;
    }

    if (d->finished || !d->running)
        return true;

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

    Q_UNUSED(thr)
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

