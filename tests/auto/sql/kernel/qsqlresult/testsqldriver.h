// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTSQLDRIVER_H
#define TESTSQLDRIVER_H

#include <QtSql/QSqlResult>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlRecord>
#include <private/qsqldriver_p.h>

class TestSqlDriverResult : public QSqlResult
{
public:
    TestSqlDriverResult(const QSqlDriver *driver)
        : QSqlResult(driver) {}
    ~TestSqlDriverResult() {}

    bool savePrepare(const QString& sqlquery) override
    {
        return QSqlResult::savePrepare(sqlquery);
    }

    QList<QVariant> boundValues() const { return QSqlResult::boundValues(); }

protected:
    QVariant data(int /* index */) override { return QVariant(); }
    bool isNull(int /* index */) override { return false; }
    bool reset(const QString & /* query */) override { return false; }
    bool fetch(int /* index */) override { return false; }
    bool fetchFirst() override { return false; }
    bool fetchLast() override { return false; }
    int size() override { return 0; }
    int numRowsAffected() override { return 0; }
    QSqlRecord record() const override { return QSqlRecord(); }
};

class TestSqlDriver : public QSqlDriver
{
    Q_DECLARE_PRIVATE(QSqlDriver)

public:
    TestSqlDriver() {}
    ~TestSqlDriver() {}

    bool hasFeature(DriverFeature f) const override {
        switch (f) {
        case QSqlDriver::PreparedQueries:
        case QSqlDriver::NamedPlaceholders:
            return true;
        default:
            break;
        }
        return false;
    }
    bool open(const QString & /* db */, const QString & /* user */,
              const QString & /* password */, const QString & /* host */,
              int /* port */, const QString & /* options */) override
        { return false; }
    void close() override {}

    QSqlResult *createResult() const override { return new TestSqlDriverResult(this); }
};

#endif // TESTSQLDRIVER_H
