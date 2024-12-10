// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qcolorspace.h>
#include <qimage.h>
#include <qimagereader.h>
#include <qrgbafloat.h>

#include <private/qcolorspace_p.h>

Q_DECLARE_METATYPE(QColorSpace::NamedColorSpace)
Q_DECLARE_METATYPE(QColorSpace::Primaries)
Q_DECLARE_METATYPE(QColorSpace::TransferFunction)

class tst_QColorSpace : public QObject
{
    Q_OBJECT

public:
    tst_QColorSpace();

private slots:
    void movable();
    void namedColorSpaces_data();
    void namedColorSpaces();

    void toIccProfile_data();
    void toIccProfile();

    void fromIccProfile_data();
    void fromIccProfile();

    void imageConversion_data();
    void imageConversion();
    void imageConversion64_data();
    void imageConversion64();
    void imageConversion64PM_data();
    void imageConversion64PM();
    void imageConversionOverLargerGamut_data();
    void imageConversionOverLargerGamut();
    void imageConversionOverLargerGamut2_data();
    void imageConversionOverLargerGamut2();
    void imageConversionOverAnyGamutFP_data();
    void imageConversionOverAnyGamutFP();
    void imageConversionOverAnyGamutFP2_data();
    void imageConversionOverAnyGamutFP2();
    void imageConversionOverNonThreeComponentMatrix_data();
    void imageConversionOverNonThreeComponentMatrix();
    void loadImage();

    void primaries();
    void primariesXyz();

#ifdef QT_BUILD_INTERNAL
    void primaries2_data();
    void primaries2();
#endif

    void invalidPrimaries();

    void changeTransferFunction();
    void changePrimaries();

    void transferFunctionTable();

    void description();
    void whitePoint_data();
    void whitePoint();
    void setWhitePoint();
    void grayColorSpace();
    void grayColorSpaceEffectivelySRgb();

    void scaleAlphaValue();
    void hdrColorSpaces();
};

tst_QColorSpace::tst_QColorSpace()
{ }


void tst_QColorSpace::movable()
{
    QColorSpace cs1 = QColorSpace::SRgb;
    QColorSpace cs2 = QColorSpace::SRgbLinear;
    QVERIFY(cs1.isValid());
    QVERIFY(cs2.isValid());
    QCOMPARE(cs1, QColorSpace::SRgb);

    cs2 = std::move(cs1);
    QVERIFY(!cs1.isValid());
    QVERIFY(cs2.isValid());
    QCOMPARE(cs2, QColorSpace::SRgb);
    QVERIFY(cs1 != QColorSpace::SRgb);
    QCOMPARE(cs1, QColorSpace());

    QColorSpace cs3(std::move(cs2));
    QVERIFY(!cs2.isValid());
    QVERIFY(cs3.isValid());
    QCOMPARE(cs3, QColorSpace::SRgb);
    QCOMPARE(cs2, QColorSpace());
}

void tst_QColorSpace::namedColorSpaces_data()
{
    QTest::addColumn<QColorSpace::NamedColorSpace>("namedColorSpace");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<QColorSpace::Primaries>("primariesId");
    QTest::addColumn<QColorSpace::TransferFunction>("transferFunctionId");

    QTest::newRow("sRGB") << QColorSpace::SRgb << true
                          << QColorSpace::Primaries::SRgb
                          << QColorSpace::TransferFunction::SRgb;
    QTest::newRow("sRGB Linear") << QColorSpace::SRgbLinear << true
                                 << QColorSpace::Primaries::SRgb
                                 << QColorSpace::TransferFunction::Linear;
    QTest::newRow("Adobe RGB") << QColorSpace::AdobeRgb << true
                               << QColorSpace::Primaries::AdobeRgb
                               << QColorSpace::TransferFunction::Gamma;
    QTest::newRow("Display-P3") << QColorSpace::DisplayP3 << true
                                << QColorSpace::Primaries::DciP3D65
                                << QColorSpace::TransferFunction::SRgb;
    QTest::newRow("ProPhoto RGB") << QColorSpace::ProPhotoRgb << true
                                  << QColorSpace::Primaries::ProPhotoRgb
                                  << QColorSpace::TransferFunction::ProPhotoRgb;
    QTest::newRow("BT.2020") << QColorSpace::Bt2020 << true
                             << QColorSpace::Primaries::Bt2020
                             << QColorSpace::TransferFunction::Bt2020;
    QTest::newRow("BT.2100 PQ") << QColorSpace::Bt2100Pq << true
                                << QColorSpace::Primaries::Bt2020
                                << QColorSpace::TransferFunction::St2084;
    QTest::newRow("BT.2100 HLG") << QColorSpace::Bt2100Hlg << true
                                 << QColorSpace::Primaries::Bt2020
                                 << QColorSpace::TransferFunction::Hlg;
    QTest::newRow("0") << QColorSpace::NamedColorSpace(0)
                       << false
                       << QColorSpace::Primaries::Custom
                       << QColorSpace::TransferFunction::Custom;
    QTest::newRow("1027") << QColorSpace::NamedColorSpace(1027)
                          << false
                          << QColorSpace::Primaries::Custom
                          << QColorSpace::TransferFunction::Custom;
}

void tst_QColorSpace::namedColorSpaces()
{
    QFETCH(QColorSpace::NamedColorSpace, namedColorSpace);
    QFETCH(bool, isValid);
    QFETCH(QColorSpace::Primaries, primariesId);
    QFETCH(QColorSpace::TransferFunction, transferFunctionId);

    if (!isValid)
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("QColorSpace attempted constructed from invalid QColorSpace::NamedColorSpace"));
    QColorSpace colorSpace = namedColorSpace;

    QCOMPARE(colorSpace.isValid(), isValid);
    QCOMPARE(colorSpace.primaries(), primariesId);
    QCOMPARE(colorSpace.transferFunction(), transferFunctionId);
}


void tst_QColorSpace::toIccProfile_data()
{
    namedColorSpaces_data();
}

void tst_QColorSpace::toIccProfile()
{
    QFETCH(QColorSpace::NamedColorSpace, namedColorSpace);
    QFETCH(bool, isValid);
    QFETCH(QColorSpace::Primaries, primariesId);
    QFETCH(QColorSpace::TransferFunction, transferFunctionId);

    Q_UNUSED(primariesId);
    Q_UNUSED(transferFunctionId);

    if (!isValid)
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("QColorSpace attempted constructed from invalid QColorSpace::NamedColorSpace"));
    QColorSpace colorSpace = namedColorSpace;
    QByteArray iccProfile = colorSpace.iccProfile();
    QCOMPARE(iccProfile.isEmpty(), !isValid);

    if (!isValid)
        return;

    QColorSpace colorSpace2 = QColorSpace::fromIccProfile(iccProfile);
    QVERIFY(colorSpace2.isValid());

    QCOMPARE(colorSpace2, colorSpace);

    QByteArray iccProfile2 = colorSpace2.iccProfile();
    QVERIFY(!iccProfile2.isEmpty());

    QCOMPARE(iccProfile2, iccProfile);
}

