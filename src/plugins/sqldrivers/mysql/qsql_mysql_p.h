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

#ifndef QSQL_MYSQL_H
#define QSQL_MYSQL_H

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

#include <mysql.h>

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_MYSQL
#else
#define Q_EXPORT_SQLDRIVER_MYSQL Q_SQL_EXPORT
#endif

QT_BEGIN_NAMESPACE

class QMYSQLDriverPrivate;

class Q_EXPORT_SQLDRIVER_MYSQL QMYSQLDriver : public QSqlDriver
{
    friend class QMYSQLResultPrivate;
    Q_DECLARE_PRIVATE(QMYSQLDriver)
    Q_OBJECT
public:
    explicit QMYSQLDriver(QObject *parent=nullptr);
    explicit QMYSQLDriver(MYSQL *con, QObject * parent=nullptr);
    ~QMYSQLDriver();
    bool hasFeature(DriverFeature f) const override;
    bool open(const QString & db,
               const QString & user,
               const QString & password,
               const QString & host,
               int port,
               const QString& connOpts) override;
    void close() override;
    QSqlResult *createResult() const override;
    QStringList tables(QSql::TableType) const override;
    QSqlIndex primaryIndex(const QString& tablename) const override;
    QSqlRecord record(const QString& tablename) const override;
    QString formatValue(const QSqlField &field,
                                     bool trimStrings) const override;
    QVariant handle() const override;
    QString escapeIdentifier(const QString &identifier, IdentifierType type) const override;

    bool isIdentifierEscaped(const QString &identifier, IdentifierType type) const override;

protected:
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;
private:
    void init();
};

QT_END_NAMESPACE

#endif // QSQL_MYSQL_H
