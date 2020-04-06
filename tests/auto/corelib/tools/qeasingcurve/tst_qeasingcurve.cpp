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

#include <qeasingcurve.h>

#include <utility> // for std::move()

class tst_QEasingCurve : public QObject
{
    Q_OBJECT
private slots:
    void type();
    void propertyDefaults();
    void valueForProgress_data();
    void valueForProgress();
    void setCustomType();
    void operators();
    void properties();
    void metaTypes();
    void propertyOrderIsNotImportant();
    void bezierSpline_data();
    void bezierSpline();
    void tcbSpline_data();
    void tcbSpline();
    void testCbrtDouble();
    void testCbrtFloat();
    void cpp11();
    void quadraticEquation();
    void streamInOut_data();
    void streamInOut();
};

void tst_QEasingCurve::type()
{
    {
    QEasingCurve curve(QEasingCurve::Linear);
    QCOMPARE(curve.period(), 0.3);
    QCOMPARE(curve.amplitude(), 1.0);

    curve.setPeriod(5);
    curve.setAmplitude(3);
    QCOMPARE(curve.period(), 5.0);
    QCOMPARE(curve.amplitude(), 3.0);

    curve.setType(QEasingCurve::InElastic);
    QCOMPARE(curve.period(), 5.0);
    QCOMPARE(curve.amplitude(), 3.0);
    }

    {
    QEasingCurve curve(QEasingCurve::InElastic);
    QCOMPARE(curve.period(), 0.3);
    QCOMPARE(curve.amplitude(), 1.0);
    curve.setAmplitude(2);
    QCOMPARE(curve.type(), QEasingCurve::InElastic);
    curve.setType(QEasingCurve::Linear);
    }

    {
    // check bounaries
    QEasingCurve curve(QEasingCurve::InCubic);
    QTest::ignoreMessage(QtWarningMsg, "QEasingCurve: Invalid curve type 9999");
    curve.setType((QEasingCurve::Type)9999);
    QCOMPARE(curve.type(), QEasingCurve::InCubic);
    QTest::ignoreMessage(QtWarningMsg, "QEasingCurve: Invalid curve type -9999");
    curve.setType((QEasingCurve::Type)-9999);
    QCOMPARE(curve.type(), QEasingCurve::InCubic);
    QTest::ignoreMessage(QtWarningMsg, QString::fromLatin1("QEasingCurve: Invalid curve type %1")
                        .arg(QEasingCurve::NCurveTypes).toLatin1().constData());
    curve.setType(QEasingCurve::NCurveTypes);
    QCOMPARE(curve.type(), QEasingCurve::InCubic);
    QTest::ignoreMessage(QtWarningMsg, QString::fromLatin1("QEasingCurve: Invalid curve type %1")
                        .arg(QEasingCurve::Custom).toLatin1().constData());
    curve.setType(QEasingCurve::Custom);
    QCOMPARE(curve.type(), QEasingCurve::InCubic);
    QTest::ignoreMessage(QtWarningMsg, QString::fromLatin1("QEasingCurve: Invalid curve type %1")
                        .arg(-1).toLatin1().constData());
    curve.setType((QEasingCurve::Type)-1);
    QCOMPARE(curve.type(), QEasingCurve::InCubic);
    curve.setType(QEasingCurve::Linear);
    QCOMPARE(curve.type(), QEasingCurve::Linear);
    curve.setType(QEasingCurve::CosineCurve);
    QCOMPARE(curve.type(), QEasingCurve::CosineCurve);
    }
}

void tst_QEasingCurve::propertyDefaults()
{
    {
    // checks if the defaults are correct, but also demonstrates a weakness with the API.
    QEasingCurve curve(QEasingCurve::InElastic);
    QCOMPARE(curve.period(), 0.3);
    QCOMPARE(curve.amplitude(), 1.0);
    QCOMPARE(curve.overshoot(), qreal(1.70158));
    curve.setType(QEasingCurve::InBounce);
    QCOMPARE(curve.period(), 0.3);
    QCOMPARE(curve.amplitude(), 1.0);
    QCOMPARE(curve.overshoot(), qreal(1.70158));
    curve.setType(QEasingCurve::Linear);
    QCOMPARE(curve.period(), 0.3);
    QCOMPARE(curve.amplitude(), 1.0);
    QCOMPARE(curve.overshoot(), qreal(1.70158));
    curve.setType(QEasingCurve::InElastic);
    QCOMPARE(curve.period(), 0.3);
    QCOMPARE(curve.amplitude(), 1.0);
    QCOMPARE(curve.overshoot(), qreal(1.70158));
    curve.setPeriod(0.4);
    curve.setAmplitude(0.6);
    curve.setOvershoot(1.0);
    curve.setType(QEasingCurve::Linear);
    QCOMPARE(curve.period(), 0.4);
    QCOMPARE(curve.amplitude(), 0.6);
    QCOMPARE(curve.overshoot(), 1.0);
    curve.setType(QEasingCurve::InElastic);
    QCOMPARE(curve.period(), 0.4);
    QCOMPARE(curve.amplitude(), 0.6);
    QCOMPARE(curve.overshoot(), 1.0);
    }
}

