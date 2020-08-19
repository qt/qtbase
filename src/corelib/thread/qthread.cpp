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
#include "qthreadstorage.h"
#include "qmutex.h"
#include "qreadwritelock.h"
#include "qabstracteventdispatcher.h"

#include <qeventloop.h>

#include "qthread_p.h"
#include "private/qcoreapplication_p.h"

#include <limits>

QT_BEGIN_NAMESPACE

/*
  QThreadData
*/

QThreadData::QThreadData(int initialRefCount)
    : _ref(initialRefCount), loopLevel(0), scopeLevel(0),
      eventDispatcher(nullptr),
      quitNow(false), canWait(true), isAdopted(false), requiresCoreApplication(true)
{
    // fprintf(stderr, "QThreadData %p created\n", this);
}

QThreadData::~QThreadData()
{
#if QT_CONFIG(thread)
    Q_ASSERT(_ref.loadRelaxed() == 0);
#endif

    // In the odd case that Qt is running on a secondary thread, the main
    // thread instance will have been dereffed asunder because of the deref in
    // QThreadData::current() and the deref in the pthread_destroy. To avoid
    // crashing during QCoreApplicationData's global static cleanup we need to
    // safeguard the main thread here.. This fix is a bit crude, but it solves
    // the problem...
    if (this->thread.loadAcquire() == QCoreApplicationPrivate::theMainThread.loadAcquire()) {
       QCoreApplicationPrivate::theMainThread.storeRelease(nullptr);
       QThreadData::clearCurrentThreadData();
    }

    // ~QThread() sets thread to nullptr, so if it isn't null here, it's
    // because we're being run before the main object itself. This can only
    // happen for QAdoptedThread. Note that both ~QThreadPrivate() and
    // ~QObjectPrivate() will deref this object again, but that is acceptable
    // because this destructor is still running (the _ref sub-object has not
    // been destroyed) and there's no reentrancy. The refcount will become
    // negative, but that's acceptable.
    QThread *t = thread.loadAcquire();
    thread.storeRelease(nullptr);
    delete t;

    for (int i = 0; i < postEventList.size(); ++i) {
        const QPostEvent &pe = postEventList.at(i);
        if (pe.event) {
            --pe.receiver->d_func()->postedEvents;
            pe.event->posted = false;
            delete pe.event;
        }
    }

    // fprintf(stderr, "QThreadData %p destroyed\n", this);
}

void QThreadData::ref()
{
#if QT_CONFIG(thread)
    (void) _ref.ref();
    Q_ASSERT(_ref.loadRelaxed() != 0);
#endif
}

void QThreadData::deref()
{
#if QT_CONFIG(thread)
    if (!_ref.deref())
        delete this;
#endif
}

QAbstractEventDispatcher *QThreadData::createEventDispatcher()
{
    QAbstractEventDispatcher *ed = QThreadPrivate::createEventDispatcher(this);
    eventDispatcher.storeRelease(ed);
    ed->startingUp();
    return ed;
}

/*
  QAdoptedThread
*/

QAdoptedThread::QAdoptedThread(QThreadData *data)
    : QThread(*new QThreadPrivate(data))
{
    // thread should be running and not finished for the lifetime
    // of the application (even if QCoreApplication goes away)
#if QT_CONFIG(thread)
    d_func()->running = true;
    d_func()->finished = false;
    init();
#endif

    // fprintf(stderr, "new QAdoptedThread = %p\n", this);
}

QAdoptedThread::~QAdoptedThread()
{
    // fprintf(stderr, "~QAdoptedThread = %p\n", this);
}

#if QT_CONFIG(thread)
void QAdoptedThread::run()
{
    // this function should never be called
    qFatal("QAdoptedThread::run(): Internal error, this implementation should never be called.");
}

/*
  QThreadPrivate
*/

