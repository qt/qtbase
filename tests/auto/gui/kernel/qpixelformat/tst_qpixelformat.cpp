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

#include <QtGui/qpixelformat.h>

class tst_QPixelFormat : public QObject
{
    Q_OBJECT

private slots:
    void testOperators();
    void testQVectorOfFormats();
    void testRGB();
    void testCMYK();
    void testHSLandHSV();
    void testYUV_data();
    void testYUV();
    void testEnums();
};

void tst_QPixelFormat::testOperators()
{
    QPixelFormat first = qPixelFormatRgba(8,8,8,8,QPixelFormat::UsesAlpha, QPixelFormat::AtBeginning, QPixelFormat::Premultiplied);
    QPixelFormat second = qPixelFormatRgba(8,8,8,8,QPixelFormat::UsesAlpha, QPixelFormat::AtBeginning, QPixelFormat::Premultiplied);
    QCOMPARE(first, second);

    QPixelFormat third = qPixelFormatRgba(8,8,8,8,QPixelFormat::UsesAlpha, QPixelFormat::AtEnd, QPixelFormat::NotPremultiplied);
    QVERIFY(first != third);
}

void tst_QPixelFormat::testQVectorOfFormats()
{
    QVector<QPixelFormat> reallocedVector;
    QVector<QPixelFormat> reservedVector;
    reservedVector.reserve(QImage::NImageFormats);
    for (int i = 0; i < QImage::NImageFormats; i++) {
        if (i == 0 || i == 2) // skip invalid and monolsb
            continue;
        QImage::Format image_format = static_cast<QImage::Format>(i);
        QPixelFormat format = QImage::toPixelFormat(image_format);
        reallocedVector.append(format);
        reservedVector.append(format);
    }

    for (int i = 0; i < reallocedVector.size(); i++) {
        QCOMPARE(reallocedVector.at(i), reservedVector.at(i));
    }
}

void tst_QPixelFormat::testRGB()
{
    QPixelFormat argb8888 = qPixelFormatRgba(8,8,8,8,QPixelFormat::UsesAlpha,QPixelFormat::AtBeginning, QPixelFormat::Premultiplied);
    QCOMPARE(argb8888.redSize(), uchar(8));
    QCOMPARE(argb8888.greenSize(), uchar(8));
    QCOMPARE(argb8888.blueSize(), uchar(8));
    QCOMPARE(argb8888.alphaSize(), uchar(8));

    QPixelFormat rgb565 = qPixelFormatRgba(5,6,5,0,QPixelFormat::IgnoresAlpha,QPixelFormat::AtBeginning, QPixelFormat::NotPremultiplied);
    QCOMPARE(rgb565.redSize(), uchar(5));
    QCOMPARE(rgb565.greenSize(), uchar(6));
    QCOMPARE(rgb565.blueSize(), uchar(5));
    QCOMPARE(rgb565.alphaSize(), uchar(0));
    QCOMPARE(rgb565.bitsPerPixel(), uchar(16));

    QPixelFormat rgba1235 = qPixelFormatRgba(1,2,3,5,QPixelFormat::IgnoresAlpha, QPixelFormat::AtEnd, QPixelFormat::Premultiplied);
    QCOMPARE(rgba1235.redSize(), uchar(1));
    QCOMPARE(rgba1235.greenSize(), uchar(2));
    QCOMPARE(rgba1235.blueSize(), uchar(3));
    QCOMPARE(rgba1235.alphaSize(), uchar(5));
    QCOMPARE(rgba1235.bitsPerPixel(), uchar(1 + 2 + 3 + 5));
}

void tst_QPixelFormat::testCMYK()
{
    QPixelFormat cmyk6 = qPixelFormatCmyk(6);
    QCOMPARE(cmyk6.cyanSize(), uchar(6));
    QCOMPARE(cmyk6.magentaSize(), uchar(6));
    QCOMPARE(cmyk6.yellowSize(), uchar(6));
    QCOMPARE(cmyk6.blackSize(), uchar(6));
    QCOMPARE(cmyk6.bitsPerPixel(), uchar(6*4));

    QPixelFormat cmykWithAlpha = qPixelFormatCmyk(8,8);
    QCOMPARE(cmykWithAlpha.bitsPerPixel(), uchar(8*5));
}
void tst_QPixelFormat::testHSLandHSV()
{
    QPixelFormat hsl = qPixelFormatHsl(3,5);

    QCOMPARE(hsl.hueSize(), uchar(3));
    QCOMPARE(hsl.saturationSize(), uchar(3));
    QCOMPARE(hsl.lightnessSize(), uchar(3));
    QCOMPARE(hsl.bitsPerPixel(), uchar(3 * 3 + 5));

    QPixelFormat hsv = qPixelFormatHsv(5,7);

    QCOMPARE(hsv.hueSize(), uchar(5));
    QCOMPARE(hsv.saturationSize(), uchar(5));
    QCOMPARE(hsv.brightnessSize(), uchar(5));
    QCOMPARE(hsv.bitsPerPixel(), uchar(5 * 3 + 7));
}