typedef QList<int> IntList;
typedef QList<qreal> RealList;

void tst_QEasingCurve::valueForProgress_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<IntList>("at");
    QTest::addColumn<RealList>("expected");
    // automatically generated.
    // note that values are scaled from range [0,1] to range [0, 100] in order to store them as
    // integer values and avoid fp inaccuracies
    QTest::newRow("Linear") << int(QEasingCurve::Linear)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.1000 << 0.2000 << 0.3000 << 0.4000 << 0.5000 << 0.6000 << 0.7000 << 0.8000 << 0.9000 << 1.0000);

    QTest::newRow("InQuad") << int(QEasingCurve::InQuad)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0100 << 0.0400 << 0.0900 << 0.1600 << 0.2500 << 0.3600 << 0.4900 << 0.6400 << 0.8100 << 1.0000);

    QTest::newRow("OutQuad") << int(QEasingCurve::OutQuad)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.1900 << 0.3600 << 0.5100 << 0.6400 << 0.7500 << 0.8400 << 0.9100 << 0.9600 << 0.9900 << 1.0000);

    QTest::newRow("InOutQuad") << int(QEasingCurve::InOutQuad)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0200 << 0.0800 << 0.1800 << 0.3200 << 0.5000 << 0.6800 << 0.8200 << 0.9200 << 0.9800 << 1.0000);

    QTest::newRow("OutInQuad") << int(QEasingCurve::OutInQuad)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.1800 << 0.3200 << 0.4200 << 0.4800 << 0.5000 << 0.5200 << 0.5800 << 0.6800 << 0.8200 << 1.0000);

    QTest::newRow("InCubic") << int(QEasingCurve::InCubic)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0010 << 0.0080 << 0.0270 << 0.0640 << 0.1250 << 0.2160 << 0.3430 << 0.5120 << 0.7290 << 1.0000);

    QTest::newRow("OutCubic") << int(QEasingCurve::OutCubic)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.2710 << 0.4880 << 0.6570 << 0.7840 << 0.8750 << 0.9360 << 0.9730 << 0.9920 << 0.9990 << 1.0000);

    QTest::newRow("InOutCubic") << int(QEasingCurve::InOutCubic)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0040 << 0.0320 << 0.1080 << 0.2560 << 0.5000 << 0.7440 << 0.8920 << 0.9680 << 0.9960 << 1.0000);

    QTest::newRow("OutInCubic") << int(QEasingCurve::OutInCubic)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.2440 << 0.3920 << 0.4680 << 0.4960 << 0.5000 << 0.5040 << 0.5320 << 0.6080 << 0.7560 << 1.0000);

    QTest::newRow("InQuart") << int(QEasingCurve::InQuart)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0001 << 0.0016 << 0.0081 << 0.0256 << 0.0625 << 0.1296 << 0.2401 << 0.4096 << 0.6561 << 1.0000);

    QTest::newRow("OutQuart") << int(QEasingCurve::OutQuart)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.3439 << 0.5904 << 0.7599 << 0.8704 << 0.9375 << 0.9744 << 0.9919 << 0.9984 << 0.9999 << 1.0000);

    QTest::newRow("InOutQuart") << int(QEasingCurve::InOutQuart)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0008 << 0.0128 << 0.0648 << 0.2048 << 0.5000 << 0.7952 << 0.9352 << 0.9872 << 0.9992 << 1.0000);

    QTest::newRow("OutInQuart") << int(QEasingCurve::OutInQuart)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.2952 << 0.4352 << 0.4872 << 0.4992 << 0.5000 << 0.5008 << 0.5128 << 0.5648 << 0.7048 << 1.0000);

    QTest::newRow("InQuint") << int(QEasingCurve::InQuint)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0000 << 0.0003 << 0.0024 << 0.0102 << 0.0313 << 0.0778 << 0.1681 << 0.3277 << 0.5905 << 1.0000);

    QTest::newRow("OutQuint") << int(QEasingCurve::OutQuint)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.4095 << 0.6723 << 0.8319 << 0.9222 << 0.9688 << 0.9898 << 0.9976 << 0.9997 << 1.0000 << 1.0000);

    QTest::newRow("InOutQuint") << int(QEasingCurve::InOutQuint)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0002 << 0.0051 << 0.0389 << 0.1638 << 0.5000 << 0.8362 << 0.9611 << 0.9949 << 0.9998 << 1.0000);

    QTest::newRow("OutInQuint") << int(QEasingCurve::OutInQuint)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.3362 << 0.4611 << 0.4949 << 0.4998 << 0.5000 << 0.5002 << 0.5051 << 0.5389 << 0.6638 << 1.0000);

    QTest::newRow("InSine") << int(QEasingCurve::InSine)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0123 << 0.0489 << 0.1090 << 0.1910 << 0.2929 << 0.4122 << 0.5460 << 0.6910 << 0.8436 << 1.0000);

    QTest::newRow("OutSine") << int(QEasingCurve::OutSine)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.1564 << 0.3090 << 0.4540 << 0.5878 << 0.7071 << 0.8090 << 0.8910 << 0.9511 << 0.9877 << 1.0000);

    QTest::newRow("InOutSine") << int(QEasingCurve::InOutSine)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0245 << 0.0955 << 0.2061 << 0.3455 << 0.5000 << 0.6545 << 0.7939 << 0.9045 << 0.9755 << 1.0000);

    QTest::newRow("OutInSine") << int(QEasingCurve::OutInSine)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.1545 << 0.2939 << 0.4045 << 0.4755 << 0.5000 << 0.5245 << 0.5955 << 0.7061 << 0.8455 << 1.0000);

    QTest::newRow("InExpo") << int(QEasingCurve::InExpo)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0010 << 0.0029 << 0.0068 << 0.0146 << 0.0303 << 0.0615 << 0.1240 << 0.2490 << 0.4990 << 1.0000);

    QTest::newRow("OutExpo") << int(QEasingCurve::OutExpo)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.5005 << 0.7507 << 0.8759 << 0.9384 << 0.9697 << 0.9854 << 0.9932 << 0.9971 << 0.9990 << 1.0000);

    QTest::newRow("InOutExpo") << int(QEasingCurve::InOutExpo)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0015 << 0.0073 << 0.0308 << 0.1245 << 0.5003 << 0.8754 << 0.9692 << 0.9927 << 0.9985 << 1.0000);

    QTest::newRow("OutInExpo") << int(QEasingCurve::OutInExpo)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.3754 << 0.4692 << 0.4927 << 0.4985 << 0.5000 << 0.5015 << 0.5073 << 0.5308 << 0.6245 << 1.0000);

    QTest::newRow("InCirc") << int(QEasingCurve::InCirc)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0050 << 0.0202 << 0.0461 << 0.0835 << 0.1340 << 0.2000 << 0.2859 << 0.4000 << 0.5641 << 1.0000);

    QTest::newRow("OutCirc") << int(QEasingCurve::OutCirc)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.4359 << 0.6000 << 0.7141 << 0.8000 << 0.8660 << 0.9165 << 0.9539 << 0.9798 << 0.9950 << 1.0000);

    QTest::newRow("InOutCirc") << int(QEasingCurve::InOutCirc)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0101 << 0.0417 << 0.1000 << 0.2000 << 0.5000 << 0.8000 << 0.9000 << 0.9583 << 0.9899 << 1.0000);

    QTest::newRow("OutInCirc") << int(QEasingCurve::OutInCirc)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.3000 << 0.4000 << 0.4583 << 0.4899 << 0.5000 << 0.5101 << 0.5417 << 0.6000 << 0.7000 << 1.0000);

    QTest::newRow("InElastic") << int(QEasingCurve::InElastic)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0020 << -0.0020 << -0.0039 << 0.0156 << -0.0156 << -0.0313 << 0.1250 << -0.1250 << -0.2500 << 1.0000);

    QTest::newRow("OutElastic") << int(QEasingCurve::OutElastic)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 1.2500 << 1.1250 << 0.8750 << 1.0313 << 1.0156 << 0.9844 << 1.0039 << 1.0020 << 0.9980 << 1.0000);

    QTest::newRow("InOutElastic") << int(QEasingCurve::InOutElastic)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << -0.0010 << 0.0078 << -0.0156 << -0.0625 << 0.5000 << 1.0625 << 1.0156 << 0.9922 << 1.0010 << 1.0000);

    QTest::newRow("OutInElastic") << int(QEasingCurve::OutInElastic)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.3750 << 0.5625 << 0.4922 << 0.4980 << 0.5000 << 0.4961 << 0.5078 << 0.5313 << 0.2500 << 1.0000);

    QTest::newRow("InBack") << int(QEasingCurve::InBack)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << -0.0143 << -0.0465 << -0.0802 << -0.0994 << -0.0877 << -0.0290 << 0.0929 << 0.2942 << 0.5912 << 1.0000);

    QTest::newRow("OutBack") << int(QEasingCurve::OutBack)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.4088 << 0.7058 << 0.9071 << 1.0290 << 1.0877 << 1.0994 << 1.0802 << 1.0465 << 1.0143 << 1.0000);

    QTest::newRow("InOutBack") << int(QEasingCurve::InOutBack)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << -0.0375 << -0.0926 << -0.0788 << 0.0899 << 0.5000 << 0.9101 << 1.0788 << 1.0926 << 1.0375 << 1.0000);

    QTest::newRow("OutInBack") << int(QEasingCurve::OutInBack)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.3529 << 0.5145 << 0.5497 << 0.5232 << 0.5000 << 0.4768 << 0.4503 << 0.4855 << 0.6471 << 1.0000);

    QTest::newRow("InBounce") << int(QEasingCurve::InBounce)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0119 << 0.0600 << 0.0694 << 0.2275 << 0.2344 << 0.0900 << 0.3194 << 0.6975 << 0.9244 << 1.0000);

    QTest::newRow("OutBounce") << int(QEasingCurve::OutBounce)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0756 << 0.3025 << 0.6806 << 0.9100 << 0.7656 << 0.7725 << 0.9306 << 0.9400 << 0.9881 << 1.0000);

    QTest::newRow("InOutBounce") << int(QEasingCurve::InOutBounce)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0300 << 0.1138 << 0.0450 << 0.3488 << 0.5000 << 0.6512 << 0.9550 << 0.8863 << 0.9700 << 1.0000);

    QTest::newRow("OutInBounce") << int(QEasingCurve::OutInBounce)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.1513 << 0.4100 << 0.2725 << 0.4400 << 0.5000 << 0.5600 << 0.7275 << 0.5900 << 0.8488 << 1.0000);

    QTest::newRow("InCurve") << int(QEasingCurve::InCurve)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0245 << 0.1059 << 0.2343 << 0.3727 << 0.5000 << 0.6055 << 0.7000 << 0.8000 << 0.9000 << 1.0000);

    QTest::newRow("OutCurve") << int(QEasingCurve::OutCurve)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.1000 << 0.2000 << 0.3000 << 0.3945 << 0.5000 << 0.6273 << 0.7657 << 0.8941 << 0.9755 << 1.0000);

    QTest::newRow("SineCurve") << int(QEasingCurve::SineCurve)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.0000 << 0.0955 << 0.3455 << 0.6545 << 0.9045 << 1.0000 << 0.9045 << 0.6545 << 0.3455 << 0.0955 << 0.0000);

    QTest::newRow("CosineCurve") << int(QEasingCurve::CosineCurve)
         << (IntList()  << 0 << 10 << 20 << 30 << 40 << 50 << 60 << 70 << 80 << 90 << 100)
         << (RealList() << 0.5000 << 0.7939 << 0.9755 << 0.9755 << 0.7939 << 0.5000 << 0.2061 << 0.0245 << 0.0245 << 0.2061 << 0.5000);

}

