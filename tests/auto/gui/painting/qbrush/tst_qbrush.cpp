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

#include "qbrush.h"
#include <QPainter>
#include <QBitmap>
#include <private/qpixmap_raster_p.h>

#include <qdebug.h>

class tst_QBrush : public QObject
{
    Q_OBJECT

public:
    tst_QBrush();

private slots:
    void operator_eq_eq();
    void operator_eq_eq_data();

    void stream();
    void stream_data();

    void badStyles();

    void testQLinearGradientSetters();
    void testQRadialGradientSetters();
    void testQConicalGradientSetters();
    void testQGradientCopyConstructor();

    void gradientStops();

    void textures();

    void swap();
    void nullBrush();
    void isOpaque();
    void debug();

    void textureBrushStream();
    void textureBrushComparison();
};


tst_QBrush::tst_QBrush()
{
}

void tst_QBrush::operator_eq_eq_data()
{
    QTest::addColumn<QBrush>("brush1");
    QTest::addColumn<QBrush>("brush2");
    QTest::addColumn<bool>("isEqual");

    QLinearGradient lg(10, 10, 100, 100);
    lg.setColorAt(0, Qt::red);
    lg.setColorAt(0.5, Qt::blue);
    lg.setColorAt(1, Qt::green);

    QTest::newRow("black vs black") << QBrush(Qt::black) << QBrush(Qt::black) << true;
    QTest::newRow("black vs blue") << QBrush(Qt::black) << QBrush(Qt::blue) << false;

    QTest::newRow("red vs no") << QBrush(Qt::red) << QBrush(Qt::NoBrush) << false;
    QTest::newRow("no vs no") << QBrush(Qt::NoBrush) << QBrush(Qt::NoBrush) << true;

    QTest::newRow("lg vs same lg") << QBrush(lg) << QBrush(lg) << true;
    QTest::newRow("lg vs diff lg") << QBrush(lg) << QBrush(QLinearGradient(QPoint(0, 0), QPoint(1, 1)))
                                << false;

    QTest::newRow("rad vs con") << QBrush(QRadialGradient(0, 0, 0, 0, 0)) << QBrush(QConicalGradient(0, 0, 0)) << false;

    QBrush b1(lg);
    QBrush b2(lg);
    b1.setTransform(QTransform().scale(2, 2));
    QTest::newRow("lg with transform vs same lg") << b1 << b2 << false;

    b2.setTransform(QTransform().scale(2, 2));
    QTest::newRow("lg w/transform vs same lg w/same transform") << b1 << b2 << true;

}

void tst_QBrush::operator_eq_eq()
{
    QFETCH(QBrush, brush1);
    QFETCH(QBrush, brush2);
    QFETCH(bool, isEqual);
    QCOMPARE(brush1 == brush2, isEqual);
}

void tst_QBrush::stream_data()
{
    QTest::addColumn<QBrush>("brush");

    QLinearGradient lg(10, 10, 100, 100);
    lg.setColorAt(0, Qt::red);
    lg.setColorAt(0.5, Qt::blue);
    lg.setColorAt(1, Qt::green);

    QTest::newRow("black") << QBrush(Qt::black);
    QTest::newRow("red") << QBrush(Qt::red);
    QTest::newRow("no") << QBrush(Qt::NoBrush);
    QTest::newRow("lg") << QBrush(lg);
    QTest::newRow("rad") << QBrush(QRadialGradient(0, 0, 0, 0, 0));
    QTest::newRow("con") << QBrush(QConicalGradient(0, 0, 0));
}

void tst_QBrush::stream()
{
    QFETCH(QBrush, brush);

    QByteArray data;

    {
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << brush;
    }

    QBrush cmp;
    {
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream >> cmp;
    }

    QCOMPARE(brush.style(), cmp.style());
    QCOMPARE(brush.color(), cmp.color());
    QCOMPARE(brush, cmp);
}

