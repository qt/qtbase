// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QTest>

class tst_Skip: public QObject
{
    Q_OBJECT

private slots:
    void test_data();
    void test();

    void emptytest_data();
    void emptytest();

    void singleSkip_data();
    void singleSkip();
};


void tst_Skip::test_data()
{
    QTest::addColumn<bool>("booll");
    QTest::newRow("local 1") << false;
    QTest::newRow("local 2") << true;

    QSKIP("skipping all");
}

void tst_Skip::test()
{
    QFAIL("this line should never be reached, since we skip in the _data function");
}

void tst_Skip::emptytest_data()
{
    QSKIP("skipping all");
}

void tst_Skip::emptytest()
{
    QFAIL("this line should never be reached, since we skip in the _data function");
}

void tst_Skip::singleSkip_data()
{
    QTest::addColumn<bool>("booll");
    QTest::newRow("local 1") << false;
    QTest::newRow("local 2") << true;
}

void tst_Skip::singleSkip()
{
    QFETCH(bool, booll);
    if (!booll)
        QSKIP("skipping one");
    qDebug("this line should only be reached once (%s)", booll ? "true" : "false");
}

QTEST_MAIN(tst_Skip)

#include "tst_skip.moc"