/*
  "fixedpoint" number that is scaled up by 10000.
  This is to work around two bugs (precision and rounding error) in QString::setNum().
  It does not trim off trailing zeros. This is good, just to emphasize the precision.
*/
QString fixedToString(int value)
{
    QString str;
    if (value < 0) {
        str+= QLatin1Char('-');
        value = -value;
    }

    QString digitArg(QLatin1String("%1."));
    for (int i = 10000; i >= 1; i/=10) {
        int digit = value/i;
        value -= digit*i;
        str.append(digitArg.arg(digit));
        digitArg = QLatin1String("%1");
    }
    return str;
}

void tst_QEasingCurve::valueForProgress()
{
#if 0
    // used to generate data tables...
    QFile out;
    out.open(stdout, QIODevice::WriteOnly);
    for (int c = QEasingCurve::Linear; c < QEasingCurve::NCurveTypes - 1; ++c) {
        QEasingCurve curve((QEasingCurve::Type)c);
        QMetaObject mo = QEasingCurve::staticMetaObject;
        QString strCurve = QLatin1String(mo.enumerator(mo.indexOfEnumerator("Type")).key(c));
        QString strInputs;
        QString strOutputs;

        for (int t = 0; t <= 100; t+= 10) {
            qreal ease = curve.valueForProgress(t/qreal(100));
            strInputs += QString::fromLatin1(" << %1").arg(t);
            strOutputs += " << " + fixedToString(qRound(ease*10000));
        }
        QString str = QString::fromLatin1("    QTest::newRow(\"%1\") << int(QEasingCurve::%2)\n"
                                                "         << (IntList() %3)\n"
                                                "         << (RealList()%4);\n\n")
                                      .arg(strCurve)
                                      .arg(strCurve)
                                      .arg(strInputs)
                                      .arg(strOutputs);
        out.write(str.toLatin1().constData());
    }
    out.close();
    exit(1);
#else
    QFETCH(int, type);
    QFETCH(IntList, at);
    QFETCH(RealList, expected);

    QEasingCurve curve((QEasingCurve::Type)type);
    // in theory the baseline should't have an error of more than 0.00005 due to how its rounded,
    // but due to FP imprecision, we have to adjust the error a bit more.
    const qreal errorBound = 0.00006;
    for (int i = 0; i < at.count(); ++i) {
        const qreal ex = expected.at(i);
        const qreal error = qAbs(ex - curve.valueForProgress(at.at(i)/qreal(100)));
        QVERIFY(error <= errorBound);
    }

    if (type != QEasingCurve::SineCurve && type != QEasingCurve::CosineCurve) {
        QVERIFY( !(curve.valueForProgress(0) > 0) );
        QVERIFY( !(curve.valueForProgress(1) < 1) );
    }
#endif
}

