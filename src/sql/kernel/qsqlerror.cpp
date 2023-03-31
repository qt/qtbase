// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsqlerror.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSqlError &s)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QSqlError(" << s.nativeErrorCode() << ", " << s.driverText()
        << ", " << s.databaseText() << ')';
    return dbg;
}
#endif

class QSqlErrorPrivate
{
public:
    QString driverError;
    QString databaseError;
    QSqlError::ErrorType errorType;
    QString errorCode;
};


/*!
    \class QSqlError
    \brief The QSqlError class provides SQL database error information.

    \ingroup database
    \inmodule QtSql

    A QSqlError object can provide database-specific error data,
    including the driverText() and databaseText() messages (or both
    concatenated together as text()), and the nativeErrorCode() and
    type().

    \sa QSqlDatabase::lastError(), QSqlQuery::lastError()
*/

/*!
    \enum QSqlError::ErrorType

    This enum type describes the context in which the error occurred, e.g., a connection error, a statement error, etc.

    \value NoError  No error occurred.
    \value ConnectionError  Connection error.
    \value StatementError  SQL statement syntax error.
    \value TransactionError  Transaction failed error.
    \value UnknownError  Unknown error.
*/

/*! \fn QSqlError::QSqlError(QSqlError &&other)
    Move-constructs a QSqlError instance, making it point at the same
    object that \a other was pointing to.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.

    \since 5.10
*/

/*! \fn QSqlError::operator=(QSqlError &&other)
    Move-assigns \a other to this QSqlError instance.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.

    \since 5.10
*/

/*! \fn QSqlError::swap(QSqlError &other)
    Swaps error \a other with this error. This operation is very fast
    and never fails.

    \since 5.10
*/

/*!
    Constructs an error containing the driver error text \a
    driverText, the database-specific error text \a databaseText, the
    type \a type and the error code \a code.

    \note DB2: It is possible for DB2 to report more than one error code.
    When this happens, \c ; is used as separator between the error codes.
*/
QSqlError::QSqlError(const QString &driverText, const QString &databaseText,
                     ErrorType type, const QString &code)
    : d(new QSqlErrorPrivate)
{
    d->driverError = driverText;
    d->databaseError = databaseText;
    d->errorType = type;
    d->errorCode = code;
}


/*!
    Creates a copy of \a other.
*/
QSqlError::QSqlError(const QSqlError &other)
    : d(new QSqlErrorPrivate(*other.d))
{
}

/*!
    Assigns the \a other error's values to this error.
*/

QSqlError &QSqlError::operator=(const QSqlError &other)
{
    if (&other == this)
        return *this;
    if (d && other.d)
        *d = *other.d;
    else if (d)
        *d = QSqlErrorPrivate();
    else if (other.d)
        d = new QSqlErrorPrivate(*other.d);
    return *this;
}

/*!
    Compare the \a other error's values to this error and returns \c true, if it equal.
*/

bool QSqlError::operator==(const QSqlError &other) const
{
    return (d->errorType == other.d->errorType);
}


/*!
    Compare the \a other error's values to this error and returns \c true if it is not equal.
*/

bool QSqlError::operator!=(const QSqlError &other) const
{
    return (d->errorType != other.d->errorType);
}


/*!
    Destroys the object and frees any allocated resources.
*/

QSqlError::~QSqlError()
{
    delete d;
}

/*!
    Returns the text of the error as reported by the driver. This may
    contain database-specific descriptions. It may also be empty.

    \sa databaseText(), text()
*/
QString QSqlError::driverText() const
{
    return d->driverError;
}

/*!
    Returns the text of the error as reported by the database. This
    may contain database-specific descriptions; it may be empty.

    \sa driverText(), text()
*/

QString QSqlError::databaseText() const
{
    return d->databaseError;
}

/*!
    Returns the error type, or -1 if the type cannot be determined.
*/

QSqlError::ErrorType QSqlError::type() const
{
    return d->errorType;
}

/*!
    Returns the database-specific error code, or an empty string if
    it cannot be determined.
*/

QString QSqlError::nativeErrorCode() const
{
    return d->errorCode;
}

/*!
    This is a convenience function that returns databaseText() and
    driverText() concatenated into a single string.

    \sa driverText(), databaseText()
*/

QString QSqlError::text() const
{
    QString result = d->databaseError;
    if (!d->databaseError.isEmpty() && !d->driverError.isEmpty() && !d->databaseError.endsWith(u'\n'))
        result += u' ';
    result += d->driverError;
    return result;
}

/*!
    Returns \c true if an error is set, otherwise false.

    Example:
    \snippet code/src_sql_kernel_qsqlerror.cpp 0

    \sa type()
*/
bool QSqlError::isValid() const
{
    return d->errorType != NoError;
}

QT_END_NAMESPACE
