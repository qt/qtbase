// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

#include <qcolorspace.h>
#include <qcolortransform.h>
#include <qrgbafloat.h>
#include <QtGui/private/qcolortransform_p.h>

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
    void mapRGBAFP16x4_data();
    void mapRGBAFP16x4();
    void mapRGBAFP32x4_data();
    void mapRGBAFP32x4();
    void mapQColor_data();
    void mapQColor();
    void mapRGB32Prepared_data();
    void mapRGB32Prepared();

    void transformIsIdentity();
};

tst_QColorTransform::tst_QColorTransform()
{ }


void tst_QColorTransform::mapRGB32_data()
{
    QTest::addColumn<QColorTransform>("transform");
    QTest::addColumn<bool>("sharesRed");

    QColorSpace srgb(QColorSpace::SRgb);
    QColorSpace srgbLinear(QColorSpace::SRgbLinear);
    QColorSpace adobeRgb(QColorSpace::AdobeRgb);
    QColorSpace adobeRgbLinear = adobeRgb.withTransferFunction(QColorSpace::TransferFunction::Linear);
    QColorSpace dp3(QColorSpace::DisplayP3);
    QColorSpace dp3Linear = dp3.withTransferFunction(QColorSpace::TransferFunction::Linear);

    QTest::newRow("default")                        << QColorTransform()                                        << true;
    QTest::newRow("sRGB to Linear sRGB")            << srgb.transformationToColorSpace(srgbLinear)              << true;
    QTest::newRow("AdobeRGB to sRGB")               << adobeRgb.transformationToColorSpace(srgb)                << true;
    QTest::newRow("Linear AdobeRGB to AdobeRGB")    << adobeRgbLinear.transformationToColorSpace(adobeRgb)      << true;
    QTest::newRow("Linear AdobeRGB to Linear sRGB") << adobeRgbLinear.transformationToColorSpace(srgbLinear)    << true;
    QTest::newRow("sRgb to AdobeRGB")               << srgb.transformationToColorSpace(adobeRgb)                << true;
    QTest::newRow("DP3 to sRGB")                    << dp3.transformationToColorSpace(srgb)                     << false;
    QTest::newRow("DP3 to Linear DP3")              << dp3.transformationToColorSpace(dp3Linear)                << false;
    QTest::newRow("Linear DP3 to Linear sRGB")      << dp3Linear.transformationToColorSpace(srgbLinear)         << false;
}

