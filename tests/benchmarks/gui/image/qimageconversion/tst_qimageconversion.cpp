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


class tst_QImageConversion : public QObject
{
    Q_OBJECT
private slots:
    void convertRgb888ToRGB32_data();
    void convertRgb888ToRGB32();

private:
    QImage generateImageRgb888(int width, int height);
};

void tst_QImageConversion::convertRgb888ToRGB32_data()
{
    QTest::addColumn<QImage>("inputImage");
    // height = 5000 to get interesting timing.

    // 3 pixels wide -> smaller than regular vector of 128bits
    QTest::newRow("width: 3px; height: 5000px;") << generateImageRgb888(3, 5000);

    // 8 pixels wide -> potential for 2 vectors
    QTest::newRow("width: 8px; height: 5000px;") << generateImageRgb888(3, 5000);

    // 16 pixels, minimum for the SSSE3 implementation
    QTest::newRow("width: 16px; height: 5000px;") << generateImageRgb888(16, 5000);

    // 50 pixels, more realistic use case
    QTest::newRow("width: 50px; height: 5000px;") << generateImageRgb888(50, 5000);

    // 2000 pixels -> typical values for pictures
    QTest::newRow("width: 2000px; height: 5000px;") << generateImageRgb888(2000, 5000);
}

void tst_QImageConversion::convertRgb888ToRGB32()
{
    QFETCH(QImage, inputImage);

    QBENCHMARK {
        volatile QImage output = inputImage.convertToFormat(QImage::Format_RGB32);
        // we need the volatile and the following to make sure the compiler does not do
        // anything stupid :)
        (void)output;
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

QTEST_MAIN(tst_QImageConversion)
#include "tst_qimageconversion.moc"