void tst_QBrush::testQLinearGradientSetters()
{
    QLinearGradient lg;

    QCOMPARE(lg.start(), QPointF(0, 0));
    QCOMPARE(lg.finalStop(), QPointF(1, 1));

    lg.setStart(101, 102);
    QCOMPARE(lg.start(), QPointF(101, 102));

    lg.setStart(QPointF(201, 202));
    QCOMPARE(lg.start(), QPointF(201, 202));

    lg.setFinalStop(103, 104);
    QCOMPARE(lg.finalStop(), QPointF(103, 104));

    lg.setFinalStop(QPointF(203, 204));
    QCOMPARE(lg.finalStop(), QPointF(203, 204));
}

void tst_QBrush::testQRadialGradientSetters()
{
    QRadialGradient rg;

    QCOMPARE(rg.radius(), qreal(1.0));
    QCOMPARE(rg.center(), QPointF(0, 0));
    QCOMPARE(rg.focalPoint(), QPointF(0, 0));

    rg.setRadius(100);
    QCOMPARE(rg.radius(), qreal(100.0));

    rg.setCenter(101, 102);
    QCOMPARE(rg.center(), QPointF(101, 102));

    rg.setCenter(QPointF(201, 202));
    QCOMPARE(rg.center(), QPointF(201, 202));

    rg.setFocalPoint(103, 104);
    QCOMPARE(rg.focalPoint(), QPointF(103, 104));

    rg.setFocalPoint(QPointF(203, 204));
    QCOMPARE(rg.focalPoint(), QPointF(203, 204));
}

void tst_QBrush::testQConicalGradientSetters()
{
    QConicalGradient cg;

    QCOMPARE(cg.angle(), qreal(0.0));
    QCOMPARE(cg.center(), QPointF(0, 0));

    cg.setAngle(100);
    QCOMPARE(cg.angle(), qreal(100.0));

    cg.setCenter(102, 103);
    QCOMPARE(cg.center(), QPointF(102, 103));

    cg.setCenter(QPointF(202, 203));
    QCOMPARE(cg.center(), QPointF(202, 203));
}

void tst_QBrush::testQGradientCopyConstructor()
{
    {
        QLinearGradient lg1(101, 102, 103, 104);

        QLinearGradient lg2 = lg1;
        QCOMPARE(lg1.start(), lg2.start());
        QCOMPARE(lg1.finalStop(), lg2.finalStop());

        QGradient g = lg1;
        QCOMPARE(((QLinearGradient *) &g)->start(), lg1.start());
        QCOMPARE(((QLinearGradient *) &g)->finalStop(), lg1.finalStop());
    }

    {
        QRadialGradient rg1(101, 102, 103, 104, 105);

        QRadialGradient rg2 = rg1;
        QCOMPARE(rg1.center(), rg2.center());
        QCOMPARE(rg1.focalPoint(), rg2.focalPoint());
        QCOMPARE(rg1.radius(), rg2.radius());

        QGradient g = rg1;
        QCOMPARE(((QRadialGradient *) &g)->center(), rg1.center());
        QCOMPARE(((QRadialGradient *) &g)->focalPoint(), rg1.focalPoint());
        QCOMPARE(((QRadialGradient *) &g)->radius(), rg1.radius());
    }

    {
        QConicalGradient cg1(101, 102, 103);

        QConicalGradient cg2 = cg1;
        QCOMPARE(cg1.center(), cg2.center());
        QCOMPARE(cg1.angle(), cg2.angle());

        QGradient g = cg1;
        QCOMPARE(((QConicalGradient *) &g)->center(), cg1.center());
        QCOMPARE(((QConicalGradient *) &g)->angle(), cg1.angle());
    }

}