void tst_QColorTransform::mapRGB32()
{
    QFETCH(QColorTransform, transform);
    QFETCH(bool, sharesRed);
    // Do basic sanity tests of conversions between similar sane color spaces

    QRgb testColor = qRgb(32, 64, 128);
    QRgb result = transform.map(testColor);
    QVERIFY(qRed(result) < qGreen(result));
    QVERIFY(qGreen(result) < qBlue(result));
    QCOMPARE(qAlpha(result), 255);
    if (transform.isIdentity())
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = qRgb(128, 64, 32);
    result = transform.map(testColor);
    QVERIFY(qRed(result) > qGreen(result));
    QVERIFY(qGreen(result) > qBlue(result));
    QCOMPARE(qAlpha(result), 255);
    if (transform.isIdentity())
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = qRgba(15, 31, 63, 128);
    result = transform.map(testColor);
    QVERIFY(qRed(result) < qGreen(result));
    QVERIFY(qGreen(result) < qBlue(result));
    QCOMPARE(qAlpha(result), 128);
    if (transform.isIdentity())
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
    QFETCH(bool, sharesRed);

    QRgba64 testColor = QRgba64::fromRgba(128, 64, 32, 255);
    QRgba64 result = transform.map(testColor);
    QVERIFY(result.red() > result.green());
    QVERIFY(result.green() > result.blue());
    QCOMPARE(result.alpha(), 0xffff);
    if (transform.isIdentity())
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

void tst_QColorTransform::mapRGBAFP16x4_data()
{
    mapRGB32_data();
}

void tst_QColorTransform::mapRGBAFP16x4()
{
    QFETCH(QColorTransform, transform);
    QFETCH(bool, sharesRed);

    QRgbaFloat16 testColor = QRgbaFloat16::fromRgba(128, 64, 32, 255);
    QRgbaFloat16 result = transform.map(testColor);
    QVERIFY(result.red() > result.green());
    QVERIFY(result.green() > result.blue());
    QCOMPARE(result.alpha(), 1.0f);
    if (transform.isIdentity())
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = QRgbaFloat16{0.0f, 0.0f, 0.0f, 1.0f};
    result = transform.map(testColor);
    QCOMPARE(result, testColor);

    testColor = QRgbaFloat16{1.0f, 1.0f, 1.0f, 1.0f};
    result = transform.map(testColor);
    QCOMPARE(result, testColor);

    testColor = QRgbaFloat16{1.0f, 1.0f, 0.0f, 1.0f};
    result = transform.map(testColor);
    QCOMPARE(result.alpha(), 1.0f);
    if (sharesRed)
        QCOMPARE(result.red(), 1.0f);

    testColor = QRgbaFloat16{0.0f, 1.0f, 1.0f, 1.0f};
    result = transform.map(testColor);
    // QRgbaFloat16 might overflow blue if we convert to a smaller gamut:
    QCOMPARE(result.blue16(), 65535);
    QCOMPARE(result.alpha(), 1.0f);
}

void tst_QColorTransform::mapRGBAFP32x4_data()
{
    mapRGB32_data();
}

void tst_QColorTransform::mapRGBAFP32x4()
{
    QFETCH(QColorTransform, transform);
    QFETCH(bool, sharesRed);

    QRgbaFloat32 testColor = QRgbaFloat32::fromRgba(128, 64, 32, 255);
    QRgbaFloat32 result = transform.map(testColor);
    QVERIFY(result.red() > result.green());
    QVERIFY(result.green() > result.blue());
    QCOMPARE(result.alpha(), 1.0f);
    if (transform.isIdentity())
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = QRgbaFloat32{0.0f, 0.0f, 0.0f, 1.0f};
    result = transform.map(testColor);
    QCOMPARE(result.red(), 0.0f);
    QCOMPARE(result.green(), 0.0f);
    QCOMPARE(result.blue(), 0.0f);
    QCOMPARE(result.alpha(), 1.0f);

    testColor = QRgbaFloat32{1.0f, 1.0f, 1.0f, 1.0f};
    result = transform.map(testColor);
    QCOMPARE(result.red(), 1.0f);
    QCOMPARE(result.green(), 1.0f);
    QCOMPARE(result.blue(), 1.0f);
    QCOMPARE(result.alpha(), 1.0f);

    testColor = QRgbaFloat32{1.0f, 1.0f, 0.0f, 1.0f};
    result = transform.map(testColor);
    QCOMPARE(result.alpha(), 1.0f);
    if (sharesRed)
        QCOMPARE(result.red(), 1.0f);

    testColor = QRgbaFloat32{0.0f, 1.0f, 1.0f, 1.0f};
    result = transform.map(testColor);
    // QRgbaFloat16 might overflow blue if we convert to a smaller gamut:
    QCOMPARE(result.blue16(), 65535);
    QCOMPARE(result.alpha(), 1.0f);
}

void tst_QColorTransform::mapQColor_data()
{
    mapRGB32_data();
}

void tst_QColorTransform::mapQColor()
{
    QFETCH(QColorTransform, transform);
    QFETCH(bool, sharesRed);

    QColor testColor(32, 64, 128);
    QColor result = transform.map(testColor);
    QVERIFY(result.redF() < result.greenF());
    QVERIFY(result.greenF() < result.blueF());
    QCOMPARE(result.alphaF(), 1.0f);
    if (transform.isIdentity())
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

void tst_QColorTransform::mapRGB32Prepared_data()
{
    mapRGB32_data();
}

void tst_QColorTransform::mapRGB32Prepared()
{
    QFETCH(QColorTransform, transform);
    QFETCH(bool, sharesRed);

    // The same tests as mapRGB32 but prepared, to use the LUT code-paths
    if (!transform.isIdentity())
        QColorTransformPrivate::get(transform)->prepare();

    QRgb testColor = qRgb(32, 64, 128);
    QRgb result = transform.map(testColor);
    QVERIFY(qRed(result) < qGreen(result));
    QVERIFY(qGreen(result) < qBlue(result));
    QCOMPARE(qAlpha(result), 255);
    if (transform.isIdentity())
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = qRgb(128, 64, 32);
    result = transform.map(testColor);
    QVERIFY(qRed(result) > qGreen(result));
    QVERIFY(qGreen(result) > qBlue(result));
    QCOMPARE(qAlpha(result), 255);
    if (transform.isIdentity())
        QVERIFY(result == testColor);
    else
        QVERIFY(result != testColor);

    testColor = qRgba(15, 31, 63, 128);
    result = transform.map(testColor);
    QVERIFY(qRed(result) < qGreen(result));
    QVERIFY(qGreen(result) < qBlue(result));
    QCOMPARE(qAlpha(result), 128);
    if (transform.isIdentity())
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

void tst_QColorTransform::transformIsIdentity()
{
    QColorTransform ct;
    QVERIFY(ct.isIdentity());

    QColorSpace cs = QColorSpace::SRgb;
    ct = cs.transformationToColorSpace(QColorSpace::SRgb);
    QVERIFY(ct.isIdentity());

    ct = cs.transformationToColorSpace(QColorSpace::SRgbLinear);
    QVERIFY(!ct.isIdentity());

    ct = cs.withTransferFunction(QColorSpace::TransferFunction::Linear).transformationToColorSpace(QColorSpace::SRgbLinear);
    QVERIFY(ct.isIdentity());
}

QTEST_MAIN(tst_QColorTransform)
#include "tst_qcolortransform.moc"
