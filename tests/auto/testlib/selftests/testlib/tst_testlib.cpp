// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
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

public:
    tst_TestLib();

private slots:
    void basics() const;
    void delays() const;
    void reals_data() const;
    void reals() const;
};

tst_TestLib::tst_TestLib()
{
    // Set object name, so that it's printed out when some comparison fails.
    // Othewise object address will be printed, which will not allow
    // tst_sefltest to compare the output with expected.
    setObjectName("TestObject");
}

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
