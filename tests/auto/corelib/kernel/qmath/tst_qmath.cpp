// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <qmath.h>
#include <qfloat16.h>

class tst_QMath : public QObject
{
    Q_OBJECT
private slots:
    void fastSinCos();
    void degreesToRadians_data();
    void degreesToRadians();
    void radiansToDegrees_data();
    void radiansToDegrees();
    void trigonometry_data();
    void trigonometry();
    void hypotenuse();
    void funcs_data();
    void funcs();
    void qNextPowerOfTwo32S_data();
    void qNextPowerOfTwo32S();
    void qNextPowerOfTwo64S_data();
    void qNextPowerOfTwo64S();
    void qNextPowerOfTwo32U_data();
    void qNextPowerOfTwo32U();
    void qNextPowerOfTwo64U_data();
    void qNextPowerOfTwo64U();
};

void tst_QMath::fastSinCos()
{
    // Test evenly spaced angles from 0 to 2pi radians.
    const int LOOP_COUNT = 100000;
    const qreal loopAngle = 2 * M_PI / (LOOP_COUNT - 1);
    for (int i = 0; i < LOOP_COUNT; ++i) {
        qreal angle = i * loopAngle;
        QVERIFY(qAbs(qSin(angle) - qFastSin(angle)) < 1e-5);
        QVERIFY(qAbs(qCos(angle) - qFastCos(angle)) < 1e-5);
    }
}

void tst_QMath::degreesToRadians_data()
{
    QTest::addColumn<float>("degreesFloat");
    QTest::addColumn<float>("radiansFloat");
    QTest::addColumn<double>("degreesDouble");
    QTest::addColumn<double>("radiansDouble");

    QTest::newRow( "pi" ) << 180.0f << float(M_PI) << 180.0 << M_PI;
    QTest::newRow( "doublepi" ) << 360.0f << float(2 * M_PI) << 360.0 << 2 * M_PI;
    QTest::newRow( "halfpi" ) << 90.0f << float(M_PI_2) << 90.0 << M_PI_2;

    QTest::newRow( "random" ) << 123.1234567f << 2.1489097058516724f << 123.123456789123456789 << 2.148909707407169856192285627;
    QTest::newRow( "bigrandom" ) << 987654321.9876543f << 17237819.79023679f << 987654321987654321.987654321987654321 << 17237819790236794.0;

    QTest::newRow( "zero" ) << 0.0f << 0.0f << 0.0 << 0.0;

    QTest::newRow( "minuspi" ) << -180.0f << float(-M_PI) << 180.0 << M_PI;
    QTest::newRow( "minusdoublepi" ) << -360.0f << float(-2 * M_PI) << -360.0 << -2 * M_PI;
    QTest::newRow( "minushalfpi" ) << -90.0f << float(-M_PI_2) << -90.0 << -M_PI_2;

    QTest::newRow( "minusrandom" ) << -123.1234567f << -2.1489097058516724f << -123.123456789123456789 << -2.148909707407169856192285627;
    QTest::newRow( "minusbigrandom" ) << -987654321.9876543f << -17237819.79023679f << -987654321987654321.987654321987654321 << -17237819790236794.0;
}

void tst_QMath::degreesToRadians()
{
    QFETCH(float, degreesFloat);
    QFETCH(float, radiansFloat);
    QFETCH(double, degreesDouble);
    QFETCH(double, radiansDouble);

    QCOMPARE(qDegreesToRadians(degreesFloat), radiansFloat);
    QCOMPARE(qDegreesToRadians(degreesDouble), radiansDouble);
}

void tst_QMath::radiansToDegrees_data()
{
    QTest::addColumn<float>("radiansFloat");
    QTest::addColumn<float>("degreesFloat");
    QTest::addColumn<double>("radiansDouble");
    QTest::addColumn<double>("degreesDouble");

    QTest::newRow( "pi" ) << float(M_PI) << 180.0f << M_PI << 180.0;
    QTest::newRow( "doublepi" ) << float(2 * M_PI) << 360.0f << 2 * M_PI << 360.0;
    QTest::newRow( "halfpi" ) << float(M_PI_2) << 90.0f << M_PI_2 << 90.0;

    QTest::newRow( "random" ) << 123.1234567f << 7054.454427971739f << 123.123456789123456789 << 7054.4544330781363896676339209079742431640625;
    QTest::newRow( "bigrandom" ) << 987654321.9876543f << 56588424267.74745f << 987654321987654321.987654321987654321 << 56588424267747450880.0;

    QTest::newRow( "zero" ) << 0.0f << 0.0f << 0.0 << 0.0;

    QTest::newRow( "minuspi" ) << float(-M_PI) << -180.0f << -M_PI << -180.0;
    QTest::newRow( "minusdoublepi" ) << float(-2 * M_PI) << -360.0f << -2 * M_PI << -360.0;
    QTest::newRow( "minushalfpi" ) << float(-M_PI_2) << -90.0f << -M_PI_2 << -90.0;

    QTest::newRow( "minusrandom" ) << -123.1234567f << -7054.454427971739f << -123.123456789123456789 << -7054.4544330781363896676339209079742431640625;
    QTest::newRow( "minusbigrandom" ) << -987654321.9876543f << -56588424267.74745f << -987654321987654321.987654321987654321 << -56588424267747450880.0;
}