static qreal discreteEase(qreal progress)
{
    return qFloor(progress * 10) / qreal(10.0);
}

void tst_QEasingCurve::setCustomType()
{
    QEasingCurve curve;
    curve.setCustomType(&discreteEase);
    QCOMPARE(curve.type(), QEasingCurve::Custom);
    QCOMPARE(curve.valueForProgress(0.0), 0.0);
    QCOMPARE(curve.valueForProgress(0.05), 0.0);
    QCOMPARE(curve.valueForProgress(0.10), 0.1);
    QCOMPARE(curve.valueForProgress(0.15), 0.1);
    QCOMPARE(curve.valueForProgress(0.20), 0.2);
    QCOMPARE(curve.valueForProgress(0.25), 0.2);
    // QTBUG-69947, MinGW 7.3, 8.1 x86 returns 0.2
#if defined(Q_CC_MINGW)
#if !defined(__GNUC__) || defined(__MINGW64__)
    QCOMPARE(curve.valueForProgress(0.30), 0.3);
#endif
#endif
    QCOMPARE(curve.valueForProgress(0.35), 0.3);
    QCOMPARE(curve.valueForProgress(0.999999), 0.9);

    curve.setType(QEasingCurve::Linear);
    QCOMPARE(curve.type(), QEasingCurve::Linear);
    QCOMPARE(curve.valueForProgress(0.0), 0.0);
    QCOMPARE(curve.valueForProgress(0.1), 0.1);
    QCOMPARE(curve.valueForProgress(0.5), 0.5);
    QCOMPARE(curve.valueForProgress(0.99), 0.99);
}

