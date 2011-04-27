/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore>
#include <QtTest/QtTest>

class tst_Subtest: public QObject
{
    Q_OBJECT
public slots:
    void init();
    void initTestCase();
    void initTestCase_data();

    void cleanup();
    void cleanupTestCase();

private slots:
    void testGlobal_data();
    void testGlobal();

    void skip_data();
    void skip();

    void skipLocal_data() { testGlobal_data(); }
    void skipLocal();

    void skipSingle_data() { testGlobal_data(); }
    void skipSingle();
};


void tst_Subtest::initTestCase()
{
    printf("initTestCase %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::initTestCase_data()
{
    QTest::addColumn<bool>("booli");
    QTest::newRow("1") << false;
    QTest::newRow("2") << true;
}

void tst_Subtest::cleanupTestCase()
{
    printf("cleanupTestCase %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::init()
{
    printf("init %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::cleanup()
{
    printf("cleanup %s %s\n",
            QTest::currentTestFunction() ? QTest::currentTestFunction() : "(null)",
            QTest::currentDataTag() ? QTest::currentDataTag() : "(null)");
}

void tst_Subtest::testGlobal_data()
{
    QTest::addColumn<bool>("booll");
    QTest::newRow("local 1") << false;
    QTest::newRow("local 2") << true;
}

void tst_Subtest::testGlobal()
{
    QFETCH_GLOBAL(bool, booli);
    printf("global: %d\n", booli);
    QFETCH(bool, booll);
    printf("local: %d\n", booll);
}

void tst_Subtest::skip_data()
{
    QTest::addColumn<bool>("booll");
    QTest::newRow("local 1") << false;
    QTest::newRow("local 2") << true;

    QSKIP("skipping", SkipAll);
}

void tst_Subtest::skip()
{
    printf("this line should never be reached\n");
}

void tst_Subtest::skipSingle()
{
    QFETCH_GLOBAL(bool, booli);
    QFETCH(bool, booll);

    if (booli && !booll)
        QSKIP("skipping", SkipSingle);
    printf("global: %d, local %d\n", booli, booll);
}

void tst_Subtest::skipLocal()
{
    QSKIP("skipping", SkipAll);
}

QTEST_MAIN(tst_Subtest)

#include "tst_globaldata.moc"