void tst_QMath::radiansToDegrees()
{
    QFETCH(float, radiansFloat);
    QFETCH(float, degreesFloat);
    QFETCH(double, radiansDouble);
    QFETCH(double, degreesDouble);

    QCOMPARE(qRadiansToDegrees(radiansFloat), degreesFloat);
    QCOMPARE(qRadiansToDegrees(radiansDouble), degreesDouble);
}

void tst_QMath::trigonometry_data()
{
    QTest::addColumn<double>("x");
    QTest::addColumn<double>("y");
    QTest::addColumn<double>("angle");

    QTest::newRow("zero") << 1.0 << 0.0 << 0.0;
    QTest::newRow("turn/4") << 0.0 << 1.0 << M_PI_2;
    QTest::newRow("turn/2") << -1.0 << 0.0 << M_PI;
    QTest::newRow("3*turn/4") << 0.0 << -1.0 << -M_PI_2;
}

void tst_QMath::trigonometry()
{
    QFETCH(const double, x);
    QFETCH(const double, y);
    QFETCH(const double, angle);
    const double hypot = qHypot(x, y);
    QVERIFY(hypot > 0);
    QCOMPARE(qAtan2(y, x), angle);
    QCOMPARE(qSin(angle), y / hypot);
    if (x >= 0 && (y || x)) // aSin() always in right half-plane
        QCOMPARE(qAsin(y / hypot), angle);
    QCOMPARE(qCos(angle), x / hypot);
    if (y >= 0 && (y || x)) // aCos() always in upper half-plane
        QCOMPARE(qAcos(x / hypot), angle);
    if (x > 0) {
        QCOMPARE(qTan(angle), y / x);
        QCOMPARE(qAtan(y / x), angle);
    }
}

