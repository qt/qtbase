// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qthread.h"
#include "qthread_p.h"

#include "qcoreapplication.h"
#include <private/qcoreapplication_p.h>
#include <private/qeventdispatcher_win_p.h>
#include "qloggingcategory.h"
#include "qmutex.h"
#include "qthreadstorage.h"

#include <qt_windows.h>

#ifndef _MT
#  define _MT
#endif // _MT
#include <process.h>

extern "C" {
// MinGW is missing the declaration of SetThreadDescription:
WINBASEAPI
HRESULT
WINAPI
SetThreadDescription(
    _In_ HANDLE hThread,
    _In_ PCWSTR lpThreadDescription
    );
}

#ifndef THREAD_POWER_THROTTLING_EXECUTION_SPEED
#define THREAD_POWER_THROTTLING_EXECUTION_SPEED 0x1
#define THREAD_POWER_THROTTLING_CURRENT_VERSION 1

typedef struct _THREAD_POWER_THROTTLING_STATE {
    ULONG Version;
    ULONG ControlMask;
    ULONG StateMask;
} THREAD_POWER_THROTTLING_STATE;
#endif


QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcQThread, "qt.core.thread", QtWarningMsg)

#if QT_CONFIG(thread)

Q_CONSTINIT static thread_local QThreadData *currentThreadData = nullptr;

static void destroy_current_thread_data(void *p)
{
    QThreadData *data = static_cast<QThreadData *>(p);
    QThread *thread = data->thread.loadAcquire();

    if (data->isAdopted) {
        // If this is an adopted thread, then QThreadData owns the QThread and
        // this is very likely the last reference. These pointers cannot be
        // null and there is no race.
        QThreadPrivate *thread_p = static_cast<QThreadPrivate *>(QObjectPrivate::get(thread));
        thread_p->finish();
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

static QThreadData *get_thread_data()
{
    return currentThreadData;
}

static void set_thread_data(QThreadData *data) noexcept
{
    if (data) {
        struct Cleanup {
            Cleanup() { QThreadStoragePrivate::init(); }
            ~Cleanup() { destroy_current_thread_data(currentThreadData); }
        };
        static thread_local Cleanup currentThreadCleanup;
    }
    currentThreadData = data;
}

/*
    QThreadData
*/
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
    d_func()->handle = GetCurrentThread();
}

/**************************************************************************
 ** QThreadPrivate
 *************************************************************************/

#endif // QT_CONFIG(thread)

QAbstractEventDispatcher *QThreadPrivate::createEventDispatcher(QThreadData *data)
{
    Q_UNUSED(data);
    return new QEventDispatcherWin32;
}

#if QT_CONFIG(thread)

unsigned int __stdcall QT_ENSURE_STACK_ALIGNED_FOR_SSE QThreadPrivate::start(void *arg) noexcept
{
    QThread *thr = reinterpret_cast<QThread *>(arg);
    QThreadData *data = QThreadData::get2(thr);
    // If a QThread is restarted, reuse the QBindingStatus, too
    data->reuseBindingStatusForNewNativeThread();

    data->ref();
    set_thread_data(data);
    data->threadId.storeRelaxed(QThread::currentThreadId());

    QThread::setTerminationEnabled(false);

    {
        QMutexLocker locker(&thr->d_func()->mutex);
        data->quitNow = thr->d_func()->exited;

        if (thr->d_func()->serviceLevel != QThread::QualityOfService::Auto)
            thr->d_func()->setQualityOfServiceLevel(thr->d_func()->serviceLevel);
    }

    data->ensureEventDispatcher();
    data->eventDispatcher.loadRelaxed()->startingUp();

    // sets the name of the current thread.
    QString threadName = std::exchange(thr->d_func()->objectName, {});
    if (Q_LIKELY(threadName.isEmpty()))
        threadName = QString::fromUtf8(thr->metaObject()->className());
#ifndef QT_WIN_SERVER_2016_COMPAT
    SetThreadDescription(GetCurrentThread(), reinterpret_cast<const wchar_t *>(threadName.utf16()));
#else
    HMODULE kernelbase = GetModuleHandleW(L"kernelbase.dll");
    if (kernelbase != NULL) {
        typedef HRESULT (WINAPI *DESCFUNC)(HANDLE, PCWSTR);

        DESCFUNC setThreadDescription =
            (DESCFUNC)GetProcAddress(kernelbase, "SetThreadDescription");
        if (setThreadDescription != NULL) {
            setThreadDescription(GetCurrentThread(),
                                 reinterpret_cast<const wchar_t *>(threadName.utf16()));
        }
    }
#endif

    emit thr->started(QThread::QPrivateSignal());
    QThread::setTerminationEnabled(true);
    thr->run();

    thr->d_func()->finish();
    return 0;
}

void QThreadPrivate::setQualityOfServiceLevel(QThread::QualityOfService qosLevel)
{
    Q_Q(QThread);
    serviceLevel = qosLevel;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10_RS3)
    qCDebug(lcQThread) << "Setting thread QoS class to" << qosLevel << "for thread" << q;

    THREAD_POWER_THROTTLING_STATE state;
    memset(&state, 0, sizeof(state));
    state.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;

    switch (qosLevel) {
    case QThread::QualityOfService::Auto:
        state.ControlMask = 0; // Unset control of QoS
        state.StateMask = 0;
        break;
    case QThread::QualityOfService::Eco:
        state.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        state.StateMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        break;
    case QThread::QualityOfService::High:
        state.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        state.StateMask = 0; // Ask to disable throttling
        break;
    }
    if (!SetThreadInformation(::GetCurrentThread(), THREAD_INFORMATION_CLASS::ThreadPowerThrottling,
                              &state, sizeof(state))) {
        qErrnoWarning("Failed to set thread power throttling state");
    }
#endif
}