void tst_QColorSpace::fromIccProfile_data()
{
    QTest::addColumn<QString>("testProfile");
    QTest::addColumn<QColorSpace::NamedColorSpace>("namedColorSpace");
    QTest::addColumn<QColorSpace::TransferFunction>("transferFunction");
    QTest::addColumn<QColorSpace::TransformModel>("transformModel");
    QTest::addColumn<QColorSpace::ColorModel>("colorModel");
    QTest::addColumn<QString>("description");

    QString prefix = QFINDTESTDATA("resources/");
    // Read the official sRGB ICCv2 profile:
    QTest::newRow("sRGB2014 (ICCv2)") << prefix + "sRGB2014.icc" << QColorSpace::SRgb
                                      << QColorSpace::TransferFunction::SRgb
                                      << QColorSpace::TransformModel::ThreeComponentMatrix
                                      << QColorSpace::ColorModel::Rgb << QString("sRGB2014");
    // My monitor's profile:
    QTest::newRow("HP ZR30w (ICCv4)") << prefix + "HP_ZR30w.icc" << QColorSpace::NamedColorSpace(0)
                                      << QColorSpace::TransferFunction::Gamma
                                      << QColorSpace::TransformModel::ThreeComponentMatrix
                                      << QColorSpace::ColorModel::Rgb << QString("HP Z30i");
    // A profile to HD TV
    QTest::newRow("VideoHD") << prefix + "VideoHD.icc" << QColorSpace::NamedColorSpace(0)
                             << QColorSpace::TransferFunction::Custom
                             << QColorSpace::TransformModel::ElementListProcessing
                             << QColorSpace::ColorModel::Rgb << QString("HDTV (Rec. 709)");
    // sRGB on PCSLab format
    QTest::newRow("sRGB ICCv4 Appearance") << prefix + "sRGB_ICC_v4_Appearance.icc" << QColorSpace::NamedColorSpace(0)
                                           << QColorSpace::TransferFunction::Custom
                                           << QColorSpace::TransformModel::ElementListProcessing
                                           << QColorSpace::ColorModel::Rgb << QString("sRGB_ICC_v4_Appearance.icc");
    // Grayscale profile
    QTest::newRow("sGrey-v4") << prefix + "sGrey-v4.icc" << QColorSpace::NamedColorSpace(0)
                                << QColorSpace::TransferFunction::SRgb
                                << QColorSpace::TransformModel::ThreeComponentMatrix
                                << QColorSpace::ColorModel::Gray << QString("sGry");
    // CMYK profile
    QTest::newRow("CGATS compat") << prefix + "CGATS001Compat-v2-micro.icc" << QColorSpace::NamedColorSpace(0)
                                  << QColorSpace::TransferFunction::Custom
                                  << QColorSpace::TransformModel::ElementListProcessing
                                  << QColorSpace::ColorModel::Cmyk << QString("uCMY");
    // BT.2100 PQ profile
    QTest::newRow("BT.2100 PQ") << prefix + "Rec. ITU-R BT.2100 PQ.icc" << QColorSpace::Bt2100Pq
                                  << QColorSpace::TransferFunction::St2084
                                  << QColorSpace::TransformModel::ThreeComponentMatrix
                                  << QColorSpace::ColorModel::Rgb << QString("Rec. ITU-R BT.2100 PQ");
}

