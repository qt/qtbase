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

#ifndef QSQL_DB2_H
#define QSQL_DB2_H

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_DB2
#else
#define Q_EXPORT_SQLDRIVER_DB2 Q_SQL_EXPORT
#endif

#include <QtSql/qsqlresult.h>
#include <QtSql/qsqldriver.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE
class QDB2Driver;
class QDB2DriverPrivate;
class QDB2ResultPrivate;
class QSqlRecord;

class QDB2Result : public QSqlResult
{
public:
    QDB2Result(const QDB2Driver* dr, const QDB2DriverPrivate* dp);
    ~QDB2Result();
    bool prepare(const QString& query);
    bool exec();
    QVariant handle() const;

protected:
    QVariant data(int field);
    bool reset (const QString& query);
    bool fetch(int i);
    bool fetchNext();
    bool fetchFirst();
    bool fetchLast();
    bool isNull(int i);
    int size();
    int numRowsAffected();
    QSqlRecord record() const;
    void virtual_hook(int id, void *data);
    bool nextResult();

private:
    QDB2ResultPrivate* d;
};

class Q_EXPORT_SQLDRIVER_DB2 QDB2Driver : public QSqlDriver
{
    Q_OBJECT
public:
    explicit QDB2Driver(QObject* parent = 0);
    QDB2Driver(Qt::HANDLE env, Qt::HANDLE con, QObject* parent = 0);
    ~QDB2Driver();
    bool hasFeature(DriverFeature) const;
    void close();
    QSqlRecord record(const QString& tableName) const;
    QStringList tables(QSql::TableType type) const;
    QSqlResult *createResult() const;
    QSqlIndex primaryIndex(const QString& tablename) const;
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    QString formatValue(const QSqlField &field, bool trimStrings) const;
    QVariant handle() const;
    bool open(const QString& db,
               const QString& user,
               const QString& password,
               const QString& host,
               int port,
               const QString& connOpts);
    QString escapeIdentifier(const QString &identifier, IdentifierType type) const;

private:
    bool setAutoCommit(bool autoCommit);
    QDB2DriverPrivate* d;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSQL_DB2_H
