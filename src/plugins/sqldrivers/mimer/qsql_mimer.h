// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Mimer Information Technology
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QSQL_MIMER_H
#define QSQL_MIMER_H

#include <QtSql/qsqldriver.h>
#include <QUuid>
#include <mimerapi.h>

#ifdef QT_PLUGIN
#    define Q_EXPORT_SQLDRIVER_MIMER
#else
#    define Q_EXPORT_SQLDRIVER_MIMER Q_SQL_EXPORT
#endif

QT_BEGIN_NAMESPACE

class QMimerSQLDriverPrivate;

class Q_EXPORT_SQLDRIVER_MIMER QMimerSQLDriver : public QSqlDriver
{
    friend class QMimerSQLResultPrivate;
    Q_DECLARE_PRIVATE(QMimerSQLDriver)
    Q_OBJECT
public:
    explicit QMimerSQLDriver(QObject *parent = nullptr);
    explicit QMimerSQLDriver(MimerSession *conn, QObject *parent = nullptr);
    ~QMimerSQLDriver() override;
    bool hasFeature(DriverFeature f) const override;
    bool open(const QString &db, const QString &user, const QString &password, const QString &host,
              int port, const QString &connOpts) override;
    void close() override;
    QSqlResult *createResult() const override;
    QStringList tables(QSql::TableType type) const override;
    QSqlIndex primaryIndex(const QString &tablename) const override;
    QSqlRecord record(const QString &tablename) const override;
    QVariant handle() const override;
    QString escapeIdentifier(const QString &identifier, IdentifierType type) const override;
protected:
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

private:
};

QT_END_NAMESPACE

#endif // QSQL_MIMER
