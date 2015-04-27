/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include <qtest.h>
#include <QImage>

Q_DECLARE_METATYPE(QImage::Format)

class tst_QImageConversion : public QObject
{
    Q_OBJECT
private slots:
    void convertRgb888ToRgb32_data();
    void convertRgb888ToRgb32();

    void convertRgb888ToRgbx8888_data();
    void convertRgb888ToRgbx8888();

    void convertRgb32ToRgb888_data();
    void convertRgb32ToRgb888();

    void convertRgb16_data();
    void convertRgb16();

    void convertRgb32_data();
    void convertRgb32();

    void convertGeneric_data();
    void convertGeneric();

    void convertGenericInplace_data();
    void convertGenericInplace();

private:
    QImage generateImageRgb888(int width, int height);
    QImage generateImageRgb16(int width, int height);
    QImage generateImageRgb32(int width, int height);
    QImage generateImageArgb32(int width, int height);
};

void tst_QImageConversion::convertRgb888ToRgb32_data()
{
    QTest::addColumn<QImage>("inputImage");

    // height = 5000 to get interesting timing.

    // 3 pixels wide -> smaller than regular vector of 128bits
    QTest::newRow("width: 3px; height: 5000px;") << generateImageRgb888(3, 5000);

    // 8 pixels wide -> potential for 2 vectors
    QTest::newRow("width: 8px; height: 5000px;") << generateImageRgb888(8, 5000);

    // 16 pixels, minimum for the SSSE3 implementation
    QTest::newRow("width: 16px; height: 5000px;") << generateImageRgb888(16, 5000);

    // 200 pixels, more realistic use case
    QTest::newRow("width: 200px; height: 5000px;") << generateImageRgb888(200, 5000);

    // 2000 pixels -> typical values for pictures
    QTest::newRow("width: 2000px; height: 2000px;") << generateImageRgb888(2000, 2000);
}

void tst_QImageConversion::convertRgb888ToRgb32()
{
    QFETCH(QImage, inputImage);

    QBENCHMARK {
        volatile QImage output = inputImage.convertToFormat(QImage::Format_RGB32);
        // we need the volatile and the following to make sure the compiler does not do
        // anything stupid :)
        (void)output;
    }
}

void tst_QImageConversion::convertRgb888ToRgbx8888_data()
{
    convertRgb888ToRgb32_data();
}

void tst_QImageConversion::convertRgb888ToRgbx8888()
{
    QFETCH(QImage, inputImage);

    QBENCHMARK {
        volatile QImage output = inputImage.convertToFormat(QImage::Format_RGBX8888);
        // we need the volatile and the following to make sure the compiler does not do
        // anything stupid :)
        (void)output;
    }
}

void tst_QImageConversion::convertRgb32ToRgb888_data()
{
    QTest::addColumn<QImage>("inputImage");
    // height = 5000 to get interesting timing.

    // 3 pixels wide -> smaller than regular vector of 128bits
    QTest::newRow("width: 3px; height: 5000px;") << generateImageRgb32(3, 5000);

    // 8 pixels wide -> potential for 2 vectors
    QTest::newRow("width: 8px; height: 5000px;") << generateImageRgb32(8, 5000);

    // 16 pixels, minimum for the SSSE3 implementation
    QTest::newRow("width: 16px; height: 5000px;") << generateImageRgb32(16, 5000);

    // 50 pixels, more realistic use case
    QTest::newRow("width: 50px; height: 5000px;") << generateImageRgb32(50, 5000);

    // 2000 pixels -> typical values for pictures
    QTest::newRow("width: 2000px; height: 2000px;") << generateImageRgb32(2000, 2000);
}

void tst_QImageConversion::convertRgb32ToRgb888()
{
    QFETCH(QImage, inputImage);

    QBENCHMARK {
        volatile QImage output = inputImage.convertToFormat(QImage::Format_RGB888);
        // we need the volatile and the following to make sure the compiler does not do
        // anything stupid :)
        (void)output;
    }
}

