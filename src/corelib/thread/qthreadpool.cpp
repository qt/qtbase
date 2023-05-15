// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qthreadpool.h"
#include "qthreadpool_p.h"
#include "qdeadlinetimer.h"
#include "qcoreapplication.h"

#include <algorithm>
#include <memory>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*
    QThread wrapper, provides synchronization against a ThreadPool
*/
class QThreadPoolThread : public QThread
{
    Q_OBJECT
public:
    QThreadPoolThread(QThreadPoolPrivate *manager);
    void run() override;
    void registerThreadInactive();

    QWaitCondition runnableReady;
    QThreadPoolPrivate *manager;
    QRunnable *runnable;
};

/*
    QThreadPool private class.
*/


/*!
    \internal
*/
QThreadPoolThread::QThreadPoolThread(QThreadPoolPrivate *manager)
    :manager(manager), runnable(nullptr)
{
    setStackSize(manager->stackSize);
}

/*
    \internal
*/
void QThreadPoolThread::run()
{
    QMutexLocker locker(&manager->mutex);
    for(;;) {
        QRunnable *r = runnable;
        runnable = nullptr;

        do {
            if (r) {
                // If autoDelete() is false, r might already be deleted after run(), so check status now.
                const bool del = r->autoDelete();

                // run the task
                locker.unlock();
#ifndef QT_NO_EXCEPTIONS
                try {
#endif
                    r->run();
#ifndef QT_NO_EXCEPTIONS
                } catch (...) {
                    qWarning("Qt Concurrent has caught an exception thrown from a worker thread.\n"
                             "This is not supported, exceptions thrown in worker threads must be\n"
                             "caught before control returns to Qt Concurrent.");
                    registerThreadInactive();
                    throw;
                }
#endif

                if (del)
                    delete r;
                locker.relock();
            }

            // if too many threads are active, stop working in this one
            if (manager->tooManyThreadsActive())
                break;

            // all work is done, time to wait for more
            if (manager->queue.isEmpty())
                break;

            QueuePage *page = manager->queue.first();
            r = page->pop();

            if (page->isFinished()) {
                manager->queue.removeFirst();
                delete page;
            }
        } while (true);

        // this thread is about to be deleted, do not wait or expire
        if (!manager->allThreads.contains(this)) {
            registerThreadInactive();
            return;
        }

        // if too many threads are active, expire this thread
        if (manager->tooManyThreadsActive()) {
            manager->expiredThreads.enqueue(this);
            registerThreadInactive();
            return;
        }
        manager->waitingThreads.enqueue(this);
        registerThreadInactive();
        // wait for work, exiting after the expiry timeout is reached
        runnableReady.wait(locker.mutex(), QDeadlineTimer(manager->expiryTimeout));
        // this thread is about to be deleted, do not work or expire
        if (!manager->allThreads.contains(this)) {
            Q_ASSERT(manager->queue.isEmpty());
            return;
        }
        if (manager->waitingThreads.removeOne(this)) {
            manager->expiredThreads.enqueue(this);
            return;
        }
        ++manager->activeThreads;
    }
}

void QThreadPoolThread::registerThreadInactive()
{
    if (--manager->activeThreads == 0)
        manager->noActiveThreads.wakeAll();
}


/*
    \internal
*/
QThreadPoolPrivate:: QThreadPoolPrivate()
{ }

bool QThreadPoolPrivate::tryStart(QRunnable *task)
{
    Q_ASSERT(task != nullptr);
    if (allThreads.isEmpty()) {
        // always create at least one thread
        startThread(task);
        return true;
    }

    // can't do anything if we're over the limit
    if (areAllThreadsActive())
        return false;

    if (!waitingThreads.isEmpty()) {
        // recycle an available thread
        enqueueTask(task);
        waitingThreads.takeFirst()->runnableReady.wakeOne();
        return true;
    }

    if (!expiredThreads.isEmpty()) {
        // restart an expired thread
        QThreadPoolThread *thread = expiredThreads.dequeue();
        Q_ASSERT(thread->runnable == nullptr);

        ++activeThreads;

        thread->runnable = task;

        // Ensure that the thread has actually finished, otherwise the following
        // start() has no effect.
        thread->wait();
        Q_ASSERT(thread->isFinished());
        thread->start(threadPriority);
        return true;
    }

    // start a new thread
    startThread(task);
    return true;
}

