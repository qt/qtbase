// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
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
    QCOMPARE(result.boundValues().size(), 3);
}

void tst_QSqlResult::parseOfBoundValues()
{
    TestSqlDriver testDriver;
    TestSqlDriverResult result(&testDriver);
    QVERIFY(result.savePrepare("SELECT :1 AS \":2\""));
    QCOMPARE(result.boundValues().size(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS ':2'"));
    QCOMPARE(result.boundValues().size(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS [:2]"));
    if (testDriver.dbmsType() == QSqlDriver::PostgreSQL)
        QCOMPARE(result.boundValues().size(), 2);
    else
        QCOMPARE(result.boundValues().size(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS [:2]]]"));
    if (testDriver.dbmsType() == QSqlDriver::PostgreSQL)
        QCOMPARE(result.boundValues().size(), 2);
    else
        QCOMPARE(result.boundValues().size(), 1);
    QVERIFY(result.savePrepare("SELECT :1 AS [:2]]]]]"));
    if (testDriver.dbmsType() == QSqlDriver::PostgreSQL)
        QCOMPARE(result.boundValues().size(), 2);
    else
        QCOMPARE(result.boundValues().size(), 1);

    QVERIFY(result.savePrepare("SELECT ? AS \"?\""));
    QCOMPARE(result.boundValues().size(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS '?'"));
    QCOMPARE(result.boundValues().size(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS [?]"));
    if (testDriver.dbmsType() == QSqlDriver::PostgreSQL)
        QCOMPARE(result.boundValues().size(), 2);
    else
        QCOMPARE(result.boundValues().size(), 1);

    QVERIFY(result.savePrepare("SELECT ? AS \"'?\""));
    QCOMPARE(result.boundValues().size(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS '?\"'"));
    QCOMPARE(result.boundValues().size(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS '?''?'"));
    QCOMPARE(result.boundValues().size(), 1);
    QVERIFY(result.savePrepare("SELECT ? AS [\"?']"));
    QCOMPARE(result.boundValues().size(), 1);
}

QTEST_MAIN( tst_QSqlResult )
#include "tst_qsqlresult.moc"