void tst_QEasingCurve::operators()
{
    { // member-swap()
        QEasingCurve ec1, ec2;
        ec2.setCustomType(&discreteEase);
        ec1.swap(ec2);
        QCOMPARE(ec1.type(), QEasingCurve::Custom);
    }

    // operator=
    QEasingCurve curve;
    QEasingCurve curve2;
    curve.setCustomType(&discreteEase);
    curve2 = curve;
    QCOMPARE(curve2.type(), QEasingCurve::Custom);
    QCOMPARE(curve2.valueForProgress(0.0), 0.0);
    QCOMPARE(curve2.valueForProgress(0.05), 0.0);
    QCOMPARE(curve2.valueForProgress(0.15), 0.1);
    QCOMPARE(curve2.valueForProgress(0.25), 0.2);
    QCOMPARE(curve2.valueForProgress(0.35), 0.3);
    QCOMPARE(curve2.valueForProgress(0.999999), 0.9);

    // operator==
    curve.setType(QEasingCurve::InBack);
    curve2 = curve;
    curve2.setOvershoot(qreal(1.70158));
    QCOMPARE(curve.overshoot(), curve2.overshoot());
    QVERIFY(curve2 == curve);

    curve.setOvershoot(3.0);
    QVERIFY(curve2 != curve);
    curve2.setOvershoot(3.0);
    QVERIFY(curve2 == curve);

    curve2.setType(QEasingCurve::Linear);
    QCOMPARE(curve.overshoot(), curve2.overshoot());
    QVERIFY(curve2 != curve);
    curve2.setType(QEasingCurve::InBack);
    QCOMPARE(curve.overshoot(), curve2.overshoot());
    QVERIFY(curve2 == curve);

    QEasingCurve curve3;
    QEasingCurve curve4;
    curve4.setAmplitude(curve4.amplitude());
    QEasingCurve curve5;
    curve5.setAmplitude(0.12345);
    QVERIFY(curve3 == curve4); // default value and not assigned
    QVERIFY(curve3 != curve5); // unassinged and other value
    QVERIFY(curve4 != curve5);
}

class tst_QEasingProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing)
public:
    tst_QEasingProperties(QObject *parent = 0) : QObject(parent) {}

    QEasingCurve easing() const { return e; }
    void setEasing(const QEasingCurve& value) { e = value; }

private:
    QEasingCurve e;
};

// Test getting and setting easing properties via the metaobject system.
void tst_QEasingCurve::properties()
{
    tst_QEasingProperties obj;

    QEasingCurve inOutBack(QEasingCurve::InOutBack);
    qreal overshoot = 1.5;
    inOutBack.setOvershoot(overshoot);
    qreal amplitude = inOutBack.amplitude();
    qreal period = inOutBack.period();

    obj.setEasing(inOutBack);

    QEasingCurve easing = qvariant_cast<QEasingCurve>(obj.property("easing"));
    QCOMPARE(easing.type(), QEasingCurve::InOutBack);
    QCOMPARE(easing.overshoot(), overshoot);
    QCOMPARE(easing.amplitude(), amplitude);
    QCOMPARE(easing.period(), period);

    QEasingCurve linear(QEasingCurve::Linear);
    overshoot = linear.overshoot();
    amplitude = linear.amplitude();
    period = linear.period();

    obj.setProperty("easing",
                    QVariant::fromValue(QEasingCurve(QEasingCurve::Linear)));

    easing = qvariant_cast<QEasingCurve>(obj.property("easing"));
    QCOMPARE(easing.type(), QEasingCurve::Linear);
    QCOMPARE(easing.overshoot(), overshoot);
    QCOMPARE(easing.amplitude(), amplitude);
    QCOMPARE(easing.period(), period);
}

void tst_QEasingCurve::metaTypes()
{
    QVERIFY(QMetaType::type("QEasingCurve") == QMetaType::QEasingCurve);

    QCOMPARE(QByteArray(QMetaType::typeName(QMetaType::QEasingCurve)),
             QByteArray("QEasingCurve"));

    QVERIFY(QMetaType::isRegistered(QMetaType::QEasingCurve));

    QVERIFY(qMetaTypeId<QEasingCurve>() == QMetaType::QEasingCurve);
}