void tst_QImageConversion::convertRgb16_data()
{
    QTest::addColumn<QImage>("inputImage");
    QTest::addColumn<QImage::Format>("outputFormat");
    QImage rgb16 = generateImageRgb16(1000, 1000);

    QTest::newRow("rgb32") << rgb16 << QImage::Format_RGB32;
    QTest::newRow("rgb888") << rgb16 << QImage::Format_RGB888;
    QTest::newRow("rgb666") << rgb16 << QImage::Format_RGB666;
    QTest::newRow("rgb555") << rgb16 << QImage::Format_RGB555;
}

void tst_QImageConversion::convertRgb16()
{
    QFETCH(QImage, inputImage);
    QFETCH(QImage::Format, outputFormat);

    QBENCHMARK {
        QImage output = inputImage.convertToFormat(outputFormat);
        output.constBits();
    }
}

void tst_QImageConversion::convertRgb32_data()
{
    QTest::addColumn<QImage>("inputImage");
    QTest::addColumn<QImage::Format>("outputFormat");
    QImage rgb32 = generateImageRgb32(1000, 1000);
    QImage argb32 = generateImageArgb32(1000, 1000);
    QImage argb32pm = argb32.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    QTest::newRow("rgb32 -> rgb16") << rgb32 << QImage::Format_RGB16;
    QTest::newRow("rgb32 -> argb32") << rgb32 << QImage::Format_ARGB32;
    QTest::newRow("rgb32 -> argb32pm") << rgb32 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("rgb32 -> rgbx8888") << rgb32 << QImage::Format_RGBX8888;
    QTest::newRow("rgb32 -> rgba8888") << rgb32 << QImage::Format_RGBA8888;
    QTest::newRow("rgb32 -> rgba8888pm") << rgb32 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("rgb32 -> rgb30") << rgb32 << QImage::Format_RGB30;
    QTest::newRow("rgb32 -> a2bgr30") << rgb32 << QImage::Format_A2BGR30_Premultiplied;
    QTest::newRow("rgb32 -> rgb888") << rgb32 << QImage::Format_RGB888;
    QTest::newRow("rgb32 -> rgb666") << rgb32 << QImage::Format_RGB666;
    QTest::newRow("rgb32 -> rgb555") << rgb32 << QImage::Format_RGB555;

    QTest::newRow("argb32 -> rgb16") << argb32 << QImage::Format_RGB16;
    QTest::newRow("argb32 -> rgb32") << argb32 << QImage::Format_RGB32;
    QTest::newRow("argb32 -> argb32pm") << argb32 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("argb32 -> rgbx8888") << argb32 << QImage::Format_RGBX8888;
    QTest::newRow("argb32 -> rgba8888") << argb32 << QImage::Format_RGBA8888;
    QTest::newRow("argb32 -> rgba8888pm") << argb32 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("argb32 -> rgb30") << argb32 << QImage::Format_RGB30;
    QTest::newRow("argb32 -> a2bgr30") << argb32 << QImage::Format_A2BGR30_Premultiplied;
    QTest::newRow("argb32 -> rgb888") << argb32 << QImage::Format_RGB888;
    QTest::newRow("argb32 -> rgb666") << argb32 << QImage::Format_RGB666;
    QTest::newRow("argb32 -> argb8565pm") << argb32 << QImage::Format_ARGB8565_Premultiplied;
    QTest::newRow("argb32 -> argb4444pm") << argb32 << QImage::Format_ARGB4444_Premultiplied;

    QTest::newRow("argb32pm -> rgb16") << argb32pm << QImage::Format_RGB16;
    QTest::newRow("argb32pm -> rgb32") << argb32pm << QImage::Format_RGB32;
    QTest::newRow("argb32pm -> argb32") << argb32pm << QImage::Format_ARGB32;
    QTest::newRow("argb32pm -> rgbx8888") << argb32pm << QImage::Format_RGBX8888;
    QTest::newRow("argb32pm -> rgba8888") << argb32pm << QImage::Format_RGBA8888;
    QTest::newRow("argb32pm -> rgba8888pm") << argb32pm << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("argb32pm -> rgb30") << argb32pm << QImage::Format_RGB30;
    QTest::newRow("argb32pm -> a2bgr30") << argb32pm << QImage::Format_A2BGR30_Premultiplied;
    QTest::newRow("argb32pm -> rgb888") << argb32pm << QImage::Format_RGB888;
    QTest::newRow("argb32pm -> rgb666") << argb32pm << QImage::Format_RGB666;
    QTest::newRow("argb32pm -> argb8565pm") << argb32pm << QImage::Format_ARGB8565_Premultiplied;
    QTest::newRow("argb32pm -> argb4444pm") << argb32pm << QImage::Format_ARGB4444_Premultiplied;
}

