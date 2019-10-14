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
        QImage output = inputImage.convertToFormat(QImage::Format_RGB32);
        output.constBits();
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
        QImage output = inputImage.convertToFormat(QImage::Format_RGBX8888);
        output.constBits();
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
        QImage output = inputImage.convertToFormat(QImage::Format_RGB888);
        output.constBits();
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
    QTest::newRow("argb8565") << rgb16 << QImage::Format_ARGB8565_Premultiplied;
    QTest::newRow("argb8555") << rgb16 << QImage::Format_ARGB8555_Premultiplied;
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
    QTest::newRow("rgb32 -> bgr888") << rgb32 << QImage::Format_BGR888;
    QTest::newRow("rgb32 -> rgb666") << rgb32 << QImage::Format_RGB666;
    QTest::newRow("rgb32 -> rgb555") << rgb32 << QImage::Format_RGB555;
    QTest::newRow("rgb32 -> argb8565pm") << rgb32 << QImage::Format_ARGB8565_Premultiplied;

    QTest::newRow("argb32 -> rgb16") << argb32 << QImage::Format_RGB16;
    QTest::newRow("argb32 -> rgb32") << argb32 << QImage::Format_RGB32;
    QTest::newRow("argb32 -> argb32pm") << argb32 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("argb32 -> rgbx8888") << argb32 << QImage::Format_RGBX8888;
    QTest::newRow("argb32 -> rgba8888") << argb32 << QImage::Format_RGBA8888;
    QTest::newRow("argb32 -> rgba8888pm") << argb32 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("argb32 -> rgb30") << argb32 << QImage::Format_RGB30;
    QTest::newRow("argb32 -> a2bgr30") << argb32 << QImage::Format_A2BGR30_Premultiplied;
    QTest::newRow("argb32 -> rgb888") << argb32 << QImage::Format_RGB888;
    QTest::newRow("argb32 -> bgr888") << argb32 << QImage::Format_BGR888;
    QTest::newRow("argb32 -> rgb666") << argb32 << QImage::Format_RGB666;
    QTest::newRow("argb32 -> argb8565pm") << argb32 << QImage::Format_ARGB8565_Premultiplied;
    QTest::newRow("argb32 -> argb4444pm") << argb32 << QImage::Format_ARGB4444_Premultiplied;
    QTest::newRow("argb32 -> argb6666pm") << argb32 << QImage::Format_ARGB6666_Premultiplied;
    QTest::newRow("argb32 -> rgba64") << argb32 << QImage::Format_RGBA64;
    QTest::newRow("argb32 -> rgba64pm") << argb32 << QImage::Format_RGBA64_Premultiplied;

    QTest::newRow("argb32pm -> rgb16") << argb32pm << QImage::Format_RGB16;
    QTest::newRow("argb32pm -> rgb32") << argb32pm << QImage::Format_RGB32;
    QTest::newRow("argb32pm -> argb32") << argb32pm << QImage::Format_ARGB32;
    QTest::newRow("argb32pm -> rgbx8888") << argb32pm << QImage::Format_RGBX8888;
    QTest::newRow("argb32pm -> rgba8888") << argb32pm << QImage::Format_RGBA8888;
    QTest::newRow("argb32pm -> rgba8888pm") << argb32pm << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("argb32pm -> rgb30") << argb32pm << QImage::Format_RGB30;
    QTest::newRow("argb32pm -> a2bgr30") << argb32pm << QImage::Format_A2BGR30_Premultiplied;
    QTest::newRow("argb32pm -> rgb888") << argb32pm << QImage::Format_RGB888;
    QTest::newRow("argb32pm -> bgr888") << argb32pm << QImage::Format_BGR888;
    QTest::newRow("argb32pm -> rgb666") << argb32pm << QImage::Format_RGB666;
    QTest::newRow("argb32pm -> argb8565pm") << argb32pm << QImage::Format_ARGB8565_Premultiplied;
    QTest::newRow("argb32pm -> argb4444pm") << argb32pm << QImage::Format_ARGB4444_Premultiplied;
    QTest::newRow("argb32pm -> argb6666pm") << argb32pm << QImage::Format_ARGB6666_Premultiplied;
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
    QImage i8 = argb32.convertToFormat(QImage::Format_Indexed8);
    QImage rgba32 = argb32.convertToFormat(QImage::Format_RGBA8888);
    QImage bgr30 = rgb32.convertToFormat(QImage::Format_BGR30);
    QImage a2rgb30 = argb32.convertToFormat(QImage::Format_A2RGB30_Premultiplied);
    QImage rgb666 = rgb32.convertToFormat(QImage::Format_RGB666);
    QImage argb4444 = argb32.convertToFormat(QImage::Format_ARGB4444_Premultiplied);
    QImage rgba64pm = argb32.convertToFormat(QImage::Format_RGBA64_Premultiplied);
    QImage rgb888 = rgb32.convertToFormat(QImage::Format_RGB888);
    QImage bgr888 = rgb32.convertToFormat(QImage::Format_BGR888);

    QTest::newRow("indexed8 -> rgb32") << i8 << QImage::Format_RGB32;
    QTest::newRow("indexed8 -> argb32") << i8 << QImage::Format_ARGB32;
    QTest::newRow("indexed8 -> argb32pm") << i8 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("indexed8 -> rgbx8888") << i8 << QImage::Format_RGBX8888;
    QTest::newRow("indexed8 -> rgb16") << i8 << QImage::Format_RGB16;

    QTest::newRow("rgba8888 -> rgb32") << rgba32 << QImage::Format_RGB32;
    QTest::newRow("rgba8888 -> argb32") << rgba32 << QImage::Format_ARGB32;
    QTest::newRow("rgba8888 -> argb32pm") << rgba32 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("rgba8888 -> rgbx8888") << rgba32 << QImage::Format_RGBX8888;
    QTest::newRow("rgba8888 -> rgba8888pm") << rgba32 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("rgba8888 -> rgb888") << rgba32 << QImage::Format_RGB888;
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

    QTest::newRow("rgb666 -> rgb32") << rgb666 << QImage::Format_RGB32;
    QTest::newRow("rgb666 -> argb32") << rgb666 << QImage::Format_ARGB32;
    QTest::newRow("rgb666 -> argb32pm") << rgb666 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("rgb666 -> rgb888") << rgb666 << QImage::Format_RGB888;
    QTest::newRow("rgb666 -> rgb16") << rgb666 << QImage::Format_RGB16;
    QTest::newRow("rgb666 -> rgb555") << rgb666 << QImage::Format_RGB555;
    QTest::newRow("rgb666 -> rgb30") << rgb666 << QImage::Format_RGB30;

    QTest::newRow("argb4444pm -> rgb32") << argb4444 << QImage::Format_RGB32;
    QTest::newRow("argb4444pm -> argb32") << argb4444 << QImage::Format_ARGB32;
    QTest::newRow("argb4444pm -> argb32pm") << argb4444 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("argb4444pm -> rgbx8888") << argb4444 << QImage::Format_RGBX8888;
    QTest::newRow("argb4444pm -> rgba8888pm") << argb4444 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("argb4444pm -> rgb30") << argb4444 << QImage::Format_RGB30;
    QTest::newRow("argb4444pm -> a2bgr30") << argb4444 << QImage::Format_A2BGR30_Premultiplied;

    QTest::newRow("rgba64pm -> argb32") << rgba64pm << QImage::Format_ARGB32;
    QTest::newRow("rgba64pm -> rgbx8888") << rgba64pm << QImage::Format_RGBX8888;
    QTest::newRow("rgba64pm -> rgba8888pm") << rgba64pm << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("rgba64pm -> rgb30") << rgba64pm << QImage::Format_RGB30;
    QTest::newRow("rgba64pm -> a2bgr30") << rgba64pm << QImage::Format_A2BGR30_Premultiplied;
    QTest::newRow("rgba64pm -> rgba64") << rgba64pm << QImage::Format_RGBA64;

    QTest::newRow("rgb888 -> rgb32") << rgb888 << QImage::Format_RGB32;
    QTest::newRow("rgb888 -> argb32") << rgb888 << QImage::Format_ARGB32;
    QTest::newRow("rgb888 -> argb32pm") << rgb888 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("rgb888 -> rgbx8888") << rgb888 << QImage::Format_RGBX8888;
    QTest::newRow("rgb888 -> rgba8888pm") << rgb888 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("rgb888 -> bgr888") << rgb888 << QImage::Format_BGR888;

    QTest::newRow("bgr888 -> rgb32") << bgr888 << QImage::Format_RGB32;
    QTest::newRow("bgr888 -> argb32") << bgr888 << QImage::Format_ARGB32;
    QTest::newRow("bgr888 -> argb32pm") << bgr888 << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("bgr888 -> rgbx8888") << bgr888 << QImage::Format_RGBX8888;
    QTest::newRow("bgr888 -> rgba8888pm") << bgr888 << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("bgr888 -> rgb888") << bgr888 << QImage::Format_RGB888;
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
    QImage argb6666 = argb32.convertToFormat(QImage::Format_ARGB6666_Premultiplied);
    QImage argb4444 = argb32.convertToFormat(QImage::Format_ARGB4444_Premultiplied);
    QImage rgb16 = argb32.convertToFormat(QImage::Format_RGB16);
    QImage rgb30 = argb32.convertToFormat(QImage::Format_RGB30);
    QImage rgb888 = argb32.convertToFormat(QImage::Format_RGB888);

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

    QTest::newRow("argb6666pm -> argb8565pm -> argb6666pm") << argb6666 << QImage::Format_ARGB8565_Premultiplied;
    QTest::newRow("argb6666pm -> rgb888 -> argb6666pm") << argb6666 << QImage::Format_RGB888;

    QTest::newRow("argb4444pm -> rgb16 -> argb4444pm") << argb4444 << QImage::Format_RGB16;
    QTest::newRow("argb4444pm -> rgb444 -> argb4444pm") << argb4444 << QImage::Format_RGB444;

    QTest::newRow("rgb16 -> rgb555 -> rgb16") << rgb16 << QImage::Format_RGB555;
    QTest::newRow("rgb16 -> rgb444 -> rgb16") << rgb16 << QImage::Format_RGB444;
    QTest::newRow("rgb16 -> argb4444pm -> rgb16") << rgb16 << QImage::Format_ARGB4444_Premultiplied;

    QTest::newRow("rgb30 -> bgr30 -> rgb30") << rgb30 << QImage::Format_BGR30;
    QTest::newRow("rgb888 -> bgr888 -> rgb888") << rgb888 << QImage::Format_BGR888;
}

void tst_QImageConversion::convertGenericInplace()
{
    QFETCH(QImage, inputImage);
    QFETCH(QImage::Format, outputFormat);

    QImage::Format inputFormat = inputImage.format();
    QImage tmpImage = std::move(inputImage);

    QBENCHMARK {
        tmpImage = (std::move(tmpImage).convertToFormat(outputFormat)).convertToFormat(inputFormat);
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
