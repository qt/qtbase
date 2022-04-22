/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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


#include <QTest>

#include <qcolorspace.h>
#include <qcolortransform.h>

class tst_QColorTransform : public QObject
{
    Q_OBJECT

public:
    tst_QColorTransform();

private slots:
    void mapRGB32_data();
    void mapRGB32();
    void mapRGB64_data();
    void mapRGB64();
    void mapQColor_data();
    void mapQColor();
};

tst_QColorTransform::tst_QColorTransform()
{ }


void tst_QColorTransform::mapRGB32_data()
{
    QTest::addColumn<QColorTransform>("transform");
    QTest::addColumn<bool>("isIdentity");
    QTest::addColumn<bool>("sharesRed");

    QTest::newRow("default") << QColorTransform() << true << true;
    QTest::newRow("sRGB to Linear") << QColorSpace(QColorSpace::SRgb).transformationToColorSpace(QColorSpace::SRgbLinear) << false << true;
    QTest::newRow("AdobeRGB to sRGB") << QColorSpace(QColorSpace::AdobeRgb).transformationToColorSpace(QColorSpace::SRgb) << false << true;
    QTest::newRow("Linear AdobeRGB to Linear sRGB")
        << QColorSpace(QColorSpace::AdobeRgb).withTransferFunction(QColorSpace::TransferFunction::Linear).transformationToColorSpace(
            QColorSpace::SRgb)
        << false << true;
    QTest::newRow("sRgb to AdobeRGB") << QColorSpace(QColorSpace::SRgb).transformationToColorSpace(QColorSpace::AdobeRgb) << false << true;
    QTest::newRow("DP3 to sRGB") << QColorSpace(QColorSpace::DisplayP3).transformationToColorSpace(QColorSpace::SRgb) << false << false;
    QTest::newRow("DP3 to Linear DP3")
        << QColorSpace(QColorSpace::DisplayP3).transformationToColorSpace(
            QColorSpace(QColorSpace::DisplayP3).withTransferFunction(QColorSpace::TransferFunction::Linear))
        << false << false;
    QTest::newRow("Linear DP3 to Linear sRGB")
        << QColorSpace(QColorSpace::DisplayP3).withTransferFunction(QColorSpace::TransferFunction::Linear).transformationToColorSpace(
            QColorSpace::SRgb)
        << false << false;
}

void tst_QColorTransform::mapRGB32()
{
    QFETCH(QColorTransform, transform);
    QFETCH(bool, isIdentity);
    QFETCH(bool, sharesRed);
    // Do basic sanity tests of conversions between similar sane color spaces

    QRgb testColor = qRgb(32, 64, 128);
    QRgb result = transform.map(testColor);
    QVERIFY(qRed(result) < qGreen(result));
    QVERIFY(qGreen(result) < qBlue(result));
    QCOMPARE(qAlpha(result), 255);
    if (isIdentity)
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = qRgb(128, 64, 32);
    result = transform.map(testColor);
    QVERIFY(qRed(result) > qGreen(result));
    QVERIFY(qGreen(result) > qBlue(result));
    QCOMPARE(qAlpha(result), 255);
    if (isIdentity)
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = qRgba(15, 31, 63, 128);
    result = transform.map(testColor);
    QVERIFY(qRed(result) < qGreen(result));
    QVERIFY(qGreen(result) < qBlue(result));
    QCOMPARE(qAlpha(result), 128);
    if (isIdentity)
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = qRgb(0, 0, 0);
    result = transform.map(testColor);
    QCOMPARE(qRed(result), 0);
    QCOMPARE(qGreen(result), 0);
    QCOMPARE(qBlue(result), 0);
    QCOMPARE(qAlpha(result), 255);

    testColor = qRgb(255, 255, 255);
    result = transform.map(testColor);
    QCOMPARE(qRed(result), 255);
    QCOMPARE(qGreen(result), 255);
    QCOMPARE(qBlue(result), 255);
    QCOMPARE(qAlpha(result), 255);

    testColor = qRgb(255, 255, 0);
    result = transform.map(testColor);
    QCOMPARE(qAlpha(result), 255);
    if (sharesRed)
        QCOMPARE(qRed(result), 255);

    testColor = qRgb(0, 255, 255);
    result = transform.map(testColor);
    QCOMPARE(qBlue(result), 255);
    QCOMPARE(qAlpha(result), 255);
}

void tst_QColorTransform::mapRGB64_data()
{
    mapRGB32_data();
}

void tst_QColorTransform::mapRGB64()
{
    QFETCH(QColorTransform, transform);
    QFETCH(bool, isIdentity);
    QFETCH(bool, sharesRed);

    QRgba64 testColor = QRgba64::fromRgba(128, 64, 32, 255);
    QRgba64 result = transform.map(testColor);
    QVERIFY(result.red() > result.green());
    QVERIFY(result.green() > result.blue());
    QCOMPARE(result.alpha(), 0xffff);
    if (isIdentity)
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = QRgba64::fromRgba64(0, 0, 0, 0xffff);
    result = transform.map(testColor);
    QCOMPARE(result, testColor);

    testColor = QRgba64::fromRgba64(0xffff, 0xffff, 0xffff, 0xffff);
    result = transform.map(testColor);
    QCOMPARE(result, testColor);

    testColor = QRgba64::fromRgba64(0xffff, 0xffff, 0, 0xffff);
    result = transform.map(testColor);
    QCOMPARE(result.alpha(), 0xffff);
    if (sharesRed)
        QCOMPARE(result.red(), 0xffff);

    testColor = QRgba64::fromRgba64(0, 0xffff, 0xffff, 0xffff);
    result = transform.map(testColor);
    QCOMPARE(result.blue(), 0xffff);
    QCOMPARE(result.alpha(), 0xffff);
}

void tst_QColorTransform::mapQColor_data()
{
    mapRGB32_data();
}

void tst_QColorTransform::mapQColor()
{
    QFETCH(QColorTransform, transform);
    QFETCH(bool, isIdentity);
    QFETCH(bool, sharesRed);

    QColor testColor(32, 64, 128);
    QColor result = transform.map(testColor);
    QVERIFY(result.redF() < result.greenF());
    QVERIFY(result.greenF() < result.blueF());
    QCOMPARE(result.alphaF(), 1.0f);
    if (isIdentity)
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = Qt::black;
    result = transform.map(testColor);
    QCOMPARE(result, testColor);

    testColor = Qt::white;
    result = transform.map(testColor);
    QCOMPARE(result, testColor);

    testColor = QColor(255, 255, 0);
    result = transform.map(testColor);
    QCOMPARE(result.alphaF(), 1);
    if (sharesRed)
        QVERIFY(result.redF() >= 1.0f);

    testColor = QColor(0, 255, 255);
    result = transform.map(testColor);
    QCOMPARE(result.alphaF(), 1.0f);
    QVERIFY(result.blueF() >= 1.0f);
}

QTEST_MAIN(tst_QColorTransform)
#include "tst_qcolortransform.moc"
