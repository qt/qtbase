/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtConcurrent module of the Qt Toolkit.
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

/*!
    \page qtconcurrentrun.html
    \title Concurrent Run and Run With Promise
    \ingroup thread

    The QtConcurrent::run() and QtConcurrent::runWithPromise()
    functions run a function in a separate thread.
    The return value of the function is made available through the QFuture API.
    The function passed to QtConcurrent::run() is able to report merely
    a single computation result to its caller, while the function passed to
    QtConcurrent::runWithPromise() can make use of the additional
    QPromise API, which enables multiple result reporting, progress reporting,
    suspending the computation when requested by the caller, or stopping
    the computation on the caller's demand.

    These functions are part of the Qt Concurrent framework.

    \section1 Concurrent Run

    The function passed to QtConcurrent::run() may report the result
    through its return value.

    \section2 Running a Function in a Separate Thread

    To run a function in another thread, use QtConcurrent::run():

    \snippet code/src_concurrent_qtconcurrentrun.cpp 0

    This will run \e aFunction in a separate thread obtained from the default
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

    \section2 Returning Values from the Function

    Any return value from the function is available via QFuture:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 2

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

    The QtConcurrent::runWithPromise() enables more control
    for the running task comparing to QtConcurrent::run().
    It allows progress reporting of the running task,
    reporting multiple results, suspending the execution
    if it was requested, or canceling the task on caller's
    demand.

    \section2 The mandatory QPromise argument

    The function passed to QtConcurrent::runWithPromise() is expected
    to have an additional argument of \e {QPromise<T> &} type, where
    T is the type of the computation result (it should match the type T
    of QFuture<T> returned by the QtConcurrent::runWithPromise()), like e.g.:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 9

    The \e promise argument is instantiated inside the
    QtConcurrent::runWithPromise() function, and its reference
    is passed to the invoked \e aFunction, so the user
    doesn't need to instantiate it by himself, nor pass it explicitly
    when calling QtConcurrent::runWithPromise().

    The additional argument of QPromise type always needs to appear
    as a first argument on function's arguments list, like:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 10

    \section2 Reporting results

    In contrast to QtConcurrent::run(), the function passed to
    QtConcurrent::runWithPromise() is expected to always return void type.
    Result reporting is done through the additional argument of QPromise type.
    It also enables multiple result reporting, like:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 11

    \section2 Suspending and canceling the execution

    The QPromise API also enables suspending and canceling the computation, if requested:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 12

    The call to \e future.suspend() requests the running task to
    hold its execution. After calling this method, the running task
    will suspend after the next call to \e promise.suspendIfRequested()
    in its iteration loop. In this case the running task will
    block on a call to \e promise.suspendIfRequested(). The blocked
    call will unblock after the \e future.resume() is called.
    Note, that internally suspendIfRequested() uses wait condition
    in order to unblock, so the running thread goes into an idle state
    instead of wasting its resources when blocked in order to periodically
    check if the resume request came from the caller's thread.

    The call to \e future.cancel() from the last line causes that the next
    call to \e promise.isCanceled() will return \c true and
    \e aFunction will return immediately without any further result reporting.

    \section2 Progress reporting

    It's also possible to report the progress of a task
    independently of result reporting, like:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 13

    The caller installs the \e QFutureWatcher for the \e QFuture
    returned by QtConcurrent::runWithPromise() in order to
    connect to its \e progressValueChanged() signal and update
    e.g. the graphical user interface accordingly.

    \section2 Invoking functions with overloaded operator()()

    By default, QtConcurrent::runWithPromise() doesn't support functors with
    overloaded operator()(). In case of overloaded functors the user
    needs to explicitly specify the result type
    as a template parameter passed to runWithPromise, like:

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

    T is the same type as the return value of \a function. Non-void return
    values can be accessed via the QFuture::result() function.

    \note The QFuture returned can only be used to query for the
    running/finished status and the return value of the function. In particular,
    canceling or pausing can be issued only if the computations behind the future
    has not been started.

    \sa {Concurrent Run}
*/

/*!
    \since 5.4
    \fn QFuture<T> QtConcurrent::run(QThreadPool *pool, Function function, ...);

    Runs \a function in a separate thread. The thread is taken from the
    QThreadPool \a pool. Note that \a function may not run immediately; \a function
    will only be run once a thread becomes available.

    T is the same type as the return value of \a function. Non-void return
    values can be accessed via the QFuture::result() function.

    \note The QFuture returned can only be used to query for the
    running/finished status and the return value of the function. In particular,
    canceling or pausing can be issued only if the computations behind the future
    has not been started.

    \sa {Concurrent Run}
*/

/*!
    \since 6.0
    \fn QFuture<T> QtConcurrent::runWithPromise(Function function, ...);

    Equivalent to
    \code
    QtConcurrent::runWithPromise(QThreadPool::globalInstance(), function, ...);
    \endcode

    Runs \a function in a separate thread. The thread is taken from the global
    QThreadPool. Note that \a function may not run immediately; \a function
    will only be run once a thread becomes available.

    The \a function is expected to return void
    and must take an additional argument of \e {QPromise<T> &} type,
    placed as a first argument in function's argument list. T is the result type
    and it is the same for the returned \e QFuture<T>.

    Similar to QtConcurrent::run(), the QFuture returned can be used to query for the
    running/finished status and the value reported by the function. In addition,
    it may be used for suspending or canceling the running task, fetching
    multiple results from the called /a function or monitoring progress
    reported by the \a function.

    \sa {Concurrent Run With Promise}
*/

/*!
    \since 6.0
    \fn QFuture<T> QtConcurrent::runWithPromise(QThreadPool *pool, Function function, ...);

    Runs \a function in a separate thread. The thread is taken from the
    QThreadPool \a pool. Note that \a function may not run immediately; \a function
    will only be run once a thread becomes available.

    The \a function is expected to return void
    and must take an additional argument of \e {QPromise<T> &} type,
    placed as a first argument in function's argument list. T is the result type
    and it is the same for the returned \e QFuture<T>.

    Similar to QtConcurrent::run(), the QFuture returned can be used to query for the
    running/finished status and the value reported by the function. In addition,
    it may be used for suspending or canceling the running task, fetching
    multiple results from the called /a function or monitoring progress
    reported by the \a function.

    \sa {Concurrent Run With Promise}
*/
