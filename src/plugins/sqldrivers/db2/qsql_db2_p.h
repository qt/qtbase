// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSQL_DB2_H
#define QSQL_DB2_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_DB2
#else
#define Q_EXPORT_SQLDRIVER_DB2 Q_SQL_EXPORT
#endif

#include <QtSql/qsqldriver.h>

QT_BEGIN_NAMESPACE

class QDB2DriverPrivate;

class Q_EXPORT_SQLDRIVER_DB2 QDB2Driver : public QSqlDriver
{
    Q_DECLARE_PRIVATE(QDB2Driver)
    Q_OBJECT
    friend class QDB2ResultPrivate;

public:
    explicit QDB2Driver(QObject *parent = nullptr);
    QDB2Driver(Qt::HANDLE env, Qt::HANDLE con, QObject *parent = nullptr);
    ~QDB2Driver();
    bool hasFeature(DriverFeature) const override;
    void close() override;
    QSqlRecord record(const QString &tableName) const override;
    QStringList tables(QSql::TableType type) const override;
    QSqlResult *createResult() const override;
    QSqlIndex primaryIndex(const QString &tablename) const override;
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;
    QString formatValue(const QSqlField &field, bool trimStrings) const override;
    QVariant handle() const override;
    bool open(const QString &db,
               const QString &user,
               const QString &password,
               const QString &host,
               int port,
               const QString& connOpts) override;
    QString escapeIdentifier(const QString &identifier, IdentifierType type) const override;

private:
    bool setAutoCommit(bool autoCommit);
};

QT_END_NAMESPACE

#endif // QSQL_DB2_H
