/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSQL_TDS_H
#define QSQL_TDS_H

#include <QtSql/qsqlresult.h>
#include <QtSql/qsqldriver.h>
#include <QtSql/private/qsqlcachedresult_p.h>

#ifdef Q_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef Q_USE_SYBASE
#define DBNTWIN32 // indicates 32bit windows dblib
#endif
#include <winsock2.h>
#include <QtCore/qt_windows.h>
#include <sqlfront.h>
#include <sqldb.h>
#define CS_PUBLIC
#else
#include <sybfront.h>
#include <sybdb.h>
#endif //Q_OS_WIN32

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_TDS
#else
#define Q_EXPORT_SQLDRIVER_TDS Q_SQL_EXPORT
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QTDSDriverPrivate;
class QTDSResultPrivate;
class QTDSDriver;

class QTDSResult : public QSqlCachedResult
{
public:
    explicit QTDSResult(const QTDSDriver* db);
    ~QTDSResult();
    QVariant handle() const;

protected:
    void cleanup();
    bool reset (const QString& query);
    int size();
    int numRowsAffected();
    bool gotoNext(QSqlCachedResult::ValueCache &values, int index);
    QSqlRecord record() const;

private:
    QTDSResultPrivate* d;
};

class Q_EXPORT_SQLDRIVER_TDS QTDSDriver : public QSqlDriver
{
    Q_OBJECT
    friend class QTDSResult;
public:
    explicit QTDSDriver(QObject* parent = 0);
    QTDSDriver(LOGINREC* rec, const QString& host, const QString &db, QObject* parent = 0);
    ~QTDSDriver();
    bool hasFeature(DriverFeature f) const;
    bool open(const QString & db,
               const QString & user,
               const QString & password,
               const QString & host,
               int port,
               const QString& connOpts);
    void close();
    QStringList tables(QSql::TableType) const;
    QSqlResult *createResult() const;
    QSqlRecord record(const QString& tablename) const;
    QSqlIndex primaryIndex(const QString& tablename) const;

    QString formatValue(const QSqlField &field,
                         bool trimStrings) const;
    QVariant handle() const;

    QString escapeIdentifier(const QString &identifier, IdentifierType type) const;

protected:
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
private:
    void init();
    QTDSDriverPrivate *d;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSQL_TDS_H
