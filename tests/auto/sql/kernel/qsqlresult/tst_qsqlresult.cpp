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

#include <QtTest/QtTest>
#include <QtSql/QtSql>

#include "testsqldriver.h"

class tst_QSqlResult : public QObject
{
    Q_OBJECT

public:
    tst_QSqlResult();

private slots:
    void positionalToNamedBinding();
    void parseOfBoundValues();

};

tst_QSqlResult::tst_QSqlResult()
{
}

void tst_QSqlResult::positionalToNamedBinding()
{
    TestSqlDriver testDriver;
    TestSqlDriverResult result(&testDriver);
    QString query("INSERT INTO MYTABLE (ID, NAME, BIRTH) VALUES(?, ?, ?)");
    QVERIFY(result.savePrepare(query));
    QCOMPARE(result.boundValues().count(), 3);
}

void tst_QSqlResult::parseOfBoundValues()
{
    TestSqlDriver testDriver;
    TestSqlDriverResult result(&testDriver);
    QVERIFY(result.savePrepare("SELECT :1 AS \":2\""));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS ':2'"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS [:2]"));
    if (testDriver.dbmsType() == QSqlDriver::PostgreSQL)
        QCOMPARE(result.boundValues().count(), 2);
    else
        QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS [:2]]]"));
    if (testDriver.dbmsType() == QSqlDriver::PostgreSQL)
        QCOMPARE(result.boundValues().count(), 2);
    else
        QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS [:2]]]]]"));
    if (testDriver.dbmsType() == QSqlDriver::PostgreSQL)
        QCOMPARE(result.boundValues().count(), 2);
    else
        QCOMPARE(result.boundValues().count(), 1);

    QVERIFY(result.savePrepare("SELECT ? AS \"?\""));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS '?'"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS [?]"));
    if (testDriver.dbmsType() == QSqlDriver::PostgreSQL)
        QCOMPARE(result.boundValues().count(), 2);
    else
        QCOMPARE(result.boundValues().count(), 1);

    QVERIFY(result.savePrepare("SELECT ? AS \"'?\""));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS '?\"'"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS '?''?'"));
    QCOMPARE(result.boundValues().count(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS [\"?']"));
    QCOMPARE(result.boundValues().count(), 1);
}

QTEST_MAIN( tst_QSqlResult )
#include "tst_qsqlresult.moc"
