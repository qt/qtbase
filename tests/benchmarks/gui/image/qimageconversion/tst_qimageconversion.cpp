/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <qtest.h>
#include <QImage>

Q_DECLARE_METATYPE(QImage::Format)

class tst_QImageConversion : public QObject
{
    Q_OBJECT
private slots:
    void convertRgb888ToRgb32_data();
    void convertRgb888ToRgb32();

    void convertRgb32ToRgb888_data();
    void convertRgb32ToRgb888();

    void convertGeneric_data();
    void convertGeneric();

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

    // 50 pixels, more realistic use case
    QTest::newRow("width: 50px; height: 5000px;") << generateImageRgb888(50, 5000);

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


void tst_QImageConversion::convertGeneric_data()
{
    QTest::addColumn<QImage>("inputImage");
    QTest::addColumn<QImage::Format>("outputFormat");
    QImage rgb16 = generateImageRgb16(1000, 1000);
    QImage rgb32 = generateImageRgb32(1000, 1000);
    QImage argb32 = generateImageArgb32(1000, 1000);

    QTest::newRow("rgb16 -> rgb32") << rgb16 << QImage::Format_RGB32;
    QTest::newRow("rgb16 -> rgb888") << rgb16 << QImage::Format_RGB888;
    QTest::newRow("rgb16 -> rgb666") << rgb16 << QImage::Format_RGB666;
    QTest::newRow("rgb16 -> rgb555") << rgb16 << QImage::Format_RGB555;

    QTest::newRow("rgb32 -> rgb16") << rgb32 << QImage::Format_RGB16;
    QTest::newRow("rgb32 -> rgb888") << rgb32 << QImage::Format_RGB888;
    QTest::newRow("rgb32 -> rgb666") << rgb32 << QImage::Format_RGB666;
    QTest::newRow("rgb32 -> rgb555") << rgb32 << QImage::Format_RGB555;

    QTest::newRow("argb32 -> rgba8888") << argb32 << QImage::Format_RGBA8888;
    QTest::newRow("argb32 -> rgb888") << argb32 << QImage::Format_RGB888;
    QTest::newRow("argb32 -> rgb666") << argb32 << QImage::Format_RGB666;
    QTest::newRow("argb32 -> argb8565pm") << argb32 << QImage::Format_ARGB8565_Premultiplied;
    QTest::newRow("argb32 -> argb4444pm") << argb32 << QImage::Format_ARGB4444_Premultiplied;
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
    const int byteWidth = width * 4;

    for (int y = 0; y < image.height(); ++y) {
        uchar *scanline = image.scanLine(y);
        for (int x = 0; x < byteWidth; ++x)
            scanline[x] = x ^ y;
    }
    return image;
}

QTEST_MAIN(tst_QImageConversion)
#include "tst_qimageconversion.moc"