void tst_QMath::hypotenuse()
{
    // Correct return-types, particularly when qfloat16 is involved:
    static_assert(std::is_same<decltype(qHypot(qfloat16(1), qfloat16(1), qfloat16(1),
                                               qfloat16(1), qfloat16(1), qfloat16(1),
                                               qfloat16(1), qfloat16(1), qfloat16(1))),
                               qfloat16>::value);
    static_assert(std::is_same<decltype(qHypot(qfloat16(3), qfloat16(4), qfloat16(12))),
                               qfloat16>::value);
    static_assert(std::is_same<decltype(qHypot(qfloat16(3), qfloat16(4), 12.0f)), float>::value);
    static_assert(std::is_same<decltype(qHypot(qfloat16(3), 4.0f, qfloat16(12))), float>::value);
    static_assert(std::is_same<decltype(qHypot(3.0f, qfloat16(4), qfloat16(12))), float>::value);
    static_assert(std::is_same<decltype(qHypot(qfloat16(3), 4.0f, 12.0f)), float>::value);
    static_assert(std::is_same<decltype(qHypot(3.0f, qfloat16(4), 12.0f)), float>::value);
    static_assert(std::is_same<decltype(qHypot(3.0f, 4.0f, qfloat16(12))), float>::value);
    static_assert(std::is_same<decltype(qHypot(qfloat16(3), qfloat16(4))), qfloat16>::value);
    static_assert(std::is_same<decltype(qHypot(3.0f, qfloat16(4))), float>::value);
    static_assert(std::is_same<decltype(qHypot(qfloat16(3), 4.0f)), float>::value);
    static_assert(std::is_same<decltype(qHypot(3.0, qfloat16(4))), double>::value);
    static_assert(std::is_same<decltype(qHypot(qfloat16(3), 4.0)), double>::value);
    static_assert(std::is_same<decltype(qHypot(qfloat16(3), 4)), double>::value);
    static_assert(std::is_same<decltype(qHypot(3, qfloat16(4))), double>::value);
    static_assert(std::is_same<decltype(qHypot(qfloat16(3), 4.0L)), long double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0L, qfloat16(4))), long double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0f, 4.0f)), float>::value);
    static_assert(std::is_same<decltype(qHypot(3.0f, 4.0)), double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0f, 4)), double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0f, 4.0L)), long double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0, 4.0f)), double>::value);
    static_assert(std::is_same<decltype(qHypot(3, 4.0f)), double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0L, 4.0f)), long double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0, 4.0L)), long double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0L, 4.0)), long double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0, 4.0)), double>::value);
    static_assert(std::is_same<decltype(qHypot(3, 4.0)), double>::value);
    static_assert(std::is_same<decltype(qHypot(3.0, 4)), double>::value);
    static_assert(std::is_same<decltype(qHypot(3, 4)), double>::value);

    // Works for all numeric types:
    QCOMPARE(qHypot(3, 4), 5);
    QCOMPARE(qHypot(qfloat16(5), qfloat16(12)), qfloat16(13));
    QCOMPARE(qHypot(3.0f, 4.0f, 12.0f), 13.0f);
    QCOMPARE(qHypot(3.0, 4.0, 12.0, 84.0), 85.0);
    QCOMPARE(qHypot(3.0f, 4.0f, 12.0f, 84.0f, 720.0f), 725.0f);
    QCOMPARE(qHypot(3.0, 4.0, 12.0, 84.0, 3612.0), 3613.0);
    // Integral gets promoted to double:
    QCOMPARE(qHypot(1, 1), M_SQRT2);
    // Caller can mix types freely:
    QCOMPARE(qHypot(3.0f, 4, 12.0, 84.0f, qfloat16(720), 10500), 10525);
    // NaN wins over any finite:
    QCOMPARE(qHypot(3, 4.0, 12.0f, qQNaN()), qQNaN());
    QCOMPARE(qHypot(3, 4.0, qQNaN(), 12.0f), qQNaN());
    QCOMPARE(qHypot(3, qQNaN(), 4.0, 12.0f), qQNaN());
    QCOMPARE(qHypot(qQNaN(), 3, 4.0, 12.0f), qQNaN());
    // but Infinity beats NaN:
    QCOMPARE(qHypot(3, 4.0f, -qInf(), qQNaN()), qInf());
    QCOMPARE(qHypot(3, -qInf(), 4.0f, qQNaN()), qInf());
    QCOMPARE(qHypot(-qInf(), 3, 4.0f, qQNaN()), qInf());
    QCOMPARE(qHypot(qQNaN(), 3, -qInf(), 4.0f), qInf());
    QCOMPARE(qHypot(3, qQNaN(), 4.0f, -qInf()), qInf());
    QCOMPARE(qHypot(3, 4.0f, qQNaN(), -qInf()), qInf());
    // Components whose squares sum to zero don't change the end result:
    const double minD = std::numeric_limits<double>::min();
    QVERIFY(minD * minD + minD * minD == 0); // *NOT* QCOMPARE
    QCOMPARE(qHypot(minD, minD, 12.0), 12.0);
    const float minF = std::numeric_limits<float>::min();
    QVERIFY(minF * minF + minF * minF == 0.0f); // *NOT* QCOMPARE
    QCOMPARE(qHypot(minF, minF, 12.0f), 12.0f);
    const qfloat16 minF16 = std::numeric_limits<qfloat16>::min();
    QVERIFY(minF16 * minF16 + minF16 * minF16 == qfloat16(0)); // *NOT* QCOMPARE
    QCOMPARE(qHypot(minF16, minF16, qfloat16(12)), qfloat16(12));
}

void tst_QMath::funcs_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<int>("floor");
    QTest::addColumn<int>("ceil");
    QTest::addColumn<double>("abs");
    QTest::addColumn<double>("sqrt");
    QTest::addColumn<double>("log");
    QTest::addColumn<double>("exp");
    QTest::addColumn<double>("cube");
    const double nan = qQNaN();

    QTest::newRow("0") << 0.0 << 0 << 0 << 0.0 << 0.0 << nan << 1.0 << 0.0;
    QTest::newRow("1.44")
        << 1.44 << 1 << 2 << 1.44 << 1.2 << 0.36464311358790924 << 4.220695816996552 << 2.985984;
    QTest::newRow("-1.44")
        << -1.44 << -2 << -1 << 1.44 << nan << nan << 0.23692775868212176 << -2.985984;
}

void tst_QMath::funcs()
{
    QFETCH(double, value);
    QTEST(qFloor(value), "floor");
    QTEST(qCeil(value), "ceil");
    QTEST(qFabs(value), "abs");
    if (value >= 0)
        QTEST(qSqrt(value), "sqrt");
    if (value > 0)
        QTEST(qLn(value), "log");
    QTEST(qExp(value), "exp");
    QTEST(qPow(value, 3), "cube");
}

