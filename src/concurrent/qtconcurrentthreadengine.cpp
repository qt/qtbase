// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtconcurrentthreadengine.h"

#include <QtCore/private/qsimd_p.h>

#if !defined(QT_NO_CONCURRENT) || defined(Q_QDOC)

QT_BEGIN_NAMESPACE

namespace QtConcurrent {

/*!
  \class QtConcurrent::ThreadEngineBarrier
  \inmodule QtConcurrent
  \internal
*/

/*!
  \enum QtConcurrent::ThreadFunctionResult
  \internal
*/

/*!
  \class QtConcurrent::ThreadEngineBase
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::ThreadEngine
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::ThreadEngineStarterBase
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::ThreadEngineStarter
  \inmodule QtConcurrent
  \internal
*/

/*!
  \fn [qtconcurrentthreadengine-1] template <typename ThreadEngine> ThreadEngineStarter<typename ThreadEngine::ResultType> QtConcurrent::startThreadEngine(ThreadEngine *threadEngine)
  \internal
*/

ThreadEngineBarrier::ThreadEngineBarrier()
:count(0) { }

void ThreadEngineBarrier::acquire()
{
    forever {
        int localCount = count.loadRelaxed();
        if (localCount < 0) {
            if (count.testAndSetOrdered(localCount, localCount -1))
                return;
        } else {
            if (count.testAndSetOrdered(localCount, localCount + 1))
                return;
        }
        qYieldCpu();
    }
}

int ThreadEngineBarrier::release()
{
    forever {
        int localCount = count.loadRelaxed();
        if (localCount == -1) {
            if (count.testAndSetOrdered(-1, 0)) {
                semaphore.release();
                return 0;
            }
        } else if (localCount < 0) {
            if (count.testAndSetOrdered(localCount, localCount + 1))
                return qAbs(localCount + 1);
        } else {
            if (count.testAndSetOrdered(localCount, localCount - 1))
                return localCount - 1;
        }
        qYieldCpu();
    }
}

// Wait until all threads have been released
void ThreadEngineBarrier::wait()
{
    forever {
        int localCount = count.loadRelaxed();
        if (localCount == 0)
            return;

        Q_ASSERT(localCount > 0); // multiple waiters are not allowed.
        if (count.testAndSetOrdered(localCount, -localCount)) {
            semaphore.acquire();
            return;
        }
        qYieldCpu();
    }
}

int ThreadEngineBarrier::currentCount()
{
    return count.loadRelaxed();
}

// releases a thread, unless this is the last thread.
// returns true if the thread was released.
bool ThreadEngineBarrier::releaseUnlessLast()
{
    forever {
        int localCount = count.loadRelaxed();
        if (qAbs(localCount) == 1) {
            return false;
        } else if (localCount < 0) {
            if (count.testAndSetOrdered(localCount, localCount + 1))
                return true;
        } else {
            if (count.testAndSetOrdered(localCount, localCount - 1))
                return true;
        }
        qYieldCpu();
    }
}

ThreadEngineBase::ThreadEngineBase(QThreadPool *pool)
    : futureInterface(nullptr), threadPool(pool)
{
    setAutoDelete(false);
}

ThreadEngineBase::~ThreadEngineBase() {}

void ThreadEngineBase::startSingleThreaded()
{
    start();
    while (threadFunction() != ThreadFinished)
        ;
    finish();
}

void ThreadEngineBase::startThread()
{
    startThreadInternal();
}

void ThreadEngineBase::acquireBarrierSemaphore()
{
    barrier.acquire();
}

void ThreadEngineBase::reportIfSuspensionDone() const
{
    if (futureInterface && futureInterface->isSuspending())
        futureInterface->reportSuspended();
}

bool ThreadEngineBase::isCanceled()
{
    if (futureInterface)
        return futureInterface->isCanceled();
    else
        return false;
}

void ThreadEngineBase::waitForResume()
{
    if (futureInterface)
        futureInterface->waitForResume();
}

bool ThreadEngineBase::isProgressReportingEnabled()
{
    // If we don't have a QFuture, there is no-one to report the progress to.
    return (futureInterface != nullptr);
}

void ThreadEngineBase::setProgressValue(int progress)
{
    if (futureInterface)
        futureInterface->setProgressValue(progress);
}

void ThreadEngineBase::setProgressRange(int minimum, int maximum)
{
    if (futureInterface)
        futureInterface->setProgressRange(minimum, maximum);
}

bool ThreadEngineBase::startThreadInternal()
{
    if (this->isCanceled())
        return false;

    barrier.acquire();
    if (!threadPool->tryStart(this)) {
        barrier.release();
        return false;
    }
    return true;
}

void ThreadEngineBase::startThreads()
{
    while (shouldStartThread() && startThreadInternal())
        ;
}

void ThreadEngineBase::threadExit()
{
    const bool asynchronous = (futureInterface != nullptr);
    const int lastThread = (barrier.release() == 0);

    if (lastThread && asynchronous)
        this->asynchronousFinish();
}

// Called by a worker thread that wants to be throttled. If the current number
// of running threads is larger than one the thread is allowed to exit and
// this function returns one.
bool ThreadEngineBase::threadThrottleExit()
{
    return barrier.releaseUnlessLast();
}

void ThreadEngineBase::run() // implements QRunnable.
{
    if (this->isCanceled()) {
        threadExit();
        return;
    }

    startThreads();

#ifndef QT_NO_EXCEPTIONS
    try {
#endif
        while (threadFunction() == ThrottleThread) {
            // threadFunction returning ThrottleThread means it that the user
            // struct wants to be throttled by making a worker thread exit.
            // Respect that request unless this is the only worker thread left
            // running, in which case it has to keep going.
            if (threadThrottleExit()) {
                return;
            } else {
                // If the last worker thread is throttled and the state is "suspending",
                // it means that suspension has been requested, and it is already
                // in effect (because all previous threads have already exited).
                // Report the "Suspended" state.
                reportIfSuspensionDone();
            }
        }

#ifndef QT_NO_EXCEPTIONS
    } catch (QException &e) {
        handleException(e);
    } catch (...) {
        handleException(QUnhandledException(std::current_exception()));
    }
#endif
    threadExit();
}

#ifndef QT_NO_EXCEPTIONS

void ThreadEngineBase::handleException(const QException &exception)
{
    if (futureInterface) {
        futureInterface->reportException(exception);
    } else {
        QMutexLocker lock(&mutex);
        if (!exceptionStore.hasException())
            exceptionStore.setException(exception);
    }
}
#endif


} // namespace QtConcurrent

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT
