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
#include <QtGui>
#include <QString>

#include <qtest.h>

void paint(QPaintDevice *device)
{
    QPainter p(device);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(0, 0, device->width(), device->height(), Qt::transparent);

    QLinearGradient g(QPoint(0, 0), QPoint(1, 1));
//    g.setCoordinateMode(QGradient::ObjectBoundingMode);
    g.setColorAt(0, Qt::magenta);
    g.setColorAt(0, Qt::white);

//    p.setOpacity(0.8);
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(0.9);
    p.drawEllipse(0, 0, device->width(), device->height());
}

QLatin1String compositionModes[] = {
    QLatin1String("SourceOver"),
    QLatin1String("DestinationOver"),
    QLatin1String("Clear"),
    QLatin1String("Source"),
    QLatin1String("Destination"),
    QLatin1String("SourceIn"),
    QLatin1String("DestinationIn"),
    QLatin1String("SourceOut"),
    QLatin1String("DestinationOut"),
    QLatin1String("SourceAtop"),
    QLatin1String("DestinationAtop"),
    QLatin1String("Xor"),

    //svg 1.2 blend modes
    QLatin1String("Plus"),
    QLatin1String("Multiply"),
    QLatin1String("Screen"),
    QLatin1String("Overlay"),
    QLatin1String("Darken"),
    QLatin1String("Lighten"),
    QLatin1String("ColorDodge"),
    QLatin1String("ColorBurn"),
    QLatin1String("HardLight"),
    QLatin1String("SoftLight"),
    QLatin1String("Difference"),
    QLatin1String("Exclusion")
};

enum BrushType { ImageBrush, SolidBrush };
QLatin1String brushTypes[] = {
    QLatin1String("ImageBrush"),
    QLatin1String("SolidBrush"),
};

class BlendBench : public QObject
{
    Q_OBJECT
private slots:
    void blendBench_data();
    void blendBench();

    void blendBenchAlpha_data();
    void blendBenchAlpha();

    void unalignedBlendArgb32_data();
    void unalignedBlendArgb32();
};

void BlendBench::blendBench_data()
{
    int first = 0;
    int limit = 12;
    if (qApp->arguments().contains("--extended")) {
        first = 12;
        limit = 24;
    }

    QTest::addColumn<int>("brushType");
    QTest::addColumn<int>("compositionMode");

    for (int brush = ImageBrush; brush <= SolidBrush; ++brush)
        for (int mode = first; mode < limit; ++mode)
            QTest::newRow(QString("brush=%1; mode=%2")
                          .arg(brushTypes[brush]).arg(compositionModes[mode]).toLatin1().data())
                << brush << mode;
}

void BlendBench::blendBench()
{
    QFETCH(int, brushType);
    QFETCH(int, compositionMode);

    QImage img(512, 512, QImage::Format_ARGB32_Premultiplied);
    QImage src(512, 512, QImage::Format_ARGB32_Premultiplied);
    paint(&src);
    QPainter p(&img);
    p.setPen(Qt::NoPen);

    p.setCompositionMode(QPainter::CompositionMode(compositionMode));
    if (brushType == ImageBrush) {
        p.setBrush(QBrush(src));
    } else if (brushType == SolidBrush) {
        p.setBrush(QColor(127, 127, 127, 127));
    }

    QBENCHMARK {
        p.drawRect(0, 0, 512, 512);
    }
}

void BlendBench::blendBenchAlpha_data()
{
    blendBench_data();
}

void BlendBench::blendBenchAlpha()
{
    QFETCH(int, brushType);
    QFETCH(int, compositionMode);

    QImage img(512, 512, QImage::Format_ARGB32_Premultiplied);
    QImage src(512, 512, QImage::Format_ARGB32_Premultiplied);
    paint(&src);
    QPainter p(&img);
    p.setPen(Qt::NoPen);

    p.setCompositionMode(QPainter::CompositionMode(compositionMode));
    if (brushType == ImageBrush) {
        p.setBrush(QBrush(src));
    } else if (brushType == SolidBrush) {
        p.setBrush(QColor(127, 127, 127, 127));
    }
    p.setOpacity(0.7f);

    QBENCHMARK {
        p.drawRect(0, 0, 512, 512);
    }
}

void BlendBench::unalignedBlendArgb32_data()
{
    // The performance of blending can depend of the alignment of the data
    // on 16 bytes. Some SIMD instruction set have significantly better
    // memory access when the memory is aligned on 16 bytes boundary.

    // offset in 32 bits words
    QTest::addColumn<int>("offset");
    QTest::newRow("aligned on 16 bytes") << 0;
    QTest::newRow("unaligned by 4 bytes") << 1;
    QTest::newRow("unaligned by 8 bytes") << 2;
    QTest::newRow("unaligned by 12 bytes") << 3;
}

void BlendBench::unalignedBlendArgb32()
{
    const int dimension = 1024;

    // We use dst aligned by design. We don't want to test all the combination of alignemnt for src and dst.
    // Moreover, it make sense for us to align dst in the implementation because it is accessed more often.
    uchar *dstMemory = static_cast<uchar*>(qMallocAligned((dimension * dimension * sizeof(quint32)), 16));
    QImage destination(dstMemory, dimension, dimension, QImage::Format_ARGB32_Premultiplied);
    destination.fill(0x12345678); // avoid special cases of alpha

    uchar *srcMemory = static_cast<uchar*>(qMallocAligned((dimension * dimension * sizeof(quint32)) + 16, 16));
    QFETCH(int, offset);
    uchar *imageSrcMemory = srcMemory + (offset * sizeof(quint32));

    QImage src(imageSrcMemory, dimension, dimension, QImage::Format_ARGB32_Premultiplied);
    src.fill(0x87654321);

    QPainter painter(&destination);
    QBENCHMARK {
        painter.drawImage(QPoint(), src);
    }

    qFreeAligned(srcMemory);
    qFreeAligned(dstMemory);
}

QTEST_MAIN(BlendBench)

#include "main.moc"