void tst_QColorSpace::fromIccProfile()
{
    QFETCH(QString, testProfile);
    QFETCH(QColorSpace::NamedColorSpace, namedColorSpace);
    QFETCH(QColorSpace::TransferFunction, transferFunction);
    QFETCH(QColorSpace::TransformModel, transformModel);
    QFETCH(QColorSpace::ColorModel, colorModel);
    QFETCH(QString, description);

    QFile file(testProfile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray iccProfile = file.readAll();
    QColorSpace fileColorSpace = QColorSpace::fromIccProfile(iccProfile);
    QVERIFY(fileColorSpace.isValid());

    if (namedColorSpace)
        QCOMPARE(fileColorSpace, namedColorSpace);

    QCOMPARE(fileColorSpace.transferFunction(), transferFunction);
    QCOMPARE(fileColorSpace.transformModel(), transformModel);
    QCOMPARE(fileColorSpace.colorModel(), colorModel);
    QCOMPARE(fileColorSpace.description(), description);

    QByteArray iccProfile2 = fileColorSpace.iccProfile();
    QCOMPARE(iccProfile, iccProfile2);
    QColorSpace fileColorSpace2 = QColorSpace::fromIccProfile(iccProfile2);
    QCOMPARE(fileColorSpace2, fileColorSpace);

    // Change description to force generation of new icc profile data.
    fileColorSpace2.setDescription("Hello my QTest description");
    iccProfile2 = fileColorSpace2.iccProfile();
    QCOMPARE_NE(iccProfile, iccProfile2);
    fileColorSpace2 = QColorSpace::fromIccProfile(iccProfile2);
    QVERIFY(fileColorSpace2.isValid());
    // Note, we do not currently compare description in color space equality
    QCOMPARE(fileColorSpace2, fileColorSpace);
}

void tst_QColorSpace::imageConversion_data()
{
    QTest::addColumn<QColorSpace::NamedColorSpace>("fromColorSpace");
    QTest::addColumn<QColorSpace::NamedColorSpace>("toColorSpace");
    QTest::addColumn<int>("tolerance");

    QTest::newRow("sRGB -> Display-P3") << QColorSpace::SRgb << QColorSpace::DisplayP3 << 0;
    QTest::newRow("sRGB -> Adobe RGB") << QColorSpace::SRgb << QColorSpace::AdobeRgb << 2;
    QTest::newRow("Adobe RGB -> sRGB") << QColorSpace::AdobeRgb << QColorSpace::SRgb << 2;
    QTest::newRow("Adobe RGB -> Display-P3") << QColorSpace::AdobeRgb << QColorSpace::DisplayP3 << 4;
    QTest::newRow("Display-P3 -> Adobe RGB") << QColorSpace::DisplayP3 << QColorSpace::AdobeRgb << 2;
    QTest::newRow("Display-P3 -> sRGB") << QColorSpace::DisplayP3 << QColorSpace::SRgb << 0;
    QTest::newRow("sRGB -> sRGB Linear") << QColorSpace::SRgb << QColorSpace::SRgbLinear << 0;
    QTest::newRow("sRGB Linear -> sRGB") << QColorSpace::SRgbLinear << QColorSpace::SRgb << 0;
}

void tst_QColorSpace::imageConversion()
{
    QFETCH(QColorSpace::NamedColorSpace, fromColorSpace);
    QFETCH(QColorSpace::NamedColorSpace, toColorSpace);
    QFETCH(int, tolerance);

    QImage testImage(256, 1, QImage::Format_RGB32);

    for (int i = 0; i < 256; ++i)
        testImage.setPixel(i, 0, qRgb(i, i, i));

    testImage.setColorSpace(fromColorSpace);
    QCOMPARE(testImage.colorSpace(), QColorSpace(fromColorSpace));

    testImage.convertToColorSpace(toColorSpace);
    QCOMPARE(testImage.colorSpace(), QColorSpace(toColorSpace));

    int lastRed = 0;
    int lastGreen = 0;
    int lastBlue = 0;
    for (int i = 0; i < 256; ++i) {
        QRgb p = testImage.pixel(i, 0);
        QCOMPARE_GE(qRed(p), lastRed);
        QCOMPARE_GE(qGreen(p), lastGreen);
        QCOMPARE_GE(qBlue(p), lastBlue);
        lastRed = qRed(p);
        lastGreen = qGreen(p);
        lastBlue = qBlue(p);
    }

    lastRed = 0;
    lastGreen = 0;
    lastBlue = 0;
    testImage.convertToColorSpace(fromColorSpace);
    QCOMPARE(testImage.colorSpace(), QColorSpace(fromColorSpace));
    for (int i = 0; i < 256; ++i) {
        QRgb p = testImage.pixel(i, 0);
        QCOMPARE_LE(qAbs(qRed(p) - qBlue(p)), tolerance);
        QCOMPARE_LE(qAbs(qRed(p) - qGreen(p)), tolerance);
        QCOMPARE_LE(qAbs(qGreen(p) - qBlue(p)), tolerance);
        QCOMPARE_LE(lastRed   - qRed(p), tolerance / 2);
        QCOMPARE_LE(lastBlue  - qBlue(p), tolerance / 2);
        QCOMPARE_LE(lastGreen - qGreen(p), tolerance / 2);
        lastRed = qRed(p);
        lastGreen = qGreen(p);
        lastBlue = qBlue(p);
    }
}

void tst_QColorSpace::imageConversion64_data()
{
    QTest::addColumn<QColorSpace::NamedColorSpace>("fromColorSpace");
    QTest::addColumn<QColorSpace::NamedColorSpace>("toColorSpace");

    QTest::newRow("sRGB -> Display-P3") << QColorSpace::SRgb << QColorSpace::DisplayP3;
    QTest::newRow("sRGB -> Adobe RGB") << QColorSpace::SRgb << QColorSpace::AdobeRgb;
    QTest::newRow("Display-P3 -> sRGB") << QColorSpace::DisplayP3 << QColorSpace::SRgb;
    QTest::newRow("Display-P3 -> Adobe RGB") << QColorSpace::DisplayP3 << QColorSpace::AdobeRgb;
    QTest::newRow("sRGB -> sRGB Linear") << QColorSpace::SRgb << QColorSpace::SRgbLinear;
    QTest::newRow("sRGB Linear -> sRGB") << QColorSpace::SRgbLinear << QColorSpace::SRgb;
}

void tst_QColorSpace::imageConversion64()
{
    QFETCH(QColorSpace::NamedColorSpace, fromColorSpace);
    QFETCH(QColorSpace::NamedColorSpace, toColorSpace);

    QImage testImage(256, 1, QImage::Format_RGBX64);

    for (int i = 0; i < 256; ++i)
        testImage.setPixel(i, 0, qRgb(i, i, i));

    testImage.setColorSpace(fromColorSpace);
    QCOMPARE(testImage.colorSpace(), QColorSpace(fromColorSpace));

    testImage.convertToColorSpace(toColorSpace);
    QCOMPARE(testImage.colorSpace(), QColorSpace(toColorSpace));

    int lastRed = 0;
    int lastGreen = 0;
    int lastBlue = 0;
    for (int i = 0; i < 256; ++i) {
        QRgb p = testImage.pixel(i, 0);
        QCOMPARE_GE(qRed(p), lastRed);
        QCOMPARE_GE(qGreen(p), lastGreen);
        QCOMPARE_GE(qBlue(p), lastBlue);
        lastRed = qRed(p);
        lastGreen = qGreen(p);
        lastBlue = qBlue(p);
    }

    lastRed = 0;
    lastGreen = 0;
    lastBlue = 0;
    testImage.convertToColorSpace(fromColorSpace);
    QCOMPARE(testImage.colorSpace(), QColorSpace(fromColorSpace));
    for (int i = 0; i < 256; ++i) {
        QRgb p = testImage.pixel(i, 0);
        QCOMPARE(qRed(p),  qGreen(p));
        QCOMPARE(qRed(p),  qBlue(p));
        QCOMPARE_GE(qRed(p), lastRed);
        QCOMPARE_GE(qGreen(p), lastGreen);
        QCOMPARE_GE(qBlue(p), lastBlue);
        lastRed = qRed(p);
        lastGreen = qGreen(p);
        lastBlue = qBlue(p);
    }
    QCOMPARE(lastRed, 255);
    QCOMPARE(lastGreen, 255);
    QCOMPARE(lastBlue, 255);
}

void tst_QColorSpace::imageConversion64PM_data()
{
    imageConversion64_data();
}

void tst_QColorSpace::imageConversion64PM()
{
    QFETCH(QColorSpace::NamedColorSpace, fromColorSpace);
    QFETCH(QColorSpace::NamedColorSpace, toColorSpace);

    QImage testImage(256, 16, QImage::Format_RGBA64_Premultiplied);

    for (int j = 0; j < 16; ++j) {
        int a = j * 15;
        for (int i = 0; i < 256; ++i) {
            QRgba64 color = QRgba64::fromRgba(i, i, i, a);
            testImage.setPixelColor(i, j, QColor::fromRgba64(color));
        }
    }

    testImage.setColorSpace(fromColorSpace);
    QCOMPARE(testImage.colorSpace(), QColorSpace(fromColorSpace));

    testImage.convertToColorSpace(toColorSpace);
    QCOMPARE(testImage.colorSpace(), QColorSpace(toColorSpace));

    int lastRed = 0;
    int lastGreen = 0;
    int lastBlue = 0;
    for (int j = 0; j < 16; ++j) {
        const int expectedAlpha = j * 15;
        for (int i = 0; i < 256; ++i) {
            QRgb p = testImage.pixel(i, j);
            QCOMPARE_GE(qRed(p), lastRed);
            QCOMPARE_GE(qGreen(p), lastGreen);
            QCOMPARE_GE(qBlue(p), lastBlue);
            QCOMPARE(qAlpha(p), expectedAlpha);
            lastRed = qRed(p);
            lastGreen = qGreen(p);
            lastBlue = qBlue(p);
        }
        QCOMPARE_LE(lastRed, expectedAlpha);
        QCOMPARE_LE(lastGreen, expectedAlpha);
        QCOMPARE_LE(lastBlue, expectedAlpha);
        lastRed = 0;
        lastGreen = 0;
        lastBlue = 0;
    }

    testImage.convertToColorSpace(fromColorSpace);
    QCOMPARE(testImage.colorSpace(), QColorSpace(fromColorSpace));
    for (int j = 0; j < 16; ++j) {
        const int expectedAlpha = j * 15;
        for (int i = 0; i < 256; ++i) {
            QRgb expected = qPremultiply(qRgba(i, i, i, expectedAlpha));
            QRgb p = testImage.pixel(i, j);
            QCOMPARE_LE(qAbs(qRed(p) - qGreen(p)), 1);
            QCOMPARE_LE(qAbs(qRed(p) - qBlue(p)), 1);
            QCOMPARE(qAlpha(p), expectedAlpha);
            QCOMPARE_GE(qRed(p), lastRed);
            QCOMPARE_GE(qGreen(p), lastGreen);
            QCOMPARE_GE(qBlue(p), lastBlue);
            QCOMPARE_LE(qAbs(qRed(p) - qRed(expected)), 1);
            QCOMPARE_LE(qAbs(qGreen(p) - qGreen(expected)), 1);
            QCOMPARE_LE(qAbs(qBlue(p) - qBlue(expected)), 1);
            lastRed = qRed(p);
            lastGreen = qGreen(p);
            lastBlue = qBlue(p);
        }
        QCOMPARE(lastRed, expectedAlpha);
        QCOMPARE(lastGreen, expectedAlpha);
        QCOMPARE(lastBlue, expectedAlpha);
        lastRed = 0;
        lastGreen = 0;
        lastBlue = 0;
    }
}

void tst_QColorSpace::imageConversionOverLargerGamut_data()
{
    QTest::addColumn<QColorSpace::NamedColorSpace>("fromColorSpace");
    QTest::addColumn<QColorSpace::NamedColorSpace>("toColorSpace");

    QTest::newRow("sRGB -> Display-P3") << QColorSpace::SRgb << QColorSpace::DisplayP3;
    QTest::newRow("sRGB -> Adobe RGB") << QColorSpace::SRgb << QColorSpace::AdobeRgb;
    QTest::newRow("sRGB -> ProPhoto RGB") << QColorSpace::SRgb << QColorSpace::ProPhotoRgb;
    QTest::newRow("Display-P3 -> ProPhoto RGB") << QColorSpace::DisplayP3 << QColorSpace::ProPhotoRgb;
    QTest::newRow("Adobe RGB -> ProPhoto RGB") << QColorSpace::AdobeRgb << QColorSpace::ProPhotoRgb;
}

void tst_QColorSpace::imageConversionOverLargerGamut()
{
    QFETCH(QColorSpace::NamedColorSpace, fromColorSpace);
    QFETCH(QColorSpace::NamedColorSpace, toColorSpace);

    QColorSpace csfrom(fromColorSpace);
    QColorSpace csto(toColorSpace);
    csfrom.setTransferFunction(QColorSpace::TransferFunction::Linear);
    csto.setTransferFunction(QColorSpace::TransferFunction::Linear);

    QImage testImage(256, 256, QImage::Format_RGBX64);
    testImage.setColorSpace(csfrom);
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 256; ++x)
            testImage.setPixel(x, y, qRgb(x, y, qAbs(x - y)));

    QImage resultImage = testImage.convertedToColorSpace(csto);
    for (int y = 0; y < 256; ++y) {
        int lastRed = 0;
        for (int x = 0; x < 256; ++x) {
            QRgb p = resultImage.pixel(x, y);
            QCOMPARE_GE(qRed(p), lastRed);
            lastRed = qRed(p);
        }
    }
    for (int x = 0; x < 256; ++x) {
        int lastGreen = 0;
        for (int y = 0; y < 256; ++y) {
            QRgb p = resultImage.pixel(x, y);
            QCOMPARE_GE(qGreen(p), lastGreen);
            lastGreen = qGreen(p);
        }
    }

    resultImage.convertToColorSpace(csfrom);
    // The images are not exactly identical at 4x16bit, but they preserve 4x8bit accuracy.
    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < 256; ++x) {
            QCOMPARE(resultImage.pixel(x, y), testImage.pixel(x, y));
        }
    }
}

