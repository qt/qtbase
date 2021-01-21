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

#ifndef QSQL_TDS_H
#define QSQL_TDS_H

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

QT_BEGIN_NAMESPACE

class QSqlResult;
class QTDSDriverPrivate;

class Q_EXPORT_SQLDRIVER_TDS QTDSDriver : public QSqlDriver
{
    Q_DECLARE_PRIVATE(QTDSDriver)
    Q_OBJECT
    friend class QTDSResultPrivate;
public:
    explicit QTDSDriver(QObject* parent = nullptr);
    QTDSDriver(LOGINREC* rec, const QString& host, const QString &db, QObject* parent = nullptr);
    ~QTDSDriver();
    bool hasFeature(DriverFeature f) const override;
    bool open(const QString &db,
               const QString &user,
               const QString &password,
               const QString &host,
               int port,
               const QString &connOpts) override;
    void close() override;
    QStringList tables(QSql::TableType) const override;
    QSqlResult *createResult() const override;
    QSqlRecord record(const QString &tablename) const override;
    QSqlIndex primaryIndex(const QString &tablename) const override;

    QString formatValue(const QSqlField &field,
                         bool trimStrings) const override;
    QVariant handle() const override;

    QString escapeIdentifier(const QString &identifier, IdentifierType type) const override;

protected:
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;
private:
    void init();
};

QT_END_NAMESPACE

#endif // QSQL_TDS_H
