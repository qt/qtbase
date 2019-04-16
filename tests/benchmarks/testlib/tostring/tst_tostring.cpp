/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include <QtCore/qmath.h> // pi, e

// Tests for QTest::toString
class tst_toString : public QObject
{
    Q_OBJECT

    template <typename T> void numeric_data();
    template <typename T> void numeric();

private slots:
    void floatTst_data() { numeric_data<float>(); }
    void floatTst() { numeric<float>(); }
    void doubleTst_data() { numeric_data<double>(); }
    void doubleTst() { numeric<double>(); }
    void intTst_data() { numeric_data<int>(); }
    void intTst() { numeric<int>(); }
    void unsignedTst_data() { numeric_data<unsigned>(); }
    void unsignedTst() { numeric<unsigned>(); }
    void quint64Tst_data() { numeric_data<quint64>(); }
    void quint64Tst() { numeric<quint64>(); }
    void qint64Tst_data() { numeric_data<qint64>(); }
    void qint64Tst() { numeric<qint64>(); }
};

template <typename T>
void tst_toString::numeric_data()
{
    QTest::addColumn<T>("datum");
    const bool floaty = std::is_floating_point<T>::value;

    QTest::newRow("zero") << T(0);
    QTest::newRow("one") << T(1);
    if (floaty) {
        QTest::newRow("pi") << T(M_PI);
        QTest::newRow("e") << T(M_E);

        // Stress canonicalisation of leading zeros on exponents:
        QTest::newRow("milli") << T(1e-3);
        QTest::newRow("micro") << T(1e-6);
        QTest::newRow("mu0") << T(.4e-6 * M_PI); // Henry/metre
        QTest::newRow("Planck") << T(662.606876e-36); // Joule.second/turn
    }
    QTest::newRow("2e9") << T(2000000000);
    QTest::newRow("c.s/m") << T(299792458);
    QTest::newRow("Avogadro") << T(6.022045e+23); // things/mol (.996 << 79, so ints overflow to max)

    QTest::newRow("lowest") << std::numeric_limits<T>::lowest();
    QTest::newRow("max") << std::numeric_limits<T>::max();
    if (floaty) {
        QTest::newRow("min") << std::numeric_limits<T>::min();

        if (std::numeric_limits<T>::has_infinity) {
            const T uge = std::numeric_limits<T>::infinity();
            QTest::newRow("inf") << uge;
            QTest::newRow("-inf") << -uge;
        }
        if (std::numeric_limits<T>::has_quiet_NaN)
            QTest::newRow("nan") << std::numeric_limits<T>::quiet_NaN();
    }
}

template <typename T>
void tst_toString::numeric()
{
    QFETCH(T, datum);

    QBENCHMARK {
        QTest::toString<T>(datum);
    }
}

QTEST_MAIN(tst_toString)
#include "tst_tostring.moc"
