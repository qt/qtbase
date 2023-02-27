// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qthread.h"
#include "qthread_p.h"
#include "qthreadstorage.h"
#include "qmutex.h"

#include <qcoreapplication.h>
#include <qpointer.h>

#include <private/qcoreapplication_p.h>
#include <private/qeventdispatcher_win_p.h>

#include <qt_windows.h>

#ifndef _MT
#  define _MT
#endif // _MT
#include <process.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(thread)

void qt_watch_adopted_thread(const HANDLE adoptedThreadHandle, QThread *qthread);
DWORD WINAPI qt_adopted_thread_watcher_function(LPVOID);

static DWORD qt_current_thread_data_tls_index = TLS_OUT_OF_INDEXES;
void qt_create_tls()
{
    if (qt_current_thread_data_tls_index != TLS_OUT_OF_INDEXES)
        return;
    Q_CONSTINIT static QBasicMutex mutex;
    QMutexLocker locker(&mutex);
    if (qt_current_thread_data_tls_index != TLS_OUT_OF_INDEXES)
        return;
    qt_current_thread_data_tls_index = TlsAlloc();
}

static void qt_free_tls()
{
    if (qt_current_thread_data_tls_index != TLS_OUT_OF_INDEXES) {
        TlsFree(qt_current_thread_data_tls_index);
        qt_current_thread_data_tls_index = TLS_OUT_OF_INDEXES;
    }
}
Q_DESTRUCTOR_FUNCTION(qt_free_tls)

/*
    QThreadData
*/
void QThreadData::clearCurrentThreadData()
{
    TlsSetValue(qt_current_thread_data_tls_index, 0);
}

QThreadData *QThreadData::current(bool createIfNecessary)
{
    qt_create_tls();
    QThreadData *threadData = reinterpret_cast<QThreadData *>(TlsGetValue(qt_current_thread_data_tls_index));
    if (!threadData && createIfNecessary) {
        threadData = new QThreadData;
        // This needs to be called prior to new AdoptedThread() to
        // avoid recursion.
        TlsSetValue(qt_current_thread_data_tls_index, threadData);
        QT_TRY {
            threadData->thread = new QAdoptedThread(threadData);
        } QT_CATCH(...) {
            TlsSetValue(qt_current_thread_data_tls_index, 0);
            threadData->deref();
            threadData = 0;
            QT_RETHROW;
        }
        threadData->deref();
        threadData->isAdopted = true;
        threadData->threadId.storeRelaxed(reinterpret_cast<Qt::HANDLE>(quintptr(GetCurrentThreadId())));

        if (!QCoreApplicationPrivate::theMainThread) {
            QCoreApplicationPrivate::theMainThread = threadData->thread.loadRelaxed();
        } else {
            HANDLE realHandle = INVALID_HANDLE_VALUE;
            DuplicateHandle(GetCurrentProcess(),
                    GetCurrentThread(),
                    GetCurrentProcess(),
                    &realHandle,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS);
            qt_watch_adopted_thread(realHandle, threadData->thread);
        }
    }
    return threadData;
}

void QAdoptedThread::init()
{
    d_func()->handle = GetCurrentThread();
    d_func()->id = GetCurrentThreadId();
}

static QList<HANDLE> qt_adopted_thread_handles;
static QList<QThread *> qt_adopted_qthreads;
Q_CONSTINIT static QBasicMutex qt_adopted_thread_watcher_mutex;
static DWORD qt_adopted_thread_watcher_id = 0;
static HANDLE qt_adopted_thread_wakeup = 0;

