/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

class tst_Counting : public QObject
{
    Q_OBJECT

private slots:
    void testPassPass_data();
    void testPassPass();

    void testPassSkip_data();
    void testPassSkip();

    void testPassFail_data();
    void testPassFail();

    void testSkipPass_data();
    void testSkipPass();

    void testSkipSkip_data();
    void testSkipSkip();

    void testSkipFail_data();
    void testSkipFail();

    void testFailPass_data();
    void testFailPass();

    void testFailSkip_data();
    void testFailSkip();

    void testFailFail_data();
    void testFailFail();

private:
    void helper();
};

enum TestResult
{
    Pass,
    Fail,
    Skip
};

Q_DECLARE_METATYPE(TestResult);

void tst_Counting::helper()
{
    QFETCH(TestResult, result);

    switch (result) {
        case Pass:
            QVERIFY(true);
            break;
        case Fail:
            QVERIFY(false);
            break;
        case Skip:
            QSKIP("Skipping");
            break;
    }
}

void tst_Counting::testPassPass_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Pass;
    QTest::newRow("row 2") << Pass;
}

void tst_Counting::testPassPass()
{
    helper();
}

void tst_Counting::testPassSkip_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Pass;
    QTest::newRow("row 2") << Skip;
}

void tst_Counting::testPassSkip()
{
    helper();
}

void tst_Counting::testPassFail_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Pass;
    QTest::newRow("row 2") << Fail;
}

void tst_Counting::testPassFail()
{
    helper();
}

void tst_Counting::testSkipPass_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Skip;
    QTest::newRow("row 2") << Pass;
}

void tst_Counting::testSkipPass()
{
    helper();
}

void tst_Counting::testSkipSkip_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Skip;
    QTest::newRow("row 2") << Skip;
}

void tst_Counting::testSkipSkip()
{
    helper();
}

void tst_Counting::testSkipFail_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Skip;
    QTest::newRow("row 2") << Fail;
}

void tst_Counting::testSkipFail()
{
    helper();
}

void tst_Counting::testFailPass_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Fail;
    QTest::newRow("row 2") << Pass;
}

void tst_Counting::testFailPass()
{
    helper();
}

void tst_Counting::testFailSkip_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Fail;
    QTest::newRow("row 2") << Skip;
}

void tst_Counting::testFailSkip()
{
    helper();
}

void tst_Counting::testFailFail_data()
{
    QTest::addColumn<TestResult>("result");
    QTest::newRow("row 1") << Fail;
    QTest::newRow("row 2") << Fail;
}

void tst_Counting::testFailFail()
{
    helper();
}

QTEST_MAIN(tst_Counting)
#include "tst_counting.moc"
