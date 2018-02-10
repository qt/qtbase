/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include <QPainter>
#include <QImage>

Q_DECLARE_METATYPE(QImage::Format)

#define SIZE 400

class tst_DrawTexture : public QObject
{
    Q_OBJECT

    void paint(bool smooth);

private slots:
    void simpleUpscale_data();
    void simpleUpscale();
    void simpleUpscaleSmooth_data();
    void simpleUpscaleSmooth();

    void downscale_data();
    void downscale();
    void downscaleSmooth_data();
    void downscaleSmooth();

    void upscale_data();
    void upscale();
    void upscaleSmooth_data();
    void upscaleSmooth();

    void rotate_data();
    void rotate();
    void rotateSmooth_data();
    void rotateSmooth();

    void perspective_data();
    void perspective();
    void perspectiveSmooth_data();
    void perspectiveSmooth();
};

void tst_DrawTexture::simpleUpscale_data()
{
    QTest::addColumn<QImage::Format>("sourceFormat");
    QTest::addColumn<QImage::Format>("targetFormat");
    QTest::addColumn<QTransform>("transform");


    QTransform matrix;
    matrix.scale(1.5, 1.5);
    QTest::newRow("rgb32 1.5x,1.5x on rgb32") << QImage::Format_RGB32
                                              << QImage::Format_RGB32
                                              << matrix;
    QTest::newRow("argb32pm 1.5x,1.5x on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                    << QImage::Format_ARGB32_Premultiplied
                                                    << matrix;
    QTest::newRow("argb32 1.5x,1.5x on argb32pm") << QImage::Format_ARGB32
                                                  << QImage::Format_ARGB32_Premultiplied
                                                  << matrix;
    QTest::newRow("rgba8888pm 1.5x,1.5x on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                        << QImage::Format_RGBA8888_Premultiplied
                                                        << matrix;
    QTest::newRow("rgb16 1.5x,1.5x on rgb16") << QImage::Format_RGB16
                                              << QImage::Format_RGB16
                                              << matrix;
    QTest::newRow("rgb16 1.5x,1.5x on argb32pm") << QImage::Format_RGB16
                                                 << QImage::Format_ARGB32_Premultiplied
                                                 << matrix;
    QTest::newRow("argb32pm 1.5x,1.5x on rgb30") << QImage::Format_ARGB32_Premultiplied
                                                 << QImage::Format_RGB30
                                                 << matrix;
    matrix.reset();
    matrix.scale(5, 5);
    QTest::newRow("rgb32 5x,5x on rgb32") << QImage::Format_RGB32
                                          << QImage::Format_RGB32
                                          << matrix;
    QTest::newRow("argb32pm 5x,5x on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                << QImage::Format_ARGB32_Premultiplied
                                                << matrix;
    QTest::newRow("argb32 5x,5x on argb32pm") << QImage::Format_ARGB32
                                              << QImage::Format_ARGB32_Premultiplied
                                              << matrix;
    QTest::newRow("rgba8888pm 5x,5x on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                    << QImage::Format_RGBA8888_Premultiplied
                                                    << matrix;
    QTest::newRow("rgb16 5x,5x on rgb16") << QImage::Format_RGB16
                                          << QImage::Format_RGB16
                                          << matrix;
    QTest::newRow("rgb16 5x,5x on rgb32") << QImage::Format_RGB16
                                          << QImage::Format_RGB32
                                          << matrix;
    QTest::newRow("argb32pm 5x,5x on rgb30") << QImage::Format_ARGB32_Premultiplied
                                             << QImage::Format_RGB30
                                             << matrix;
    matrix.reset();
    matrix.translate(0, SIZE);
    matrix.scale(16, -1);
    QTest::newRow("rgb32 16x,-1x on rgb32") << QImage::Format_RGB32
                                            << QImage::Format_RGB32
                                            << matrix;
    QTest::newRow("argb32pm 16x,-1x on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                  << QImage::Format_ARGB32_Premultiplied
                                                  << matrix;
    QTest::newRow("argb32 16x,-1x on argb32pm") << QImage::Format_ARGB32
                                                << QImage::Format_ARGB32_Premultiplied
                                                << matrix;
    QTest::newRow("rgba8888pm 16x,-1x on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                      << QImage::Format_RGBA8888_Premultiplied
                                                      << matrix;
    QTest::newRow("rgb16 16x,-1x on rgb16") << QImage::Format_RGB16
                                            << QImage::Format_RGB16
                                            << matrix;
    QTest::newRow("argb32pm 16x,-1x on rgb30") << QImage::Format_ARGB32_Premultiplied
                                               << QImage::Format_RGB30
                                                << matrix;
}

void tst_DrawTexture::downscale_data()
{
    QTest::addColumn<QImage::Format>("sourceFormat");
    QTest::addColumn<QImage::Format>("targetFormat");
    QTest::addColumn<QTransform>("transform");


    QTransform matrix;
    matrix.translate(SIZE, 0);
    matrix.scale(-1.5, 1.5);
    QTest::newRow("rgb32 -1.5x,1.5x on rgb32") << QImage::Format_RGB32
                                               << QImage::Format_RGB32
                                               << matrix;
    QTest::newRow("argb32pm -1.5x,1.5x on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                     << QImage::Format_ARGB32_Premultiplied
                                                     << matrix;
    QTest::newRow("argb32 -1.5x,1.5x on argb32pm") << QImage::Format_ARGB32
                                                   << QImage::Format_ARGB32_Premultiplied
                                                   << matrix;
    QTest::newRow("rgba8888pm -1.5x,1.5x on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                         << QImage::Format_RGBA8888_Premultiplied
                                                         << matrix;
    QTest::newRow("rgb16 -1.5x,1.5x on rgb16") << QImage::Format_RGB16
                                               << QImage::Format_RGB16
                                               << matrix;
    QTest::newRow("argb32pm -1.5x,1.5x on rgb30") << QImage::Format_ARGB32_Premultiplied
                                                  << QImage::Format_RGB30
                                                  << matrix;
    matrix.reset();
    matrix.scale(.5, .5);
    QTest::newRow("rgb32 .5x,.5x on rgb32") << QImage::Format_RGB32
                                            << QImage::Format_RGB32
                                            << matrix;
    QTest::newRow("argb32pm .5x,.5x on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                  << QImage::Format_ARGB32_Premultiplied
                                                  << matrix;
    QTest::newRow("argb32 .5x,.5x on argb32pm") << QImage::Format_ARGB32
                                                << QImage::Format_ARGB32_Premultiplied
                                                << matrix;
    QTest::newRow("rgba8888pm .5x,.5x on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                      << QImage::Format_RGBA8888_Premultiplied
                                                      << matrix;
    QTest::newRow("rgb16 .5x,.5x on rgb16") << QImage::Format_RGB16
                                            << QImage::Format_RGB16
                                            << matrix;
    QTest::newRow("argb32pm .5x,.5x on rgb30") << QImage::Format_ARGB32_Premultiplied
                                               << QImage::Format_RGB30
                                               << matrix;
    matrix.reset();
    matrix.scale(.2, 2);
    QTest::newRow("rgb32 .2x,2x on rgb32") << QImage::Format_RGB32
                                           << QImage::Format_RGB32
                                           << matrix;
    QTest::newRow("argb32pm .2x,2x on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                 << QImage::Format_ARGB32_Premultiplied
                                                 << matrix;
    QTest::newRow("argb32 .2x,2x on argb32pm") << QImage::Format_ARGB32
                                               << QImage::Format_ARGB32_Premultiplied
                                               << matrix;
    QTest::newRow("rgba8888pm .2x,2x on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                     << QImage::Format_RGBA8888_Premultiplied
                                                     << matrix;
    QTest::newRow("rgb16 .2x,2x on rgb16") << QImage::Format_RGB16
                                           << QImage::Format_RGB16
                                           << matrix;
    QTest::newRow("argb32pm .2x,2x on rgb30") << QImage::Format_ARGB32_Premultiplied
                                              << QImage::Format_RGB30
                                              << matrix;
}

void tst_DrawTexture::upscale_data()
{
    QTest::addColumn<QImage::Format>("sourceFormat");
    QTest::addColumn<QImage::Format>("targetFormat");
    QTest::addColumn<QTransform>("transform");


    QTransform matrix;
    matrix.translate(SIZE, 0);
    matrix.scale(-8, 8);
    QTest::newRow("rgb32 -8x,8x on rgb32") << QImage::Format_RGB32
                                           << QImage::Format_RGB32
                                           << matrix;
    QTest::newRow("argb32pm -8x,8x on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                 << QImage::Format_ARGB32_Premultiplied
                                                 << matrix;
    QTest::newRow("argb32 -8x,8x on argb32pm") << QImage::Format_ARGB32
                                               << QImage::Format_ARGB32_Premultiplied
                                               << matrix;
    QTest::newRow("rgba8888pm -8x,8x on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                     << QImage::Format_RGBA8888_Premultiplied
                                                     << matrix;
    QTest::newRow("rgb16 -8x,8x on rgb16") << QImage::Format_RGB16
                                           << QImage::Format_RGB16
                                           << matrix;
    QTest::newRow("argb32pm -8x,8x on rgb30") << QImage::Format_ARGB32_Premultiplied
                                              << QImage::Format_RGB30
                                              << matrix;
    matrix.reset();
    matrix.translate(SIZE, SIZE);
    matrix.scale(-10, -10);
    QTest::newRow("rgb32 -10x,-10x on rgb32") << QImage::Format_RGB32
                                              << QImage::Format_RGB32
                                              << matrix;
    QTest::newRow("argb32pm -10x,-10x on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                    << QImage::Format_ARGB32_Premultiplied
                                                    << matrix;
    QTest::newRow("argb32 -10x,-10x on argb32pm") << QImage::Format_ARGB32
                                                  << QImage::Format_ARGB32_Premultiplied
                                                  << matrix;
    QTest::newRow("rgba8888pm -10x,-10x on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                        << QImage::Format_RGBA8888_Premultiplied
                                                        << matrix;
    QTest::newRow("rgb16 -10x,-10x on rgb16") << QImage::Format_RGB16
                                              << QImage::Format_RGB16
                                              << matrix;
    QTest::newRow("argb32pm -10x,-10x on rgb30") << QImage::Format_ARGB32_Premultiplied
                                              << QImage::Format_RGB30
                                              << matrix;

    matrix.reset();
    matrix.translate(SIZE, 0);
    matrix.scale(-1, 16);
    QTest::newRow("rgb32 -1x,16x on rgb32") << QImage::Format_RGB32
                                            << QImage::Format_RGB32
                                            << matrix;
    QTest::newRow("argb32pm -1x,16x on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                  << QImage::Format_ARGB32_Premultiplied
                                                  << matrix;
    QTest::newRow("argb32 -1x,16x on argb32pm") << QImage::Format_ARGB32
                                                << QImage::Format_ARGB32_Premultiplied
                                                << matrix;
    QTest::newRow("rgba8888pm -1x,16x on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                      << QImage::Format_RGBA8888_Premultiplied
                                                      << matrix;
    QTest::newRow("rgb16 -1x,16x on rgb16") << QImage::Format_RGB16
                                            << QImage::Format_RGB16
                                            << matrix;
    QTest::newRow("argb32pm -1x,16x on rgb30") << QImage::Format_ARGB32_Premultiplied
                                               << QImage::Format_RGB30
                                               << matrix;
}

void tst_DrawTexture::rotate_data()
{
    QTest::addColumn<QImage::Format>("sourceFormat");
    QTest::addColumn<QImage::Format>("targetFormat");
    QTest::addColumn<QTransform>("transform");

    QTransform matrix;
    matrix.translate(SIZE/2, SIZE/2);
    matrix.rotate(-90);
    matrix.translate(-SIZE/2, -SIZE/2);
    QTest::newRow("rgb32 -90deg on rgb32") << QImage::Format_RGB32
                                           << QImage::Format_RGB32
                                           << matrix;
    QTest::newRow("argb32pm -90deg on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                 << QImage::Format_ARGB32_Premultiplied
                                                 << matrix;
    QTest::newRow("argb32 -90deg on argb32pm") << QImage::Format_ARGB32
                                               << QImage::Format_ARGB32_Premultiplied
                                               << matrix;
    QTest::newRow("rgba8888pm -90deg on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                     << QImage::Format_RGBA8888_Premultiplied
                                                     << matrix;
    QTest::newRow("rgb16 -90deg rgb16") << QImage::Format_RGB16
                                        << QImage::Format_RGB16
                                        << matrix;
    QTest::newRow("argb32pm -90deg on rgb30") << QImage::Format_ARGB32_Premultiplied
                                              << QImage::Format_RGB30
                                              << matrix;
    matrix.reset();
    matrix.translate(SIZE/2, SIZE/2);
    matrix.rotate(45);
    matrix.translate(-SIZE/2, -SIZE/2);
    QTest::newRow("rgb32 45deg on rgb32") << QImage::Format_RGB32
                                          << QImage::Format_RGB32
                                          << matrix;
    QTest::newRow("argb32pm 45deg on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                << QImage::Format_ARGB32_Premultiplied
                                                << matrix;
    QTest::newRow("argb32 45deg on argb32pm") << QImage::Format_ARGB32
                                              << QImage::Format_ARGB32_Premultiplied
                                              << matrix;
    QTest::newRow("rgba8888pm 45deg on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                    << QImage::Format_RGBA8888_Premultiplied
                                                    << matrix;
    QTest::newRow("rgb16 45deg on rgb16") << QImage::Format_RGB16
                                          << QImage::Format_RGB16
                                          << matrix;
    QTest::newRow("argb32pm 45deg on rgb30") << QImage::Format_ARGB32_Premultiplied
                                             << QImage::Format_RGB30
                                             << matrix;

    matrix.reset();
    matrix.translate(SIZE/2, SIZE/2);
    matrix.rotate(135);
    matrix.scale(2, 2);
    matrix.translate(-SIZE/4, -SIZE/4);
    QTest::newRow("rgb32 rotate+scale on rgb32") << QImage::Format_RGB32
                                                 << QImage::Format_RGB32
                                                 << matrix;
    QTest::newRow("argb32pm rotate+scale on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                       << QImage::Format_ARGB32_Premultiplied
                                                       << matrix;
    QTest::newRow("argb32 rotate+scale on argb32pm") << QImage::Format_ARGB32
                                                     << QImage::Format_ARGB32_Premultiplied
                                                     << matrix;
    QTest::newRow("rgba8888pm rotate+scale on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                           << QImage::Format_RGBA8888_Premultiplied
                                                           << matrix;
    QTest::newRow("rgb16 rotate+scale on rgb16") << QImage::Format_RGB16
                                                 << QImage::Format_RGB16
                                                 << matrix;
    QTest::newRow("argb32pm rotate+scale on rgb30") << QImage::Format_ARGB32_Premultiplied
                                                    << QImage::Format_RGB30
                                                    << matrix;
}

void tst_DrawTexture::perspective_data()
{
    QTest::addColumn<QImage::Format>("sourceFormat");
    QTest::addColumn<QImage::Format>("targetFormat");
    QTest::addColumn<QTransform>("transform");

    QTransform matrix;
    QPolygonF quad1, quad2;
    quad1 << QPointF(0.0, 0.0) <<  QPointF(SIZE,0.0) << QPointF(SIZE,SIZE) << QPointF(0.0,SIZE);
    quad2 << QPointF(SIZE/6, SIZE/6) << QPointF(SIZE*4/5,SIZE/5) << QPointF(SIZE*4/5,SIZE*5/6) << QPointF(SIZE/5,SIZE*3/5);
    QTransform::quadToQuad(quad1, quad2, matrix);

    QTest::newRow("rgb32 perspective1 on rgb32") << QImage::Format_RGB32
                                                 << QImage::Format_RGB32
                                                 << matrix;
    QTest::newRow("argb32pm perspective1 on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                       << QImage::Format_ARGB32_Premultiplied
                                                       << matrix;
    QTest::newRow("argb32 perspective1 on argb32pm") << QImage::Format_ARGB32
                                                     << QImage::Format_ARGB32_Premultiplied
                                                     << matrix;
    QTest::newRow("rgba8888pm perspective1 on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                           << QImage::Format_RGBA8888_Premultiplied
                                                           << matrix;
    QTest::newRow("rgb16 perspective1 on rgb16") << QImage::Format_RGB16
                                                 << QImage::Format_RGB16
                                                 << matrix;
    QTest::newRow("argb32pm perspective1 on rgb30") << QImage::Format_ARGB32_Premultiplied
                                                    << QImage::Format_RGB30
                                                    << matrix;

    matrix.reset();
    quad1.clear(); quad2.clear();
    quad1 << QPointF(0.0, 0.0) <<  QPointF(SIZE,0.0) << QPointF(SIZE,SIZE) << QPointF(0.0,SIZE);
    quad2 << QPointF(0.0, 0.0) <<  QPointF(SIZE*4/5,SIZE/4) << QPointF(SIZE*4/5,SIZE*3/4) << QPointF(0.0,SIZE);
    QTransform::quadToQuad(quad1, quad2, matrix);

    QTest::newRow("rgb32 perspective2 on rgb32") << QImage::Format_RGB32
                                                 << QImage::Format_RGB32
                                                 << matrix;
    QTest::newRow("argb32pm perspective2 on argb32pm") << QImage::Format_ARGB32_Premultiplied
                                                       << QImage::Format_ARGB32_Premultiplied
                                                       << matrix;
    QTest::newRow("argb32 perspective2 on argb32pm") << QImage::Format_ARGB32
                                                     << QImage::Format_ARGB32_Premultiplied
                                                     << matrix;
    QTest::newRow("rgba8888pm perspective2 on rgba8888pm") << QImage::Format_RGBA8888_Premultiplied
                                                           << QImage::Format_RGBA8888_Premultiplied
                                                           << matrix;
    QTest::newRow("rgb16 perspective2 on rgb16") << QImage::Format_RGB16
                                                 << QImage::Format_RGB16
                                                 << matrix;
    QTest::newRow("argb32pm perspective2 on rgb30") << QImage::Format_ARGB32_Premultiplied
                                                    << QImage::Format_RGB30
                                                    << matrix;
}

void tst_DrawTexture::simpleUpscaleSmooth_data()
{
    simpleUpscale_data();
}

void tst_DrawTexture::downscaleSmooth_data()
{
    downscale_data();
}

void tst_DrawTexture::upscaleSmooth_data()
{
    upscale_data();
}

void tst_DrawTexture::rotateSmooth_data()
{
    rotate_data();
}

void tst_DrawTexture::perspectiveSmooth_data()
{
    perspective_data();
}

static QImage createImage(const QSize &size, QImage::Format format, bool smooth)
{
    QImage base(size, format);
    base.fill(Qt::transparent);
    QLinearGradient grad(0.0, 0.0, 1.0, 0.0);
    grad.setCoordinateMode(QGradient::ObjectBoundingMode);
    grad.setColorAt(0.0, Qt::red);
    grad.setColorAt(0.4, Qt::white);
    grad.setColorAt(0.6, Qt::white);
    grad.setColorAt(1.0, Qt::blue);
    QBrush brush(grad);
    QPainter p(&base);
    p.setRenderHint(QPainter::Antialiasing, smooth);
    p.setBrush(brush);
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, size.width(), size.height());
    p.end();
    return base;
}


void tst_DrawTexture::paint(bool smooth)
{
    QFETCH(QImage::Format, sourceFormat);
    QFETCH(QImage::Format, targetFormat);
    QFETCH(QTransform, transform);

    QSize size(SIZE, SIZE);
    QRect rect(QPoint(0,0), size);
    if (transform.isAffine())
        rect = transform.inverted().mapRect(rect);

    QImage sourceImage = createImage(rect.size(), sourceFormat, smooth);
    QImage targetImage(size, targetFormat);
    targetImage.fill(Qt::gray);

    QPainter p(&targetImage);
    p.setRenderHints(QPainter::SmoothPixmapTransform, smooth);
    p.setRenderHints(QPainter::Antialiasing, smooth);
    p.setWorldTransform(transform, false);

    QBENCHMARK {
        p.drawImage(0, 0, sourceImage);
    }
//    targetImage.save(QString::fromLatin1(QTest::currentTestFunction()) + QChar('_') + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".png"));
}

void tst_DrawTexture::simpleUpscale()
{
    paint(false);
}

void tst_DrawTexture::upscale()
{
    paint(false);
}

void tst_DrawTexture::downscale()
{
    paint(false);
}

void tst_DrawTexture::rotate()
{
    paint(false);
}

void tst_DrawTexture::perspective()
{
    paint(false);
}

void tst_DrawTexture::simpleUpscaleSmooth()
{
    paint(true);
}

void tst_DrawTexture::upscaleSmooth()
{
    paint(true);
}

void tst_DrawTexture::downscaleSmooth()
{
    paint(true);
}

void tst_DrawTexture::rotateSmooth()
{
    paint(true);
}

void tst_DrawTexture::perspectiveSmooth()
{
    paint(true);
}

QTEST_MAIN(tst_DrawTexture)

#include "tst_drawtexture.moc"
