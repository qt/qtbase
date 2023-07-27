// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*!
    \page qtconcurrentrun.html
    \title Concurrent Run
    \brief A simple way to run a task in a separate thread.
    \ingroup thread

    The QtConcurrent::run() function runs a function in a separate thread.
    The return value of the function is made available through the QFuture API.

    QtConcurrent::run() is an overloaded method. You can think of these overloads as slightly
    different \e modes.
    In \l {Concurrent Run (basic mode)} {basic mode}, the function passed to QtConcurrent::run()
    is able to report merely a single computation result to its caller.
    In \l {Concurrent Run With Promise} {run with promise mode}, the function passed to
    QtConcurrent::run() can make use of the additional
    QPromise API, which enables multiple result reporting, progress reporting,
    suspending the computation when requested by the caller, or stopping
    the computation on the caller's demand.

    This function is a part of the Qt Concurrent framework.

    \section1 Concurrent Run (basic mode)

    The function passed to QtConcurrent::run() may report the result
    through its return value.

    \section2 Running a Function in a Separate Thread

    To run a function in another thread, use QtConcurrent::run():

    \snippet code/src_concurrent_qtconcurrentrun.cpp 0

    This will run \c aFunction in a separate thread obtained from the default
    QThreadPool. You can use the QFuture and QFutureWatcher classes to monitor
    the status of the function.

    To use a dedicated thread pool, you can pass the QThreadPool as
    the first argument:

    \snippet code/src_concurrent_qtconcurrentrun.cpp explicit-pool-0

    \section2 Passing Arguments to the Function

    Passing arguments to the function is done by adding them to the
    QtConcurrent::run() call immediately after the function name. For example:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 1

    A copy of each argument is made at the point where QtConcurrent::run() is
    called, and these values are passed to the thread when it begins executing
    the function. Changes made to the arguments after calling
    QtConcurrent::run() are \e not visible to the thread.

    Note that QtConcurrent::run does not support calling overloaded functions
    directly. For example, the code below won't compile:

//! [run-with-overload-calls]
    \snippet code/src_concurrent_qtconcurrentrun.cpp 15

    The easiest workaround is to call the overloaded function through lambda:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 16

    Or you can tell the compiler which overload to choose by using a
    \c static_cast:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 17

    Or qOverload:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 18