/*
  Test to ensure that regardless of what order properties are set, they should produce the same
  behavior.
 */
void tst_QEasingCurve::propertyOrderIsNotImportant()
{

    QEasingCurve c1;
    c1.setPeriod(1);
    c1.setType(QEasingCurve::OutSine);
    QVERIFY(c1.valueForProgress(0.75) > 0.9);

    QEasingCurve c2;
    c2.setType(QEasingCurve::OutSine);
    c2.setPeriod(1);

    QCOMPARE(c1.valueForProgress(0.75), c2.valueForProgress(0.75));
}

void tst_QEasingCurve::bezierSpline_data()
{
    QTest::addColumn<QString>("definition");
    QTest::addColumn<IntList>("at");
    QTest::addColumn<RealList>("expected");

    QTest::newRow("EasingCurve") << QString::fromLatin1("0.2,0 0.6,0.09 0.7,1.0 0.7,0.97 0.74,0.96 0.74,0.95 0.81,0.97 0.9,0.97 1,1")
         << (IntList()  << 0 << 70 << 74 << 100)
         << (RealList() << 0.0000 << 1.0000 << 0.9500 << 1.0000);

    //This curve is likely to be numerical instable
    QTest::newRow("NastyCurve") << QString::fromLatin1("0.2,0.2 0.126667,0.646667 0.2,0.8 0.624,0.984 0.930667,0.946667 1,1")
         << (IntList()  << 0 << 20 << 30 << 50 << 75 << 100)
         << (RealList() << 0.0000 << 0.8000 << 0.8402 << 0.9029 << 0.9515 << 1.0000);

    QTest::newRow("ComplexCurve") << QString::fromLatin1("0,0.47174849 0.17393079,0.35634291 0.18950309,0.47179766 0.2487779,0.91126755 "
                                             "0.27029205,-0.11275513 0.33421971,0.12062718 0.41170105,-0.10157488 0.4140625,0.16796875 "
                                             "0.4140625,0.16796875 0.4140625,0.16796875 0.59658877,0.36978503 0.67931151,0.89255893 0.711253,0.44658283 "
                                             "0.88203125,0.43671875 0.88203125,0.43671875 0.86087213,0.78786873 0.99609375,0.4921875 1,1")
         << (IntList()  << 0 << 15 << 20 << 30 << 40 << 70 << 50 << 80 << 100)
         << (RealList() << 0.0000 << 0.4134 << 0.5367 << 0.1107 << 0.0505 << 0.7299 << 0.3030 << 0.4886 << 1.0000);
}

static inline void setupBezierSpline(QEasingCurve *easingCurve, const QString &string)
{
    QStringList pointStr = string.split(QLatin1Char(' '));

    QVector<QPointF> points;
    foreach (const QString &str, pointStr) {
        QStringList coordStr = str.split(QLatin1Char(','));
        QPointF point(coordStr.first().toDouble(), coordStr.last().toDouble());
        points.append(point);
    }

    QVERIFY(points.count() % 3 == 0);

    for (int i = 0; i < points.count() / 3; i++) {
        QPointF c1 = points.at(i * 3);
        QPointF c2 = points.at(i * 3 + 1);
        QPointF p1 = points.at(i * 3 + 2);
        easingCurve->addCubicBezierSegment(c1, c2, p1);
    }
}

void tst_QEasingCurve::bezierSpline()
{
    QFETCH(QString, definition);
    QFETCH(IntList, at);
    QFETCH(RealList, expected);

    QEasingCurve bezierEasingCurve(QEasingCurve::BezierSpline);
    setupBezierSpline(&bezierEasingCurve, definition);

    const qreal errorBound = 0.002;
    for (int i = 0; i < at.count(); ++i) {
        const qreal ex = expected.at(i);
        const qreal value = bezierEasingCurve.valueForProgress(at.at(i)/qreal(100));
        const qreal error = qAbs(ex - value);
        if (error > errorBound)
            QCOMPARE(value, ex);
        QVERIFY(error <= errorBound);
    }

    QVERIFY( !(bezierEasingCurve.valueForProgress(0) > 0) );
    QVERIFY( !(bezierEasingCurve.valueForProgress(1) < 1) );
}