inline bool comparePriority(int priority, const QueuePage *p)
{
    return p->priority() < priority;
}

void QThreadPoolPrivate::enqueueTask(QRunnable *runnable, int priority)
{
    Q_ASSERT(runnable != nullptr);
    for (QueuePage *page : std::as_const(queue)) {
        if (page->priority() == priority && !page->isFull()) {
            page->push(runnable);
            return;
        }
    }
    auto it = std::upper_bound(queue.constBegin(), queue.constEnd(), priority, comparePriority);
    queue.insert(std::distance(queue.constBegin(), it), new QueuePage(runnable, priority));
}

int QThreadPoolPrivate::activeThreadCount() const
{
    return (allThreads.size()
            - expiredThreads.size()
            - waitingThreads.size()
            + reservedThreads);
}

void QThreadPoolPrivate::tryToStartMoreThreads()
{
    // try to push tasks on the queue to any available threads
    while (!queue.isEmpty()) {
        QueuePage *page = queue.first();
        if (!tryStart(page->first()))
            break;

        page->pop();

        if (page->isFinished()) {
            queue.removeFirst();
            delete page;
        }
    }
}

bool QThreadPoolPrivate::areAllThreadsActive() const
{
    const int activeThreadCount = this->activeThreadCount();
    return activeThreadCount >= maxThreadCount() && (activeThreadCount - reservedThreads) >= 1;
}

bool QThreadPoolPrivate::tooManyThreadsActive() const
{
    const int activeThreadCount = this->activeThreadCount();
    return activeThreadCount > maxThreadCount() && (activeThreadCount - reservedThreads) > 1;
}

/*!
    \internal
*/
void QThreadPoolPrivate::startThread(QRunnable *runnable)
{
    Q_ASSERT(runnable != nullptr);
    auto thread = std::make_unique<QThreadPoolThread>(this);
    if (objectName.isEmpty())
        objectName = u"Thread (pooled)"_s;
    thread->setObjectName(objectName);
    Q_ASSERT(!allThreads.contains(thread.get())); // if this assert hits, we have an ABA problem (deleted threads don't get removed here)
    allThreads.insert(thread.get());
    ++activeThreads;

    thread->runnable = runnable;
    thread.release()->start(threadPriority);
}

/*!
    \internal

    Helper function only to be called from waitForDone(int)

    Deletes all current threads.
*/
void QThreadPoolPrivate::reset()
{
    // move the contents of the set out so that we can iterate without the lock
    auto allThreadsCopy = std::exchange(allThreads, {});
    expiredThreads.clear();
    waitingThreads.clear();

    mutex.unlock();

    for (QThreadPoolThread *thread : std::as_const(allThreadsCopy)) {
        if (thread->isRunning()) {
            thread->runnableReady.wakeAll();
            thread->wait();
        }
        delete thread;
    }

    mutex.lock();
}

/*!
    \internal

    Helper function only to be called from waitForDone(int)
*/
bool QThreadPoolPrivate::waitForDone(const QDeadlineTimer &timer)
{
    while (!(queue.isEmpty() && activeThreads == 0) && !timer.hasExpired())
        noActiveThreads.wait(&mutex, timer);

    return queue.isEmpty() && activeThreads == 0;
}

bool QThreadPoolPrivate::waitForDone(int msecs)
{
    QMutexLocker locker(&mutex);
    QDeadlineTimer timer(msecs);
    if (!waitForDone(timer))
        return false;
    reset();
    // New jobs might have started during reset, but return anyway
    // as the active thread and task count did reach 0 once, and
    // race conditions are outside our scope.
    return true;
}

