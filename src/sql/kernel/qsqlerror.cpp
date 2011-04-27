/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsqlerror.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSqlError &s)
{
    dbg.nospace() << "QSqlError(" << s.number() << ", " << s.driverText() <<
                     ", " << s.databaseText() << ')';
    return dbg.space();
}
#endif

/*!
    \class QSqlError
    \brief The QSqlError class provides SQL database error information.

    \ingroup database
    \inmodule QtSql

    A QSqlError object can provide database-specific error data,
    including the driverText() and databaseText() messages (or both
    concatenated together as text()), and the error number() and
    type(). The functions all have setters so that you can create and
    return QSqlError objects from your own classes, for example from
    your own SQL drivers.

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

    \omitvalue None
    \omitvalue Connection
    \omitvalue Statement
    \omitvalue Transaction
    \omitvalue Unknown
*/

/*!
    Constructs an error containing the driver error text \a
    driverText, the database-specific error text \a databaseText, the
    type \a type and the optional error number \a number.
*/

QSqlError::QSqlError(const QString& driverText, const QString& databaseText, ErrorType type,
                    int number)
    : driverError(driverText), databaseError(databaseText), errorType(type), errorNumber(number)
{
}

/*!
    Creates a copy of \a other.
*/
QSqlError::QSqlError(const QSqlError& other)
    : driverError(other.driverError), databaseError(other.databaseError),
      errorType(other.errorType),
      errorNumber(other.errorNumber)
{
}

/*!
    Assigns the \a other error's values to this error.
*/

QSqlError& QSqlError::operator=(const QSqlError& other)
{
    driverError = other.driverError;
    databaseError = other.databaseError;
    errorType = other.errorType;
    errorNumber = other.errorNumber;
    return *this;
}

/*!
    Destroys the object and frees any allocated resources.
*/

QSqlError::~QSqlError()
{
}

/*!
    Returns the text of the error as reported by the driver. This may
    contain database-specific descriptions. It may also be empty.

    \sa setDriverText() databaseText() text()
*/
QString QSqlError::driverText() const
{
    return driverError;
}

/*!
    Sets the driver error text to the value of \a driverText.

    \sa driverText() setDatabaseText() text()
*/

void QSqlError::setDriverText(const QString& driverText)
{
    driverError = driverText;
}

/*!
    Returns the text of the error as reported by the database. This
    may contain database-specific descriptions; it may be empty.

    \sa setDatabaseText() driverText() text()
*/

QString QSqlError::databaseText() const
{
    return databaseError;
}

/*!
    Sets the database error text to the value of \a databaseText.

    \sa databaseText() setDriverText() text()
*/

void QSqlError::setDatabaseText(const QString& databaseText)
{
    databaseError = databaseText;
}

/*!
    Returns the error type, or -1 if the type cannot be determined.

    \sa setType()
*/

QSqlError::ErrorType QSqlError::type() const
{
    return errorType;
}

/*!
    Sets the error type to the value of \a type.

    \sa type()
*/

void QSqlError::setType(ErrorType type)
{
    errorType = type;
}

/*!
    Returns the database-specific error number, or -1 if it cannot be
    determined.

    \sa setNumber()
*/

int QSqlError::number() const
{
    return errorNumber;
}

/*!
    Sets the database-specific error number to \a number.

    \sa number()
*/

void QSqlError::setNumber(int number)
{
    errorNumber = number;
}

/*!
    This is a convenience function that returns databaseText() and
    driverText() concatenated into a single string.

    \sa driverText() databaseText()
*/

QString QSqlError::text() const
{
    QString result = databaseError;
    if (!databaseError.endsWith(QLatin1String("\n")))
        result += QLatin1Char(' ');
    result += driverError;
    return result;
}

/*!
    Returns true if an error is set, otherwise false.

    Example:
    \snippet doc/src/snippets/code/src_sql_kernel_qsqlerror.cpp 0

    \sa type()
*/
bool QSqlError::isValid() const
{
    return errorType != NoError;
}

QT_END_NAMESPACE
