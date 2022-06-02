// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
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

private:
    template <typename T> void compare();
    template <typename T> void compare_eq();

private slots:
    void floatCompare_data() { numeric_data<float>(); }
    void floatCompare() { compare<float>(); }
    void floatCompareEq_data() { numeric_data<float>(); }
    void floatCompareEq() { compare_eq<float>(); }
    void doubleCompare_data() { numeric_data<double>(); }
    void doubleCompare() { compare<double>(); }
    void doubleCompareEq_data() { numeric_data<double>(); }
    void doubleCompareEq() { compare_eq<double>(); }
    void intCompare_data() { numeric_data<int>(); }
    void intCompare() { compare<int>(); }
    void intCompareEq_data() { numeric_data<int>(); }
    void intCompareEq() { compare_eq<int>(); }
    void unsignedCompare_data() { numeric_data<unsigned>(); }
    void unsignedCompare() { compare<unsigned>(); }
    void unsignedCompareEq_data() { numeric_data<unsigned>(); }
    void unsignedCompareEq() { compare_eq<unsigned>(); }
    void quint64Compare_data() { numeric_data<quint64>(); }
    void quint64Compare() { compare<quint64>(); }
    void quint64CompareEq_data() { numeric_data<quint64>(); }
    void quint64CompareEq() { compare_eq<quint64>(); }
    void qint64Compare_data() { numeric_data<qint64>(); }
    void qint64Compare() { compare<qint64>(); }
    void qint64CompareEq_data() { numeric_data<qint64>(); }
    void qint64CompareEq() { compare_eq<qint64>(); }
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
        auto tst = QTest::toString(datum);
        delete [] tst;
    }
}

template <typename T>
void tst_toString::compare()
{
    QFETCH(T, datum);

    QBENCHMARK {
        QCOMPARE(datum, datum);
    }
}

template <typename T>
void tst_toString::compare_eq()
{
    QFETCH(T, datum);

    if constexpr (std::numeric_limits<T>::has_quiet_NaN) {
        if (qIsNaN(datum))
            QSKIP("Unlike QCOMPARE, QCOMPARE_EQ doesn't handle NaN specially");
    }

    QBENCHMARK {
        QCOMPARE_EQ(datum, datum);
    }
}

QTEST_MAIN(tst_toString)
#include "tst_tostring.moc"
