// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QSQL_FIREBIRD_P_H
#define QSQL_FIREBIRD_P_H

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

#include <QtSql/qsqldriver.h>
#include <QtSql/qsqlquery.h>

QT_BEGIN_NAMESPACE

class QFirebirdDriver;
class QFirebirdDriverPrivate;
class QFirebirdResultPrivate;
class QFirebirdResult;

class QFirebirdDriver : public QSqlDriver
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFirebirdDriver)
    friend class QFirebirdResult;
    friend class QFirebirdResultPrivate;

public:
    explicit QFirebirdDriver(QObject *parent = nullptr);
    ~QFirebirdDriver() override;

    bool hasFeature(DriverFeature feature) const override;

    bool open(const QString &db,
              const QString &user,
              const QString &password,
              const QString &host,
              int port,
              const QString &connOpts) override;
    void close() override;

    QSqlResult *createResult() const override;

    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

    QStringList tables(QSql::TableType type) const override;
    QSqlRecord record(const QString &tableName) const override;
    QSqlIndex primaryIndex(const QString &tableName) const override;

    QString escapeIdentifier(const QString &identifier, IdentifierType type) const override;
    QString formatValue(const QSqlField &field, bool trimStrings) const override;

    QVariant handle() const override;
    int maximumIdentifierLength(IdentifierType type) const override;

    bool subscribeToNotification(const QString &name) override;
    bool unsubscribeFromNotification(const QString &name) override;
    QStringList subscribedToNotifications() const override;

    bool cancelQuery() override;

private Q_SLOTS:
    void qHandleEventNotification(const QString &name);

private:
    // Prepare, bind and execute a schema-introspection query.
    QSqlQuery execMetadataQuery(const QString &sql,
                                const QVariantList &binds = {}) const;
};

QT_END_NAMESPACE

#endif // QSQL_FIREBIRD_P_H
