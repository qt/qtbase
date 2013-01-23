/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtTest/QtTest>

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
};


void tst_Subtest::initTestCase()
{
    qDebug() << "initTestCase"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::cleanupTestCase()
{
    qDebug() << "cleanupTestCase"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::init()
{
    qDebug() << "init"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::cleanup()
{
    qDebug() << "cleanup"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::test1()
{
    qDebug() << "test1"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::test2_data()
{
    qDebug() << "test2_data"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");

    QTest::addColumn<QString>("str");

    QTest::newRow("data0") << QString("hello0");
    QTest::newRow("data1") << QString("hello1");
    QTest::newRow("data2") << QString("hello2");

    qDebug() << "test2_data end";
}

void tst_Subtest::test2()
{
    qDebug() << "test2"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");

    static int count = 0;

    QFETCH(QString, str);
    QCOMPARE(str, QString("hello%1").arg(count++));

    qDebug() << "test2 end";
}

void tst_Subtest::test3_data()
{
    qDebug() << "test3_data"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");

    QTest::addColumn<QString>("str");

    QTest::newRow("data0") << QString("hello0");
    QTest::newRow("data1") << QString("hello1");
    QTest::newRow("data2") << QString("hello2");

    qDebug() << "test3_data end";
}

void tst_Subtest::test3()
{
    qDebug() << "test2"
             << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)")
             << (QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");

    QFETCH(QString, str);

    // second and third time we call this it should FAIL
    QCOMPARE(str, QString("hello0"));

    qDebug() << "test2 end";
}

QTEST_MAIN(tst_Subtest)

#include "tst_subtest.moc"