void tst_QColorSpace::imageConversionOverLargerGamut2_data()
{
    QTest::addColumn<QImage::Format>("format");

    QTest::newRow("rgbx16x4") << QImage::Format_RGBX16FPx4;
    QTest::newRow("rgba16x4") << QImage::Format_RGBA16FPx4;
    QTest::newRow("rgba16x4PM") << QImage::Format_RGBA16FPx4_Premultiplied;
    QTest::newRow("rgbx32x4") << QImage::Format_RGBX32FPx4;
    QTest::newRow("rgba32x4") << QImage::Format_RGBA32FPx4;
    QTest::newRow("rgba32x4PM") << QImage::Format_RGBA32FPx4_Premultiplied;
}

void tst_QColorSpace::imageConversionOverLargerGamut2()
{
    QFETCH(QImage::Format, format);

    QColorSpace csfrom = QColorSpace::DisplayP3;
    QColorSpace csto = QColorSpace::SRgb;

    QImage testImage(256, 256, format);
    testImage.setColorSpace(csfrom);
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 256; ++x)
            testImage.setPixel(x, y, qRgba(x, y, 16, 255));

    QImage resultImage = testImage.convertedToColorSpace(csto);
    for (int y = 0; y < 256; ++y) {
        float lastRed = -256.0f;
        for (int x = 0; x < 256; ++x) {
            float pr = resultImage.pixelColor(x, y).redF();
            QVERIFY(pr >= lastRed);
            lastRed = pr;
        }
    }
    for (int x = 0; x < 256; ++x) {
        float lastGreen = -256.0f;
        for (int y = 0; y < 256; ++y) {
            float pg = resultImage.pixelColor(x, y).greenF();
            QVERIFY(pg >= lastGreen);
            lastGreen = pg;
        }
    }
    // Test colors outside of sRGB are converted to values outside of 0-1 range.
    QVERIFY(resultImage.pixelColor(255, 0).redF() > 1.0f);
    QVERIFY(resultImage.pixelColor(255, 0).greenF() < 0.0f);
    QVERIFY(resultImage.pixelColor(0, 255).redF() < 0.0f);
    QVERIFY(resultImage.pixelColor(0, 255).greenF() > 1.0f);
}

void tst_QColorSpace::imageConversionOverAnyGamutFP_data()
{
    QTest::addColumn<QColorSpace::NamedColorSpace>("fromColorSpace");
    QTest::addColumn<QColorSpace::NamedColorSpace>("toColorSpace");

    QTest::newRow("sRGB -> Display-P3") << QColorSpace::SRgb << QColorSpace::DisplayP3;
    QTest::newRow("sRGB -> Adobe RGB") << QColorSpace::SRgb << QColorSpace::AdobeRgb;
    QTest::newRow("sRGB -> ProPhoto RGB") << QColorSpace::SRgb << QColorSpace::ProPhotoRgb;
    QTest::newRow("Adobe RGB -> sRGB") << QColorSpace::AdobeRgb << QColorSpace::SRgb;
    QTest::newRow("Adobe RGB -> Display-P3") << QColorSpace::AdobeRgb << QColorSpace::DisplayP3;
    QTest::newRow("Adobe RGB -> ProPhoto RGB") << QColorSpace::AdobeRgb << QColorSpace::ProPhotoRgb;
    QTest::newRow("Display-P3 -> sRGB") << QColorSpace::DisplayP3 << QColorSpace::SRgb;
    QTest::newRow("Display-P3 -> Adobe RGB") << QColorSpace::DisplayP3 << QColorSpace::AdobeRgb;
    QTest::newRow("Display-P3 -> ProPhoto RGB") << QColorSpace::DisplayP3 << QColorSpace::ProPhotoRgb;
}

