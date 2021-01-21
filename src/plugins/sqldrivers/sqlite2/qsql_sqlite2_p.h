/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSQL_SQLITE2_H
#define QSQL_SQLITE2_H

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

#if defined (Q_OS_WIN32)
# include <QtCore/qt_windows.h>
#endif

struct sqlite;

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_SQLITE2
#else
#define Q_EXPORT_SQLDRIVER_SQLITE2 Q_SQL_EXPORT
#endif

QT_BEGIN_NAMESPACE

class QSqlResult;
class QSQLite2DriverPrivate;

class Q_EXPORT_SQLDRIVER_SQLITE2 QSQLite2Driver : public QSqlDriver
{
    friend class QSQLite2ResultPrivate;
    Q_DECLARE_PRIVATE(QSQLite2Driver)
    Q_OBJECT
public:
    explicit QSQLite2Driver(QObject *parent = nullptr);
    explicit QSQLite2Driver(sqlite *connection, QObject *parent = nullptr);
    ~QSQLite2Driver();
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

    QSqlRecord record(const QString &tablename) const override;
    QSqlIndex primaryIndex(const QString &table) const override;
    QVariant handle() const override;
    QString escapeIdentifier(const QString &identifier, IdentifierType) const override;
};

QT_END_NAMESPACE

#endif // QSQL_SQLITE2_H
