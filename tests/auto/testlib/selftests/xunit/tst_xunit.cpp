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

class tst_Xunit : public QObject
{
    Q_OBJECT

public:
    tst_Xunit();

private slots:
    void testFunc1();
    void testFunc2();
    void testFunc3();
    void testFunc4();
    void testFunc5();
    void testFunc6();
    void testFunc7();
};

tst_Xunit::tst_Xunit()
{
}

void tst_Xunit::testFunc1()
{
    QWARN("just a QWARN() !");
    QCOMPARE(1,1);
}

void tst_Xunit::testFunc2()
{
    qDebug("a qDebug() call with comment-ending stuff -->");
    QCOMPARE(2, 3);
}

void tst_Xunit::testFunc3()
{
    QSKIP("skipping this function!");
}

void tst_Xunit::testFunc4()
{
    QFAIL("a forced failure!");
}

/*
    Note there are two testfunctions which give expected failures.
    This is so we can test that expected failures don't add to failure
    counts and unexpected passes do.  If we had one xfail and one xpass
    testfunction, we couldn't test which one of them adds to the failure
    count.
*/

void tst_Xunit::testFunc5()
{
    QEXPECT_FAIL("", "this failure is expected", Abort);
    QVERIFY(false);
}

void tst_Xunit::testFunc6()
{
    QEXPECT_FAIL("", "this failure is also expected", Abort);
    QVERIFY(false);
}

void tst_Xunit::testFunc7()
{
    QEXPECT_FAIL("", "this pass is unexpected", Abort);
    QVERIFY(true);
}


QTEST_APPLESS_MAIN(tst_Xunit)
#include "tst_xunit.moc"
