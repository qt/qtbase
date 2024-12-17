// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSQL_PSQL_H
#define QSQL_PSQL_H

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

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_PSQL
#else
#define Q_EXPORT_SQLDRIVER_PSQL Q_SQL_EXPORT
#endif

typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;

QT_BEGIN_NAMESPACE

class QPSQLDriverPrivate;

class Q_EXPORT_SQLDRIVER_PSQL QPSQLDriver : public QSqlDriver
{
    friend class QPSQLResultPrivate;
    Q_DECLARE_PRIVATE(QPSQLDriver)
    Q_OBJECT
public:
    enum Protocol {
        VersionUnknown = -1,
        Version6 = 6,
        Version7 = 7,
        Version7_1 = 8,
        Version7_3 = 9,
        Version7_4 = 10,
        Version8 = 11,
        Version8_1 = 12,
        Version8_2 = 13,
        Version8_3 = 14,
        Version8_4 = 15,
        Version9 = 16,
        Version9_1 = 17,
        Version9_2 = 18,
        Version9_3 = 19,
        Version9_4 = 20,
        Version9_5 = 21,
        Version9_6 = 22,
        Version10 = 23,
        Version11 = 24,
        Version12 = 25,
        UnknownLaterVersion = 100000
    };

    explicit QPSQLDriver(QObject *parent = nullptr);
    explicit QPSQLDriver(PGconn *conn, QObject *parent = nullptr);
    ~QPSQLDriver();
    bool hasFeature(DriverFeature f) const override;
    bool open(const QString &db,
              const QString &user,
              const QString &password,
              const QString &host,
              int port,
              const QString &connOpts) override;
    bool isOpen() const override;
    void close() override;
    QSqlResult *createResult() const override;
    QStringList tables(QSql::TableType) const override;
    QSqlIndex primaryIndex(const QString &tablename) const override;
    QSqlRecord record(const QString &tablename) const override;

    Protocol protocol() const;
    QVariant handle() const override;

    QString escapeIdentifier(const QString &identifier, IdentifierType type) const override;
    QString formatValue(const QSqlField &field, bool trimStrings) const override;

    template <bool forPreparedStatement>
    QString formatValue(const QSqlField &field, bool trimStrings = false) const;

    bool subscribeToNotification(const QString &name) override;
    bool unsubscribeFromNotification(const QString &name) override;
    QStringList subscribedToNotifications() const override;

protected:
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

private Q_SLOTS:
    void _q_handleNotification();
};

QT_END_NAMESPACE

#endif // QSQL_PSQL_H