void QThreadPoolPrivate::clear()
{
    QMutexLocker locker(&mutex);
    while (!queue.isEmpty()) {
        auto *page = queue.takeLast();
        while (!page->isFinished()) {
            QRunnable *r = page->pop();
            if (r && r->autoDelete()) {
                locker.unlock();
                delete r;
                locker.relock();
            }
        }
        delete page;
    }
}

/*!
    \since 5.9

    Attempts to remove the specified \a runnable from the queue if it is not yet started.
    If the runnable had not been started, returns \c true, and ownership of \a runnable
    is transferred to the caller (even when \c{runnable->autoDelete() == true}).
    Otherwise returns \c false.

    \note If \c{runnable->autoDelete() == true}, this function may remove the wrong
    runnable. This is known as the \l{https://en.wikipedia.org/wiki/ABA_problem}{ABA problem}:
    the original \a runnable may already have executed and has since been deleted.
    The memory is re-used for another runnable, which then gets removed instead of
    the intended one. For this reason, we recommend calling this function only for
    runnables that are not auto-deleting.

    \sa start(), QRunnable::autoDelete()
*/
bool QThreadPool::tryTake(QRunnable *runnable)
{
    Q_D(QThreadPool);

    if (runnable == nullptr)
        return false;

    QMutexLocker locker(&d->mutex);
    for (QueuePage *page : std::as_const(d->queue)) {
        if (page->tryTake(runnable)) {
            if (page->isFinished()) {
                d->queue.removeOne(page);
                delete page;
            }
            return true;
        }
    }

    return false;
}

    /*!
     \internal
     Searches for \a runnable in the queue, removes it from the queue and
     runs it if found. This function does not return until the runnable
     has completed.
     */
void QThreadPoolPrivate::stealAndRunRunnable(QRunnable *runnable)
{
    Q_Q(QThreadPool);
    if (!q->tryTake(runnable))
        return;
    // If autoDelete() is false, runnable might already be deleted after run(), so check status now.
    const bool del = runnable->autoDelete();

    runnable->run();

    if (del)
        delete runnable;
}

/*!
    \class QThreadPool
    \inmodule QtCore
    \brief The QThreadPool class manages a collection of QThreads.
    \since 4.4
    \threadsafe

    \ingroup thread

    QThreadPool manages and recycles individual QThread objects to help reduce
    thread creation costs in programs that use threads. Each Qt application
    has one global QThreadPool object, which can be accessed by calling
    globalInstance().

    To use one of the QThreadPool threads, subclass QRunnable and implement
    the run() virtual function. Then create an object of that class and pass
    it to QThreadPool::start().

    \snippet code/src_corelib_concurrent_qthreadpool.cpp 0

    QThreadPool deletes the QRunnable automatically by default. Use
    QRunnable::setAutoDelete() to change the auto-deletion flag.

    QThreadPool supports executing the same QRunnable more than once
    by calling tryStart(this) from within QRunnable::run().
    If autoDelete is enabled the QRunnable will be deleted when
    the last thread exits the run function. Calling start()
    multiple times with the same QRunnable when autoDelete is enabled
    creates a race condition and is not recommended.

    Threads that are unused for a certain amount of time will expire. The
    default expiry timeout is 30000 milliseconds (30 seconds). This can be
    changed using setExpiryTimeout(). Setting a negative expiry timeout
    disables the expiry mechanism.

    Call maxThreadCount() to query the maximum number of threads to be used.
    If needed, you can change the limit with setMaxThreadCount(). The default
    maxThreadCount() is QThread::idealThreadCount(). The activeThreadCount()
    function returns the number of threads currently doing work.

    The reserveThread() function reserves a thread for external
    use. Use releaseThread() when your are done with the thread, so
    that it may be reused.  Essentially, these functions temporarily
    increase or reduce the active thread count and are useful when
    implementing time-consuming operations that are not visible to the
    QThreadPool.

    Note that QThreadPool is a low-level class for managing threads, see
    the Qt Concurrent module for higher level alternatives.

    \sa QRunnable
*/