void tst_QColorSpace::imageConversionOverAnyGamutFP()
{
    QFETCH(QColorSpace::NamedColorSpace, fromColorSpace);
    QFETCH(QColorSpace::NamedColorSpace, toColorSpace);

    QColorSpace csfrom(fromColorSpace);
    QColorSpace csto(toColorSpace);
    csfrom.setTransferFunction(QColorSpace::TransferFunction::Linear);
    csto.setTransferFunction(QColorSpace::TransferFunction::Linear);

    QImage testImage(256, 256, QImage::Format_RGBX32FPx4);
    testImage.setColorSpace(csfrom);
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 256; ++x)
            testImage.setPixel(x, y, qRgb(x, y, 0));

    QImage resultImage = testImage.convertedToColorSpace(csto);
    resultImage.convertToColorSpace(csfrom);

    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < 256; ++x) {
            QCOMPARE(resultImage.pixel(x, y), testImage.pixel(x, y));
        }
    }
}

void tst_QColorSpace::imageConversionOverAnyGamutFP2_data()
{
    imageConversionOverAnyGamutFP_data();
}

void tst_QColorSpace::imageConversionOverAnyGamutFP2()
{
    QFETCH(QColorSpace::NamedColorSpace, fromColorSpace);
    QFETCH(QColorSpace::NamedColorSpace, toColorSpace);

    // Same as imageConversionOverAnyGamutFP but using format switching transform
    QColorSpace csfrom(fromColorSpace);
    QColorSpace csto(toColorSpace);
    csfrom.setTransferFunction(QColorSpace::TransferFunction::Linear);
    csto.setTransferFunction(QColorSpace::TransferFunction::Linear);

    QImage testImage(256, 256, QImage::Format_RGB32);
    testImage.setColorSpace(csfrom);
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 256; ++x)
            testImage.setPixel(x, y, qRgb(x, y, 0));

    QImage resultImage = testImage.convertedToColorSpace(csto, QImage::Format_RGBX32FPx4);
    resultImage.convertToColorSpace(csfrom, QImage::Format_RGB32);

    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < 256; ++x) {
            QCOMPARE(resultImage.pixel(x, y), testImage.pixel(x, y));
        }
    }
}

void tst_QColorSpace::imageConversionOverNonThreeComponentMatrix_data()
{
    QTest::addColumn<QColorSpace>("fromColorSpace");
    QTest::addColumn<QColorSpace>("toColorSpace");

    QString prefix = QFINDTESTDATA("resources/");
    QFile file1(prefix + "VideoHD.icc");
    QFile file2(prefix + "sRGB_ICC_v4_Appearance.icc");
    QVERIFY(file1.open(QFile::ReadOnly));
    QVERIFY(file2.open(QFile::ReadOnly));
    QByteArray iccProfile1 = file1.readAll();
    QByteArray iccProfile2 = file2.readAll();
    QColorSpace hdtvColorSpace = QColorSpace::fromIccProfile(iccProfile1);
    QColorSpace srgbPcsColorSpace = QColorSpace::fromIccProfile(iccProfile2);

    QTest::newRow("sRGB PCSLab -> sRGB") << srgbPcsColorSpace << QColorSpace(QColorSpace::SRgb);
    QTest::newRow("sRGB -> sRGB PCSLab") << QColorSpace(QColorSpace::SRgb) << srgbPcsColorSpace;
    QTest::newRow("HDTV -> sRGB") << hdtvColorSpace << QColorSpace(QColorSpace::SRgb);
    QTest::newRow("sRGB -> HDTV") << QColorSpace(QColorSpace::SRgb) << hdtvColorSpace;
    QTest::newRow("sRGB PCSLab -> HDTV") << srgbPcsColorSpace << hdtvColorSpace;
    QTest::newRow("HDTV -> sRGB PCSLab") << hdtvColorSpace << srgbPcsColorSpace;
}

void tst_QColorSpace::imageConversionOverNonThreeComponentMatrix()
{
    QFETCH(QColorSpace, fromColorSpace);
    QFETCH(QColorSpace, toColorSpace);
    QVERIFY(fromColorSpace.isValid());
    QVERIFY(toColorSpace.isValidTarget());

    QVERIFY(!fromColorSpace.transformationToColorSpace(toColorSpace).isIdentity());

    QImage testImage(256, 256, QImage::Format_RGBX64);
    testImage.setColorSpace(fromColorSpace);
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 256; ++x)
            testImage.setPixel(x, y, qRgb(x, y, 0));

    QImage resultImage = testImage.convertedToColorSpace(toColorSpace);
    QCOMPARE(resultImage.size(), testImage.size());
    for (int y = 0; y < 256; ++y) {
        int lastRed = 0;
        for (int x = 0; x < 256; ++x) {
            QRgb p = resultImage.pixel(x, y);
            QVERIFY(qRed(p) >= lastRed);
            lastRed = qRed(p);
        }
    }
    for (int x = 0; x < 256; ++x) {
        int lastGreen = 0;
        for (int y = 0; y < 256; ++y) {
            QRgb p = resultImage.pixel(x, y);
            QVERIFY(qGreen(p) >= lastGreen);
            lastGreen = qGreen(p);
        }
    }
}

void tst_QColorSpace::loadImage()
{
    QString prefix = QFINDTESTDATA("resources/");
    QImageReader reader(prefix + "ProPhoto.jpg");
    QImage image = reader.read();

    QVERIFY(!image.isNull());
    QVERIFY(image.colorSpace().isValid());
    QCOMPARE(image.colorSpace(), QColorSpace::ProPhotoRgb);
    QVERIFY(!image.colorSpace().iccProfile().isEmpty());

    QColorSpace defaultProPhotoRgb = QColorSpace::ProPhotoRgb;
    QVERIFY(!defaultProPhotoRgb.iccProfile().isEmpty());

    // Test the iccProfile getter returns the ICC profile from the image
    // which since we didn't write it, isn't identical to our defaults.
    QVERIFY(defaultProPhotoRgb.iccProfile() != image.colorSpace().iccProfile());

    QColorTransform transform = image.colorSpace().transformationToColorSpace(QColorSpace::SRgb);
    float maxRed = 0;
    float maxBlue = 0;
    float maxRed2 = 0;
    float maxBlue2 = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor p = image.pixelColor(x, y);
            maxRed = std::max(maxRed, p.redF());
            maxBlue = std::max(maxBlue, p.blueF());
            p = transform.map(p);
            maxRed2 = std::max(maxRed2, p.redF());
            maxBlue2 = std::max(maxBlue2, p.blueF());

        }
    }
    // ProPhotoRgb can be a lot more red and blue than SRgb can, so it will have lower values.
    QVERIFY(maxRed2 > maxRed);
    QVERIFY(maxBlue2 > maxBlue);
}

