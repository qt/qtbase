// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qexception.h"
#include "QtCore/qshareddata.h"

#if !defined(QT_NO_EXCEPTIONS) || defined(Q_QDOC)

QT_BEGIN_NAMESPACE

/*!
    \class QException
    \inmodule QtCore
    \brief The QException class provides a base class for exceptions that can be transferred across threads.
    \since 5.0

    Qt Concurrent supports throwing and catching exceptions across thread
    boundaries, provided that the exception inherits from QException
    and implements two helper functions:

    \snippet code/src_corelib_thread_qexception.cpp 0

    QException subclasses must be thrown by value and
    caught by reference:

    \snippet code/src_corelib_thread_qexception.cpp 1

    If you throw an exception that is not a subclass of QException,
    the \l{Qt Concurrent} functions will throw a QUnhandledException
    in the receiver thread.

    When using QFuture, transferred exceptions will be thrown when calling the following functions:
    \list
    \li QFuture::waitForFinished()
    \li QFuture::result()
    \li QFuture::resultAt()
    \li QFuture::results()
    \endlist
*/

/*!
    \fn QException::raise() const
    In your QException subclass, reimplement raise() like this:

    \snippet code/src_corelib_thread_qexception.cpp 2
*/

/*!
    \fn QException::clone() const
    In your QException subclass, reimplement clone() like this:

    \snippet code/src_corelib_thread_qexception.cpp 3
*/

/*!
    \class QUnhandledException
    \inmodule QtCore

    \brief The QUnhandledException class represents an unhandled exception in a
    Qt Concurrent worker thread.
    \since 5.0

    If a worker thread throws an exception that is not a subclass of QException,
    the \l{Qt Concurrent} functions will throw a QUnhandledException on the receiver
    thread side. The information about the actual exception that has been thrown
    will be saved in the QUnhandledException class and can be obtained using the
    exception() method. For example, you can process the exception held by
    QUnhandledException in the following way:

    \snippet code/src_corelib_thread_qexception.cpp 4

    Inheriting from this class is not supported.
*/

/*!
    \fn QUnhandledException::raise() const
    \internal
*/

/*!
    \fn QUnhandledException::clone() const
    \internal
*/

/*!
    Destroys this QException object.
*/
QException::~QException() noexcept
{
}

/*!
    \fn QException::QException()

    Constructs a QException object.
*/

/*!
    \fn QException::QException(const QException &other)

    Creates a copy of \a other.

    \note Be careful when using this function, as you risk slicing.

    \sa clone()
*/

/*!
    \fn QException &QException::operator=(const QException &other)

    Copy-assigns \a other over this object.

    \note Be careful when using this function, as you risk slicing.
*/

void QException::raise() const
{
    QException e = *this;
    throw e;
}

QException *QException::clone() const
{
    return new QException(*this);
}

class QUnhandledExceptionPrivate : public QSharedData
{
public:
    QUnhandledExceptionPrivate(std::exception_ptr exception) noexcept : exceptionPtr(exception) { }
    std::exception_ptr exceptionPtr;
};

/*!
    \fn QUnhandledException::QUnhandledException(std::exception_ptr exception = nullptr) noexcept
    \since 6.0

    Constructs a new QUnhandledException object. Saves the pointer to the actual
    exception object if \a exception is passed.

    \sa exception()
*/
QUnhandledException::QUnhandledException(std::exception_ptr exception) noexcept
    : d(new QUnhandledExceptionPrivate(exception))
{
}

/*!
    Move-constructs a QUnhandledException, making it point to the same
    object as \a other was pointing to.
*/
QUnhandledException::QUnhandledException(QUnhandledException &&other) noexcept
    : d(std::exchange(other.d, {}))
{
}

/*!
    Constructs a QUnhandledException object as a copy of \a other.
*/
QUnhandledException::QUnhandledException(const QUnhandledException &other) noexcept
    : d(other.d)
{
}

/*!
    Assigns \a other to this QUnhandledException object and returns a reference
    to this QUnhandledException object.
*/
QUnhandledException &QUnhandledException::operator=(const QUnhandledException &other) noexcept
{
    d = other.d;
    return *this;
}

/*!
    \fn void QUnhandledException::swap(QUnhandledException &other)
    \since 6.0
    \memberswap{unhandled exception object}
*/

/*!
    \since 6.0

    Returns a \l{https://en.cppreference.com/w/cpp/error/exception_ptr}{pointer} to
    the actual exception that has been saved in this QUnhandledException. Returns a
    \c null pointer, if it does not point to an exception object.
*/
std::exception_ptr QUnhandledException::exception() const
{
    return d->exceptionPtr;
}

QUnhandledException::~QUnhandledException() noexcept
{
}

void QUnhandledException::raise() const
{
    QUnhandledException e = *this;
    throw e;
}

QUnhandledException *QUnhandledException::clone() const
{
    return new QUnhandledException(*this);
}

#if !defined(Q_QDOC)

namespace QtPrivate {

void ExceptionStore::setException(const QException &e)
{
    Q_ASSERT(!hasException());
    try {
        e.raise();
    } catch (...) {
        exceptionHolder = std::current_exception();
    }
}

void ExceptionStore::setException(std::exception_ptr e)
{
    Q_ASSERT(!hasException());
    exceptionHolder = e;
}

bool ExceptionStore::hasException() const
{
    return !!exceptionHolder;
}

std::exception_ptr ExceptionStore::exception() const
{
    return exceptionHolder;
}

void ExceptionStore::throwPossibleException()
{
    if (hasException())
        std::rethrow_exception(exceptionHolder);
}

void ExceptionStore::rethrowException() const
{
    Q_ASSERT(hasException());
    std::rethrow_exception(exceptionHolder);
}

} // namespace QtPrivate

#endif //Q_QDOC

QT_END_NAMESPACE

#endif // QT_NO_EXCEPTIONS