QThreadPrivate::QThreadPrivate(QThreadData *d)
    : QObjectPrivate(), running(false), finished(false),
      isInFinish(false), interruptionRequested(false),
      exited(false), returnCode(-1),
      stackSize(0), priority(QThread::InheritPriority), data(d)
{

// INTEGRITY doesn't support self-extending stack. The default stack size for
// a pthread on INTEGRITY is too small so we have to increase the default size
// to 128K.
#ifdef Q_OS_INTEGRITY
    stackSize = 128 * 1024;
#elif defined(Q_OS_RTEMS)
    static bool envStackSizeOk = false;
    static const int envStackSize = qEnvironmentVariableIntValue("QT_DEFAULT_THREAD_STACK_SIZE", &envStackSizeOk);
    if (envStackSizeOk)
        stackSize = envStackSize;
#endif

#if defined (Q_OS_WIN)
    handle = 0;
#  ifndef Q_OS_WINRT
    id = 0;
#  endif
    waiters = 0;
    terminationEnabled = true;
    terminatePending = false;
#endif

    if (!data)
        data = new QThreadData;
}

QThreadPrivate::~QThreadPrivate()
{
    data->deref();
}

/*!
    \class QThread
    \inmodule QtCore
    \brief The QThread class provides a platform-independent way to
    manage threads.

    \ingroup thread

    A QThread object manages one thread of control within the
    program. QThreads begin executing in run(). By default, run() starts the
    event loop by calling exec() and runs a Qt event loop inside the thread.

    You can use worker objects by moving them to the thread using
    QObject::moveToThread().

    \snippet code/src_corelib_thread_qthread.cpp worker

    The code inside the Worker's slot would then execute in a
    separate thread. However, you are free to connect the
    Worker's slots to any signal, from any object, in any thread. It
    is safe to connect signals and slots across different threads,
    thanks to a mechanism called \l{Qt::QueuedConnection}{queued
    connections}.

    Another way to make code run in a separate thread, is to subclass QThread
    and reimplement run(). For example:

    \snippet code/src_corelib_thread_qthread.cpp reimpl-run

    In that example, the thread will exit after the run function has returned.
    There will not be any event loop running in the thread unless you call
    exec().

    It is important to remember that a QThread instance \l{QObject#Thread
    Affinity}{lives in} the old thread that instantiated it, not in the
    new thread that calls run(). This means that all of QThread's queued
    slots and \l {QMetaObject::invokeMethod()}{invoked methods} will execute
    in the old thread. Thus, a developer who wishes to invoke slots in the
    new thread must use the worker-object approach; new slots should not be
    implemented directly into a subclassed QThread.

    Unlike queued slots or invoked methods, methods called directly on the
    QThread object will execute in the thread that calls the method. When
    subclassing QThread, keep in mind that the constructor executes in the
    old thread while run() executes in the new thread. If a member variable
    is accessed from both functions, then the variable is accessed from two
    different threads. Check that it is safe to do so.

    \note Care must be taken when interacting with objects across different
    threads. As a general rule, functions can only be called from the thread
    that created the QThread object itself (e.g. setPriority()), unless the
    documentation says otherwise. See \l{Synchronizing Threads} for details.

    \section1 Managing Threads

    QThread will notifiy you via a signal when the thread is
    started() and finished(), or you can use isFinished() and
    isRunning() to query the state of the thread.

    You can stop the thread by calling exit() or quit(). In extreme
    cases, you may want to forcibly terminate() an executing thread.
    However, doing so is dangerous and discouraged. Please read the
    documentation for terminate() and setTerminationEnabled() for
    detailed information.

    From Qt 4.8 onwards, it is possible to deallocate objects that
    live in a thread that has just ended, by connecting the
    finished() signal to QObject::deleteLater().

    Use wait() to block the calling thread, until the other thread
    has finished execution (or until a specified time has passed).

    QThread also provides static, platform independent sleep
    functions: sleep(), msleep(), and usleep() allow full second,
    millisecond, and microsecond resolution respectively. These
    functions were made public in Qt 5.0.

    \note wait() and the sleep() functions should be unnecessary in
    general, since Qt is an event-driven framework. Instead of
    wait(), consider listening for the finished() signal. Instead of
    the sleep() functions, consider using QTimer.

    The static functions currentThreadId() and currentThread() return
    identifiers for the currently executing thread. The former
    returns a platform specific ID for the thread; the latter returns
    a QThread pointer.

    To choose the name that your thread will be given (as identified
    by the command \c{ps -L} on Linux, for example), you can call
    \l{QObject::setObjectName()}{setObjectName()} before starting the thread.
    If you don't call \l{QObject::setObjectName()}{setObjectName()},
    the name given to your thread will be the class name of the runtime
    type of your thread object (for example, \c "RenderThread" in the case of the
    \l{Mandelbrot Example}, as that is the name of the QThread subclass).
    Note that this is currently not available with release builds on Windows.

    \sa {Thread Support in Qt}, QThreadStorage, {Synchronizing Threads},
        {Mandelbrot Example}, {Semaphores Example}, {Wait Conditions Example}
*/