void tst_QEasingCurve::tcbSpline_data()
{
    QTest::addColumn<QString>("definition");
    QTest::addColumn<IntList>("at");
    QTest::addColumn<RealList>("expected");

    QTest::newRow("NegativeCurved") << QString::fromLatin1("0.0,0.0,0,0,0 0.4,0.8,0.0,1,0.0 1.0,1.0,0.0,0.0,0")
         << (IntList()  << 0 << 66 << 73 << 100)
         << (RealList() << 0.0000 << 0.9736 << 0.9774 << 1.0000);

    //This curve is likely to be numerical instable
    QTest::newRow("Corner") << QString::fromLatin1("0.0,0.0,0,0,0 0.4,0.8,0.0,-1,0.0 1.0,1.0,0.0,0.0,0")
         << (IntList()  << 0 << 20 << 30 << 50 << 75 << 100)
         << (RealList() << 0.0000 << 0.3999 << 0.5996 << 0.8334 << 0.9166 << 1.0000);

    QTest::newRow("RoundCurve") << QString::fromLatin1("0.0,0.0,0,0,0 0.4,0.8,0.0,0,0.0 1.0,1.0,0.0,0.0,0")
         << (IntList()  << 0 << 15 << 20 << 30 << 40 << 70 << 50 << 80 << 100)
         << (RealList() << 0.0000 << 0.3478 << 0.4663 << 0.6664 << 0.8000 << 0.9399 << 0.8746 << 0.9567 << 1.0000);

    QTest::newRow("Bias") << QString::fromLatin1("0.0,0.0,0,0,0 0.4,0.8,0.1,0,1.0 1.0,1.0,0.0,0.0,0")
         << (IntList()  << 0 << 15 << 20 << 30 << 40 << 70 << 50 << 80 << 100)
         << (RealList() << 0.0000 << 0.2999 << 0.3998 << 0.5997 << 0.8000 << 0.9676 << 0.9136 << 0.9725 << 1.0000);
}

static inline void setupTCBSpline(QEasingCurve *easingCurve, const QString &string)
{
    QStringList pointStr = string.split(QLatin1Char(' '));

    foreach (const QString &str, pointStr) {
        QStringList coordStr = str.split(QLatin1Char(','));
        Q_ASSERT(coordStr.count() == 5);
        QPointF point(coordStr.first().toDouble(), coordStr.at(1).toDouble());
        qreal t = coordStr.at(2).toDouble();
        qreal c = coordStr.at(3).toDouble();
        qreal b = coordStr.at(4).toDouble();
        easingCurve->addTCBSegment(point, t, c ,b);
    }
}

void tst_QEasingCurve::tcbSpline()
{
    QFETCH(QString, definition);
    QFETCH(IntList, at);
    QFETCH(RealList, expected);

    QEasingCurve tcbEasingCurve(QEasingCurve::TCBSpline);
    setupTCBSpline(&tcbEasingCurve, definition);

    const qreal errorBound = 0.002;
    for (int i = 0; i < at.count(); ++i) {
        const qreal ex = expected.at(i);
        const qreal value = tcbEasingCurve.valueForProgress(at.at(i)/qreal(100));
        const qreal error = qAbs(ex - value);
        if (error > errorBound)
            QCOMPARE(value, ex);
        QVERIFY(error <= errorBound);
    }

    QVERIFY( !(tcbEasingCurve.valueForProgress(0) > 0) );
    QVERIFY( !(tcbEasingCurve.valueForProgress(1) < 1) );
}

/*This is single precision code for a cubic root used inside the spline easing curve.
  This code is tested here explicitly. See: qeasingcurve.cpp */

float static inline _fast_cbrt(float x)
{
    union {
        float f;
        quint32 i;
    } ux;

    const unsigned int B1 = 709921077;

    ux.f = x;
    ux.i = (ux.i / 3 + B1);

    return ux.f;
}

/*This is double precision code for a cubic root used inside the spline easing curve.
  This code is tested here explicitly. See: qeasingcurve.cpp */

double static inline _fast_cbrt(double d)
{
    union {
        double d;
        quint32 pt[2];
    } ut, ux;

    const unsigned int B1 = 715094163;


#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    const int h0 = 1;
#else
    const int h0 = 0;
#endif
    ut.d = 0.0;
    ux.d = d;

    quint32 hx = ux.pt[h0]; //high word of d
    ut.pt[h0] = hx/3 + B1;

    return ut.d;
}

void tst_QEasingCurve::testCbrtDouble()
{
    const double errorBound = 0.0001;

    for (int i = 0; i < 100000; i++) {
        double d = double(i) / 1000.0;
        double t = _fast_cbrt(d);

        const double t_cubic = t * t * t;
        const double f = t_cubic + t_cubic + d;
        if (f != 0.0)
            t = t * (t_cubic + d + d) / f;

        double expected = std::pow(d, 1.0/3.0);

        const qreal error = qAbs(expected - t);

        if (!(error < errorBound)) {
            qWarning() << d;
            qWarning() << error;
        }

        QVERIFY(error < errorBound);
    }
}

void tst_QEasingCurve::testCbrtFloat()
{
    const float errorBound = 0.0005f;

    for (int i = 0; i < 100000; i++) {
        float f = float(i) / 1000.0f;
        float t = _fast_cbrt(f);

        const float t_cubic = t * t * t;
        const float fac = t_cubic + t_cubic + f;
        if (fac != 0.0f)
            t = t * (t_cubic + f + f) / fac;

        float expected = std::pow(f, float(1.0/3.0));

        const qreal error = qAbs(expected - t);

        if (!(error < errorBound)) {
            qWarning() << f;
            qWarning() << error;
        }

        QVERIFY(error < errorBound);
    }
}