void tst_QColorSpace::primaries()
{
    QColor black = QColor::fromRgbF(0.0f, 0.0f, 0.0f);
    QColor white = QColor::fromRgbF(1.0f, 1.0f, 1.0f);
    QColor red = QColor::fromRgbF(1.0f, 0.0f, 0.0f);
    QColor green = QColor::fromRgbF(0.0f, 1.0f, 0.0f);
    QColor blue = QColor::fromRgbF(0.0f, 0.0f, 1.0f);

    QColorTransform toAdobeRgb = QColorSpace(QColorSpace::SRgb).transformationToColorSpace(QColorSpace::AdobeRgb);

    QColor tblack = toAdobeRgb.map(black);
    QColor twhite = toAdobeRgb.map(white);
    QColor tred = toAdobeRgb.map(red);
    QColor tgreen = toAdobeRgb.map(green);
    QColor tblue = toAdobeRgb.map(blue);

    // Black is black
    QCOMPARE(tblack, black);

    // This white hasn't changed
    QCOMPARE(twhite, white);

    // Adobe's red and blue gamut corners are the same as sRGB's
    // So, a color in the red corner, will stay in the red corner
    // the same for blue, but not for green.
    QVERIFY(tred.greenF() < 0.001);
    QVERIFY(tred.blueF() < 0.001);
    QVERIFY(tblue.redF() < 0.001);
    QVERIFY(tblue.greenF() < 0.001);
    QVERIFY(tgreen.redF() > 0.2);
    QVERIFY(tgreen.blueF() > 0.2);
}

void tst_QColorSpace::primariesXyz()
{
    QColorSpace sRgb = QColorSpace::SRgb;
    QColorSpace adobeRgb = QColorSpace::AdobeRgb;
    QColorSpace displayP3 = QColorSpace::DisplayP3;
    QColorSpace proPhotoRgb = QColorSpace::ProPhotoRgb;

    // Check if our calculated matrices, match the precalculated ones.
    QCOMPARE(QColorSpacePrivate::get(sRgb)->toXyz, QColorMatrix::toXyzFromSRgb());
    QCOMPARE(QColorSpacePrivate::get(adobeRgb)->toXyz, QColorMatrix::toXyzFromAdobeRgb());
    QCOMPARE(QColorSpacePrivate::get(displayP3)->toXyz, QColorMatrix::toXyzFromDciP3D65());
    QCOMPARE(QColorSpacePrivate::get(proPhotoRgb)->toXyz, QColorMatrix::toXyzFromProPhotoRgb());
}

#ifdef QT_BUILD_INTERNAL
void tst_QColorSpace::primaries2_data()
{
    QTest::addColumn<QColorSpace::Primaries>("primariesId");

    QTest::newRow("sRGB") << QColorSpace::Primaries::SRgb;
    QTest::newRow("DCI-P3 (D65)") << QColorSpace::Primaries::DciP3D65;
    QTest::newRow("Adobe RGB (1998)") << QColorSpace::Primaries::AdobeRgb;
    QTest::newRow("ProPhoto RGB") << QColorSpace::Primaries::ProPhotoRgb;
}

void tst_QColorSpace::primaries2()
{
    QFETCH(QColorSpace::Primaries, primariesId);
    QColorSpace::PrimaryPoints primaries = QColorSpace::PrimaryPoints::fromPrimaries(primariesId);

    QColorSpace original(primariesId,  QColorSpace::TransferFunction::Linear);
    QColorSpace custom1(primaries.whitePoint, primaries.redPoint,
                        primaries.greenPoint, primaries.bluePoint, QColorSpace::TransferFunction::Linear);
    QCOMPARE(original, custom1);

    // A custom color swizzled color-space:
    QColorSpace custom2(primaries.whitePoint, primaries.bluePoint,
                        primaries.greenPoint, primaries.redPoint, QColorSpace::TransferFunction::Linear);

    QVERIFY(custom1 != custom2);
    QColor color1(255, 127, 63);
    QColor color2 = custom1.transformationToColorSpace(custom2).map(color1);
    QCOMPARE(color2.red(),   color1.blue());
    QCOMPARE(color2.green(), color1.green());
    QCOMPARE(color2.blue(),  color1.red());
    QCOMPARE(color2.alpha(), color1.alpha());
    QColor color3 = custom2.transformationToColorSpace(custom1).map(color2);
    QCOMPARE(color3.red(),   color1.red());
    QCOMPARE(color3.green(), color1.green());
    QCOMPARE(color3.blue(),  color1.blue());
    QCOMPARE(color3.alpha(), color1.alpha());
}
#endif

void tst_QColorSpace::invalidPrimaries()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("QColorSpace attempted constructed from invalid primaries"));
    QColorSpace custom(QPointF(), QPointF(), QPointF(), QPointF(), QColorSpace::TransferFunction::Linear);
    QVERIFY(!custom.isValid());
}

void tst_QColorSpace::changeTransferFunction()
{
    QColorSpace sRgb = QColorSpace::SRgb;

    QColorSpace sRgbLinear = sRgb.withTransferFunction(QColorSpace::TransferFunction::Linear);
    QCOMPARE(sRgbLinear.transferFunction(), QColorSpace::TransferFunction::Linear);
    QCOMPARE(sRgbLinear.gamma(), 1.0f);
    QCOMPARE(sRgbLinear.primaries(), QColorSpace::Primaries::SRgb);
    QCOMPARE(sRgbLinear, QColorSpace::SRgbLinear);
    QCOMPARE(sRgbLinear, QColorSpace(QColorSpace::SRgbLinear));
    QVERIFY(sRgbLinear != sRgb);
    QCOMPARE(sRgbLinear.withTransferFunction(QColorSpace::TransferFunction::SRgb), sRgb);

    QColorSpace aRgb = QColorSpace::AdobeRgb;
    aRgb.setTransferFunction(QColorSpace::TransferFunction::SRgb);
    QCOMPARE(aRgb.transferFunction(), QColorSpace::TransferFunction::SRgb);
    QCOMPARE(aRgb.primaries(), QColorSpace::Primaries::AdobeRgb);
    QVERIFY(aRgb != QColorSpace(QColorSpace::AdobeRgb));
    QVERIFY(aRgb != sRgb);
    QCOMPARE(aRgb.withTransferFunction(QColorSpace::TransferFunction::Gamma, 2.2f),
             QColorSpace(QColorSpace::AdobeRgb));
    QVERIFY(aRgb != QColorSpace(QColorSpace::AdobeRgb));
    aRgb.setTransferFunction(QColorSpace::TransferFunction::Gamma, 2.2f);
    QVERIFY(aRgb == QColorSpace(QColorSpace::AdobeRgb));

    QColorSpace undefined;
    QCOMPARE(undefined.withTransferFunction(QColorSpace::TransferFunction::Linear), undefined);

    QColorSpace partial;
    partial.setTransferFunction(QColorSpace::TransferFunction::SRgb);
    QCOMPARE(partial.transferFunction(), QColorSpace::TransferFunction::SRgb);
    QVERIFY(!partial.isValid());

    partial.setPrimaries(QColorSpace::Primaries::SRgb);
    QVERIFY(partial.isValid());
    QCOMPARE(partial, QColorSpace(QColorSpace::SRgb));
}

