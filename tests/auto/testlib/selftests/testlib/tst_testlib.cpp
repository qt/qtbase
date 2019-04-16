/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
#include <QtCore/QtMath>
#include <QtCore/QtNumeric>

/* Test QTest functions not covered by other parts of the selftest.  Tests that
 * involve crashing or exiting should be added as separate tests in their own
 * right.  Tests that form a coherent group on a related theme should also go in
 * their own directory.  Tests that fail in order to exercise QTest internals
 * are fine.
 */

class tst_TestLib : public QObject
{
Q_OBJECT
private slots:
    void basics() const;
    void delays() const;
    void reals_data() const;
    void reals() const;
};

void tst_TestLib::basics() const
{
    QVERIFY(QByteArray(QTest::currentAppName()).contains("testlib"));

    QCOMPARE(QTest::testObject(), nullptr); // last, because it should fail
}

QT_BEGIN_NAMESPACE

namespace QTest {
    // Defined; not declared in the public header, but used by qtdeclarative.
    int defaultKeyDelay();
    int defaultMouseDelay();
}

QT_END_NAMESPACE

void tst_TestLib::delays() const
{
    QVERIFY(QTest::defaultMouseDelay() >= 0);
    QVERIFY(QTest::defaultKeyDelay() >= 0);
}

void tst_TestLib::reals_data() const
{
    QTest::addColumn<double>("actual");
    QTest::addColumn<double>("expected");

    QTest::newRow("zero") << 0.0 << 0.0;
#define ADDROW(func) QTest::addRow("self-%s", #func) << func() << func()
    ADDROW(qQNaN);
    ADDROW(qInf);
#undef ADDROW // Just used so as to exercise addRow()
    QTest::newRow("infineg") << -qInf() << -qInf();
    QTest::newRow("Sin(turn/4)") << qSin(9 * M_PI_2) << 1.0;
    QTest::newRow("Cos(turn/2)") << qCos(15 * M_PI) << -1.0;
}

void tst_TestLib::reals() const
{
    QFETCH(double, actual);
    QFETCH(double, expected);
    QCOMPARE(actual, expected);
}

QTEST_APPLESS_MAIN(tst_TestLib)

#include "tst_testlib.moc"