void tst_QImageConversion::convertRgb32()
{
    QFETCH(QImage, inputImage);
    QFETCH(QImage::Format, outputFormat);

    QBENCHMARK {
        QImage output = inputImage.convertToFormat(outputFormat);
        output.constBits();
    }
}

void tst_QImageConversion::convertGeneric_data()
{
    QTest::addColumn<QImage>("inputImage");
    QTest::addColumn<QImage::Format>("outputFormat");
    QImage rgb32 = generateImageRgb32(1000, 1000);
    QImage argb32 = generateImageArgb32(1000, 1000);
    QImage rgba32 = argb32.convertToFormat(QImage::Format_RGBA8888);
    QImage bgr30 = rgb32.convertToFormat(QImage::Format_BGR30);
    QImage a2rgb30 = argb32.convertToFormat(QImage::Format_A2RGB30_Premultiplied);

    QTest::newRow("rgba8888 -> rgb32") << rgba32 << QImage::Format_RGB32;
    QTest::newRow("rgba8888 -> argb32") << rgba32 << QImage::Format_ARGB32;
    QTest::newRow("rgba8888 -> argb32pm") << rgba32 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("rgba8888 -> rgbx8888") << rgba32 << QImage::Format_RGBX8888;
    QTest::newRow("rgba8888 -> rgba8888pm") << rgba32 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("rgba8888 -> rgb30") << rgba32 << QImage::Format_RGB30;
    QTest::newRow("rgba8888 -> a2bgr30") << rgba32 << QImage::Format_A2BGR30_Premultiplied;

    QTest::newRow("bgr30 -> rgb32") << bgr30 << QImage::Format_RGB32;
    QTest::newRow("bgr30 -> argb32") << bgr30 << QImage::Format_ARGB32;
    QTest::newRow("bgr30 -> argb32pm") << bgr30 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("bgr30 -> rgbx8888") << bgr30 << QImage::Format_RGBX8888;
    QTest::newRow("bgr30 -> rgba8888") << bgr30 << QImage::Format_RGBA8888;
    QTest::newRow("bgr30 -> rgba8888pm") << bgr30 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("bgr30 -> rgb30") << bgr30 << QImage::Format_RGB30;
    QTest::newRow("bgr30 -> a2bgr30") << bgr30 << QImage::Format_A2BGR30_Premultiplied;

    QTest::newRow("a2rgb30 -> rgb32") << a2rgb30 << QImage::Format_RGB32;
    QTest::newRow("a2rgb30 -> argb32") << a2rgb30 << QImage::Format_ARGB32;
    QTest::newRow("a2rgb30 -> argb32pm") << a2rgb30 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("a2rgb30 -> rgbx8888") << a2rgb30 << QImage::Format_RGBX8888;
    QTest::newRow("a2rgb30 -> rgba8888") << a2rgb30 << QImage::Format_RGBA8888;
    QTest::newRow("a2rgb30 -> rgba8888pm") << a2rgb30 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("a2rgb30 -> rgb30") << a2rgb30 << QImage::Format_RGB30;
    QTest::newRow("a2rgb30 -> bgr30") << a2rgb30 << QImage::Format_BGR30;
    QTest::newRow("a2rgb30 -> a2bgr30") << a2rgb30 << QImage::Format_A2BGR30_Premultiplied;
}

void tst_QImageConversion::convertGeneric()
{
    QFETCH(QImage, inputImage);
    QFETCH(QImage::Format, outputFormat);

    QBENCHMARK {
        QImage output = inputImage.convertToFormat(outputFormat);
        output.constBits();
    }
}

