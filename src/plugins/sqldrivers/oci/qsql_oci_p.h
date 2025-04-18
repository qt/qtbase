// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSQL_OCI_H
#define QSQL_OCI_H

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
#include <QtSql/private/qsqlcachedresult_p.h>

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_OCI
#else
#define Q_EXPORT_SQLDRIVER_OCI Q_SQL_EXPORT
#endif

typedef struct OCIEnv OCIEnv;
typedef struct OCISvcCtx OCISvcCtx;

QT_BEGIN_NAMESPACE

class QSqlResult;
class QOCIDriverPrivate;
class QOCIResultPrivate;

class Q_EXPORT_SQLDRIVER_OCI QOCIDriver : public QSqlDriver
{
    Q_DECLARE_PRIVATE(QOCIDriver)
    Q_OBJECT
    friend class QOCICols;
    friend class QOCIResultPrivate;

public:
    explicit QOCIDriver(QObject *parent = nullptr);
    QOCIDriver(OCIEnv *env, OCISvcCtx *ctx, QObject *parent = nullptr);
    ~QOCIDriver();
    bool hasFeature(DriverFeature f) const override;
    bool open(const QString &db,
              const QString &user,
              const QString &password,
              const QString &host,
              int port,
              const QString &connOpts) override;
    void close() override;
    QSqlResult *createResult() const override;
    QStringList tables(QSql::TableType) const override;
    QSqlRecord record(const QString &tablename) const override;
    QSqlIndex primaryIndex(const QString& tablename) const override;
    QString formatValue(const QSqlField &field,
                        bool trimStrings) const override;
    QVariant handle() const override;
    QString escapeIdentifier(const QString &identifier, IdentifierType) const override;
    int maximumIdentifierLength(IdentifierType type) const override;

protected:
    bool                beginTransaction() override;
    bool                commitTransaction() override;
    bool                rollbackTransaction() override;
};

class Q_EXPORT_SQLDRIVER_OCI QOCIResult : public QSqlCachedResult
{
    friend class QOCICols;
public:
    explicit QOCIResult(const QOCIDriver * db);
    ~QOCIResult() override;
    bool prepare(const QString& query) override;
    bool exec() override;
    QVariant handle() const override;

protected:
    bool isCursor;

    bool gotoNext(ValueCache &values, int index) override;
    bool reset(const QString& query) override;
    int size() override;
    int numRowsAffected() override;
    QSqlRecord record() const override;
    QVariant lastInsertId() const override;
    bool execBatch(bool arrayBind = false) override;
    void virtual_hook(int id, void *data) override;
    bool fetchNext() override;
    bool internal_prepare();

private:
    Q_DECLARE_PRIVATE(QOCIResult)
};

QT_END_NAMESPACE

#endif // QSQL_OCI_H
