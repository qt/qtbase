/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

Q_DECLARE_METATYPE(QTest::TestFailMode)

class tst_ExpectFail: public QObject
{
    Q_OBJECT

private slots:
    void expectAndContinue() const;
    void expectAndAbort() const;
    void xfailWithQString() const;
    void dataDrivenTest_data() const;
    void dataDrivenTest() const;
};

void tst_ExpectFail::expectAndContinue() const
{
    qDebug("begin");
    QEXPECT_FAIL("", "This should xfail", Continue);
    QVERIFY(false);
    qDebug("after");
}

void tst_ExpectFail::expectAndAbort() const
{
    qDebug("begin");
    QEXPECT_FAIL("", "This should xfail", Abort);
    QVERIFY(false);
    qDebug("this should not be reached");
}

void tst_ExpectFail::xfailWithQString() const
{
    QEXPECT_FAIL("", QString("A string").toLatin1().constData(), Continue);
    QVERIFY(false);

    int bugNo = 5;
    QString msg("The message");
    QEXPECT_FAIL( "", QString("Bug %1 (%2)").arg(bugNo).arg(msg).toLatin1().constData(), Continue);
    QVERIFY(false);
}

void tst_ExpectFail::dataDrivenTest_data() const
{
    QTest::addColumn<bool>("shouldPass");
    QTest::addColumn<QTest::TestFailMode>("failMode");

    QTest::newRow("Pass 1")   << true  << QTest::Abort;
    QTest::newRow("Pass 2")   << true  << QTest::Continue;
    QTest::newRow("Abort")    << false << QTest::Abort;
    QTest::newRow("Continue") << false << QTest::Continue;
}

void tst_ExpectFail::dataDrivenTest() const
{
    QFETCH(bool, shouldPass);
    QFETCH(QTest::TestFailMode, failMode);

    // You can't pass a variable as the last parameter of QEXPECT_FAIL,
    // because the macro adds "QTest::" in front of the last parameter.
    // That is why the following code appears to be a little strange.
    if (!shouldPass) {
        if (failMode == QTest::Abort)
            QEXPECT_FAIL(QTest::currentDataTag(), "This test should xfail", Abort);
        else
            QEXPECT_FAIL(QTest::currentDataTag(), "This test should xfail", Continue);
    }

    QVERIFY(shouldPass);

    // If we get here, we either expected to pass or we expected to
    // fail and the failure mode was Continue.
    if (!shouldPass)
        QCOMPARE(failMode, QTest::Continue);
}

QTEST_MAIN(tst_ExpectFail)
#include "tst_expectfail.moc"
