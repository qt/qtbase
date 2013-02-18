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

#ifndef QSQLERROR_H
#define QSQLERROR_H

#include <QtCore/qstring.h>
#include <QtSql/qsql.h>

QT_BEGIN_NAMESPACE


class Q_SQL_EXPORT QSqlError
{
public:
    enum ErrorType {
        NoError,
        ConnectionError,
        StatementError,
        TransactionError,
        UnknownError
    };
    QSqlError( const QString& driverText = QString(),
                const QString& databaseText = QString(),
                ErrorType type = NoError,
                int number = -1);
    QSqlError(const QSqlError& other);
    QSqlError& operator=(const QSqlError& other);
    bool operator==(const QSqlError& other) const;
    bool operator!=(const QSqlError& other) const;
    ~QSqlError();

    QString driverText() const;
    QString databaseText() const;
    ErrorType type() const;
    int number() const;
    QString text() const;
    bool isValid() const;

#if QT_DEPRECATED_SINCE(5, 1)
    QT_DEPRECATED void setDriverText(const QString &driverText);
    QT_DEPRECATED void setDatabaseText(const QString &databaseText);
    QT_DEPRECATED void setType(ErrorType type);
    QT_DEPRECATED void setNumber(int number);
#endif

private:
    QString driverError;
    QString databaseError;
    ErrorType errorType;
    int errorNumber;
};

#ifndef QT_NO_DEBUG_STREAM
Q_SQL_EXPORT QDebug operator<<(QDebug, const QSqlError &);
#endif

QT_END_NAMESPACE

#endif // QSQLERROR_H