/*
    For regularly terminating threads, this will be called and executed by the thread as the
    last code before the thread exits. In that case, \a arg is the current QThread.

    However, this function will also be called by QThread::terminate (as well as wait() and
    setTerminationEnabled) to give Qt a chance to update the terminated thread's state and
    process pending DeleteLater events for objects that live in the terminated thread. And for
    adopted thread, this method is called by the thread watcher.

    In those cases, \a arg will not be the current thread.
*/
void QThreadPrivate::finish(bool lockAnyway) noexcept
{
    QThreadPrivate *d = this;
    QThread *thr = q_func();

    QMutexLocker locker(lockAnyway ? &d->mutex : nullptr);
    d->threadState = QThreadPrivate::Finishing;
    d->priority = QThread::InheritPriority;
    if (lockAnyway)
        locker.unlock();
    emit thr->finished(QThread::QPrivateSignal());
    QCoreApplicationPrivate::sendPostedEvents(nullptr, QEvent::DeferredDelete, d->data);
    QThreadStoragePrivate::finish(&d->data->tls);
    if (lockAnyway)
        locker.relock();

    QAbstractEventDispatcher *eventDispatcher = d->data->eventDispatcher.loadRelaxed();
    if (eventDispatcher) {
        d->data->eventDispatcher = 0;
        if (lockAnyway)
            locker.unlock();
        eventDispatcher->closingDown();
        delete eventDispatcher;
        if (lockAnyway)
            locker.relock();
    }

    d->threadState = QThreadPrivate::Finished;
    d->interruptionRequested.store(false, std::memory_order_relaxed);

    if (!d->waiters) {
        CloseHandle(d->handle);
        d->handle = 0;
    }
}

/**************************************************************************
 ** QThread
 *************************************************************************/

Qt::HANDLE QThread::currentThreadIdImpl() noexcept
{
    return reinterpret_cast<Qt::HANDLE>(quintptr(GetCurrentThreadId()));
}

int QThread::idealThreadCount() noexcept
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

void QThread::yieldCurrentThread()
{
    SwitchToThread();
}

#endif // QT_CONFIG(thread)

void QThread::sleep(std::chrono::nanoseconds nsecs)
{
    using namespace std::chrono;
    ::Sleep(DWORD(duration_cast<milliseconds>(nsecs).count()));
}

void QThread::sleep(unsigned long secs)
{
    ::Sleep(secs * 1000);
}

void QThread::msleep(unsigned long msecs)
{
    ::Sleep(msecs);
}

void QThread::usleep(unsigned long usecs)
{
    ::Sleep((usecs / 1000) + 1);
}

#if QT_CONFIG(thread)

void QThread::start(Priority priority)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->threadState == QThreadPrivate::Finishing) {
        locker.unlock();
        wait();
        locker.relock();
    }

    if (d->threadState == QThreadPrivate::Running)
        return;

    // avoid interacting with the binding system
    d->objectName = d->extraData ? d->extraData->objectName.valueBypassingBindings()
                                 : QString();
    d->threadState = QThreadPrivate::Running;
    d->exited = false;
    d->returnCode = 0;
    d->interruptionRequested.store(false, std::memory_order_relaxed);

    /*
      NOTE: we create the thread in the suspended state, set the
      priority and then resume the thread.

      since threads are created with normal priority by default, we
      could get into a case where a thread (with priority less than
      NormalPriority) tries to create a new thread (also with priority
      less than NormalPriority), but the newly created thread preempts
      its 'parent' and runs at normal priority.
    */
#if defined(Q_CC_MSVC) && !defined(_DLL)
    // MSVC -MT or -MTd build
    d->handle = (Qt::HANDLE) _beginthreadex(NULL, d->stackSize, QThreadPrivate::start,
                                            this, CREATE_SUSPENDED, nullptr);
