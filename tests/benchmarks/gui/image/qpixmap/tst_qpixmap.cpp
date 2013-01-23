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
#include <QBitmap>
#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPixmap>
#include <private/qpixmap_raster_p.h>

class tst_QPixmap : public QObject
{
    Q_OBJECT

public:
    tst_QPixmap();

private slots:
    void fill_data();
    void fill();

    void scaled_data();
    void scaled();
    void transformed_data();
    void transformed();
    void mask_data();
    void mask();

    void fromImageReader_data();
    void fromImageReader();
};

Q_DECLARE_METATYPE(QImage::Format)
Q_DECLARE_METATYPE(Qt::AspectRatioMode)
Q_DECLARE_METATYPE(Qt::TransformationMode)

QPixmap rasterPixmap(int width, int height)
{
    QPlatformPixmap *data =
        new QRasterPlatformPixmap(QPlatformPixmap::PixmapType);

    data->resize(width, height);

    return QPixmap(data);
}

QPixmap rasterPixmap(const QSize &size)
{
    return rasterPixmap(size.width(), size.height());
}

QPixmap rasterPixmap(const QImage &image)
{
    QPlatformPixmap *data =
        new QRasterPlatformPixmap(QPlatformPixmap::PixmapType);

    data->fromImage(image, Qt::AutoColor);

    return QPixmap(data);
}

tst_QPixmap::tst_QPixmap()
{
}

void tst_QPixmap::fill_data()
{
    QTest::addColumn<bool>("opaque");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");

    QTest::newRow("opaque 16x16") << true << 16 << 16;
    QTest::newRow("!opaque 16x16") << false << 16 << 16;
    QTest::newRow("opaque 587x128") << true << 587 << 128;
    QTest::newRow("!opaque 587x128") << false << 587 << 128;
}

void tst_QPixmap::fill()
{
    QFETCH(bool, opaque);
    QFETCH(int, width);
    QFETCH(int, height);

    const QColor color = opaque ? QColor(255, 0, 0) : QColor(255, 0, 0, 200);
    QPixmap pixmap = rasterPixmap(width, height);

    QBENCHMARK {
        pixmap.fill(color);
    }
}

void tst_QPixmap::scaled_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QSize>("scale");
    QTest::addColumn<Qt::AspectRatioMode>("ratioMode");
    QTest::addColumn<Qt::TransformationMode>("transformMode");

    QTest::newRow("16x16 => 32x32") << QSize(16, 16) << QSize(32, 32)
                                    << Qt::IgnoreAspectRatio
                                    << Qt::FastTransformation;
    QTest::newRow("100x100 => 200x200") << QSize(100, 100) << QSize(200, 200)
                                        << Qt::IgnoreAspectRatio
                                        << Qt::FastTransformation;
    QTest::newRow("100x100 => 200x200") << QSize(100, 100) << QSize(200, 200)
                                        << Qt::IgnoreAspectRatio
                                        << Qt::FastTransformation;
    QTest::newRow("80x80 => 200x200") << QSize(137, 137) << QSize(200, 200)
                                      << Qt::IgnoreAspectRatio
                                      << Qt::FastTransformation;

}

void tst_QPixmap::scaled()
{
    QFETCH(QSize, size);
    QFETCH(QSize, scale);
    QFETCH(Qt::AspectRatioMode, ratioMode);
    QFETCH(Qt::TransformationMode, transformMode);

    QPixmap opaque = rasterPixmap(size);
    QPixmap transparent = rasterPixmap(size);
    opaque.fill(QColor(255, 0, 0));
    transparent.fill(QColor(255, 0, 0, 200));

    QPixmap scaled1;
    QPixmap scaled2;
    QBENCHMARK {
        scaled1 = opaque.scaled(scale, ratioMode, transformMode);
        scaled2 = transparent.scaled(scale, ratioMode, transformMode);
    }
}