void tst_QImageConversion::convertGenericInplace_data()
{
    QTest::addColumn<QImage>("inputImage");
    QTest::addColumn<QImage::Format>("outputFormat");

    QImage argb32 = generateImageArgb32(1000, 1000);
    QImage argb32pm = argb32.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage rgba8888 = argb32.convertToFormat(QImage::Format_RGBA8888);

    QTest::newRow("argb32 -> argb32pm -> argb32") << argb32 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("argb32 -> rgb32 -> argb32") << argb32 << QImage::Format_RGB32;
    QTest::newRow("argb32 -> rgba8888 -> argb32") << argb32 << QImage::Format_RGBA8888;
    QTest::newRow("argb32 -> rgba8888pm -> argb32") << argb32 << QImage::Format_RGBA8888_Premultiplied;

    QTest::newRow("argb32pm -> argb32 -> argb32pm") << argb32pm << QImage::Format_ARGB32;
    QTest::newRow("argb32pm -> rgb32 -> argb32pm") << argb32pm << QImage::Format_RGB32;
    QTest::newRow("argb32pm -> rgba8888pm -> argb32pm") << argb32pm << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("argb32pm -> rgba8888 -> argb32pm") << argb32pm << QImage::Format_RGBA8888;
    QTest::newRow("argb32pm -> rgbx8888 -> argb32pm") << argb32pm << QImage::Format_RGBX8888;

    QTest::newRow("rgba8888 -> argb32 -> rgba8888") << rgba8888 << QImage::Format_ARGB32;
    QTest::newRow("rgba8888 -> rgb32 -> rgba8888") << rgba8888 << QImage::Format_RGB32;
    QTest::newRow("rgba8888 -> argb32pm -> rgba8888") << rgba8888 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("rgba8888 -> rgba8888pm -> rgba8888") << rgba8888 << QImage::Format_RGBA8888_Premultiplied;
}

void tst_QImageConversion::convertGenericInplace()
{
    QFETCH(QImage, inputImage);
    QFETCH(QImage::Format, outputFormat);

    QImage::Format inputFormat = inputImage.format();
    QImage tmpImage = qMove(inputImage);

    QBENCHMARK {
        tmpImage = (qMove(tmpImage).convertToFormat(outputFormat)).convertToFormat(inputFormat);
    }
}

/*
 Fill a RGB888 image with "random" pixel values.
 */
QImage tst_QImageConversion::generateImageRgb888(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB888);
    const int byteWidth = width * 3;

    for (int y = 0; y < image.height(); ++y) {
        uchar *scanline = image.scanLine(y);
        for (int x = 0; x < byteWidth; ++x)
            scanline[x] = x ^ y;
    }
    return image;
}

/*
 Fill a RGB16 image with "random" pixel values.
 */
QImage tst_QImageConversion::generateImageRgb16(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB16);
    const int byteWidth = width * 2;

    for (int y = 0; y < image.height(); ++y) {
        uchar *scanline = image.scanLine(y);
        for (int x = 0; x < byteWidth; ++x)
            scanline[x] = x ^ y;
    }
    return image;
}

/*
 Fill a RGB32 image with "random" pixel values.
 */
QImage tst_QImageConversion::generateImageRgb32(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);

    for (int y = 0; y < image.height(); ++y) {
        QRgb *scanline = (QRgb*)image.scanLine(y);
        for (int x = 0; x < width; ++x)
            scanline[x] = qRgb(x, y, x ^ y);
    }
    return image;
}

/*
 Fill a ARGB32 image with "random" pixel values.
 */
QImage tst_QImageConversion::generateImageArgb32(int width, int height)
{
    QImage image(width, height, QImage::Format_ARGB32);

    for (int y = 0; y < image.height(); ++y) {
        QRgb *scanline = (QRgb*)image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            int alpha = (x ^ y) & 0x1ff;
            alpha = qMax(0, qMin(alpha - 128, 255));
            scanline[x] = qRgba(x, y, x ^ y, alpha);
        }
    }
    return image;
}

QTEST_MAIN(tst_QImageConversion)
#include "tst_qimageconversion.moc"