#else
    // MSVC -MD or -MDd or MinGW build
    d->handle = CreateThread(nullptr, d->stackSize,
                             reinterpret_cast<LPTHREAD_START_ROUTINE>(QThreadPrivate::start),
                             this, CREATE_SUSPENDED, nullptr);
#endif

    if (!d->handle) {
        qErrnoWarning("QThread::start: Failed to create thread");
        d->threadState = QThreadPrivate::NotStarted;
        return;
    }

    int prio;
    d->priority = priority;
    switch (priority) {
    case IdlePriority:
        prio = THREAD_PRIORITY_IDLE;
        break;

    case LowestPriority:
        prio = THREAD_PRIORITY_LOWEST;
        break;

    case LowPriority:
        prio = THREAD_PRIORITY_BELOW_NORMAL;
        break;

    case NormalPriority:
        prio = THREAD_PRIORITY_NORMAL;
        break;

    case HighPriority:
        prio = THREAD_PRIORITY_ABOVE_NORMAL;
        break;

    case HighestPriority:
        prio = THREAD_PRIORITY_HIGHEST;
        break;

    case TimeCriticalPriority:
        prio = THREAD_PRIORITY_TIME_CRITICAL;
        break;

    case InheritPriority:
    default:
        prio = GetThreadPriority(GetCurrentThread());
        break;
    }

    if (!SetThreadPriority(d->handle, prio)) {
        qErrnoWarning("QThread::start: Failed to set thread priority");
    }

    if (ResumeThread(d->handle) == (DWORD) -1) {
        qErrnoWarning("QThread::start: Failed to resume new thread");
    }
}

void QThread::terminate()
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    if (d->threadState != QThreadPrivate::Running)
        return;
    if (!d->terminationEnabled) {
        d->terminatePending = true;
        return;
    }

    TerminateThread(d->handle, 0);
    d->finish(false);
}

bool QThreadPrivate::wait(QMutexLocker<QMutex> &locker, QDeadlineTimer deadline)
{
    Q_ASSERT(threadState != QThreadPrivate::Finished);
    Q_ASSERT(locker.isLocked());
    QThreadPrivate *d = this;

    ++d->waiters;
    locker.mutex()->unlock();

    bool ret = false;
    switch (WaitForSingleObject(d->handle, deadline.remainingTime())) {
    case WAIT_OBJECT_0:
        ret = true;
        break;
    case WAIT_FAILED:
        qErrnoWarning("QThread::wait: Thread wait failure");
        break;
    case WAIT_ABANDONED:
    case WAIT_TIMEOUT:
    default:
        break;
    }

    locker.mutex()->lock();
    --d->waiters;

    if (ret && d->threadState < QThreadPrivate::Finished) {
        // thread was terminated by someone else

        d->finish(false);
    }

    if (d->threadState == QThreadPrivate::Finished && !d->waiters) {
        CloseHandle(d->handle);
        d->handle = 0;
    }

    return ret;
}

void QThread::setTerminationEnabled(bool enabled)
{
    QThread *thr = currentThread();
    Q_ASSERT_X(thr != 0, "QThread::setTerminationEnabled()",
               "Current thread was not started with QThread.");
    QThreadPrivate *d = thr->d_func();
    QMutexLocker locker(&d->mutex);
    d->terminationEnabled = enabled;
    if (enabled && d->terminatePending) {
        d->finish(false);
        locker.unlock(); // don't leave the mutex locked!
        _endthreadex(0);
    }
}

// Caller must hold the mutex
void QThreadPrivate::setPriority(QThread::Priority threadPriority)
{
    // copied from start() with a few modifications:

    int prio;
    priority = threadPriority;
    switch (threadPriority) {
    case QThread::IdlePriority:
        prio = THREAD_PRIORITY_IDLE;
        break;

    case QThread::LowestPriority:
        prio = THREAD_PRIORITY_LOWEST;
        break;

    case QThread::LowPriority:
        prio = THREAD_PRIORITY_BELOW_NORMAL;
        break;

    case QThread::NormalPriority:
        prio = THREAD_PRIORITY_NORMAL;
        break;

    case QThread::HighPriority:
        prio = THREAD_PRIORITY_ABOVE_NORMAL;
        break;

    case QThread::HighestPriority:
        prio = THREAD_PRIORITY_HIGHEST;
        break;

    case QThread::TimeCriticalPriority:
        prio = THREAD_PRIORITY_TIME_CRITICAL;
        break;

    default:
        return;
    }

    if (!SetThreadPriority(handle, prio)) {
        qErrnoWarning("QThread::setPriority: Failed to set thread priority");
    }
}

#endif // QT_CONFIG(thread)

QT_END_NAMESPACE