void tst_QPixmap::transformed_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<Qt::TransformationMode>("transformMode");

    QTest::newRow("16x16 rotate(90)") << QSize(16, 16)
                                      << QTransform().rotate(90)
                                      << Qt::FastTransformation;
    QTest::newRow("16x16 rotate(199)") << QSize(16, 16)
                                       << QTransform().rotate(199)
                                       << Qt::FastTransformation;
    QTest::newRow("16x16 shear(2,1)") << QSize(16, 16)
                                      << QTransform().shear(2, 1)
                                      << Qt::FastTransformation;
    QTest::newRow("16x16 rotate(199).shear(2,1)") << QSize(16, 16)
                                                  << QTransform().rotate(199).shear(2, 1)
                                                  << Qt::FastTransformation;
    QTest::newRow("100x100 rotate(90)") << QSize(100, 100)
                                        << QTransform().rotate(90)
                                        << Qt::FastTransformation;
    QTest::newRow("100x100 rotate(199)") << QSize(100, 100)
                                         << QTransform().rotate(199)
                                         << Qt::FastTransformation;
    QTest::newRow("100x100 shear(2,1)") << QSize(100, 100)
                                        << QTransform().shear(2, 1)
                                        << Qt::FastTransformation;
    QTest::newRow("100x100 shear(2,1) smooth") << QSize(100, 100)
                                               << QTransform().shear(2, 1)
                                               << Qt::SmoothTransformation;
    QTest::newRow("100x100 rotate(199).shear(2,1)") << QSize(100, 100)
                                                    << QTransform().rotate(199).shear(2, 1)
                                                    << Qt::FastTransformation;
}

void tst_QPixmap::transformed()
{
    QFETCH(QSize, size);
    QFETCH(QTransform, transform);
    QFETCH(Qt::TransformationMode, transformMode);

    QPixmap opaque = rasterPixmap(size);
    QPixmap transparent = rasterPixmap(size);
    opaque.fill(QColor(255, 0, 0));
    transparent.fill(QColor(255, 0, 0, 200));

    QPixmap transformed1;
    QPixmap transformed2;
    QBENCHMARK {
        transformed1 = opaque.transformed(transform, transformMode);
        transformed2 = transparent.transformed(transform, transformMode);
    }
}

void tst_QPixmap::mask_data()
{
    QTest::addColumn<QSize>("size");

    QTest::newRow("1x1") << QSize(1, 1);
    QTest::newRow("9x9") << QSize(9, 9);
    QTest::newRow("16x16") << QSize(16, 16);
    QTest::newRow("128x128") << QSize(128, 128);
    QTest::newRow("333x333") << QSize(333, 333);
    QTest::newRow("2048x128") << QSize(2048, 128);
}

void tst_QPixmap::mask()
{
    QFETCH(QSize, size);

    QPixmap src = rasterPixmap(size);
    src.fill(Qt::transparent);
    {
        QPainter p(&src);
        p.drawLine(QPoint(0, 0), QPoint(src.width(), src.height()));
    }

    QBENCHMARK {
        QBitmap bitmap = src.mask();
        QVERIFY(bitmap.size() == src.size());
    }
}

void tst_QPixmap::fromImageReader_data()
{
    const QString tempDir = QDir::tempPath();
    QTest::addColumn<QString>("filename");

    QImage image(2000, 2000, QImage::Format_ARGB32);
    image.fill(0);
    {
        // Generate an image with opaque and transparent pixels
        // with an interesting distribution.
        QPainter painter(&image);

        QRadialGradient radialGrad(QPointF(1000, 1000), 1000);
        radialGrad.setColorAt(0, QColor(255, 0, 0, 255));
        radialGrad.setColorAt(0.5, QColor(0, 255, 0, 255));
        radialGrad.setColorAt(0.9, QColor(0, 0, 255, 100));
        radialGrad.setColorAt(1, QColor(0, 0, 0, 0));

        painter.fillRect(image.rect(), radialGrad);
    }
    image.save("test.png");

    // RGB32
    const QString rgb32Path = tempDir + QString::fromLatin1("/rgb32.jpg");
    image.save(rgb32Path);
    QTest::newRow("gradient RGB32") << rgb32Path;

    // ARGB32
    const QString argb32Path = tempDir + QString::fromLatin1("/argb32.png");
    image.save(argb32Path);
    QTest::newRow("gradient ARGB32") << argb32Path;

    // Indexed 8
    const QString indexed8Path = tempDir + QString::fromLatin1("/indexed8.gif");
    image.save(indexed8Path);
    QTest::newRow("gradient indexed8") << indexed8Path;

}

void tst_QPixmap::fromImageReader()
{
    QFETCH(QString, filename);
    // warmup
    {
        QImageReader imageReader(filename);
        QPixmap::fromImageReader(&imageReader);
    }

    QBENCHMARK {
        QImageReader imageReader(filename);
        QPixmap::fromImageReader(&imageReader);
    }
    QFile::remove(filename);
}


QTEST_MAIN(tst_QPixmap)

#include "tst_qpixmap.moc"
