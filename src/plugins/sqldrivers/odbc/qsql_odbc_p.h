// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSQL_ODBC_H
#define QSQL_ODBC_H

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
#include <QtCore/qt_windows.h>
#endif

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_ODBC
#else
#define Q_EXPORT_SQLDRIVER_ODBC Q_SQL_EXPORT
#endif

#ifdef Q_OS_UNIX
#define HAVE_LONG_LONG 1 // force UnixODBC NOT to fall back to a struct for BIGINTs
#endif

#if defined(Q_CC_BOR)
// workaround for Borland to make sure that SQLBIGINT is defined
#  define _MSC_VER 900
#endif
#include <sql.h>
#if defined(Q_CC_BOR)
#  undef _MSC_VER
#endif

#include <sqlext.h>

QT_BEGIN_NAMESPACE

class QODBCDriverPrivate;

class Q_EXPORT_SQLDRIVER_ODBC QODBCDriver : public QSqlDriver
{
    Q_DECLARE_PRIVATE(QODBCDriver)
    Q_OBJECT
    friend class QODBCResultPrivate;

public:
    explicit QODBCDriver(QObject *parent=nullptr);
    QODBCDriver(SQLHANDLE env, SQLHANDLE con, QObject * parent=nullptr);
    virtual ~QODBCDriver();
    bool hasFeature(DriverFeature f) const override;
    void close() override;
    QSqlResult *createResult() const override;
    QStringList tables(QSql::TableType) const override;
    QSqlRecord record(const QString &tablename) const override;
    QSqlIndex primaryIndex(const QString &tablename) const override;
    QVariant handle() const override;
    QString formatValue(const QSqlField &field,
                        bool trimStrings) const override;
    bool open(const QString &db,
              const QString &user,
              const QString &password,
              const QString &host,
              int port,
              const QString &connOpts) override;

    QString escapeIdentifier(const QString &identifier, IdentifierType type) const override;

    bool isIdentifierEscaped(const QString &identifier, IdentifierType type) const override;

protected:
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

private:
    bool endTrans();
    void cleanup();
};

QT_END_NAMESPACE

#endif // QSQL_ODBC_H
