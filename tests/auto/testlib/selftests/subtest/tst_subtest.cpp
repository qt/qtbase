/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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


#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QTest>

class tst_Subtest: public QObject
{
    Q_OBJECT
public slots:
    void init();
    void initTestCase();

    void cleanup();
    void cleanupTestCase();

private slots:
    void test1();
    void test2_data();
    void test2();
    void test3_data();
    void test3();

    void multiFail();
private:
    void logNames(const char *caller);
    void table_data();
};

void tst_Subtest::logNames(const char *caller)
{
    auto orNull = [](const char *s) { return s ? s : "(null)"; };
    qDebug("%s %s %s", caller, orNull(QTest::currentTestFunction()),
           orNull(QTest::currentDataTag()));
}

void tst_Subtest::initTestCase()
{
    logNames("initTestCase");
}

void tst_Subtest::cleanupTestCase()
{
    logNames("cleanupTestCase");
}

void tst_Subtest::init()
{
    logNames("init");
}

void tst_Subtest::cleanup()
{
    logNames("cleanup");
}

void tst_Subtest::test1()
{
    logNames("test1");
}

void tst_Subtest::table_data()
{
    QTest::addColumn<QString>("str");

    QTest::newRow("data0") << QString("hello0");
    QTest::newRow("data1") << QString("hello1");
    QTest::newRow("data2") << QString("hello2");
}

void tst_Subtest::test2_data()
{
    logNames("test2_data");
    table_data();
    qDebug() << "test2_data end";
}

void tst_Subtest::test2()
{
    logNames("test2");

    static int count = 0;

    QFETCH(QString, str);
    QCOMPARE(str, QString("hello%1").arg(count++));

    qDebug() << "test2 end";
}

void tst_Subtest::test3_data()
{
    logNames("test3_data");
    table_data();
    qDebug() << "test3_data end";
}

void tst_Subtest::test3()
{
    logNames("test3");

    QFETCH(QString, str);

    // second and third time we call this it should FAIL
    QCOMPARE(str, QString("hello0"));

    qDebug() << "test3 end";
}

void tst_Subtest::multiFail()
{
    // Simulates tests which call a shared function that does common checks, or
    // that do checks in code run asynchronously from a messae loop.
    for (int i = 0; i < 10; ++i)
        []() { QFAIL("This failure message should be repeated ten times"); }();
    // FIXME QTBUG-95661: it gets counted as eleven failures, of course.
    QFAIL("But this test should only contribute one to the failure count");
}

QTEST_MAIN(tst_Subtest)

#include "tst_subtest.moc"