/*!
    Constructs a thread pool with the given \a parent.
*/
QThreadPool::QThreadPool(QObject *parent)
    : QObject(*new QThreadPoolPrivate, parent)
{
    Q_D(QThreadPool);
    connect(this, &QObject::objectNameChanged, this, [d](const QString &newName) {
        // We keep a copy of the name under our own lock, so we can access it thread-safely.
        QMutexLocker locker(&d->mutex);
        d->objectName = newName;
    });
}

/*!
    Destroys the QThreadPool.
    This function will block until all runnables have been completed.
*/
QThreadPool::~QThreadPool()
{
    Q_D(QThreadPool);
    waitForDone();
    Q_ASSERT(d->queue.isEmpty());
    Q_ASSERT(d->allThreads.isEmpty());
}

/*!
    Returns the global QThreadPool instance.
*/
QThreadPool *QThreadPool::globalInstance()
{
    Q_CONSTINIT static QPointer<QThreadPool> theInstance;
    Q_CONSTINIT static QBasicMutex theMutex;

    const QMutexLocker locker(&theMutex);
    if (theInstance.isNull() && !QCoreApplication::closingDown())
        theInstance = new QThreadPool();
    return theInstance;
}

/*!
    Returns the QThreadPool instance for Qt Gui.
    \internal
*/
QThreadPool *QThreadPoolPrivate::qtGuiInstance()
{
    Q_CONSTINIT static QPointer<QThreadPool> guiInstance;
    Q_CONSTINIT static QBasicMutex theMutex;

    const QMutexLocker locker(&theMutex);
    if (guiInstance.isNull() && !QCoreApplication::closingDown())
        guiInstance = new QThreadPool();
    return guiInstance;
}

/*!
    Reserves a thread and uses it to run \a runnable, unless this thread will
    make the current thread count exceed maxThreadCount().  In that case,
    \a runnable is added to a run queue instead. The \a priority argument can
    be used to control the run queue's order of execution.

    Note that the thread pool takes ownership of the \a runnable if
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c true,
    and the \a runnable will be deleted automatically by the thread
    pool after the \l{QRunnable::run()}{runnable->run()} returns. If
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c false,
    ownership of \a runnable remains with the caller. Note that
    changing the auto-deletion on \a runnable after calling this
    functions results in undefined behavior.
*/
void QThreadPool::start(QRunnable *runnable, int priority)
{
    if (!runnable)
        return;

    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);

    if (!d->tryStart(runnable))
        d->enqueueTask(runnable, priority);
}

/*!
    \fn template<typename Callable, QRunnable::if_callable<Callable>> void QThreadPool::start(Callable &&callableToRun, int priority)
    \overload
    \since 5.15

    Reserves a thread and uses it to run \a callableToRun, unless this thread will
    make the current thread count exceed maxThreadCount().  In that case,
    \a callableToRun is added to a run queue instead. The \a priority argument can
    be used to control the run queue's order of execution.

    \note This function participates in overload resolution only if \c Callable
    is a function or function object which can be called with zero arguments.

    \note In Qt version prior to 6.6, this function took std::function<void()>,
    and therefore couldn't handle move-only callables.
*/

/*!
    Attempts to reserve a thread to run \a runnable.

    If no threads are available at the time of calling, then this function
    does nothing and returns \c false.  Otherwise, \a runnable is run immediately
    using one available thread and this function returns \c true.

    Note that on success the thread pool takes ownership of the \a runnable if
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c true,
    and the \a runnable will be deleted automatically by the thread
    pool after the \l{QRunnable::run()}{runnable->run()} returns. If
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c false,
    ownership of \a runnable remains with the caller. Note that
    changing the auto-deletion on \a runnable after calling this
    function results in undefined behavior.
*/
bool QThreadPool::tryStart(QRunnable *runnable)
{
    if (!runnable)
        return false;

    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    if (d->tryStart(runnable))
        return true;

    return false;
}