/*!
    \fn Qt::HANDLE QThread::currentThreadId()

    Returns the thread handle of the currently executing thread.

    \warning The handle returned by this function is used for internal
    purposes and should not be used in any application code.

    \note On Windows, this function returns the DWORD (Windows-Thread
    ID) returned by the Win32 function GetCurrentThreadId(), not the pseudo-HANDLE
    (Windows-Thread HANDLE) returned by the Win32 function GetCurrentThread().
*/

/*!
    \fn int QThread::idealThreadCount()

    Returns the ideal number of threads that can be run on the system. This is done querying
    the number of processor cores, both real and logical, in the system. This function returns 1
    if the number of processor cores could not be detected.
*/

/*!
    \fn void QThread::yieldCurrentThread()

    Yields execution of the current thread to another runnable thread,
    if any. Note that the operating system decides to which thread to
    switch.
*/

/*!
    \fn void QThread::start(Priority priority)

    Begins execution of the thread by calling run(). The
    operating system will schedule the thread according to the \a
    priority parameter. If the thread is already running, this
    function does nothing.

    The effect of the \a priority parameter is dependent on the
    operating system's scheduling policy. In particular, the \a priority
    will be ignored on systems that do not support thread priorities
    (such as on Linux, see the
    \l {http://linux.die.net/man/2/sched_setscheduler}{sched_setscheduler}
    documentation for more details).

    \sa run(), terminate()
*/

/*!
    \fn void QThread::started()

    This signal is emitted from the associated thread when it starts executing,
    before the run() function is called.

    \sa finished()
*/

/*!
    \fn void QThread::finished()

    This signal is emitted from the associated thread right before it finishes executing.

    When this signal is emitted, the event loop has already stopped running.
    No more events will be processed in the thread, except for deferred deletion events.
    This signal can be connected to QObject::deleteLater(), to free objects in that thread.

    \note If the associated thread was terminated using terminate(), it is undefined from
    which thread this signal is emitted.

    \sa started()
*/

/*!
    \enum QThread::Priority

    This enum type indicates how the operating system should schedule
    newly created threads.

    \value IdlePriority scheduled only when no other threads are
           running.

    \value LowestPriority scheduled less often than LowPriority.
    \value LowPriority scheduled less often than NormalPriority.

    \value NormalPriority the default priority of the operating
           system.

    \value HighPriority scheduled more often than NormalPriority.
    \value HighestPriority scheduled more often than HighPriority.

    \value TimeCriticalPriority scheduled as often as possible.

    \value InheritPriority use the same priority as the creating
           thread. This is the default.
*/

/*!
    Returns a pointer to a QThread which manages the currently
    executing thread.
*/
QThread *QThread::currentThread()
{
    QThreadData *data = QThreadData::current();
    Q_ASSERT(data != nullptr);
    return data->thread.loadAcquire();
}