//! [run-with-overload-calls]

    \section2 Returning Values from the Function

    Any return value from the function is available via QFuture:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 2

    If you don't need the result (for example, because the function returns
    \c{void}), using the QThreadPool::start() overload taking a function object
    is more efficient.

    As documented above, passing arguments is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 3

    Note that the QFuture::result() function blocks and waits for the result
    to become available. Use QFutureWatcher to get notification when the
    function has finished execution and the result is available.

    \section2 Additional API Features

    \section3 Using Member Functions

    QtConcurrent::run() also accepts pointers to member functions. The first
    argument must be either a const reference or a pointer to an instance of
    the class. Passing by const reference is useful when calling const member
    functions; passing by pointer is useful for calling non-const member
    functions that modify the instance.

    For example, calling QByteArray::split() (a const member function) in a
    separate thread is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 4

    Calling a non-const member function is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 5

    \section3 Using Lambda Functions

    Calling a lambda function is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 6

    Calling a function modifies an object passed by reference is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 7

    Using callable object is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 8

    \section1 Concurrent Run With Promise

    The \e {Run With Promise} mode enables more control for the running
    task compared to \e basic mode of QtConcurrent::run().
    It allows progress reporting of the running task,
    reporting multiple results, suspending the execution
    if it was requested, or canceling the task on caller's
    demand.

    \section2 The mandatory QPromise argument

    The function passed to QtConcurrent::run() in \e {Run With Promise} mode
    is expected to have an additional argument of \c {QPromise<T> &} type, where
    \c T is the type of the computation result (it should match the type \c T
    of QFuture<T> returned by QtConcurrent::run()), like e.g.:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 9

    The \c promise argument is instantiated inside the QtConcurrent::run()
    function, and its reference is passed to the invoked \c aFunction, so the
    user doesn't need to instantiate it, nor pass it explicitly
    when calling QtConcurrent::run() in this mode.

    The additional argument of QPromise type always needs to appear
    as a first argument on function's arguments list, like:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 10

    \section2 Reporting results

    In contrast to \e basic mode of QtConcurrent::run(), the function passed to
    QtConcurrent::run() in \e {Run With Promise} mode is expected to always return void type.
    Result reporting is done through the additional argument of QPromise type.
    It also enables multiple result reporting, like:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 11

    \note There's no need to call QPromise::start() and QPromise::finish() to
    indicate the beginning and the end of computation (like you would normally do when
    using QPromise). QtConcurrent::run() will always call them before starting and
    after finishing the execution.

    \section2 Suspending and canceling the execution

    The QPromise API also enables suspending and canceling the computation, if requested:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 12

    The call to \c future.suspend() requests the running task to
    hold its execution. After calling this method, the running task
    will suspend after the next call to \c promise.suspendIfRequested()
    in its iteration loop. In this case the running task will
    block on a call to \c promise.suspendIfRequested(). The blocked
    call will unblock after the \c future.resume() is called.
    Note, that internally suspendIfRequested() uses wait condition
    in order to unblock, so the running thread goes into an idle state
    instead of wasting its resources when blocked in order to periodically
    check if the resume request came from the caller's thread.

    The call to \c future.cancel() from the last line causes that the next
    call to \c promise.isCanceled() will return \c true and
    \c aFunction will return immediately without any further result reporting.

    \note There's no need to call QPromise::finish() to stop the computation
    after the cancellation (like you would normally do when using QPromise).
    QtConcurrent::run() will always call it after finishing the execution.

    \section2 Progress reporting

    It's also possible to report the progress of a task
    independently of result reporting, like:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 13

    The caller installs the \c QFutureWatcher for the \c QFuture
    returned by QtConcurrent::run() in order to
    connect to its \c progressValueChanged() signal and update
    e.g. the graphical user interface accordingly.

    \section2 Invoking functions with overloaded operator()()

    By default, QtConcurrent::run() doesn't support functors with
    overloaded operator()() in \e {Run With Promise} mode. In case of overloaded
    functors the user needs to explicitly specify the result type
    as a template parameter passed to QtConcurrent::run(), like:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 14
*/

/*!
  \typedef Function
  \internal

  This typedef is a dummy required to make the \c Function
  type name known so that clang doesn't reject functions
  that use it.
*/

/*!
    \fn QFuture<T> QtConcurrent::run(Function function, ...);

    Equivalent to
    \code
    QtConcurrent::run(QThreadPool::globalInstance(), function, ...);
    \endcode

    Runs \a function in a separate thread. The thread is taken from the global
    QThreadPool. Note that \a function may not run immediately; \a function
    will only be run once a thread becomes available.

//! [run-description]
    In \l {Concurrent Run (basic mode)} {basic mode} T is the same type as the return value
    of \a function. Non-void return values can be accessed via the QFuture::result() function.

    In \l {Concurrent Run (basic mode)} {basic mode} the QFuture returned can only be used to
    query for the running/finished status and the return value of the function. In particular,
    canceling or pausing can be issued only if the computations behind the future
    has not been started.

    In \l {Concurrent Run With Promise} {run with promise mode}, the \a function is expected
    to return void and must take an additional argument of \c {QPromise<T> &} type,
    placed as a first argument in function's argument list. T is the result type
    and it is the same for the returned \c QFuture<T>.

    In \l {Concurrent Run With Promise} {run with promise mode}, similar to \e basic mode, the
    QFuture returned can be used to query for the running/finished status and the value reported
    by the function. In addition, it may be used for suspending or canceling the
    running task, fetching multiple results from the called \a function or
    monitoring progress reported by the \a function.

    \sa {Concurrent Run (basic mode)}, {Concurrent Run With Promise}, QThreadPool::start()
//! [run-description]
*/

/*!
    \since 5.4
    \fn QFuture<T> QtConcurrent::run(QThreadPool *pool, Function function, ...);

    Schedules \a function on \a pool. Note that \a function may not run
    immediately; \a function will only be run once a thread becomes available.

    \include qtconcurrentrun.cpp run-description
*/