void tst_QBrush::badStyles()
{
    // QBrush(Qt::BrushStyle) constructor
    QCOMPARE(QBrush(Qt::LinearGradientPattern).style(), Qt::NoBrush);
    QCOMPARE(QBrush(Qt::RadialGradientPattern).style(), Qt::NoBrush);
    QCOMPARE(QBrush(Qt::ConicalGradientPattern).style(), Qt::NoBrush);
    QCOMPARE(QBrush(Qt::TexturePattern).style(), Qt::NoBrush);

    // QBrush(QColor, Qt::BrushStyle) constructor
    QCOMPARE(QBrush(QColor(0, 0, 0), Qt::LinearGradientPattern).style(), Qt::NoBrush);
    QCOMPARE(QBrush(QColor(0, 0, 0), Qt::RadialGradientPattern).style(), Qt::NoBrush);
    QCOMPARE(QBrush(QColor(0, 0, 0), Qt::ConicalGradientPattern).style(), Qt::NoBrush);
    QCOMPARE(QBrush(QColor(0, 0, 0), Qt::TexturePattern).style(), Qt::NoBrush);

    // QBrush(Qt::GlobalColor, Qt::BrushStyle) constructor
    QCOMPARE(QBrush(Qt::black, Qt::LinearGradientPattern).style(), Qt::NoBrush);
    QCOMPARE(QBrush(Qt::black, Qt::RadialGradientPattern).style(), Qt::NoBrush);
    QCOMPARE(QBrush(Qt::black, Qt::ConicalGradientPattern).style(), Qt::NoBrush);
    QCOMPARE(QBrush(Qt::black, Qt::TexturePattern).style(), Qt::NoBrush);

    // Set style...
    QBrush brush(Qt::red);

    brush.setStyle(Qt::LinearGradientPattern);
    QCOMPARE(brush.style(), Qt::SolidPattern);

    brush.setStyle(Qt::RadialGradientPattern);
    QCOMPARE(brush.style(), Qt::SolidPattern);

    brush.setStyle(Qt::ConicalGradientPattern);
    QCOMPARE(brush.style(), Qt::SolidPattern);

    brush.setStyle(Qt::TexturePattern);
    QCOMPARE(brush.style(), Qt::SolidPattern);

}

void tst_QBrush::gradientStops()
{
    QLinearGradient gradient;
    gradient.setColorAt(0, Qt::red);
    gradient.setColorAt(1, Qt::blue);

    QCOMPARE(gradient.stops().size(), 2);

    QCOMPARE(gradient.stops().at(0), QGradientStop(0, QColor(Qt::red)));
    QCOMPARE(gradient.stops().at(1), QGradientStop(1, QColor(Qt::blue)));

    gradient.setColorAt(0, Qt::blue);
    gradient.setColorAt(1, Qt::red);

    QCOMPARE(gradient.stops().size(), 2);

    QCOMPARE(gradient.stops().at(0), QGradientStop(0, QColor(Qt::blue)));
    QCOMPARE(gradient.stops().at(1), QGradientStop(1, QColor(Qt::red)));

    gradient.setColorAt(0.5, Qt::green);

    QCOMPARE(gradient.stops().size(), 3);
    QCOMPARE(gradient.stops().at(1), QGradientStop(0.5, QColor(Qt::green)));

    // A hack in parseStopNode() in qsvghandler.cpp depends on inserting stops at NaN.
    gradient.setStops(QGradientStops() << QGradientStop(qQNaN(), QColor()));
    QCOMPARE(gradient.stops().size(), 1);
    QVERIFY(qIsNaN(gradient.stops().at(0).first));
    QCOMPARE(gradient.stops().at(0).second, QColor());
}

void fill(QPaintDevice *pd) {
    QPainter p(pd);

    int w = pd->width();
    int h = pd->height();

    p.fillRect(0, 0, w, h, Qt::white);
    p.fillRect(0, 0, w/3, h/3, Qt::black);
}