/*!
    Constructs a new QThread to manage a new thread. The \a parent
    takes ownership of the QThread. The thread does not begin
    executing until start() is called.

    \sa start()
*/
QThread::QThread(QObject *parent)
    : QObject(*(new QThreadPrivate), parent)
{
    Q_D(QThread);
    // fprintf(stderr, "QThreadData %p created for thread %p\n", d->data, this);
    d->data->thread.storeRelaxed(this);
}

/*!
    \internal
 */
QThread::QThread(QThreadPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QThread);
    // fprintf(stderr, "QThreadData %p taken from private data for thread %p\n", d->data, this);
    d->data->thread.storeRelaxed(this);
}

/*!
    Destroys the QThread.

    Note that deleting a QThread object will not stop the execution
    of the thread it manages. Deleting a running QThread (i.e.
    isFinished() returns \c false) will result in a program
    crash. Wait for the finished() signal before deleting the
    QThread.
*/
QThread::~QThread()
{
    Q_D(QThread);
    {
        QMutexLocker locker(&d->mutex);
        if (d->isInFinish) {
            locker.unlock();
            wait();
            locker.relock();
        }
        if (d->running && !d->finished && !d->data->isAdopted)
            qFatal("QThread: Destroyed while thread is still running");

        d->data->thread.storeRelease(nullptr);
    }
}

/*!
    \threadsafe
    Returns \c true if the thread is finished; otherwise returns \c false.

    \sa isRunning()
*/
bool QThread::isFinished() const
{
    Q_D(const QThread);
    QMutexLocker locker(&d->mutex);
    return d->finished || d->isInFinish;
}

/*!
    \threadsafe
    Returns \c true if the thread is running; otherwise returns \c false.

    \sa isFinished()
*/
bool QThread::isRunning() const
{
    Q_D(const QThread);
    QMutexLocker locker(&d->mutex);
    return d->running && !d->isInFinish;
}

/*!
    Sets the maximum stack size for the thread to \a stackSize. If \a
    stackSize is greater than zero, the maximum stack size is set to
    \a stackSize bytes, otherwise the maximum stack size is
    automatically determined by the operating system.

    \warning Most operating systems place minimum and maximum limits
    on thread stack sizes. The thread will fail to start if the stack
    size is outside these limits.

    \sa stackSize()
*/
void QThread::setStackSize(uint stackSize)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    Q_ASSERT_X(!d->running, "QThread::setStackSize",
               "cannot change stack size while the thread is running");
    d->stackSize = stackSize;
}

/*!
    Returns the maximum stack size for the thread (if set with
    setStackSize()); otherwise returns zero.

    \sa setStackSize()
*/
uint QThread::stackSize() const
{
    Q_D(const QThread);
    QMutexLocker locker(&d->mutex);
    return d->stackSize;
}

/*!
    Enters the event loop and waits until exit() is called, returning the value
    that was passed to exit(). The value returned is 0 if exit() is called via
    quit().

    This function is meant to be called from within run(). It is necessary to
    call this function to start event handling.

    \note This can only be called within the thread itself, i.e. when
    it is the current thread.

    \sa quit(), exit()
*/
int QThread::exec()
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    d->data->quitNow = false;
    if (d->exited) {
        d->exited = false;
        return d->returnCode;
    }
    locker.unlock();

    QEventLoop eventLoop;
    int returnCode = eventLoop.exec();

    locker.relock();
    d->exited = false;
    d->returnCode = -1;
    return returnCode;
}

