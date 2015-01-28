/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qmath.h>

static const double PI = 3.14159265358979323846264338327950288;

class tst_QMath : public QObject
{
    Q_OBJECT
private slots:
    void fastSinCos();
    void degreesToRadians_data();
    void degreesToRadians();
    void radiansToDegrees_data();
    void radiansToDegrees();
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
    for (int i = 0; i < LOOP_COUNT; ++i) {
        qreal angle = i * 2 * PI / (LOOP_COUNT - 1);
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

    QTest::newRow( "pi" ) << 180.0f << float(M_PI) << 180.0 << PI;
    QTest::newRow( "doublepi" ) << 360.0f << float(2*M_PI) << 360.0 << 2*PI;
    QTest::newRow( "halfpi" ) << 90.0f << float(M_PI_2) << 90.0 << PI/2;

    QTest::newRow( "random" ) << 123.1234567f << 2.1489097058516724f << 123.123456789123456789 << 2.148909707407169856192285627;
    QTest::newRow( "bigrandom" ) << 987654321.9876543f << 17237819.79023679f << 987654321987654321.987654321987654321 << 17237819790236794.0;

    QTest::newRow( "zero" ) << 0.0f << 0.0f << 0.0 << 0.0;

    QTest::newRow( "minuspi" ) << -180.0f << float(-M_PI) << 180.0 << PI;
    QTest::newRow( "minusdoublepi" ) << -360.0f << float(-2*M_PI) << -360.0 << -2*PI;
    QTest::newRow( "minushalfpi" ) << -90.0f << float(-M_PI_2) << -90.0 << -PI/2;

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

    QTest::newRow( "pi" ) << float(M_PI) << 180.0f << PI << 180.0;
    QTest::newRow( "doublepi" ) << float(2*M_PI) << 360.0f << 2*PI << 360.0;
    QTest::newRow( "halfpi" ) << float(M_PI_2) << 90.0f<< PI/2 << 90.0;

    QTest::newRow( "random" ) << 123.1234567f << 7054.454427971739f << 123.123456789123456789 << 7054.4544330781363896676339209079742431640625;
    QTest::newRow( "bigrandom" ) << 987654321.9876543f << 56588424267.74745f << 987654321987654321.987654321987654321 << 56588424267747450880.0;

    QTest::newRow( "zero" ) << 0.0f << 0.0f << 0.0 << 0.0;

    QTest::newRow( "minuspi" ) << float(-M_PI) << -180.0f << -PI << -180.0;
    QTest::newRow( "minusdoublepi" ) << float(-2*M_PI) << -360.0f << -2*PI << -360.0;
    QTest::newRow( "minushalfpi" ) << float(-M_PI_2) << -90.0f << -PI/2 << -90.0;

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
    QTest::newRow("-1") << -1 << 0U;
    QTest::newRow("-128") << -128 << 0U;
    QTest::newRow("-(2^31)") << int(0x80000000) << 0U;
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
    QTest::newRow("2^31") << 2147483648U << 0U;
    QTest::newRow("2^31 + 1") << 2147483649U << 0U;
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
    QTest::newRow("-1") << Q_INT64_C(-1) << Q_UINT64_C(0);
    QTest::newRow("-128") << Q_INT64_C(-128) << Q_UINT64_C(0);
    QTest::newRow("-(2^31)") << -Q_INT64_C(0x80000000) << Q_UINT64_C(0);
    QTest::newRow("-(2^63)") << (qint64)Q_INT64_C(0x8000000000000000) << Q_UINT64_C(0);
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
    QTest::newRow("2^63") << Q_UINT64_C(0x8000000000000000) << Q_UINT64_C(0);
    QTest::newRow("2^63 + 1") << Q_UINT64_C(0x8000000000000001) << Q_UINT64_C(0);
}

void tst_QMath::qNextPowerOfTwo64U()
{
    QFETCH(quint64, input);
    QFETCH(quint64, output);

    QCOMPARE(qNextPowerOfTwo(input), output);
}

QTEST_APPLESS_MAIN(tst_QMath)

#include "tst_qmath.moc"