void tst_QBrush::textures()
{
    QPixmap pixmap_source(10, 10);
    QImage image_source(10, 10, QImage::Format_RGB32);

    fill(&pixmap_source);
    fill(&image_source);

    // Create a pixmap brush and compare its texture and textureImage
    // to the expected image
    QBrush pixmap_brush;
    pixmap_brush.setTexture(pixmap_source);
    QImage image = pixmap_brush.texture().toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image, image_source);
    image = pixmap_brush.textureImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image, image_source);

    pixmap_brush = QBrush(pixmap_source);
    image = pixmap_brush.texture().toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image, image_source);
    image = pixmap_brush.textureImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image, image_source);

    // Create a image brush and compare its texture and textureImage
    // to the expected image
    QBrush image_brush;
    image_brush.setTextureImage(image_source);
    image = image_brush.texture().toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image, image_source);
    QCOMPARE(image_brush.textureImage(), image_source);

    image_brush = QBrush(image_source);
    image = image_brush.texture().toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image, image_source);
    QCOMPARE(image_brush.textureImage(), image_source);
}

void tst_QBrush::swap()
{
    QBrush b1(Qt::black), b2(Qt::white);
    b1.swap(b2);
    QCOMPARE(b1.color(), QColor(Qt::white));
    QCOMPARE(b2.color(), QColor(Qt::black));
}

void tst_QBrush::nullBrush()
{
    QBrush brush(QColor(100,0,0), Qt::NoBrush);
    QCOMPARE(brush.color(), QColor(100,0,0));
}

void tst_QBrush::isOpaque()
{
    QBitmap bm(8, 8);
    bm.fill(Qt::black);

    QBrush brush(bm);
    QVERIFY(!brush.isOpaque());
}

void tst_QBrush::debug()
{
    QPixmap pixmap_source(10, 10);
    fill(&pixmap_source);
    QBrush pixmap_brush;
    pixmap_brush.setTexture(pixmap_source);
    QCOMPARE(pixmap_brush.style(), Qt::TexturePattern);
    qDebug() << pixmap_brush; // don't crash
}

void tst_QBrush::textureBrushStream()
{
    QPixmap pixmap_source(10, 10);
    QImage image_source(10, 10, QImage::Format_RGB32);

    fill(&pixmap_source);
    fill(&image_source);

    QBrush pixmap_brush;
    pixmap_brush.setTexture(pixmap_source);
    QBrush image_brush;
    image_brush.setTextureImage(image_source);

    QByteArray data1;
    QByteArray data2;
    {
        QDataStream stream1(&data1, QIODevice::WriteOnly);
        QDataStream stream2(&data2, QIODevice::WriteOnly);
        stream1 << pixmap_brush;
        stream2 << image_brush;
    }

    QBrush loadedBrush1;
    QBrush loadedBrush2;
    {
        QDataStream stream1(&data1, QIODevice::ReadOnly);
        QDataStream stream2(&data2, QIODevice::ReadOnly);
        stream1 >> loadedBrush1;
        stream2 >> loadedBrush2;
    }

    QCOMPARE(loadedBrush1.style(), Qt::TexturePattern);
    QCOMPARE(loadedBrush2.style(), Qt::TexturePattern);
#ifdef Q_OS_ANDROID
    QEXPECT_FAIL("", "QTBUG-69193", Continue);
#endif
    QCOMPARE(loadedBrush1.texture(), pixmap_source);
    QCOMPARE(loadedBrush2.textureImage(), image_source);
}

void tst_QBrush::textureBrushComparison()
{
    QImage image1(10, 10, QImage::Format_RGB32);
    QRasterPlatformPixmap* ppixmap = new QRasterPlatformPixmap(QPlatformPixmap::PixmapType);
    ppixmap->fromImage(image1, Qt::NoFormatConversion);
    QPixmap pixmap(ppixmap);
    QImage image2(image1);

    QBrush pixmapBrush, imageBrush1, imageBrush2;
    pixmapBrush.setTexture(pixmap);
    imageBrush1.setTextureImage(image1);
    imageBrush2.setTextureImage(image2);

    QCOMPARE(imageBrush1, imageBrush2);
    QCOMPARE(pixmapBrush, imageBrush1);
}

QTEST_MAIN(tst_QBrush)
#include "tst_qbrush.moc"