/*!
    \threadsafe
    Tells the thread's event loop to exit with a return code.

    After calling this function, the thread leaves the event loop and
    returns from the call to QEventLoop::exec(). The
    QEventLoop::exec() function returns \a returnCode.

    By convention, a \a returnCode of 0 means success, any non-zero value
    indicates an error.

    Note that unlike the C library function of the same name, this
    function \e does return to the caller -- it is event processing
    that stops.

    No QEventLoops will be started anymore in this thread  until
    QThread::exec() has been called again. If the eventloop in QThread::exec()
    is not running then the next call to QThread::exec() will also return
    immediately.

    \sa quit(), QEventLoop
*/
void QThread::exit(int returnCode)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    d->exited = true;
    d->returnCode = returnCode;
    d->data->quitNow = true;
    for (int i = 0; i < d->data->eventLoops.size(); ++i) {
        QEventLoop *eventLoop = d->data->eventLoops.at(i);
        eventLoop->exit(returnCode);
    }
}

/*!
    \threadsafe
    Tells the thread's event loop to exit with return code 0 (success).
    Equivalent to calling QThread::exit(0).

    This function does nothing if the thread does not have an event
    loop.

    \sa exit(), QEventLoop
*/
void QThread::quit()
{ exit(); }

/*!
    The starting point for the thread. After calling start(), the
    newly created thread calls this function. The default
    implementation simply calls exec().

    You can reimplement this function to facilitate advanced thread
    management. Returning from this method will end the execution of
    the thread.

    \sa start(), wait()
*/
void QThread::run()
{
    (void) exec();
}

/*! \fn void QThread::setPriority(Priority priority)
    \since 4.1

    This function sets the \a priority for a running thread. If the
    thread is not running, this function does nothing and returns
    immediately.  Use start() to start a thread with a specific
    priority.

    The \a priority argument can be any value in the \c
    QThread::Priority enum except for \c InheritPriority.

    The effect of the \a priority parameter is dependent on the
    operating system's scheduling policy. In particular, the \a priority
    will be ignored on systems that do not support thread priorities
    (such as on Linux, see http://linux.die.net/man/2/sched_setscheduler
    for more details).

    \sa Priority, priority(), start()
*/
void QThread::setPriority(Priority priority)
{
    if (priority == QThread::InheritPriority) {
        qWarning("QThread::setPriority: Argument cannot be InheritPriority");
        return;
    }
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    if (!d->running) {
        qWarning("QThread::setPriority: Cannot set priority, thread is not running");
        return;
    }
    d->setPriority(priority);
}

/*!
    \since 4.1

    Returns the priority for a running thread.  If the thread is not
    running, this function returns \c InheritPriority.

    \sa Priority, setPriority(), start()
*/
QThread::Priority QThread::priority() const
{
    Q_D(const QThread);
    QMutexLocker locker(&d->mutex);

    // mask off the high bits that are used for flags
    return Priority(d->priority & 0xffff);
}

/*!
    \fn void QThread::sleep(unsigned long secs)

    Forces the current thread to sleep for \a secs seconds.

    Avoid using this function if you need to wait for a given condition to
    change. Instead, connect a slot to the signal that indicates the change or
    use an event handler (see \l QObject::event()).

    \note This function does not guarantee accuracy. The application may sleep
    longer than \a secs under heavy load conditions.

    \sa msleep(), usleep()
*/

/*!
    \fn void QThread::msleep(unsigned long msecs)

    Forces the current thread to sleep for \a msecs milliseconds.

    Avoid using this function if you need to wait for a given condition to
    change. Instead, connect a slot to the signal that indicates the change or
    use an event handler (see \l QObject::event()).

    \note This function does not guarantee accuracy. The application may sleep
    longer than \a msecs under heavy load conditions. Some OSes might round \a
    msecs up to 10 ms or 15 ms.

    \sa sleep(), usleep()
*/

/*!
    \fn void QThread::usleep(unsigned long usecs)

    Forces the current thread to sleep for \a usecs microseconds.

    Avoid using this function if you need to wait for a given condition to
    change. Instead, connect a slot to the signal that indicates the change or
    use an event handler (see \l QObject::event()).

    \note This function does not guarantee accuracy. The application may sleep
    longer than \a usecs under heavy load conditions. Some OSes might round \a
    usecs up to 10 ms or 15 ms; on Windows, it will be rounded up to a multiple
    of 1 ms.

    \sa sleep(), msleep()
*/