/*!
    \fn template<typename Callable, QRunnable::if_callable<Callable>> bool QThreadPool::tryStart(Callable &&callableToRun)
    \overload
    \since 5.15
    Attempts to reserve a thread to run \a callableToRun.

    If no threads are available at the time of calling, then this function
    does nothing and returns \c false.  Otherwise, \a callableToRun is run immediately
    using one available thread and this function returns \c true.

    \note This function participates in overload resolution only if \c Callable
    is a function or function object which can be called with zero arguments.

    \note In Qt version prior to 6.6, this function took std::function<void()>,
    and therefore couldn't handle move-only callables.
*/

/*! \property QThreadPool::expiryTimeout
    \brief the thread expiry timeout value in milliseconds.

    Threads that are unused for \e expiryTimeout milliseconds are considered
    to have expired and will exit. Such threads will be restarted as needed.
    The default \a expiryTimeout is 30000 milliseconds (30 seconds). If
    \a expiryTimeout is negative, newly created threads will not expire, e.g.,
    they will not exit until the thread pool is destroyed.

    Note that setting \a expiryTimeout has no effect on already running
    threads. Only newly created threads will use the new \a expiryTimeout.
    We recommend setting the \a expiryTimeout immediately after creating the
    thread pool, but before calling start().
*/

int QThreadPool::expiryTimeout() const
{
    Q_D(const QThreadPool);
    QMutexLocker locker(&d->mutex);
    return d->expiryTimeout;
}

void QThreadPool::setExpiryTimeout(int expiryTimeout)
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    if (d->expiryTimeout == expiryTimeout)
        return;
    d->expiryTimeout = expiryTimeout;
}

/*! \property QThreadPool::maxThreadCount

    \brief the maximum number of threads used by the thread pool. This property
    will default to the value of QThread::idealThreadCount() at the moment the
    QThreadPool object is created.

    \note The thread pool will always use at least 1 thread, even if
    \a maxThreadCount limit is zero or negative.

    The default \a maxThreadCount is QThread::idealThreadCount().
*/

int QThreadPool::maxThreadCount() const
{
    Q_D(const QThreadPool);
    QMutexLocker locker(&d->mutex);
    return d->requestedMaxThreadCount;
}

void QThreadPool::setMaxThreadCount(int maxThreadCount)
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);

    if (maxThreadCount == d->requestedMaxThreadCount)
        return;

    d->requestedMaxThreadCount = maxThreadCount;
    d->tryToStartMoreThreads();
}

/*! \property QThreadPool::activeThreadCount

    \brief the number of active threads in the thread pool.

    \note It is possible for this function to return a value that is greater
    than maxThreadCount(). See reserveThread() for more details.

    \sa reserveThread(), releaseThread()
*/

int QThreadPool::activeThreadCount() const
{
    Q_D(const QThreadPool);
    QMutexLocker locker(&d->mutex);
    return d->activeThreadCount();
}

/*!
    Reserves one thread, disregarding activeThreadCount() and maxThreadCount().

    Once you are done with the thread, call releaseThread() to allow it to be
    reused.

    \note Even if reserving maxThreadCount() threads or more, the thread pool
    will still allow a minimum of one thread.

    \note This function will increase the reported number of active threads.
    This means that by using this function, it is possible for
    activeThreadCount() to return a value greater than maxThreadCount() .

    \sa releaseThread()
 */
void QThreadPool::reserveThread()
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    ++d->reservedThreads;
}

/*! \property QThreadPool::stackSize
    \brief the stack size for the thread pool worker threads.

    The value of the property is only used when the thread pool creates
    new threads. Changing it has no effect for already created
    or running threads.

    The default value is 0, which makes QThread use the operating
    system default stack size.

    \since 5.10
*/
void QThreadPool::setStackSize(uint stackSize)
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    d->stackSize = stackSize;
}

uint QThreadPool::stackSize() const
{
    Q_D(const QThreadPool);
    QMutexLocker locker(&d->mutex);
    return d->stackSize;
}

