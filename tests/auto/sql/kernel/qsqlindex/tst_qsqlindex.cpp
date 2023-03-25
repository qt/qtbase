// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtSql/QtSql>

#include <QtCore/QDateTime>
#include <QtCore/QTimeZone>

#include <numeric>

#include "../qsqldatabase/tst_databases.h"

using namespace Qt::StringLiterals;

QString qtest;

class tst_QSqlIndex : public QObject
{
    Q_OBJECT

public:
    tst_QSqlIndex();

private slots:
    void construction_data();
    void construction();
    void assignment_data();
    void assignment();
    void basicFunctions();
};

tst_QSqlIndex::tst_QSqlIndex()
{
}

void tst_QSqlIndex::construction_data()
{
    QTest::addColumn<QSqlIndex>("sqlIndex");
    QTest::addColumn<QString>("cursorName");
    QTest::addColumn<QString>("name");

    const QString cursorName("cusorName"_L1);
    const QString name("name"_L1);
    QSqlIndex sqlIndex(cursorName, name);
    QTest::newRow("ctor1") << QSqlIndex() << QString() << QString();
    QTest::newRow("ctor2") << sqlIndex << cursorName << name;
    QTest::newRow("copy ctor") << QSqlIndex(sqlIndex) << cursorName << name;
    QTest::newRow("move ctor") << QSqlIndex(std::move(sqlIndex)) << cursorName << name;
}

void tst_QSqlIndex::construction()
{
    QFETCH(QSqlIndex, sqlIndex);
    QFETCH(QString, cursorName);
    QFETCH(QString, name);

    QCOMPARE(sqlIndex.cursorName(), cursorName);
    QCOMPARE(sqlIndex.name(), name);
    QCOMPARE(sqlIndex.isDescending(0), false);
    QCOMPARE(sqlIndex.count(), 0);
}

void tst_QSqlIndex::assignment_data()
{
    QTest::addColumn<QSqlIndex>("sqlIndex");
    QTest::addColumn<QString>("cursorName");
    QTest::addColumn<QString>("name");

    const QString cursorName("cusorName"_L1);
    const QString name("name"_L1);
    QSqlIndex sqlIndex(cursorName, name);
    QSqlIndex sqlIndex1 = sqlIndex;
    QSqlIndex sqlIndex2 = std::move(sqlIndex);
    sqlIndex = std::move(sqlIndex2);
    QTest::newRow("copy assignment") << sqlIndex1 << cursorName << name;
    QTest::newRow("move assignment") << sqlIndex << cursorName << name;
}

void tst_QSqlIndex::assignment()
{
    QFETCH(QSqlIndex, sqlIndex);
    QFETCH(QString, cursorName);
    QFETCH(QString, name);

    QCOMPARE(sqlIndex.cursorName(), cursorName);
    QCOMPARE(sqlIndex.name(), name);
    QCOMPARE(sqlIndex.isDescending(0), false);
    QCOMPARE(sqlIndex.count(), 0);
}

void tst_QSqlIndex::basicFunctions()
{
    QSqlIndex sqlIndex("cursorName"_L1, "name"_L1);
    const QSqlField f1("field1"_L1, QMetaType(QMetaType::UInt), "table1"_L1);
    const QSqlField f2("field2"_L1, QMetaType(QMetaType::Double), "table2"_L1);

    QCOMPARE(sqlIndex.cursorName(), "cursorName"_L1);
    sqlIndex.setCursorName("updatedCursorName"_L1);
    QCOMPARE(sqlIndex.name(), "name"_L1);
    sqlIndex.setName("updatedName"_L1);
    QCOMPARE(sqlIndex.cursorName(), "updatedCursorName"_L1);
    QCOMPARE(sqlIndex.name(), "updatedName"_L1);

    sqlIndex.append(f1);
    QCOMPARE(sqlIndex.count(), 1);
    QCOMPARE(sqlIndex.isDescending(0), false);

    sqlIndex.append(f2, true);
    QCOMPARE(sqlIndex.count(), 2);
    QCOMPARE(sqlIndex.isDescending(0), false);
    QCOMPARE(sqlIndex.isDescending(1), true);

    sqlIndex.setDescending(0, true);
    sqlIndex.setDescending(1, false);
    sqlIndex.setDescending(2, true);
    QCOMPARE(sqlIndex.count(), 2);
    QCOMPARE(sqlIndex.isDescending(0), true);
    QCOMPARE(sqlIndex.isDescending(1), false);

    QCOMPARE(sqlIndex.field(0), f1);
    QCOMPARE(sqlIndex.field(1), f2);
}

QTEST_MAIN(tst_QSqlIndex)
#include "tst_qsqlindex.moc"