/*!
    \fn void QThread::terminate()
    \threadsafe

    Terminates the execution of the thread. The thread may or may not
    be terminated immediately, depending on the operating system's
    scheduling policies. Use QThread::wait() after terminate(), to be
    sure.

    When the thread is terminated, all threads waiting for the thread
    to finish will be woken up.

    \warning This function is dangerous and its use is discouraged.
    The thread can be terminated at any point in its code path.
    Threads can be terminated while modifying data. There is no
    chance for the thread to clean up after itself, unlock any held
    mutexes, etc. In short, use this function only if absolutely
    necessary.

    Termination can be explicitly enabled or disabled by calling
    QThread::setTerminationEnabled(). Calling this function while
    termination is disabled results in the termination being
    deferred, until termination is re-enabled. See the documentation
    of QThread::setTerminationEnabled() for more information.

    \sa setTerminationEnabled()
*/

/*!
    \fn bool QThread::wait(QDeadlineTimer deadline)
    \since 5.15

    Blocks the thread until either of these conditions is met:

    \list
    \li The thread associated with this QThread object has finished
       execution (i.e. when it returns from \l{run()}). This function
       will return true if the thread has finished. It also returns
       true if the thread has not been started yet.
    \li The \a deadline is reached. This function will return false if the
       deadline is reached.
    \endlist

    A deadline timer set to \c QDeadlineTimer::Forever (the default) will never
    time out: in this case, the function only returns when the thread returns
    from \l{run()} or if the thread has not yet started.

    This provides similar functionality to the POSIX \c
    pthread_join() function.

    \sa sleep(), terminate()
*/

/*!
    \fn void QThread::setTerminationEnabled(bool enabled)

    Enables or disables termination of the current thread based on the
    \a enabled parameter. The thread must have been started by
    QThread.

    When \a enabled is false, termination is disabled.  Future calls
    to QThread::terminate() will return immediately without effect.
    Instead, the termination is deferred until termination is enabled.

    When \a enabled is true, termination is enabled.  Future calls to
    QThread::terminate() will terminate the thread normally.  If
    termination has been deferred (i.e. QThread::terminate() was
    called with termination disabled), this function will terminate
    the calling thread \e immediately.  Note that this function will
    not return in this case.

    \sa terminate()
*/

/*!
    \since 5.5
    Returns the current event loop level for the thread.

    \note This can only be called within the thread itself, i.e. when
    it is the current thread.
*/

int QThread::loopLevel() const
{
    Q_D(const QThread);
    return d->data->eventLoops.size();
}

#else // QT_CONFIG(thread)

QThread::QThread(QObject *parent)
    : QObject(*(new QThreadPrivate), parent)
{
    Q_D(QThread);
    d->data->thread.storeRelaxed(this);
}

QThread::~QThread()
{

}

void QThread::run()
{

}

int QThread::exec()
{
    return 0;
}

void QThread::start(Priority priority)
{
    Q_D(QThread);
    Q_UNUSED(priority);
    d->running = true;
}

void QThread::terminate()
{

}

void QThread::quit()
{

}

void QThread::exit(int returnCode)
{
    Q_D(QThread);
    d->data->quitNow = true;
    for (int i = 0; i < d->data->eventLoops.size(); ++i) {
        QEventLoop *eventLoop = d->data->eventLoops.at(i);
        eventLoop->exit(returnCode);
    }
}

bool QThread::wait(QDeadlineTimer deadline)
{
    Q_UNUSED(deadline);
    return false;
}

bool QThread::event(QEvent* event)
{
    return QObject::event(event);
}

Qt::HANDLE QThread::currentThreadId() noexcept
{
    return Qt::HANDLE(currentThread());
}

