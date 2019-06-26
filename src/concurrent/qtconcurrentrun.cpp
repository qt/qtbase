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
    \title Concurrent Run
    \ingroup thread

    The QtConcurrent::run() function runs a function in a separate thread.
    The return value of the function is made available through the QFuture API.

    This function is a part of the \l {Qt Concurrent} framework.

    \section1 Running a Function in a Separate Thread

    To run a function in another thread, use QtConcurrent::run():

    \snippet code/src_concurrent_qtconcurrentrun.cpp 0

    This will run \e aFunction in a separate thread obtained from the default
    QThreadPool. You can use the QFuture and QFutureWatcher classes to monitor
    the status of the function.

    To use a dedicated thread pool, you can pass the QThreadPool as
    the first argument:

    \snippet code/src_concurrent_qtconcurrentrun.cpp explicit-pool-0

    \section1 Passing Arguments to the Function

    Passing arguments to the function is done by adding them to the
    QtConcurrent::run() call immediately after the function name. For example:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 1

    A copy of each argument is made at the point where QtConcurrent::run() is
    called, and these values are passed to the thread when it begins executing
    the function. Changes made to the arguments after calling
    QtConcurrent::run() are \e not visible to the thread.

    \section1 Returning Values from the Function

    Any return value from the function is available via QFuture:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 2

    As documented above, passing arguments is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 3

    Note that the QFuture::result() function blocks and waits for the result
    to become available. Use QFutureWatcher to get notification when the
    function has finished execution and the result is available.

    \section1 Additional API Features

    \section2 Using Member Functions

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

    \section2 Using Lambda Functions

    Calling a lambda function is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 6
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
