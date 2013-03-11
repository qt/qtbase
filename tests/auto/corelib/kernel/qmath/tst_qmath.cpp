/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

QTEST_APPLESS_MAIN(tst_QMath)

#include "tst_qmath.moc"