void tst_QColorSpace::changePrimaries()
{
    QColorSpace cs = QColorSpace::SRgb;
    cs.setPrimaries(QColorSpace::Primaries::DciP3D65);
    QVERIFY(cs.isValid());
    QCOMPARE(cs, QColorSpace(QColorSpace::DisplayP3));
    QCOMPARE(cs.transformModel(), QColorSpace::TransformModel::ThreeComponentMatrix);
    cs.setTransferFunction(QColorSpace::TransferFunction::Linear);
    cs.setPrimaries(QPointF(0.3127, 0.3290), QPointF(0.640, 0.330),
                    QPointF(0.3000, 0.6000), QPointF(0.150, 0.060));
    QCOMPARE(cs, QColorSpace(QColorSpace::SRgbLinear));


    QFile iccFile(QFINDTESTDATA("resources/") + "VideoHD.icc");
    QVERIFY(iccFile.open(QFile::ReadOnly));
    QByteArray iccData = iccFile.readAll();
    QColorSpace hdtvColorSpace = QColorSpace::fromIccProfile(iccData);
    QVERIFY(hdtvColorSpace.isValid());
    QCOMPARE(hdtvColorSpace.transformModel(), QColorSpace::TransformModel::ElementListProcessing);
    QCOMPARE(hdtvColorSpace.primaries(), QColorSpace::Primaries::Custom);
    QCOMPARE(hdtvColorSpace.transferFunction(), QColorSpace::TransferFunction::Custom);
    // Unsets both primaries and transferfunction because they were inseparable in element list processing
    hdtvColorSpace.setPrimaries(QColorSpace::Primaries::SRgb);
    QVERIFY(!hdtvColorSpace.isValid());
    hdtvColorSpace.setTransferFunction(QColorSpace::TransferFunction::SRgb);
    QVERIFY(hdtvColorSpace.isValid());
    QCOMPARE(hdtvColorSpace.transformModel(), QColorSpace::TransformModel::ThreeComponentMatrix);
    QCOMPARE(hdtvColorSpace, QColorSpace(QColorSpace::SRgb));
}

void tst_QColorSpace::transferFunctionTable()
{
    QVector<quint16> linearTable = { 0, 65535 };

    // Check linearSRgb is recognized
    QColorSpace linearSRgb(QColorSpace::Primaries::SRgb, linearTable);
    QCOMPARE(linearSRgb.primaries(), QColorSpace::Primaries::SRgb);
    QCOMPARE(linearSRgb.transferFunction(), QColorSpace::TransferFunction::Linear);
    QCOMPARE(linearSRgb.gamma(), 1.0);
    QCOMPARE(linearSRgb, QColorSpace::SRgbLinear);

    // Check other linear is recognized
    QColorSpace linearARgb(QColorSpace::Primaries::AdobeRgb, linearTable);
    QCOMPARE(linearARgb.primaries(), QColorSpace::Primaries::AdobeRgb);
    QCOMPARE(linearARgb.transferFunction(), QColorSpace::TransferFunction::Linear);
    QCOMPARE(linearARgb.gamma(), 1.0);

    // Check custom transfer function.
    QVector<quint16> customTable = { 0, 10, 100, 10000, 65535 };
    QColorSpace customSRgb(QColorSpace::Primaries::SRgb, customTable);
    QCOMPARE(customSRgb.primaries(), QColorSpace::Primaries::SRgb);
    QCOMPARE(customSRgb.transferFunction(), QColorSpace::TransferFunction::Custom);

    customSRgb.setTransferFunction(linearTable);
    QCOMPARE(customSRgb, QColorSpace::SRgbLinear);
}

void tst_QColorSpace::description()
{
    QColorSpace srgb(QColorSpace::SRgb);
    QCOMPARE(srgb.description(), QLatin1String("sRGB"));

    srgb.setTransferFunction(QColorSpace::TransferFunction::ProPhotoRgb);
    QCOMPARE(srgb.description(), QString()); // No longer sRGB
    srgb.setTransferFunction(QColorSpace::TransferFunction::Linear);
    QCOMPARE(srgb.description(), QLatin1String("Linear sRGB")); // Auto-detect

    srgb.setTransferFunction(QColorSpace::TransferFunction::ProPhotoRgb);
    srgb.setDescription(QStringLiteral("My custom sRGB"));
    QCOMPARE(srgb.description(), QLatin1String("My custom sRGB"));
    srgb.setTransferFunction(QColorSpace::TransferFunction::Linear);
    QCOMPARE(srgb.description(), QLatin1String("My custom sRGB")); // User given name not reset
    srgb.setDescription(QString());
    QCOMPARE(srgb.description(), QLatin1String("Linear sRGB")); // Set to empty returns default behavior
}

void tst_QColorSpace::whitePoint_data()
{
    QTest::addColumn<QColorSpace::NamedColorSpace>("namedColorSpace");
    QTest::addColumn<QPointF>("whitePoint");

    QTest::newRow("sRGB") << QColorSpace::SRgb << QColorVector::D65Chromaticity();
    QTest::newRow("Adobe RGB") << QColorSpace::AdobeRgb << QColorVector::D65Chromaticity();
    QTest::newRow("Display-P3") << QColorSpace::DisplayP3 << QColorVector::D65Chromaticity();
    QTest::newRow("ProPhoto RGB") << QColorSpace::ProPhotoRgb << QColorVector::D50Chromaticity();
}

void tst_QColorSpace::whitePoint()
{
    QFETCH(QColorSpace::NamedColorSpace, namedColorSpace);
    QFETCH(QPointF, whitePoint);

    QColorSpace colorSpace(namedColorSpace);
    QPointF wpt = colorSpace.whitePoint();
    QCOMPARE_LE(qAbs(wpt.x() - whitePoint.x()), 0.0000001);
    QCOMPARE_LE(qAbs(wpt.y() - whitePoint.y()), 0.0000001);
}

void tst_QColorSpace::setWhitePoint()
{
    QColorSpace colorSpace(QColorSpace::SRgb);
    colorSpace.setWhitePoint(QPointF(0.33, 0.33));
    QCOMPARE_NE(colorSpace, QColorSpace(QColorSpace::SRgb));
    colorSpace.setWhitePoint(QColorVector::D65Chromaticity());
    // Check our matrix manipulations returned us to where we came from
    QCOMPARE(colorSpace, QColorSpace(QColorSpace::SRgb));
}