QThread *QThread::currentThread()
{
    return QThreadData::current()->thread.loadAcquire();
}

int QThread::idealThreadCount() noexcept
{
    return 1;
}

void QThread::yieldCurrentThread()
{

}

bool QThread::isFinished() const
{
    return false;
}

bool QThread::isRunning() const
{
    Q_D(const QThread);
    return d->running;
}

// No threads: so we can just use static variables
static QThreadData *data = 0;

QThreadData *QThreadData::current(bool createIfNecessary)
{
    if (!data && createIfNecessary) {
        data = new QThreadData;
        data->thread = new QAdoptedThread(data);
        data->threadId.storeRelaxed(Qt::HANDLE(data->thread.loadAcquire()));
        data->deref();
        data->isAdopted = true;
        if (!QCoreApplicationPrivate::theMainThread.loadAcquire())
            QCoreApplicationPrivate::theMainThread.storeRelease(data->thread.loadRelaxed());
    }
    return data;
}

void QThreadData::clearCurrentThreadData()
{
    delete data;
    data = 0;
}

/*!
    \internal
 */
QThread::QThread(QThreadPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QThread);
    // fprintf(stderr, "QThreadData %p taken from private data for thread %p\n", d->data, this);
    d->data->thread.storeRelaxed(this);
}

QThreadPrivate::QThreadPrivate(QThreadData *d) : data(d ? d : new QThreadData)
{
}

QThreadPrivate::~QThreadPrivate()
{
    data->thread.storeRelease(nullptr); // prevent QThreadData from deleting the QThreadPrivate (again).
    delete data;
}

void QThread::setStackSize(uint stackSize)
{
    Q_UNUSED(stackSize);
}

uint QThread::stackSize() const
{
    return 0;
}

#endif // QT_CONFIG(thread)

/*!
    \since 5.0

    Returns a pointer to the event dispatcher object for the thread. If no event
    dispatcher exists for the thread, this function returns \nullptr.
*/
QAbstractEventDispatcher *QThread::eventDispatcher() const
{
    Q_D(const QThread);
    return d->data->eventDispatcher.loadRelaxed();
}

/*!
    \since 5.0

    Sets the event dispatcher for the thread to \a eventDispatcher. This is
    only possible as long as there is no event dispatcher installed for the
    thread yet. That is, before the thread has been started with start() or, in
    case of the main thread, before QCoreApplication has been instantiated.
    This method takes ownership of the object.
*/
void QThread::setEventDispatcher(QAbstractEventDispatcher *eventDispatcher)
{
    Q_D(QThread);
    if (d->data->hasEventDispatcher()) {
        qWarning("QThread::setEventDispatcher: An event dispatcher has already been created for this thread");
    } else {
        eventDispatcher->moveToThread(this);
        if (eventDispatcher->thread() == this) // was the move successful?
            d->data->eventDispatcher = eventDispatcher;
        else
            qWarning("QThread::setEventDispatcher: Could not move event dispatcher to target thread");
    }
}

/*!
    \fn bool QThread::wait(unsigned long time)
    \overload
*/
bool QThread::wait(unsigned long time)
{
    if (time == std::numeric_limits<unsigned long>::max())
        return wait(QDeadlineTimer(QDeadlineTimer::Forever));
    return wait(QDeadlineTimer(time));
}

#if QT_CONFIG(thread)

/*!
    \reimp
*/
bool QThread::event(QEvent *event)
{
    if (event->type() == QEvent::Quit) {
        quit();
        return true;
    } else {
        return QObject::event(event);
    }
}

/*!
    \since 5.2
    \threadsafe

    Request the interruption of the thread.
    That request is advisory and it is up to code running on the thread to decide
    if and how it should act upon such request.
    This function does not stop any event loop running on the thread and
    does not terminate it in any way.

    \sa isInterruptionRequested()
*/

