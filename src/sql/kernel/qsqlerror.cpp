/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
    Compare the \a other error's values to this error and returns true, if it equal.
*/

bool QSqlError::operator==(const QSqlError& other) const
{
    return (errorType == other.errorType);
}


/*!
    Compare the \a other error's values to this error and returns true if it is not equal.
*/

bool QSqlError::operator!=(const QSqlError& other) const
{
    return (errorType != other.errorType);
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

    \sa databaseText(), text()
*/
QString QSqlError::driverText() const
{
    return driverError;
}

/*!
    \fn void QSqlError::setDriverText(const QString &driverText)
    \obsolete

    Sets the driver error text to the value of \a driverText.

    Use QSqlError(const QString &driverText, const QString &databaseText,
                  ErrorType type, int number) instead

    \sa driverText(), setDatabaseText(), text()
*/

#if QT_DEPRECATED_SINCE(5, 1)
void QSqlError::setDriverText(const QString& driverText)
{
    driverError = driverText;
}
#endif

/*!
    Returns the text of the error as reported by the database. This
    may contain database-specific descriptions; it may be empty.

    \sa driverText(), text()
*/

QString QSqlError::databaseText() const
{
    return databaseError;
}

/*!
    \fn void QSqlError::setDatabaseText(const QString &databaseText)
    \obsolete

    Sets the database error text to the value of \a databaseText.

    Use QSqlError(const QString &driverText, const QString &databaseText,
                  ErrorType type, int number) instead

    \sa databaseText(), setDriverText(), text()
*/

#if QT_DEPRECATED_SINCE(5, 1)
void QSqlError::setDatabaseText(const QString& databaseText)
{
    databaseError = databaseText;
}
#endif

/*!
    Returns the error type, or -1 if the type cannot be determined.
*/

QSqlError::ErrorType QSqlError::type() const
{
    return errorType;
}

/*!
    \fn void QSqlError::setType(ErrorType type)
    \obsolete

    Sets the error type to the value of \a type.

    Use QSqlError(const QString &driverText, const QString &databaseText,
                  ErrorType type, int number) instead

    \sa type()
*/

#if QT_DEPRECATED_SINCE(5, 1)
void QSqlError::setType(ErrorType type)
{
    errorType = type;
}
#endif

/*!
    Returns the database-specific error number, or -1 if it cannot be
    determined.
*/

int QSqlError::number() const
{
    return errorNumber;
}

/*!
    \fn void QSqlError::setNumber(int number)
    \obsolete

    Sets the database-specific error number to \a number.

    Use QSqlError(const QString &driverText, const QString &databaseText,
                  ErrorType type, int number) instead

    \sa number()
*/

#if QT_DEPRECATED_SINCE(5, 1)
void QSqlError::setNumber(int number)
{
    errorNumber = number;
}
#endif

/*!
    This is a convenience function that returns databaseText() and
    driverText() concatenated into a single string.

    \sa driverText(), databaseText()
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
    \snippet code/src_sql_kernel_qsqlerror.cpp 0

    \sa type()
*/
bool QSqlError::isValid() const
{
    return errorType != NoError;
}

QT_END_NAMESPACE