Q_DECLARE_METATYPE(QPixelFormat::YUVLayout)
void tst_QPixelFormat::testYUV_data()
{
    QTest::addColumn<QPixelFormat::YUVLayout>("yuv_layout");
    QTest::newRow("YUV Layout YUV444") << QPixelFormat::YUV444;
    QTest::newRow("YUV Layout YUV422") << QPixelFormat::YUV422;
    QTest::newRow("YUV Layout YUV411") << QPixelFormat::YUV411;
    QTest::newRow("YUV Layout YUV420P") << QPixelFormat::YUV420P;
    QTest::newRow("YUV Layout YUV420SP") << QPixelFormat::YUV420SP;
    QTest::newRow("YUV Layout YV12") << QPixelFormat::YV12;
    QTest::newRow("YUV Layout UYVY") << QPixelFormat::UYVY;
    QTest::newRow("YUV Layout YUYV") << QPixelFormat::YUYV;
    QTest::newRow("YUV Layout NV12") << QPixelFormat::NV12;
    QTest::newRow("YUV Layout NV21") << QPixelFormat::NV21;
    QTest::newRow("YUV Layout IMC1") << QPixelFormat::IMC1;
    QTest::newRow("YUV Layout IMC2") << QPixelFormat::IMC2;
    QTest::newRow("YUV Layout IMC3") << QPixelFormat::IMC3;
    QTest::newRow("YUV Layout IMC4") << QPixelFormat::IMC4;
    QTest::newRow("YUV Layout Y8") << QPixelFormat::Y8;
    QTest::newRow("YUV Layout Y16") << QPixelFormat::Y16;
}

void tst_QPixelFormat::testYUV()
{
    QFETCH(QPixelFormat::YUVLayout, yuv_layout);

    QPixelFormat format = qPixelFormatYuv(yuv_layout, 0);

    switch (yuv_layout) {
    case QPixelFormat::YUV444:
        QCOMPARE(format.bitsPerPixel(), uchar(24));
        break;
    case QPixelFormat::YUV422:
        QCOMPARE(format.bitsPerPixel(), uchar(16));
        break;
    case QPixelFormat::YUV411:
    case QPixelFormat::YUV420P:
    case QPixelFormat::YUV420SP:
    case QPixelFormat::YV12:
        QCOMPARE(format.bitsPerPixel(), uchar(12));
        break;
    case QPixelFormat::UYVY:
    case QPixelFormat::YUYV:
        QCOMPARE(format.bitsPerPixel(), uchar(16));
        break;
    case QPixelFormat::NV12:
    case QPixelFormat::NV21:
        QCOMPARE(format.bitsPerPixel(), uchar(12));
        break;
    case QPixelFormat::IMC1:
    case QPixelFormat::IMC2:
    case QPixelFormat::IMC3:
    case QPixelFormat::IMC4:
        QCOMPARE(format.bitsPerPixel(), uchar(12));
        break;
    case QPixelFormat::Y8:
        QCOMPARE(format.bitsPerPixel(), uchar(8));
        break;
    case QPixelFormat::Y16:
        QCOMPARE(format.bitsPerPixel(), uchar(16));
        break;
    default:
        QVERIFY(!"the value stored for the yuvLayout is wrong!");
    }

}

void tst_QPixelFormat::testEnums()
{
    QPixelFormat allSet = QPixelFormat(QPixelFormat::BGR,1,2,3,4,5,6,
                                                  QPixelFormat::UsesAlpha,
                                                  QPixelFormat::AtEnd,
                                                  QPixelFormat::Premultiplied,
                                                  QPixelFormat::FloatingPoint,
                                                  QPixelFormat::BigEndian,
                                                  (1 << 6) - 1);

    QCOMPARE(allSet.alphaUsage(), QPixelFormat::UsesAlpha);
    QCOMPARE(allSet.alphaPosition(), QPixelFormat::AtEnd);
    QCOMPARE(allSet.premultiplied(), QPixelFormat::Premultiplied);
    QCOMPARE(allSet.byteOrder(), QPixelFormat::BigEndian);
    QCOMPARE(allSet.typeInterpretation(), QPixelFormat::FloatingPoint);
    QCOMPARE(allSet.byteOrder(), QPixelFormat::BigEndian);
    QCOMPARE(allSet.subEnum(), uchar(63));

    QPixelFormat nonSet = QPixelFormat(QPixelFormat::RGB,6,5,4,3,2,1,
                                                  QPixelFormat::IgnoresAlpha,
                                                  QPixelFormat::AtBeginning,
                                                  QPixelFormat::NotPremultiplied,
                                                  QPixelFormat::UnsignedInteger,
                                                  QPixelFormat::LittleEndian);

    QCOMPARE(nonSet.alphaUsage(), QPixelFormat::IgnoresAlpha);
    QCOMPARE(nonSet.alphaPosition(), QPixelFormat::AtBeginning);
    QCOMPARE(nonSet.premultiplied(), QPixelFormat::NotPremultiplied);
    QCOMPARE(nonSet.byteOrder(), QPixelFormat::LittleEndian);
    QCOMPARE(nonSet.typeInterpretation(), QPixelFormat::UnsignedInteger);
    QCOMPARE(nonSet.byteOrder(), QPixelFormat::LittleEndian);
    QCOMPARE(nonSet.subEnum(), uchar(0));
}

#include <tst_qpixelformat.moc>
QTEST_MAIN(tst_QPixelFormat);
