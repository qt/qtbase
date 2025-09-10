// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSQL_IBASE_H
#define QSQL_IBASE_H

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
#include <ibase.h>

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_IBASE
#else
#define Q_EXPORT_SQLDRIVER_IBASE Q_SQL_EXPORT
#endif

static_assert(FB_API_VER >= 20, "Qt requires at least the Firebird 2.0 client APIs.");

QT_BEGIN_NAMESPACE

class QSqlResult;
class QIBaseDriverPrivate;

class Q_EXPORT_SQLDRIVER_IBASE QIBaseDriver : public QSqlDriver
{
    friend class QIBaseResultPrivate;
    Q_DECLARE_PRIVATE(QIBaseDriver)
    Q_OBJECT
public:
    explicit QIBaseDriver(QObject *parent = nullptr);
    explicit QIBaseDriver(isc_db_handle connection, QObject *parent = nullptr);
    virtual ~QIBaseDriver();
    bool hasFeature(DriverFeature f) const override;
    bool open(const QString &db,
                   const QString &user,
                   const QString &password,
                   const QString &host,
                   int port,
                   const QString &connOpts) override;
    bool open(const QString &db,
            const QString &user,
            const QString &password,
            const QString &host,
            int port) { return open(db, user, password, host, port, QString()); }
    void close() override;
    QSqlResult *createResult() const override;
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;
    QStringList tables(QSql::TableType) const override;

    QSqlRecord record(const QString& tablename) const override;
    QSqlIndex primaryIndex(const QString &table) const override;

    QString formatValue(const QSqlField &field, bool trimStrings) const override;
    QVariant handle() const override;

    QString escapeIdentifier(const QString &identifier, IdentifierType type) const override;

    bool subscribeToNotification(const QString &name) override;
    bool unsubscribeFromNotification(const QString &name) override;
    QStringList subscribedToNotifications() const override;
    int maximumIdentifierLength(IdentifierType type) const override;
private Q_SLOTS:
    void qHandleEventNotification(void* updatedResultBuffer);
};

QT_END_NAMESPACE

#endif // QSQL_IBASE_H