void tst_QEasingCurve::cpp11()
{
    {
    QEasingCurve ec( QEasingCurve::InOutBack );
    QEasingCurve copy = std::move(ec); // move ctor
    QCOMPARE( copy.type(), QEasingCurve::InOutBack );
    QVERIFY( *reinterpret_cast<void**>(&ec) == 0 );
    }
    {
    QEasingCurve ec( QEasingCurve::InOutBack );
    QEasingCurve copy;
    const QEasingCurve::Type type = copy.type();
    copy = std::move(ec); // move assignment op
    QCOMPARE( copy.type(), QEasingCurve::InOutBack );
    QCOMPARE( ec.type(), type );
    }
}

void tst_QEasingCurve::quadraticEquation() {
    // We find the value for a given time by solving a cubic equation.
    //     ax^3 + bx^2 + cx + d = 0
    // However, the solver also needs to take care of cases where a = 0,
    // b = 0 or c = 0, and the equation becomes quadratic, linear or invalid.
    // A naive cubic solver might divide by zero and return nan, even
    // when the solution is a real number.
    // This test should triggers those cases.

    {
        // If the control points are spaced 1/3 apart of the distance of the
        // start- and endpoint, the equation becomes linear.
        QEasingCurve test(QEasingCurve::BezierSpline);
        const qreal p1 = 1.0 / 3.0;
        const qreal p2 = 1.0 - 1.0 / 3.0;
        const qreal p3 = 1.0;

        test.addCubicBezierSegment(QPointF(p1, 0.0), QPointF(p2, 1.0), QPointF(p3, 1.0));
        QVERIFY(qAbs(test.valueForProgress(0.25) - 0.15625) < 1e-6);
        QVERIFY(qAbs(test.valueForProgress(0.5) - 0.5) < 1e-6);
        QVERIFY(qAbs(test.valueForProgress(0.75) - 0.84375) < 1e-6);
    }

    {
        // If both the start point and the first control point
        // are placed a 0.0, and the second control point is
        // placed at 1/3, we get a case where a = 0 and b != 0
        // i.e. a quadratic equation.
        QEasingCurve test(QEasingCurve::BezierSpline);
        const qreal p1 = 0.0;
        const qreal p2 = 1.0 / 3.0;
        const qreal p3 = 1.0;
        test.addCubicBezierSegment(QPointF(p1, 0.0), QPointF(p2, 1.0), QPointF(p3, 1.0));
        QVERIFY(qAbs(test.valueForProgress(0.25) - 0.5) < 1e-6);
        QVERIFY(qAbs(test.valueForProgress(0.5) - 0.792893) < 1e-6);
        QVERIFY(qAbs(test.valueForProgress(0.75) - 0.950962) < 1e-6);
    }

    {
        // If both the start point and the first control point
        // are placed a 0.0, and the second control point is
        // placed close to 1/3, we get a case where a = ~0 and b != 0.
        // It's not truly a quadratic equation, but should be treated
        // as one, because it causes some cubic solvers to fail.
        QEasingCurve test(QEasingCurve::BezierSpline);
        const qreal p1 = 0.0;
        const qreal p2 = 1.0 / 3.0 + 1e-6;
        const qreal p3 = 1.0;
        test.addCubicBezierSegment(QPointF(p1, 0.0), QPointF(p2, 1.0), QPointF(p3, 1.0));
        QVERIFY(qAbs(test.valueForProgress(0.25) - 0.499999) < 1e-6);
        QVERIFY(qAbs(test.valueForProgress(0.5) - 0.792892) < 1e-6);
        QVERIFY(qAbs(test.valueForProgress(0.75) - 0.950961) < 1e-6);
    }

    {
        // A bad case, where the segment is of zero length.
        // However, it might still happen in user code,
        // and we should return a sensible answer.
        QEasingCurve test(QEasingCurve::BezierSpline);
        const qreal p0 = 0.0;
        const qreal p1 = p0;
        const qreal p2 = p0;
        const qreal p3 = p0;
        test.addCubicBezierSegment(QPointF(p1, 0.0), QPointF(p2, 1.0), QPointF(p3, 1.0));
        test.addCubicBezierSegment(QPointF(p3, 1.0), QPointF(1.0, 1.0), QPointF(1.0, 1.0));
        QCOMPARE(test.valueForProgress(0.0), 0.0);
    }
}

void tst_QEasingCurve::streamInOut_data()
{
    QTest::addColumn<int>("version");
    QTest::addColumn<bool>("equality");

    QTest::newRow("5.11") << int(QDataStream::Qt_5_11) << false;
    QTest::newRow("5.13") << int(QDataStream::Qt_5_13) << true;
}

void tst_QEasingCurve::streamInOut()
{
    QFETCH(int, version);
    QFETCH(bool, equality);

    QEasingCurve orig;
    orig.addCubicBezierSegment(QPointF(0.43, 0.0025), QPointF(0.38, 0.51), QPointF(0.57, 0.99));

    QEasingCurve copy;

    QByteArray data;
    QDataStream dsw(&data,QIODevice::WriteOnly);
    QDataStream dsr(&data,QIODevice::ReadOnly);

    dsw.setVersion(version);
    dsr.setVersion(version);
    dsw << orig;
    dsr >> copy;

    QCOMPARE(copy == orig, equality);
}

QTEST_MAIN(tst_QEasingCurve)
#include "tst_qeasingcurve.moc"