void QThread::requestInterruption()
{
    if (this == QCoreApplicationPrivate::theMainThread.loadAcquire()) {
        qWarning("QThread::requestInterruption has no effect on the main thread");
        return;
    }
    Q_D(QThread);
    // ### Qt 6: use std::atomic_flag, and document that
    // requestInterruption/isInterruptionRequested do not synchronize with each other
    QMutexLocker locker(&d->mutex);
    if (!d->running || d->finished || d->isInFinish)
        return;
    d->interruptionRequested.store(true, std::memory_order_relaxed);
}

/*!
    \since 5.2

    Return true if the task running on this thread should be stopped.
    An interruption can be requested by requestInterruption().

    This function can be used to make long running tasks cleanly interruptible.
    Never checking or acting on the value returned by this function is safe,
    however it is advisable do so regularly in long running functions.
    Take care not to call it too often, to keep the overhead low.

    \code
    void long_task() {
         forever {
            if ( QThread::currentThread()->isInterruptionRequested() ) {
                return;
            }
        }
    }
    \endcode

    \note This can only be called within the thread itself, i.e. when
    it is the current thread.

    \sa currentThread() requestInterruption()
*/
bool QThread::isInterruptionRequested() const
{
    Q_D(const QThread);
    // fast path: check that the flag is not set:
    if (!d->interruptionRequested.load(std::memory_order_relaxed))
        return false;
    // slow path: if the flag is set, take into account run status:
    QMutexLocker locker(&d->mutex);
    return d->running && !d->finished && !d->isInFinish;
}

/*!
    \fn template <typename Function, typename... Args> QThread *QThread::create(Function &&f, Args &&... args)
    \since 5.10

    Creates a new QThread object that will execute the function \a f with the
    arguments \a args.

    The new thread is not started -- it must be started by an explicit call
    to start(). This allows you to connect to its signals, move QObjects
    to the thread, choose the new thread's priority and so on. The function
    \a f will be called in the new thread.

    Returns the newly created QThread instance.

    \note the caller acquires ownership of the returned QThread instance.

    \note this function is only available when using C++17.

    \warning do not call start() on the returned QThread instance more than once;
    doing so will result in undefined behavior.

    \sa start()
*/

/*!
    \fn template <typename Function> QThread *QThread::create(Function &&f)
    \since 5.10

    Creates a new QThread object that will execute the function \a f.

    The new thread is not started -- it must be started by an explicit call
    to start(). This allows you to connect to its signals, move QObjects
    to the thread, choose the new thread's priority and so on. The function
    \a f will be called in the new thread.

    Returns the newly created QThread instance.

    \note the caller acquires ownership of the returned QThread instance.

    \warning do not call start() on the returned QThread instance more than once;
    doing so will result in undefined behavior.

    \sa start()
*/

#if QT_CONFIG(cxx11_future)
class QThreadCreateThread : public QThread
{
public:
    explicit QThreadCreateThread(std::future<void> &&future)
        : m_future(std::move(future))
    {
    }

private:
    void run() override
    {
        m_future.get();
    }

    std::future<void> m_future;
};

QThread *QThread::createThreadImpl(std::future<void> &&future)
{
    return new QThreadCreateThread(std::move(future));
}
#endif // QT_CONFIG(cxx11_future)

/*!
    \class QDaemonThread
    \since 5.5
    \brief The QDaemonThread provides a class to manage threads that outlive QCoreApplication
    \internal

    Note: don't try to deliver events from the started() signal.
*/
QDaemonThread::QDaemonThread(QObject *parent)
    : QThread(parent)
{
    // QThread::started() is emitted from the thread we start
    connect(this, &QThread::started,
            [](){ QThreadData::current()->requiresCoreApplication = false; });
}

QDaemonThread::~QDaemonThread()
{
}

#endif // QT_CONFIG(thread)

QT_END_NAMESPACE

#include "moc_qthread.cpp"
