/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <qcolorspace.h>
#include <qimage.h>
#include <qimagereader.h>

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
    QTest::addColumn<QString>("description");

    QString prefix = QFINDTESTDATA("resources/");
    // Read the official sRGB ICCv2 profile:
    QTest::newRow("sRGB2014 (ICCv2)") << prefix + "sRGB2014.icc" << QColorSpace::SRgb
                                           << QColorSpace::TransferFunction::SRgb << QString("sRGB2014");
    // My monitor's profile:
    QTest::newRow("HP ZR30w (ICCv4)") << prefix + "HP_ZR30w.icc" << QColorSpace::NamedColorSpace(0)
                                      << QColorSpace::TransferFunction::Gamma << QString("HP Z30i");
}

void tst_QColorSpace::fromIccProfile()
{
    QFETCH(QString, testProfile);
    QFETCH(QColorSpace::NamedColorSpace, namedColorSpace);
    QFETCH(QColorSpace::TransferFunction, transferFunction);
    QFETCH(QString, description);

    QFile file(testProfile);
    file.open(QIODevice::ReadOnly);
    QByteArray iccProfile = file.readAll();
    QColorSpace fileColorSpace = QColorSpace::fromIccProfile(iccProfile);
    QVERIFY(fileColorSpace.isValid());

    if (namedColorSpace)
        QCOMPARE(fileColorSpace, namedColorSpace);

    QCOMPARE(fileColorSpace.transferFunction(), transferFunction);
    QCOMPARE(QColorSpacePrivate::get(fileColorSpace)->description, description);
}

void tst_QColorSpace::imageConversion_data()
{
    QTest::addColumn<QColorSpace::NamedColorSpace>("fromColorSpace");
    QTest::addColumn<QColorSpace::NamedColorSpace>("toColorSpace");
    QTest::addColumn<int>("tolerance");

    QTest::newRow("sRGB -> Display-P3") << QColorSpace::SRgb << QColorSpace::DisplayP3 << 0;
    QTest::newRow("sRGB -> Adobe RGB") << QColorSpace::SRgb << QColorSpace::AdobeRgb << 2;
    QTest::newRow("Display-P3 -> sRGB") << QColorSpace::DisplayP3 << QColorSpace::SRgb << 0;
    QTest::newRow("Adobe RGB -> sRGB") << QColorSpace::AdobeRgb << QColorSpace::SRgb << 2;
    QTest::newRow("Display-P3 -> Adobe RGB") << QColorSpace::DisplayP3 << QColorSpace::AdobeRgb << 2;
    QTest::newRow("sRGB -> sRGB Linear") << QColorSpace::SRgb << QColorSpace::SRgbLinear << 0;
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
        QVERIFY(qRed(p) >= lastRed);
        QVERIFY(qGreen(p) >= lastGreen);
        QVERIFY(qBlue(p) >= lastBlue);
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
        QVERIFY(qAbs(qRed(p) - qGreen(p)) <= tolerance);
        QVERIFY(qAbs(qRed(p) - qBlue(p)) <= tolerance);
        QVERIFY((lastRed   - qRed(p))   <= (tolerance / 2));
        QVERIFY((lastGreen - qGreen(p)) <= (tolerance / 2));
        QVERIFY((lastBlue  - qBlue(p))  <= (tolerance / 2));
        lastRed = qRed(p);
        lastGreen = qGreen(p);
        lastBlue = qBlue(p);
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
    qreal maxRed = 0;
    qreal maxBlue = 0;
    qreal maxRed2 = 0;
    qreal maxBlue2 = 0;
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
    QColor black = QColor::fromRgbF(0.0, 0.0, 0.0);
    QColor white = QColor::fromRgbF(1.0, 1.0, 1.0);
    QColor red = QColor::fromRgbF(1.0, 0.0, 0.0);
    QColor green = QColor::fromRgbF(0.0, 1.0, 0.0);
    QColor blue = QColor::fromRgbF(0.0, 0.0, 1.0);

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
    QColorSpacePrimaries primaries(primariesId);

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
    cs.setTransferFunction(QColorSpace::TransferFunction::Linear);
    cs.setPrimaries(QPointF(0.3127, 0.3290), QPointF(0.640, 0.330),
                    QPointF(0.3000, 0.6000), QPointF(0.150, 0.060));
    QCOMPARE(cs, QColorSpace(QColorSpace::SRgbLinear));
}

QTEST_MAIN(tst_QColorSpace)
#include "tst_qcolorspace.moc"