/*! \property QThreadPool::threadPriority
    \brief the thread priority for new worker threads.

    The value of the property is only used when the thread pool starts
    new threads. Changing it has no effect for already running threads.

    The default value is QThread::InheritPriority, which makes QThread
    use the same priority as the one the QThreadPool object lives in.

    \sa QThread::Priority

    \since 6.2
*/

void QThreadPool::setThreadPriority(QThread::Priority priority)
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    d->threadPriority = priority;
}

QThread::Priority QThreadPool::threadPriority() const
{
    Q_D(const QThreadPool);
    QMutexLocker locker(&d->mutex);
    return d->threadPriority;
}

/*!
    Releases a thread previously reserved by a call to reserveThread().

    \note Calling this function without previously reserving a thread
    temporarily increases maxThreadCount(). This is useful when a
    thread goes to sleep waiting for more work, allowing other threads
    to continue. Be sure to call reserveThread() when done waiting, so
    that the thread pool can correctly maintain the
    activeThreadCount().

    \sa reserveThread()
*/
void QThreadPool::releaseThread()
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    --d->reservedThreads;
    d->tryToStartMoreThreads();
}

/*!
    Releases a thread previously reserved with reserveThread() and uses it
    to run \a runnable.

    Note that the thread pool takes ownership of the \a runnable if
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c true,
    and the \a runnable will be deleted automatically by the thread
    pool after the \l{QRunnable::run()}{runnable->run()} returns. If
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c false,
    ownership of \a runnable remains with the caller. Note that
    changing the auto-deletion on \a runnable after calling this
    functions results in undefined behavior.

    \note Calling this when no threads are reserved results in
    undefined behavior.

    \since 6.3
    \sa reserveThread(), start()
*/
void QThreadPool::startOnReservedThread(QRunnable *runnable)
{
    if (!runnable)
        return releaseThread();

    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    Q_ASSERT(d->reservedThreads > 0);
    --d->reservedThreads;

    if (!d->tryStart(runnable)) {
        // This can only happen if we reserved max threads,
        // and something took the one minimum thread.
        d->enqueueTask(runnable, INT_MAX);
    }
}

/*!
    \fn template<typename Callable, QRunnable::if_callable<Callable>> void QThreadPool::startOnReservedThread(Callable &&callableToRun)
    \overload
    \since 6.3

    Releases a thread previously reserved with reserveThread() and uses it
    to run \a callableToRun.

    \note This function participates in overload resolution only if \c Callable
    is a function or function object which can be called with zero arguments.

    \note In Qt version prior to 6.6, this function took std::function<void()>,
    and therefore couldn't handle move-only callables.
*/

/*!
    Waits up to \a msecs milliseconds for all threads to exit and removes all
    threads from the thread pool. Returns \c true if all threads were removed;
    otherwise it returns \c false. If \a msecs is -1 (the default), the timeout
    is ignored (waits for the last thread to exit).
*/
bool QThreadPool::waitForDone(int msecs)
{
    Q_D(QThreadPool);
    return d->waitForDone(msecs);
}

/*!
    \since 5.2

    Removes the runnables that are not yet started from the queue.
    The runnables for which \l{QRunnable::autoDelete()}{runnable->autoDelete()}
    returns \c true are deleted.

    \sa start()
*/
void QThreadPool::clear()
{
    Q_D(QThreadPool);
    d->clear();
}

/*!
    \since 6.0

    Returns \c true if \a thread is a thread managed by this thread pool.
*/
bool QThreadPool::contains(const QThread *thread) const
{
    Q_D(const QThreadPool);
    const QThreadPoolThread *poolThread = qobject_cast<const QThreadPoolThread *>(thread);
    if (!poolThread)
        return false;
    QMutexLocker locker(&d->mutex);
    return d->allThreads.contains(const_cast<QThreadPoolThread *>(poolThread));
}

QT_END_NAMESPACE

#include "moc_qthreadpool.cpp"
#include "qthreadpool.moc"
