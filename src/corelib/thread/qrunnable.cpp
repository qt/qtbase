// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrunnable.h"

#include <QtCore/qlogging.h>

QT_BEGIN_NAMESPACE

QRunnable::~QRunnable()
{
}

/*!
    \internal
    Prints a warning and returns nullptr.
*/
QRunnable *QRunnable::warnNullCallable()
{
    qWarning("Trying to create null QRunnable. This may stop working.");
    return nullptr;
}


QRunnable::QGenericRunnable::~QGenericRunnable()
{
    runHelper->destroy();
}

void QRunnable::QGenericRunnable::run()
{
    runHelper->run();
}

/*!
    \class QRunnable
    \inmodule QtCore
    \since 4.4
    \brief The QRunnable class is the base class for all runnable objects.

    \ingroup thread

    The QRunnable class is an interface for representing a task or
    piece of code that needs to be executed, represented by your
    reimplementation of the run() function.

    You can use QThreadPool to execute your code in a separate
    thread. QThreadPool deletes the QRunnable automatically if
    autoDelete() returns \c true (the default). Use setAutoDelete() to
    change the auto-deletion flag.

    QThreadPool supports executing the same QRunnable more than once
    by calling QThreadPool::tryStart(this) from within the run() function.
    If autoDelete is enabled the QRunnable will be deleted when
    the last thread exits the run function. Calling QThreadPool::start()
    multiple times with the same QRunnable when autoDelete is enabled
    creates a race condition and is not recommended.

    \sa QThreadPool
*/

/*! \fn QRunnable::run()
    Implement this pure virtual function in your subclass.
*/

/*! \fn QRunnable::QRunnable()
    Constructs a QRunnable. Auto-deletion is enabled by default.

    \sa autoDelete(), setAutoDelete()
*/

/*! \fn QRunnable::~QRunnable()
    QRunnable virtual destructor.
*/

/*! \fn bool QRunnable::autoDelete() const

    Returns \c true is auto-deletion is enabled; false otherwise.

    If auto-deletion is enabled, QThreadPool will automatically delete
    this runnable after calling run(); otherwise, ownership remains
    with the application programmer.

    \sa setAutoDelete(), QThreadPool
*/

/*! \fn bool QRunnable::setAutoDelete(bool autoDelete)

    Enables auto-deletion if \a autoDelete is true; otherwise
    auto-deletion is disabled.

    If auto-deletion is enabled, QThreadPool will automatically delete
    this runnable after calling run(); otherwise, ownership remains
    with the application programmer.

    Note that this flag must be set before calling
    QThreadPool::start(). Calling this function after
    QThreadPool::start() results in undefined behavior.

    \sa autoDelete(), QThreadPool
*/

/*!
    \fn template<typename Callable, if_callable<Callable>> QRunnable *QRunnable::create(Callable &&callableToRun);
    \since 5.15

    Creates a QRunnable that calls \a callableToRun in run().

    Auto-deletion is enabled by default.

    \note This function participates in overload resolution only if \c Callable
    is a function or function object which can be called with zero arguments.

    \note In Qt versions prior to 6.6, this method took copyable functions only.

    \sa run(), autoDelete()
*/

QT_END_NAMESPACE