/*!
    \internal
    Adds an adopted thread to the list of threads that Qt watches to make sure
    the thread data is properly cleaned up. This function starts the watcher
    thread if necessary.
*/
void qt_watch_adopted_thread(const HANDLE adoptedThreadHandle, QThread *qthread)
{
    QMutexLocker lock(&qt_adopted_thread_watcher_mutex);

    if (GetCurrentThreadId() == qt_adopted_thread_watcher_id) {
        CloseHandle(adoptedThreadHandle);
        return;
    }

    qt_adopted_thread_handles.append(adoptedThreadHandle);
    qt_adopted_qthreads.append(qthread);

    // Start watcher thread if it is not already running.
    if (qt_adopted_thread_watcher_id == 0) {
        if (qt_adopted_thread_wakeup == 0) {
            qt_adopted_thread_wakeup = CreateEvent(0, false, false, 0);
            qt_adopted_thread_handles.prepend(qt_adopted_thread_wakeup);
        }

        CloseHandle(CreateThread(0, 0, qt_adopted_thread_watcher_function, 0, 0, &qt_adopted_thread_watcher_id));
    } else {
        SetEvent(qt_adopted_thread_wakeup);
    }
}

/*
    This function loops and waits for native adopted threads to finish.
    When this happens it derefs the QThreadData for the adopted thread
    to make sure it gets cleaned up properly.
*/
DWORD WINAPI qt_adopted_thread_watcher_function(LPVOID)
{
    forever {
        qt_adopted_thread_watcher_mutex.lock();

        if (qt_adopted_thread_handles.count() == 1) {
            qt_adopted_thread_watcher_id = 0;
            qt_adopted_thread_watcher_mutex.unlock();
            break;
        }

        QList<HANDLE> handlesCopy = qt_adopted_thread_handles;
        qt_adopted_thread_watcher_mutex.unlock();

        DWORD ret = WAIT_TIMEOUT;
        int count;
        int offset;
        int loops = handlesCopy.size() / MAXIMUM_WAIT_OBJECTS;
        if (handlesCopy.size() % MAXIMUM_WAIT_OBJECTS)
            ++loops;
        if (loops == 1) {
            // no need to loop, no timeout
            offset = 0;
            count = handlesCopy.count();
            ret = WaitForMultipleObjects(handlesCopy.count(), handlesCopy.constData(), false, INFINITE);
        } else {
            int loop = 0;
            do {
                offset = loop * MAXIMUM_WAIT_OBJECTS;
                count = qMin(handlesCopy.count() - offset, MAXIMUM_WAIT_OBJECTS);
                ret = WaitForMultipleObjects(count, handlesCopy.constData() + offset, false, 100);
                loop = (loop + 1) % loops;
            } while (ret == WAIT_TIMEOUT);
        }

        if (ret == WAIT_FAILED || ret >= WAIT_OBJECT_0 + uint(count)) {
            qWarning("QThread internal error while waiting for adopted threads: %d", int(GetLastError()));
            continue;
        }

        const int handleIndex = offset + ret - WAIT_OBJECT_0;
        if (handleIndex == 0) // New handle to watch was added.
            continue;
        const int qthreadIndex = handleIndex - 1;

        qt_adopted_thread_watcher_mutex.lock();
        QThreadData *data = QThreadData::get2(qt_adopted_qthreads.at(qthreadIndex));
        qt_adopted_thread_watcher_mutex.unlock();
        if (data->isAdopted) {
            QThread *thread = data->thread;
            Q_ASSERT(thread);
            auto thread_p = static_cast<QThreadPrivate *>(QObjectPrivate::get(thread));
            Q_UNUSED(thread_p);
            Q_ASSERT(!thread_p->finished);
            QThreadPrivate::finish(thread);
        }
        data->deref();

        QMutexLocker lock(&qt_adopted_thread_watcher_mutex);
        CloseHandle(qt_adopted_thread_handles.at(handleIndex));
        qt_adopted_thread_handles.remove(handleIndex);
        qt_adopted_qthreads.remove(qthreadIndex);
    }

    QThreadData *threadData = reinterpret_cast<QThreadData *>(TlsGetValue(qt_current_thread_data_tls_index));
    if (threadData)
        threadData->deref();

    return 0;
}

#if !defined(QT_NO_DEBUG) && defined(Q_CC_MSVC)

#ifndef Q_OS_WIN64
#  define ULONG_PTR DWORD
#endif

typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;      // must be 0x1000
    LPCSTR szName;     // pointer to name (in user addr space)
    HANDLE dwThreadID; // thread ID (-1=caller thread)
    DWORD dwFlags;     // reserved for future use, must be zero
} THREADNAME_INFO;

void qt_set_thread_name(HANDLE threadId, LPCSTR threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = threadId;
    info.dwFlags = 0;

    __try
    {
        RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD),
                       reinterpret_cast<const ULONG_PTR*>(&info));
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}
#endif // !QT_NO_DEBUG && Q_CC_MSVC

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

    qt_create_tls();
    TlsSetValue(qt_current_thread_data_tls_index, data);
    data->threadId.storeRelaxed(reinterpret_cast<Qt::HANDLE>(quintptr(GetCurrentThreadId())));

    QThread::setTerminationEnabled(false);

    {
        QMutexLocker locker(&thr->d_func()->mutex);
        data->quitNow = thr->d_func()->exited;
    }

    data->ensureEventDispatcher();
    data->eventDispatcher.loadRelaxed()->startingUp();

#if !defined(QT_NO_DEBUG) && defined(Q_CC_MSVC)
    // sets the name of the current thread.
    qt_set_thread_name(HANDLE(-1), thr->d_func()->objectName.isEmpty()
                        ? thr->metaObject()->className()
                        : std::exchange(thr->d_func()->objectName, {}).toLocal8Bit().constData());
#endif

    emit thr->started(QThread::QPrivateSignal());
    QThread::setTerminationEnabled(true);
    thr->run();

    finish(arg);
    return 0;
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
void QThreadPrivate::finish(void *arg, bool lockAnyway) noexcept
{
    QThread *thr = reinterpret_cast<QThread *>(arg);
    QThreadPrivate *d = thr->d_func();

    QMutexLocker locker(lockAnyway ? &d->mutex : nullptr);
    d->isInFinish = true;
    d->priority = QThread::InheritPriority;
    void **tls_data = reinterpret_cast<void **>(&d->data->tls);
    if (lockAnyway)
        locker.unlock();
    emit thr->finished(QThread::QPrivateSignal());
    QCoreApplicationPrivate::sendPostedEvents(nullptr, QEvent::DeferredDelete, d->data);
    QThreadStorageData::finish(tls_data);
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

    d->running = false;
    d->finished = true;
    d->isInFinish = false;
    d->interruptionRequested = false;

    if (!d->waiters) {
        CloseHandle(d->handle);
        d->handle = 0;
    }

    d->id = 0;
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

    if (d->isInFinish) {
        locker.unlock();
        wait();
        locker.relock();
    }

    if (d->running)
        return;

    // avoid interacting with the binding system
    d->objectName = d->extraData ? d->extraData->objectName.valueBypassingBindings()
                                 : QString();
    d->running = true;
    d->finished = false;
    d->exited = false;
    d->returnCode = 0;
    d->interruptionRequested = false;

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
                                            this, CREATE_SUSPENDED, &(d->id));
#else
    // MSVC -MD or -MDd or MinGW build
    d->handle = CreateThread(nullptr, d->stackSize,
                             reinterpret_cast<LPTHREAD_START_ROUTINE>(QThreadPrivate::start),
                             this, CREATE_SUSPENDED, reinterpret_cast<LPDWORD>(&d->id));
#endif

    if (!d->handle) {
        qErrnoWarning("QThread::start: Failed to create thread");
        d->running = false;
        d->finished = true;
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
    if (!d->running)
        return;
    if (!d->terminationEnabled) {
        d->terminatePending = true;
        return;
    }

    TerminateThread(d->handle, 0);
    QThreadPrivate::finish(this, false);
}

bool QThread::wait(QDeadlineTimer deadline)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->id == GetCurrentThreadId()) {
        qWarning("QThread::wait: Thread tried to wait on itself");
        return false;
    }
    if (d->finished || !d->running)
        return true;

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

    if (ret && !d->finished) {
        // thread was terminated by someone else

        QThreadPrivate::finish(this, false);
    }

    if (d->finished && !d->waiters) {
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
        QThreadPrivate::finish(thr, false);
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
