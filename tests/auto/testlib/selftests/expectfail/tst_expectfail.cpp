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

Q_DECLARE_METATYPE(QTest::TestFailMode)

class tst_ExpectFail: public QObject
{
    Q_OBJECT

private slots:
    void xfailAndContinue() const;
    void xfailAndAbort() const;
    void xfailTwice() const;
    void xfailWithQString() const;
    void xfailDataDriven_data() const;
    void xfailDataDriven() const;
    void xfailOnWrongRow_data() const;
    void xfailOnWrongRow() const;
    void xfailOnAnyRow_data() const;
    void xfailOnAnyRow() const;
    void xpass() const;
};

void tst_ExpectFail::xfailAndContinue() const
{
    qDebug("begin");
    QEXPECT_FAIL("", "This should xfail", Continue);
    QVERIFY(false);
    qDebug("after");
}

void tst_ExpectFail::xfailAndAbort() const
{
    qDebug("begin");
    QEXPECT_FAIL("", "This should xfail", Abort);
    QVERIFY(false);

    // If we get here the test did not correctly abort on the previous QVERIFY.
    QVERIFY2(false, "This should not be reached");
}

void tst_ExpectFail::xfailTwice() const
{
    QEXPECT_FAIL("", "Calling QEXPECT_FAIL once is fine", Abort);
    QEXPECT_FAIL("", "Calling QEXPECT_FAIL when already expecting a failure is "
                     "an error and should abort this test function", Abort);

    // If we get here the test did not correctly abort on the double call to QEXPECT_FAIL.
    QVERIFY2(false, "This should not be reached");
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

void tst_ExpectFail::xfailDataDriven_data() const
{
    QTest::addColumn<bool>("shouldPass");
    QTest::addColumn<QTest::TestFailMode>("failMode");

    QTest::newRow("Pass 1")   << true  << QTest::Abort;
    QTest::newRow("Pass 2")   << true  << QTest::Continue;
    QTest::newRow("Abort")    << false << QTest::Abort;
    QTest::newRow("Continue") << false << QTest::Continue;
}

void tst_ExpectFail::xfailDataDriven() const
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

void tst_ExpectFail::xfailOnWrongRow_data() const
{
    QTest::addColumn<int>("dummy");

    QTest::newRow("right row") << 0;
}

void tst_ExpectFail::xfailOnWrongRow() const
{
    // QEXPECT_FAIL for a row that is not the current row should be ignored.
    QEXPECT_FAIL("wrong row", "This xfail should be ignored", Abort);
    QVERIFY(true);
}

void tst_ExpectFail::xfailOnAnyRow_data() const
{
    QTest::addColumn<int>("dummy");

    QTest::newRow("first row") << 0;
    QTest::newRow("second row") << 1;
}

void tst_ExpectFail::xfailOnAnyRow() const
{
    // In a data-driven test, passing an empty first parameter to QEXPECT_FAIL
    // should mean that the failure is expected for all data rows.
    QEXPECT_FAIL("", "This test should xfail", Abort);
    QVERIFY(false);
}

void tst_ExpectFail::xpass() const
{
    QEXPECT_FAIL("", "This test should xpass", Abort);
    QVERIFY(true);

    // If we get here the test did not correctly abort on the previous
    // unexpected pass.
    QVERIFY2(false, "This should not be reached");
}

QTEST_MAIN(tst_ExpectFail)
#include "tst_expectfail.moc"