void tst_QColorSpace::grayColorSpace()
{
    QColorSpace spc;
    QCOMPARE(spc.colorModel(), QColorSpace::ColorModel::Undefined);
    QVERIFY(!spc.isValid());
    spc.setWhitePoint(QColorVector::D65Chromaticity());
    spc.setTransferFunction(QColorSpace::TransferFunction::SRgb);
    QVERIFY(spc.isValid());
    QCOMPARE(spc.colorModel(), QColorSpace::ColorModel::Gray);

    QColorSpace spc2(QColorVector::D65Chromaticity(), QColorSpace::TransferFunction::SRgb);
    QVERIFY(spc2.isValid());
    QCOMPARE(spc2.colorModel(), QColorSpace::ColorModel::Gray);
    QCOMPARE(spc, spc2);

    QImage rgbImage(1, 8, QImage::Format_RGB32);
    QImage grayImage(1, 255, QImage::Format_Grayscale8);
    // RGB images can not have gray color space
    rgbImage.setColorSpace(spc2);
    grayImage.setColorSpace(spc2);
    QCOMPARE_NE(rgbImage.colorSpace(), spc2);
    QCOMPARE(grayImage.colorSpace(), spc2);
    // But gray images can have RGB color space
    rgbImage.setColorSpace(QColorSpace::SRgb);
    grayImage.setColorSpace(QColorSpace::SRgb);
    QCOMPARE(rgbImage.colorSpace(), QColorSpace(QColorSpace::SRgb));
    QCOMPARE(grayImage.colorSpace(), QColorSpace(QColorSpace::SRgb));

    // While we can not set a grayscale color space on rgb image, we can convert to one
    QImage grayImage2 = rgbImage.convertedToColorSpace(spc2);
    QCOMPARE(grayImage2.colorSpace(), spc2);
    QCOMPARE(grayImage2.format(), QImage::Format_Grayscale8);
}

void tst_QColorSpace::grayColorSpaceEffectivelySRgb()
{
    // Test grayscale colorspace conversion by making a gray color space that should act like sRGB on gray values.
    QColorSpace sRgb(QColorSpace::SRgb);
    QColorSpace sRgbGray(QColorVector::D65Chromaticity(), QColorSpace::TransferFunction::SRgb);

    QImage grayImage1(256, 1, QImage::Format_Grayscale8);
    QImage grayImage2(256, 1, QImage::Format_Grayscale8);
    for (int i = 0; i < 256; ++i) {
        grayImage1.bits()[i] = i;
        grayImage2.bits()[i] = i;
    }
    grayImage1.setColorSpace(sRgb);
    grayImage2.setColorSpace(sRgbGray);

    QImage rgbImage1 = grayImage1.convertedTo(QImage::Format_RGB32);
    QImage rgbImage2 = grayImage2.convertedToColorSpace(sRgb, QImage::Format_RGB32);

    QCOMPARE(rgbImage1, rgbImage2);
}

void tst_QColorSpace::scaleAlphaValue()
{
    QImage image(1, 1, QImage::Format_ARGB32);
    image.setPixel(0, 0, qRgba(255, 255, 255, 125));
    image.setColorSpace(QColorSpace::SRgb);
    image.convertToColorSpace(QColorSpace::SRgbLinear, QImage::Format_RGBA64);
    QCOMPARE(reinterpret_cast<const QRgba64 *>(image.constBits())->alpha(), 257 * 125);
}

void tst_QColorSpace::hdrColorSpaces()
{
    QColorSpace bt2020linear(QColorSpace::Primaries::Bt2020, QColorSpace::TransferFunction::Linear);
    QColorTransform pqToLinear = QColorSpace(QColorSpace::Bt2100Pq).transformationToColorSpace(bt2020linear);
    QColorTransform hlgToLinear = QColorSpace(QColorSpace::Bt2100Hlg).transformationToColorSpace(bt2020linear);

    QColor maxWhite = QColor::fromRgbF(1.0f, 1.0f, 1.0f);
    QColor hlgWhite = QColor::fromRgbF(0.5f, 0.5f, 0.5f);
    QCOMPARE(hlgToLinear.map(maxWhite).redF(), 12.f);
    QCOMPARE(hlgToLinear.map(maxWhite).greenF(), 12.f);
    QCOMPARE(hlgToLinear.map(maxWhite).blueF(), 12.f);
    QCOMPARE(hlgToLinear.map(hlgWhite).redF(), 1.f);
    QCOMPARE(hlgToLinear.map(hlgWhite).greenF(), 1.f);
    QCOMPARE(hlgToLinear.map(hlgWhite).blueF(), 1.f);
    QCOMPARE(pqToLinear.map(maxWhite).redF(), 64.f);
    QCOMPARE(pqToLinear.map(maxWhite).greenF(), 64.f);
    QCOMPARE(pqToLinear.map(maxWhite).blueF(), 64.f);

    {
        QImage image(1, 1, QImage::Format_RGBA32FPx4);
        image.setPixel(0, 0, qRgba(255, 255, 255, 255));
        image.setColorSpace(QColorSpace::Bt2100Pq);
        QImage image2 = image.convertedToColorSpace(bt2020linear);
        QCOMPARE(image2.pixelColor(0, 0).redF(), 64.f);
        image.setColorSpace(QColorSpace::Bt2100Hlg);
        image2 = image.convertedToColorSpace(bt2020linear);
        QCOMPARE(image2.pixelColor(0, 0).redF(), 12.f);
    }
    {
        QImage image(1, 1, QImage::Format_RGBA32FPx4_Premultiplied);
        image.setPixel(0, 0, qRgba(255, 255, 255, 255));
        image.setColorSpace(QColorSpace::Bt2100Pq);
        QImage image2 = image.convertedToColorSpace(bt2020linear);
        QCOMPARE(image2.pixelColor(0, 0).redF(), 64.f);
        image.setColorSpace(QColorSpace::Bt2100Hlg);
        image2 = image.convertedToColorSpace(bt2020linear);
        QCOMPARE(image2.pixelColor(0, 0).redF(), 12.f);
    }
    {
        QImage image(1, 1, QImage::Format_ARGB32);
        image.setPixel(0, 0, qRgba(255, 255, 255, 255));
        image.setColorSpace(QColorSpace::Bt2100Pq);
        QImage image2 = image.convertedToColorSpace(bt2020linear, QImage::Format_RGBA32FPx4);
        QCOMPARE(image2.pixelColor(0, 0).redF(), 64.f);
        image.setColorSpace(QColorSpace::Bt2100Hlg);
        image2 = image.convertedToColorSpace(bt2020linear, QImage::Format_RGBA32FPx4);
        QCOMPARE(image2.pixelColor(0, 0).redF(), 12.f);
    }
    {
        QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
        image.setPixel(0, 0, qRgba(255, 255, 255, 255));
        image.setColorSpace(QColorSpace::Bt2100Pq);
        QImage image2 = image.convertedToColorSpace(bt2020linear, QImage::Format_RGBA32FPx4);
        QCOMPARE(image2.pixelColor(0, 0).redF(), 64.f);
        image.setColorSpace(QColorSpace::Bt2100Hlg);
        image2 = image.convertedToColorSpace(bt2020linear, QImage::Format_RGBA32FPx4);
        QCOMPARE(image2.pixelColor(0, 0).redF(), 12.f);
    }
}

QTEST_MAIN(tst_QColorSpace)
#include "tst_qcolorspace.moc"