void tst_QMath::qNextPowerOfTwo32S_data()
{
    QTest::addColumn<qint32>("input");
    QTest::addColumn<quint32>("output");

    QTest::newRow("0") << 0 << 1U;
    QTest::newRow("1") << 1 << 2U;
    QTest::newRow("2") << 2 << 4U;
    QTest::newRow("17") << 17 << 32U;
    QTest::newRow("128") << 128 << 256U;
    QTest::newRow("65535") << 65535 << 65536U;
    QTest::newRow("65536") << 65536 << 131072U;
    QTest::newRow("2^30") << (1 << 30) << (1U << 31);
    QTest::newRow("2^30 + 1") << (1 << 30) + 1 << (1U << 31);
    QTest::newRow("2^31 - 1") << 0x7FFFFFFF << (1U<<31);
}

void tst_QMath::qNextPowerOfTwo32S()
{
    QFETCH(qint32, input);
    QFETCH(quint32, output);

    QCOMPARE(qNextPowerOfTwo(input), output);
}

void tst_QMath::qNextPowerOfTwo32U_data()
{
    QTest::addColumn<quint32>("input");
    QTest::addColumn<quint32>("output");

    QTest::newRow("0") << 0U << 1U;
    QTest::newRow("1") << 1U << 2U;
    QTest::newRow("2") << 2U << 4U;
    QTest::newRow("17") << 17U << 32U;
    QTest::newRow("128") << 128U << 256U;
    QTest::newRow("65535") << 65535U << 65536U;
    QTest::newRow("65536") << 65536U << 131072U;
    QTest::newRow("2^30") << (1U << 30) << (1U << 31);
    QTest::newRow("2^30 + 1") << (1U << 30) + 1 << (1U << 31);
    QTest::newRow("2^31 - 1") << 2147483647U << 2147483648U;
}

void tst_QMath::qNextPowerOfTwo32U()
{
    QFETCH(quint32, input);
    QFETCH(quint32, output);

    QCOMPARE(qNextPowerOfTwo(input), output);
}

void tst_QMath::qNextPowerOfTwo64S_data()
{
    QTest::addColumn<qint64>("input");
    QTest::addColumn<quint64>("output");

    QTest::newRow("0") << Q_INT64_C(0) << Q_UINT64_C(1);
    QTest::newRow("1") << Q_INT64_C(1) << Q_UINT64_C(2);
    QTest::newRow("2") << Q_INT64_C(2) << Q_UINT64_C(4);
    QTest::newRow("17") << Q_INT64_C(17) << Q_UINT64_C(32);
    QTest::newRow("128") << Q_INT64_C(128) << Q_UINT64_C(256);
    QTest::newRow("65535") << Q_INT64_C(65535) << Q_UINT64_C(65536);
    QTest::newRow("65536") << Q_INT64_C(65536) << Q_UINT64_C(131072);
    QTest::newRow("2^31 - 1") << Q_INT64_C(2147483647) << Q_UINT64_C(0x80000000);
    QTest::newRow("2^31") << Q_INT64_C(2147483648) << Q_UINT64_C(0x100000000);
    QTest::newRow("2^31 + 1") << Q_INT64_C(2147483649) << Q_UINT64_C(0x100000000);
    QTest::newRow("2^63 - 1") << Q_INT64_C(0x7FFFFFFFFFFFFFFF) << Q_UINT64_C(0x8000000000000000);
}

void tst_QMath::qNextPowerOfTwo64S()
{
    QFETCH(qint64, input);
    QFETCH(quint64, output);

    QCOMPARE(qNextPowerOfTwo(input), output);
}

void tst_QMath::qNextPowerOfTwo64U_data()
{
    QTest::addColumn<quint64>("input");
    QTest::addColumn<quint64>("output");

    QTest::newRow("0") << Q_UINT64_C(0) << Q_UINT64_C(1);
    QTest::newRow("1") << Q_UINT64_C(1) << Q_UINT64_C(2);
    QTest::newRow("2") << Q_UINT64_C(2) << Q_UINT64_C(4);
    QTest::newRow("17") << Q_UINT64_C(17) << Q_UINT64_C(32);
    QTest::newRow("128") << Q_UINT64_C(128) << Q_UINT64_C(256);
    QTest::newRow("65535") << Q_UINT64_C(65535) << Q_UINT64_C(65536);
    QTest::newRow("65536") << Q_UINT64_C(65536) << Q_UINT64_C(131072);
    QTest::newRow("2^63 - 1") << Q_UINT64_C(0x7FFFFFFFFFFFFFFF)  << Q_UINT64_C(0x8000000000000000);
}

void tst_QMath::qNextPowerOfTwo64U()
{
    QFETCH(quint64, input);
    QFETCH(quint64, output);

    QCOMPARE(qNextPowerOfTwo(input), output);
}

QTEST_APPLESS_MAIN(tst_QMath)

#include "tst_qmath.moc"
