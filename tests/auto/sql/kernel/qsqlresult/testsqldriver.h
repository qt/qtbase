/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    bool savePrepare(const QString& sqlquery)
    {
        return QSqlResult::savePrepare(sqlquery);
    }

    QVector<QVariant> boundValues() const
    {
        return QSqlResult::boundValues();
    }

protected:
    QVariant data(int /* index */) { return QVariant(); }
    bool isNull(int /* index */) { return false; }
    bool reset(const QString & /* query */) { return false; }
    bool fetch(int /* index */) { return false; }
    bool fetchFirst() { return false; }
    bool fetchLast() { return false; }
    int size() { return 0; }
    int numRowsAffected() { return 0; }
    QSqlRecord record() const { return QSqlRecord(); }
};

class TestSqlDriver : public QSqlDriver
{
    Q_DECLARE_PRIVATE(QSqlDriver)

public:
    TestSqlDriver() {}
    ~TestSqlDriver() {}

    bool hasFeature(DriverFeature f) const {
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
              int /* port */, const QString & /* options */)
        { return false; }
    void close() {}

    QSqlResult *createResult() const { return new TestSqlDriverResult(this); }
};

#endif // TESTSQLDRIVER_H
