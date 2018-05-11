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
#include <QPainter>
#include <QPixmap>
#include <QDialog>
#include <QImage>
#include <QPaintEngine>
#include <QTileRules>
#include <qmath.h>

#include <private/qpixmap_raster_p.h>

Q_DECLARE_METATYPE(QPainterPath)
Q_DECLARE_METATYPE(QPainter::RenderHint)
Q_DECLARE_METATYPE(QPainter::CompositionMode)
Q_DECLARE_METATYPE(QImage::Format)

enum PrimitiveType {
    Primitive_Int_DiagLine,
    Primitive_Int_VerLine,
    Primitive_Int_HorLine,
    Primitive_Int_Rect,
    Primitive_Int_Ellipse,
    Primitive_Int_Pie,
    Primitive_Int_Arc,
    Primitive_Int_Chord,
    Primitive_Int_TriPoly,
    Primitive_Int_RectPoly,
    Primitive_Int_2RectPoly,

    Primitive_Float_DiagLine,
    Primitive_Float_VerLine,
    Primitive_Float_HorLine,
    Primitive_Float_Rect,
    Primitive_Float_Ellipse,
    Primitive_Float_Pie,
    Primitive_Float_Arc,
    Primitive_Float_Chord,
    Primitive_Float_TriPoly,
    Primitive_Float_RectPoly,
    Primitive_Float_2RectPoly,

    Primitive_Float_TriPath,
    Primitive_Float_RectPath,
    Primitive_Float_2RectPath,
    Primitive_Float_EllipsePath,
    Primitive_Last_Primitive

};


enum StateChanges {
    ChangePen         = 0x0001,
    ChangeBrush       = 0x0002,
    ChangeClip        = 0x0004,
    ChangeTransform   = 0x0008
};


struct PrimitiveSet {
    QRect i_rect;
    QLine i_line_diag;
    QLine i_line_ver;
    QLine i_line_hor;
    QPolygon i_poly_tri;
    QPolygon i_poly_2rects;
    QPolygon i_poly_rect;

    QRectF f_rect;
    QLineF f_line_diag;
    QLineF f_line_ver;
    QLineF f_line_hor;
    QPolygonF f_poly_tri;
    QPolygonF f_poly_2rects;
    QPolygonF f_poly_rect;

    QPainterPath f_path_tri;
    QPainterPath f_path_2rects;
    QPainterPath f_path_rect;
    QPainterPath f_path_ellipse;
};


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

    data->fromImage(image, Qt::AutoColor | Qt::NoFormatConversion);

    return QPixmap(data);
}


class tst_QPainter : public QObject
{
    Q_OBJECT

    public:
    tst_QPainter()
    {
        setupBrushes();
        createPrimitives();
        m_surface = surface();
    }

private slots:
    void beginAndEnd();

    void saveRestore_data();
    void saveRestore();

    void drawLine_data();
    void drawLine();
    void drawLine_clipped_data();
    void drawLine_clipped();
    void drawLine_antialiased_clipped_data();
    void drawLine_antialiased_clipped();

    void drawPixmap_data();
    void drawPixmap();

    void drawImage_data();
    void drawImage();

    void drawTiledPixmap_data();
    void drawTiledPixmap();

    void compositionModes_data();
    void compositionModes();

    void fillPrimitives_10_data() { drawPrimitives_data_helper(false); }
    void fillPrimitives_100_data() { drawPrimitives_data_helper(false); }
    void fillPrimitives_1000_data() { drawPrimitives_data_helper(false); }
    void fillPrimitives_10();
    void fillPrimitives_100();
    void fillPrimitives_1000();

    void strokePrimitives_10_data() { drawPrimitives_data_helper(true); }
    void strokePrimitives_100_data() { drawPrimitives_data_helper(true); }
    void strokePrimitives_1000_data() { drawPrimitives_data_helper(true); }
    void strokePrimitives_10();
    void strokePrimitives_100();
    void strokePrimitives_1000();

    void drawText_data();
    void drawText();

    void clipAndFill_data();
    void clipAndFill();

    void drawRoundedRect();
    void drawScaledRoundedRect();
    void drawTransformedRoundedRect();

    void drawScaledAntialiasedRoundedRect_data();
    void drawTransformedAntialiasedRoundedRect_data();
    void drawAntialiasedRoundedRect();
    void drawScaledAntialiasedRoundedRect();
    void drawTransformedAntialiasedRoundedRect();

    void drawScaledImageRoundedRect_data();
    void drawTransformedImageRoundedRect_data();
    void drawImageRoundedRect();
    void drawScaledImageRoundedRect();
    void drawTransformedImageRoundedRect();

    void drawScaledBorderPixmapRoundedRect_data();
    void drawTransformedBorderPixmapRoundedRect_data();
    void drawBorderPixmapRoundedRect();
    void drawScaledBorderPixmapRoundedRect();
    void drawTransformedBorderPixmapRoundedRect();

    void drawTransformedTransparentImage_data();
    void drawTransformedSemiTransparentImage_data();
    void drawTransformedFilledImage_data();
    void drawTransformedTransparentImage();
    void drawTransformedSemiTransparentImage();
    void drawTransformedFilledImage();

private:
    void setupBrushes();
    void createPrimitives();

    void drawPrimitives_data_helper(bool fancypens);
    void drawPixmapImage_data_helper(bool);
    void fillPrimitives_helper(QPainter *painter, PrimitiveType type, PrimitiveSet *s);

    QTransform transformForAngle(qreal angle);

    QPaintDevice *surface()
    {
        m_pixmap = rasterPixmap(1024, 1024);
        return &m_pixmap;
    }


    QMap<QString, QPen> m_pens;
    QMap<QString, QBrush> m_brushes;

    PrimitiveSet m_primitives_10;
    PrimitiveSet m_primitives_100;
    PrimitiveSet m_primitives_1000;

    QPixmap m_pixmap;
    QPaintDevice *m_surface;
    QPainter m_painter;

};

void tst_QPainter::createPrimitives()
{
    for (int i=0; i<3; ++i) {
        PrimitiveSet *ps;
        int size;
        switch (i) {
        case 0:
            ps = &m_primitives_10;
            size = 10;
            break;
        case 1:
            ps = &m_primitives_100;
            size = 100;
            break;
        case 2:
            ps = &m_primitives_1000;
            size = 1000;
            break;
        }

        ps->f_rect = QRectF(0, 0, size, size);
        ps->f_line_diag = QLineF(0, 0, size, size);
        ps->f_line_ver = QLineF(10, 0, 10, size);
        ps->f_line_hor = QLineF(0, 10, size, 10);
        ps->f_poly_rect = QPolygonF() << QPointF(0, 0)
                                      << QPointF(size, 0)
                                      << QPointF(size, size)
                                      << QPointF(0, size);
        ps->f_poly_2rects = QPolygonF() << QPointF(0, 0)
                                        << QPointF(size * 0.75, 0)
                                        << QPointF(size * 0.75, size * 0.75)
                                        << QPointF(size * 0.25, size * 0.75)
                                        << QPointF(size * 0.25, size * 0.25)
                                        << QPointF(size, size * 0.25)
                                        << QPointF(size, size)
                                        << QPointF(0, size);
        ps->f_poly_tri = QPolygonF() << QPointF(size / 2.0, 0)
                                     << QPointF(0, size)
                                     << QPointF(size, size);

        ps->f_path_tri.addPolygon(ps->f_poly_tri);
        ps->f_path_rect.addRect(ps->f_rect);
        ps->f_path_2rects.addPolygon(ps->f_poly_2rects);
        ps->f_path_ellipse.addEllipse(ps->f_rect);

        ps->i_rect = ps->f_rect.toRect();
        ps->i_line_diag = ps->f_line_diag.toLine();
        ps->i_line_hor = ps->f_line_hor.toLine();
        ps->i_line_ver = ps->f_line_ver.toLine();
        ps->i_poly_tri = ps->f_poly_tri.toPolygon();
        ps->i_poly_rect = ps->f_poly_rect.toPolygon();
        ps->i_poly_2rects = ps->f_poly_2rects.toPolygon();
    }
}

void tst_QPainter::drawLine_data()
{
    QTest::addColumn<QLine>("line");
    QTest::addColumn<QPen>("pen");

    QVector<QPen> pens;
    pens << QPen(Qt::black)
         << QPen(Qt::black, 0, Qt::DashDotLine)
         << QPen(Qt::black, 4)
         << QPen(Qt::black, 4, Qt::DashDotLine)
         << QPen(QColor(255, 0, 0, 200))
         << QPen(QColor(255, 0, 0, 200), 0, Qt::DashDotLine)
         << QPen(QColor(255, 0, 0, 200), 4)
         << QPen(QColor(255, 0, 0, 200), 4, Qt::DashDotLine);

    QStringList penNames;
    penNames << "black-0"
             << "black-0-dashdot"
             << "black-4"
             << "black-4-dashdot"
             << "alpha-0"
             << "alpha-0-dashdot"
             << "alpha-4"
             << "alpha-4-dashdot";

    int i = 0;
    foreach (QPen pen, pens) {
        const QString s = QString(QLatin1String("%1:%2")).arg(penNames[i]);
        QTest::newRow(qPrintable(s.arg("horizontal")))
            << QLine(0, 20, 100, 20) << pen;
        QTest::newRow(qPrintable(s.arg("vertical:")))
            << QLine(20, 0, 20, 100) << pen;
        QTest::newRow(qPrintable(s.arg("0-45:")))
            << QLine(0, 20, 100, 0) << pen;
        QTest::newRow(qPrintable(s.arg("45-90:")))
            << QLine(0, 100, 20, 0) << pen;
        QTest::newRow(qPrintable(s.arg("90-135:")))
            << QLine(20, 100, 0, 0) << pen;
        QTest::newRow(qPrintable(s.arg("135-180:")))
            << QLine(100, 20, 0, 0) << pen;
        QTest::newRow(qPrintable(s.arg("180-225:")))
            << QLine(100, 0, 0, 20) << pen;
        QTest::newRow(qPrintable(s.arg("225-270:")))
            << QLine(20, 0, 0, 100) << pen;
        QTest::newRow(qPrintable(s.arg("270-315:")))
            << QLine(0, 0, 20, 100) << pen;
        QTest::newRow(qPrintable(s.arg("315-360:")))
            << QLine(0, 0, 100, 20) << pen;
        ++i;
    }
}

void tst_QPainter::setupBrushes()
{
    // Solid brushes...
    m_brushes["black-brush"] = QBrush(Qt::black);
    m_brushes["white-brush"] = QBrush(Qt::white);
    m_brushes["transparent-brush"] = QBrush(QColor(255, 255, 255, 0));
    m_brushes["alpha1-brush"] = QBrush(QColor(255, 255, 255, 100));
    m_brushes["alpha2-brush"] = QBrush(QColor(255, 255, 255, 200));


    // Patterns
    m_brushes["dense1-brush"] = QBrush(Qt::Dense1Pattern);
    m_brushes["dense2-brush"] = QBrush(Qt::Dense2Pattern);
    m_brushes["dense3-brush"] = QBrush(Qt::Dense3Pattern);
    m_brushes["dense4-brush"] = QBrush(Qt::Dense4Pattern);
    m_brushes["dense5-brush"] = QBrush(Qt::Dense5Pattern);
    m_brushes["dense6-brush"] = QBrush(Qt::Dense6Pattern);
    m_brushes["dense7-brush"] = QBrush(Qt::Dense7Pattern);
    m_brushes["hor-brush"] = QBrush(Qt::HorPattern);
    m_brushes["ver-brush"] = QBrush(Qt::VerPattern);
    m_brushes["cross-brush"] = QBrush(Qt::CrossPattern);
    m_brushes["bdiag-brush"] = QBrush(Qt::BDiagPattern);
    m_brushes["fdiag-brush"] = QBrush(Qt::FDiagPattern);
    m_brushes["diagcross-brush"] = QBrush(Qt::DiagCrossPattern);

    // Gradients
    QGradientStops gradient_white_black;
    gradient_white_black << QPair<qreal, QColor>(0, QColor(Qt::white));
    gradient_white_black << QPair<qreal, QColor>(1, QColor(Qt::black));

    QGradientStops gradient_white_black10;
    for (int i=0; i<10; ++i) {
        gradient_white_black10 << QPair<qreal, QColor>(i/10.0, QColor(Qt::white));
        gradient_white_black10 << QPair<qreal, QColor>(i/10.0+0.05, QColor(Qt::black));
    }

    QGradientStops gradient_white_alpha;
    gradient_white_alpha << QPair<qreal, QColor>(0, QColor(Qt::white));
    gradient_white_alpha << QPair<qreal, QColor>(0, QColor(Qt::transparent));

    QGradientStops gradient_white_alpha10;
    for (int i=0; i<10; ++i) {
        gradient_white_alpha10 << QPair<qreal, QColor>(i/10.0, QColor(Qt::white));
        gradient_white_alpha10 << QPair<qreal, QColor>(i/10.0+0.05, QColor(Qt::black));
    }


    for (int j=0; j<4; ++j) {
        QLinearGradient lg;
        lg.setStart(0, 0);

        QRadialGradient rg;
        QConicalGradient cg;

        QGradientStops stops;
        if (j == 0) stops = gradient_white_black;
        else if (j == 1) stops = gradient_white_black10;
        else if (j == 2) stops = gradient_white_alpha;
        else if (j == 3) stops = gradient_white_alpha10;
        lg.setStops(stops);
        rg.setStops(stops);
        cg.setStops(stops);

        for (int i=0; i<6; ++i) {
            lg.setSpread((QGradient::Spread) (i % 3));
            lg.setCoordinateMode((QGradient::CoordinateMode) (j / 3));

            QString name = QString::fromLatin1("-%1%2")
                           .arg(lg.spread())
                           .arg(lg.coordinateMode());

            lg.setFinalStop(100, 0);
            m_brushes["hor-lingrad-w/b-brush" + name] = QBrush(lg);

            lg.setFinalStop(0, 100);
            m_brushes["ver-lingrad-w/b-brush" + name] = QBrush(lg);

            lg.setFinalStop(100, 100);
            m_brushes["diag-lingrad-w/b-brush" + name] = QBrush(lg);

            rg.setRadius(100);
            rg.setCenter(0, 0);
            rg.setFocalPoint(50, 50);
            m_brushes["radgrad-brush" + name] = QBrush(rg);

            cg.setCenter(0, 0);
            cg.setAngle(40);
            m_brushes["congrad-brush" + name] = QBrush(cg);
        }
    }

    // Set up pens...


//     m_pens["black-pen"] = QPen(Qt::black);
//     m_pens["white-pen"] = QPen(Qt::white);
//     m_pens["transparent-pen"] = QPen(QColor(255, 255, 255, 0));
//     m_pens["translucent1-pen"] = QPen(QColor(255, 255, 255, 100));
//     m_pens["translucent2-pen"] = QPen(QColor(255, 255, 255, 200));



}


// void QPainter_Primitives::fillRect_data() {

//     QTest::addColumn<QBrush>("brush");
//     QTest::addColumn<QSize>("size");

//     for (QMap<QString, QBrush>::const_iterator it = m_brushes.constBegin();
//          it != m_brushes.constEnd(); ++it) {
//         for (int w=2; w<1025; w*=2) {
//             for (int h=2; h<1025; h*=2) {
//                 QTest::newRow(QString("brush=%1; size=[%2,%3]").arg(it.key()).arg(w).arg(h).toLatin1().data())
//                     << *it << QSize(w, h);
//             }
//         }
//     }
// }





// void QPainter_Primitives::fillRect()
// {
//     QFETCH(QBrush, brush);
//     QFETCH(QSize, size);

//     QImage img(1024, 1024, QImage::Format_ARGB32_Premultiplied);
//     QPainter p(&img);
//     p.setPen(Qt::NoPen);
//     p.setBrush(brush);
//     QRect rect(QPoint(0, 0), size);
//     QBENCHMARK {
//         p.drawRect(rect);
//     }
// }




void tst_QPainter::beginAndEnd()
{
    QPixmap pixmap = rasterPixmap(100, 100);

    QBENCHMARK {
        QPainter p;
        p.begin(&pixmap);
        p.end();
    }
}

void tst_QPainter::drawLine()
{
    QFETCH(QLine, line);
    QFETCH(QPen, pen);

    const int offset = 5;
    QPixmap pixmapUnclipped =
        rasterPixmap(qMin(line.x1(), line.x2())
                     + 2*offset + qAbs(line.dx()),
                     qMin(line.y1(), line.y2())
                     + 2*offset + qAbs(line.dy()));
    pixmapUnclipped.fill(Qt::white);

    QPainter p(&pixmapUnclipped);
    p.translate(offset, offset);
    p.setPen(pen);
    p.paintEngine()->syncState();

    QBENCHMARK {
        p.drawLine(line);
    }

    p.end();

}

void tst_QPainter::drawLine_clipped_data()
{
    drawLine_data();
}

void tst_QPainter::drawLine_clipped()
{
    QFETCH(QLine, line);
    QFETCH(QPen, pen);

    const int offset = 5;
    QPixmap pixmapClipped
        = rasterPixmap(qMin(line.x1(), line.x2())
                       + 2*offset + qAbs(line.dx()),
                       qMin(line.y1(), line.y2())
                       + 2*offset + qAbs(line.dy()));

    const QRect clip = QRect(line.p1(), line.p2()).normalized();

    pixmapClipped.fill(Qt::white);
    QPainter p(&pixmapClipped);
    p.translate(offset, offset);
    p.setClipRect(clip);
    p.setPen(pen);
    p.paintEngine()->syncState();

    QBENCHMARK {
        p.drawLine(line);
    }

    p.end();
}


void tst_QPainter::drawLine_antialiased_clipped_data()
{
    drawLine_data();
}


void tst_QPainter::drawLine_antialiased_clipped()
{
    QFETCH(QLine, line);
    QFETCH(QPen, pen);

    const int offset = 5;
    QPixmap pixmapClipped
        = rasterPixmap(qMin(line.x1(), line.x2())
                       + 2*offset + qAbs(line.dx()),
                       qMin(line.y1(), line.y2())
                       + 2*offset + qAbs(line.dy()));

    const QRect clip = QRect(line.p1(), line.p2()).normalized();

    pixmapClipped.fill(Qt::white);
    QPainter p(&pixmapClipped);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(offset, offset);
    p.setClipRect(clip);
    p.setPen(pen);
    p.paintEngine()->syncState();

    QBENCHMARK {
        p.drawLine(line);
    }

    p.end();
}

void tst_QPainter::drawPixmapImage_data_helper(bool pixmaps)
{
    QTest::addColumn<QImage::Format>("sourceFormat");
    QTest::addColumn<QImage::Format>("targetFormat");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("type"); // 0 = circle, 1 = diag line, 2 = solid rect, 3 = alpharect

    QList<QSize> sizes;
    sizes << QSize(10, 10)
          << QSize(1000, 1000);

    const char *typeNames[] = {
        "circle",
        "line",
        "solidrect",
        "alpharect"
    };

    const char *formatNames[] = {
        "Invalid",
        "Mono",
        "MonoLSB",
        "Indexed8",
        "RGB32",
        "ARGB32",
        "ARGB32_pm",
        "RGB16",
        "ARGB8565_pm",
        "RGB666",
        "ARGB6666_pm",
        "RGB555",
        "ARGB8555_pm",
        "RGB888",
        "RGB444",
        "ARGB4444_pm",
        "RGBx8888",
        "RGBA8888",
        "RGBA8888_pm",
        "BGR30",
        "A2BGR30_pm",
        "RGB30",
        "A2RGB30_pm",
        "Alpha8",
        "Grayscale8",
    };

    const QImage::Format pixmapFormats[] = {
        QImage::Format_RGB32,
        QImage::Format_ARGB32_Premultiplied,
        QImage::Format_RGB16,
        QImage::Format_BGR30,
        QImage::Format_Invalid
    };

    const QImage::Format targetImageFormats[] = {
        QImage::Format_RGB32,
        QImage::Format_ARGB32,
        QImage::Format_ARGB32_Premultiplied,
        QImage::Format_RGB16,
        QImage::Format_ARGB8565_Premultiplied,
        QImage::Format_RGBX8888,
        QImage::Format_RGBA8888_Premultiplied,
        QImage::Format_BGR30,
        QImage::Format_A2RGB30_Premultiplied,
        QImage::Format_Grayscale8,
        QImage::Format_Invalid
    };

    const QImage::Format sourceImageFormats[] = {
        QImage::Format_Indexed8,
        QImage::Format_RGB32,
        QImage::Format_ARGB32,
        QImage::Format_ARGB32_Premultiplied,
        QImage::Format_RGB16,
        QImage::Format_RGB888,
        QImage::Format_RGBX8888,
        QImage::Format_RGBA8888,
        QImage::Format_RGB30,
        QImage::Format_Grayscale8,
        QImage::Format_Invalid
    };

    const QImage::Format *targetFormats = pixmaps ? pixmapFormats : targetImageFormats;
    for (; *targetFormats != QImage::Format_Invalid; ++targetFormats) {
        const QImage::Format *sourceFormats = pixmaps ? pixmapFormats : sourceImageFormats;
        for (; *sourceFormats != QImage::Format_Invalid; ++sourceFormats) {
            for (const QSize &s : qAsConst(sizes)) {
                for (int type=0; type<=3; ++type) {
                    QString name = QString::fromLatin1("%1 on %2, (%3x%4), %5")
                                   .arg(formatNames[*sourceFormats])
                                   .arg(formatNames[*targetFormats])
                                   .arg(s.width()).arg(s.height())
                                   .arg(typeNames[type]);
                    QTest::newRow(name.toLatin1()) << *sourceFormats
                                                   << *targetFormats
                                                   << s
                                                   << type;
                }
            }
        }
    }
}

static QImage createImage(int type, const QSize &size) {
    QImage base(size, QImage::Format_ARGB32_Premultiplied);
    base.fill(0);
    QPainter p(&base);
    p.setRenderHint(QPainter::Antialiasing);
    switch (type) {
    case 0: // ellipse
        p.setBrush(Qt::red);
        p.drawEllipse(0, 0, size.width(), size.height());
        break;
    case 1: // line
        p.drawLine(0, 0, size.width(), size.height());
        break;
    case 2:
        p.fillRect(0, 0, size.width(), size.height(), Qt::red);
        break;
    case 3:
        p.fillRect(0, 0, size.width(), size.height(), QColor(0, 255, 0, 127));
        break;
    }
    p.end();
    return base;
}


void tst_QPainter::drawPixmap_data()
{
    drawPixmapImage_data_helper(true);
}

void tst_QPainter::drawPixmap()
{
    QFETCH(QImage::Format, sourceFormat);
    QFETCH(QImage::Format, targetFormat);
    QFETCH(QSize, size);
    QFETCH(int, type);

    QImage sourceImage = createImage(type, size).convertToFormat(sourceFormat);
    QImage targetImage(size, targetFormat);

    QPixmap sourcePixmap = rasterPixmap(sourceImage);
    QPixmap targetPixmap = rasterPixmap(targetImage);

    QPainter p(&targetPixmap);

    QBENCHMARK {
        p.drawPixmap(0, 0, sourcePixmap);
    }
}

void tst_QPainter::drawImage_data()
{
    drawPixmapImage_data_helper(false);
}


void tst_QPainter::drawImage()
{
    QFETCH(QImage::Format, sourceFormat);
    QFETCH(QImage::Format, targetFormat);
    QFETCH(QSize, size);
    QFETCH(int, type);

    QImage sourceImage = createImage(type, size).convertToFormat(sourceFormat);
    QImage targetImage(size, targetFormat);

    QPainter p(&targetImage);
    QBENCHMARK {
        p.drawImage(0, 0, sourceImage);
    }
}


void tst_QPainter::compositionModes_data()
{
    QTest::addColumn<QPainter::CompositionMode>("mode");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QColor>("color");

    const int n = QPainter::RasterOp_SourceAndNotDestination;
    for (int i = 0; i <= n; ++i) {
        QString title("%1:%2");
        QTest::newRow(qPrintable(title.arg(i).arg("10x10:opaque")))
            << (QPainter::CompositionMode)(i)
            << QSize(10, 10) << QColor(255, 0, 0);
        QTest::newRow(qPrintable(title.arg(i).arg("10x10:!opaque")))
            << (QPainter::CompositionMode)(i)
            << QSize(10, 10) << QColor(127, 127, 127, 127);
        QTest::newRow(qPrintable(title.arg(i).arg("300x300:opaque")))
            << (QPainter::CompositionMode)(i)
            << QSize(300, 300) << QColor(255, 0, 0);
        QTest::newRow(qPrintable(title.arg(i).arg("300x300:!opaque")))
            << (QPainter::CompositionMode)(i)
            << QSize(300, 300) << QColor(127, 127, 127, 127);
    }
}

void tst_QPainter::compositionModes()
{
    QFETCH(QPainter::CompositionMode, mode);
    QFETCH(QSize, size);
    QFETCH(QColor, color);

    QPixmap src = rasterPixmap(size);
    src.fill(color);

    QPixmap dest = rasterPixmap(size);
    if (mode < QPainter::RasterOp_SourceOrDestination)
        color.setAlpha(127); // porter-duff needs an alpha channel
    dest.fill(color);

    QPainter p(&dest);
    p.setCompositionMode(mode);

    QBENCHMARK {
        p.drawPixmap(0, 0, src);
    }
}

void tst_QPainter::drawTiledPixmap_data()
{
    QTest::addColumn<QSize>("srcSize");
    QTest::addColumn<QSize>("dstSize");
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<QColor>("color");
    QTest::addColumn<QPainter::RenderHint>("renderHint");

    QTest::newRow("10x10=>20x20")
        << QSize(10, 10) << QSize(20, 20) << (QTransform())
        << QColor(Qt::black) << QPainter::RenderHint(0);
    QTest::newRow("10x10=>20x20, smooth")
        << QSize(10, 10) << QSize(20, 20) << (QTransform())
        << QColor(Qt::black) << QPainter::SmoothPixmapTransform;
    QTest::newRow("10x10=>20x20, !opaque")
        << QSize(10, 10) << QSize(20, 20) << (QTransform())
        << QColor(127, 127, 127, 127) << QPainter::RenderHint(0);
    QTest::newRow("10x10=>20x20, !opaque, smooth")
        << QSize(10, 10) << QSize(20, 20) << (QTransform())
        << QColor(127, 127, 127, 127) << QPainter::SmoothPixmapTransform;

    QTest::newRow("10x10=>20x20, rotate(30)")
        << QSize(10, 10) << QSize(20, 20) << (QTransform().rotate(30))
        << QColor(Qt::black) << QPainter::RenderHint(0);
    QTest::newRow("10x10=>20x20, rotate(30), smooth")
        << QSize(10, 10) << QSize(20, 20) << (QTransform().rotate(30))
        << QColor(Qt::black) << QPainter::SmoothPixmapTransform;
    QTest::newRow("10x10=>20x20, rotate(30), !opaque")
        << QSize(10, 10) << QSize(20, 20) << (QTransform().rotate(30))
        << QColor(127, 127, 127, 127) << QPainter::RenderHint(0);
    QTest::newRow("10x10=>20x20, rotate(30), !opaque, smooth")
        << QSize(10, 10) << QSize(20, 20) << (QTransform().rotate(30))
        << QColor(127, 127, 127, 127) << QPainter::SmoothPixmapTransform;

    QTest::newRow("100x100=>200x200")
        << QSize(100, 100) << QSize(200, 200) << (QTransform())
        << QColor(Qt::black) << QPainter::RenderHint(0);
    QTest::newRow("100x100=>200x200, smooth")
        << QSize(100, 100) << QSize(200, 200) << (QTransform())
        << QColor(Qt::black) << QPainter::SmoothPixmapTransform;
    QTest::newRow("100x100=>200x200, !opaque")
        << QSize(100, 100) << QSize(200, 200) << (QTransform())
        << QColor(127, 127, 127, 127) << QPainter::RenderHint(0);
    QTest::newRow("100x100=>200x200, !opaque, smooth")
        << QSize(100, 100) << QSize(200, 200) << (QTransform())
        << QColor(127, 127, 127, 127) << QPainter::SmoothPixmapTransform;

    QTest::newRow("100x100=>200x200, rotate(30)")
        << QSize(100, 100) << QSize(200, 200) << (QTransform().rotate(30))
        << QColor(Qt::black) << QPainter::RenderHint(0);
    QTest::newRow("100x100=>200x200, rotate(30), smooth")
        << QSize(100, 100) << QSize(200, 200) << (QTransform().rotate(30))
        << QColor(Qt::black) << QPainter::SmoothPixmapTransform;
    QTest::newRow("100x100=>200x200, rotate(30), !opaque")
        << QSize(100, 100) << QSize(200, 200) << (QTransform().rotate(30))
        << QColor(127, 127, 127, 127) << QPainter::RenderHint(0);
    QTest::newRow("100x100=>200x200, rotate(30), !opaque, smooth")
        << QSize(100, 100) << QSize(200, 200) << (QTransform().rotate(30))
        << QColor(127, 127, 127, 127) << QPainter::SmoothPixmapTransform;
}

void tst_QPainter::drawTiledPixmap()
{
    QFETCH(QSize, srcSize);
    QFETCH(QSize, dstSize);
    QFETCH(QTransform, transform);
    QFETCH(QColor, color);
    QFETCH(QPainter::RenderHint, renderHint);

    QPixmap src = rasterPixmap(srcSize);
    src.fill(color);

    const QRect dstRect = transform.mapRect(QRect(QPoint(), dstSize));
    QPixmap dst = rasterPixmap(dstRect.right() + 5, dstRect.bottom() + 5);
    QPainter p(&dst);
    p.setTransform(transform);
    p.setRenderHint(renderHint);

    QBENCHMARK {
        p.drawTiledPixmap(QRect(QPoint(), dstSize), src);
    }
}

void tst_QPainter::fillPrimitives_helper(QPainter *p, PrimitiveType type, PrimitiveSet *s)
{
    p->paintEngine()->syncState();

    switch (type) {
    case Primitive_Int_DiagLine:        QBENCHMARK { p->drawLine(s->i_line_diag); } break;
    case Primitive_Int_VerLine:         QBENCHMARK { p->drawLine(s->i_line_ver); } break;
    case Primitive_Int_HorLine:         QBENCHMARK { p->drawLine(s->i_line_hor); } break;
    case Primitive_Int_Rect:            QBENCHMARK { p->drawRect(s->i_rect); } break;
    case Primitive_Int_Ellipse:         QBENCHMARK { p->drawEllipse(s->i_rect); } break;
    case Primitive_Int_Pie:             QBENCHMARK { p->drawPie(s->i_rect, 45*16, 270*16); } break;
    case Primitive_Int_Arc:             QBENCHMARK { p->drawArc(s->i_rect, 45*16, 270*16); } break;
    case Primitive_Int_Chord:           QBENCHMARK { p->drawChord(s->i_rect, 45*16, 270*16); } break;
    case Primitive_Int_TriPoly:         QBENCHMARK { p->drawPolygon(s->i_poly_tri); } break;
    case Primitive_Int_RectPoly:        QBENCHMARK { p->drawPolygon(s->i_poly_rect); } break;
    case Primitive_Int_2RectPoly:       QBENCHMARK { p->drawPolygon(s->i_poly_2rects); } break;

    case Primitive_Float_DiagLine:      QBENCHMARK { p->drawLine(s->f_line_diag); } break;
    case Primitive_Float_VerLine:       QBENCHMARK { p->drawLine(s->f_line_ver); } break;
    case Primitive_Float_HorLine:       QBENCHMARK { p->drawLine(s->f_line_hor); } break;
    case Primitive_Float_Rect:          QBENCHMARK { p->drawRect(s->f_rect); } break;
    case Primitive_Float_Ellipse:       QBENCHMARK { p->drawEllipse(s->f_rect); } break;
    case Primitive_Float_Pie:           QBENCHMARK { p->drawPie(s->f_rect, 45*16, 270*16); } break;
    case Primitive_Float_Arc:           QBENCHMARK { p->drawArc(s->f_rect, 45*16, 270*16); } break;
    case Primitive_Float_Chord:         QBENCHMARK { p->drawChord(s->f_rect, 45*16, 270*16); } break;
    case Primitive_Float_TriPoly:       QBENCHMARK { p->drawPolygon(s->f_poly_tri); } break;
    case Primitive_Float_RectPoly:      QBENCHMARK { p->drawPolygon(s->f_poly_rect); } break;
    case Primitive_Float_2RectPoly:     QBENCHMARK { p->drawPolygon(s->f_poly_2rects); } break;

    case Primitive_Float_TriPath:       QBENCHMARK { p->drawPath(s->f_path_tri); } break;
    case Primitive_Float_RectPath:      QBENCHMARK { p->drawPath(s->f_path_rect); } break;
    case Primitive_Float_2RectPath:     QBENCHMARK { p->drawPath(s->f_path_2rects); } break;
    case Primitive_Float_EllipsePath:   QBENCHMARK { p->drawPath(s->f_path_ellipse); } break;
    case Primitive_Last_Primitive: break;
    }
}

void tst_QPainter::drawPrimitives_data_helper(bool fancypens)
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("aa");
    QTest::addColumn<bool>("dash");
    QTest::addColumn<int>("width");

    const char * const names[] = {
        "IDLine",
        "IVLine",
        "IHLine",
        "IRect",
        "IElli",
        "IPie",
        "IArc",
        "IChord",
        "ITriPol",
        "IRectPol",
        "I2RectPol",
        "FDLine",
        "FVLine",
        "FHLine",
        "FRect",
        "FElli",
        "FPie",
        "FArc",
        "FChord",
        "FTriPol",
        "FRectPol",
        "F2RectPol",
        "FTriPa",
        "FRectPa",
        "F2RectPa",
        "FElliPa"
    };

    if (fancypens) {
        for (int dash=0; dash<2; ++dash) {
            for (int width=0; width<=4; width+=4) {
                for (int aa=0; aa<2; ++aa) {
                    for (int type=0; type<Primitive_Last_Primitive; ++type) {
                        QString name = QString::fromLatin1(names[type]);

                        if (aa) name += " aa";
                        if (dash) name += " dotted";
                        if (width) name += QString::fromLatin1(" width=%1").arg(width);

                        QTest::newRow(name.toLatin1()) << type << (bool) aa << (bool) dash << width;
                    }
                }
            }
        }
    } else {
        for (int aa=0; aa<2; ++aa) {
            for (int type=0; type<Primitive_Last_Primitive; ++type) {
                QString name = QString::fromLatin1(names[type]);
                if (aa) name += " aa";
                QTest::newRow(name.toLatin1()) << type << (bool) aa;
            }
        }
    }
}


void tst_QPainter::fillPrimitives_10()
{
    QFETCH(int, type);
    QFETCH(bool, aa);
    QPainter p(m_surface);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::red);
    p.setRenderHint(QPainter::Antialiasing, aa);
    fillPrimitives_helper(&p, (PrimitiveType) type, &m_primitives_10);
}


void tst_QPainter::fillPrimitives_100()
{
    QFETCH(int, type);
    QFETCH(bool, aa);
    QPainter p(m_surface);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::red);
    p.setRenderHint(QPainter::Antialiasing, aa);
    fillPrimitives_helper(&p, (PrimitiveType) type, &m_primitives_100);
}


void tst_QPainter::fillPrimitives_1000()
{
    QFETCH(int, type);
    QFETCH(bool, aa);
    QPainter p(m_surface);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::red);
    p.setRenderHint(QPainter::Antialiasing, aa);
    fillPrimitives_helper(&p, (PrimitiveType) type, &m_primitives_1000);
}

void tst_QPainter::strokePrimitives_10()
{
    QFETCH(int, type);
    QFETCH(bool, aa);
    QFETCH(bool, dash);
    QFETCH(int, width);
    QPainter p(m_surface);
    p.setPen(QPen(Qt::red, width, dash ? Qt::DashLine : Qt::SolidLine));
    p.setBrush(Qt::NoBrush);
    p.setRenderHint(QPainter::Antialiasing, aa);
    fillPrimitives_helper(&p, (PrimitiveType) type, &m_primitives_10);
}

void tst_QPainter::strokePrimitives_100()
{
    QFETCH(int, type);
    QFETCH(bool, aa);
    QFETCH(bool, dash);
    QFETCH(int, width);
    QPainter p(m_surface);
    p.setPen(QPen(Qt::red, width, dash ? Qt::DashLine : Qt::SolidLine));
    p.setBrush(Qt::NoBrush);
    p.setRenderHint(QPainter::Antialiasing, aa);
    fillPrimitives_helper(&p, (PrimitiveType) type, &m_primitives_100);
}

void tst_QPainter::strokePrimitives_1000()
{
    QFETCH(int, type);
    QFETCH(bool, aa);
    QFETCH(bool, dash);
    QFETCH(int, width);
    QPainter p(m_surface);
    p.setPen(QPen(Qt::red, width, dash ? Qt::DashLine : Qt::SolidLine));
    p.setBrush(Qt::NoBrush);
    p.setRenderHint(QPainter::Antialiasing, aa);
    fillPrimitives_helper(&p, (PrimitiveType) type, &m_primitives_1000);
}

void tst_QPainter::drawText_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("a") << QString::fromLatin1("a");
    QTest::newRow("ab") << QString::fromLatin1("ab");
    QTest::newRow("abc") << QString::fromLatin1("abc");
    QTest::newRow("abcd") << QString::fromLatin1("abcd");
    QTest::newRow("abcde") << QString::fromLatin1("abcde");
    QTest::newRow("abcdef") << QString::fromLatin1("abcdef");
    QTest::newRow("abcdefg") << QString::fromLatin1("abcdefg");
}

void tst_QPainter::drawText()
{
    QFETCH(QString, text);

    QPainter p(m_surface);

    QBENCHMARK {
        p.drawText(QPointF(5, 5), text);
    }
}

void tst_QPainter::saveRestore_data()
{
    QTest::addColumn<int>("change");

    for (int i=0; i<16; ++i) {
        QString change = "change=";
        if (i == 0) change += " none";
        if (i & ChangePen) change += " pen";
        if (i & ChangeBrush) change += " brush";
        if (i & ChangeClip) change += " clip";
        if (i & ChangeTransform) change += " xform";

        QTest::newRow(change.toLatin1()) << i;
    }
}

void tst_QPainter::saveRestore()
{
    QFETCH(int, change);

    QPen pen(Qt::blue);
    QBrush brush(Qt::green);
    QRect r(100, 100, 100, 20);

    QPainter p(m_surface);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::NoBrush);

    QBENCHMARK {
        p.save();
        if (change & ChangePen) { p.setPen(pen); p.setPen(Qt::NoPen); }
        if (change & ChangeBrush) { p.setBrush(brush); p.setBrush(Qt::NoBrush); }
        if (change & ChangeClip) p.setClipRect(r);
        if (change & ChangeTransform) { p.scale(3, 5); p.scale(1/3.0, 1/5.0); }
        p.drawRect(0, 0, 1, 1);
        p.restore();
    };
}

enum ClipType {
    RectClipType,
    RectPathClipType,
    RectRegionClipType,
    RegionClipType,
    PathClipType
};

void tst_QPainter::clipAndFill_data()
{
    QTest::addColumn<int>("type");

    QTest::newRow("rect") << (int) RectClipType;
    QTest::newRow("rectpath") << (int) RectPathClipType;
    QTest::newRow("rectregion") << (int) RectRegionClipType;
    QTest::newRow("ellipseRegion") << (int) RegionClipType;
    QTest::newRow("ellipsePath") << (int) PathClipType;
}


void tst_QPainter::clipAndFill()
{
    QFETCH(int, type);

    QRegion region;
    QPainterPath path;
    QRectF rect;

    switch (type) {
    case RectClipType:
        rect = QRectF(100, 100, 100, 100);
        break;
    case RectPathClipType:
        path.addRect(100, 100, 100, 100);
        break;
    case RectRegionClipType:
        region = QRegion(100, 100, 100, 100);
        break;
    case RegionClipType:
        region = QRegion(100, 100, 100, 100, QRegion::Ellipse);
        break;
    case PathClipType:
        path.addEllipse(100, 100, 100, 100);
        break;
    }

    QPainter p(m_surface);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::red);

    QBENCHMARK {
        if (type == RectClipType)
            p.setClipRect(rect);
        else if (type == RectPathClipType || type == PathClipType)
            p.setClipPath(path);
        else
            p.setClipRegion(region);
        p.drawRect(110, 110, 10, 10);
    }
}

QTransform tst_QPainter::transformForAngle(qreal angle)
{
    const qreal inv_dist_to_plane = 1. / 1024.;

    QTransform transform;

    QTransform rotTrans;
    rotTrans.translate(-40, 0);
    QTransform rotTrans2;
    rotTrans2.translate(40, 0);

    qreal rad = qDegreesToRadians(angle);
    qreal c = ::cos(rad);
    qreal s = ::sin(rad);

    qreal x = 0;
    qreal y = 80;
    qreal z = 0;

    qreal len = x * x + y * y + z * z;
    if (len != 1.) {
        len = ::sqrt(len);
        x /= len;
        y /= len;
        z /= len;
    }

    QTransform rot(x*x*(1-c)+c, x*y*(1-c)-z*s, x*z*(1-c)+y*s*inv_dist_to_plane,
                   y*x*(1-c)+z*s, y*y*(1-c)+c, y*z*(1-c)-x*s*inv_dist_to_plane,
                   0, 0, 1);

    transform *= rotTrans;
    transform *= rot;
    transform *= rotTrans2;

    return transform;
}

void tst_QPainter::drawRoundedRect()
{
    QImage surface(100, 100, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::red);

    QBENCHMARK {
        p.drawRoundedRect(QRectF(.5, .5, 80, 80), 10, 10);
    }
}

void tst_QPainter::drawScaledRoundedRect()
{
    QImage surface(400, 400, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::red);
    p.scale(3, 3);

    QBENCHMARK {
        p.drawRoundedRect(10, 10, 80, 80, 10, 10);
    }
}

void tst_QPainter::drawTransformedRoundedRect()
{
    QImage surface(400, 400, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::red);

    QBENCHMARK {
        p.setWorldTransform(QTransform(0.956957, 0, 0.000704124, 0, 1, 0, 16.141, 0, 0.735953));
        p.drawRoundedRect(100, 100, 80, 80, 10, 10);
    }
}

void tst_QPainter::drawAntialiasedRoundedRect()
{
    QImage surface(100, 100, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::red);

    QBENCHMARK {
        p.drawRoundedRect(QRectF(.5, .5, 80, 80), 10, 10);
    }
}

void tst_QPainter::drawScaledAntialiasedRoundedRect_data()
{
    QTest::addColumn<float>("scale");

    for (float i = 0; i < 3; i += .1f)
        QTest::newRow(QString(QLatin1String("scale=%1")).arg(i).toLatin1()) << i;
}

void tst_QPainter::drawScaledAntialiasedRoundedRect()
{
    QFETCH(float, scale);

    QImage surface(400, 400, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::red);
    p.scale(scale, scale);

    QBENCHMARK {
        p.drawRoundedRect(10, 10, 80, 80, 10, 10);
    }
}

void tst_QPainter::drawTransformedAntialiasedRoundedRect_data()
{
    QTest::addColumn<QTransform>("transform");

    for (float angle = 0; angle < 360; angle += 10)
        QTest::newRow(QString(QLatin1String("angle=%1")).arg(angle).toLatin1()) << transformForAngle(angle);
}

void tst_QPainter::drawTransformedAntialiasedRoundedRect()
{
    QFETCH(QTransform, transform);

    QImage surface(400, 400, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::red);

    QBENCHMARK {
        p.setWorldTransform(transform);
        p.drawRoundedRect(100, 100, 80, 80, 10, 10);
    }
}

void tst_QPainter::drawImageRoundedRect()
{
    //setup image
    const int radius = 10;
    QImage rectImage(81, 81, QImage::Format_ARGB32_Premultiplied);
    rectImage.fill(0);
    QPainter rp(&rectImage);
    rp.setRenderHint(QPainter::Antialiasing);
    rp.setPen(Qt::black);
    rp.setBrush(Qt::red);
    rp.drawRoundedRect(QRectF(.5, .5, 80, 80), radius, radius);

    //setup surface
    QImage surface(100, 100, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    QBENCHMARK {
        p.drawImage(0,0, rectImage);
    }
}

void tst_QPainter::drawScaledImageRoundedRect_data()
{
    QTest::addColumn<int>("imageType");

    QTest::newRow("imagetype=ARGB32_Pre") << (int)QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("imagetype=ARGB8565_Pre") << (int)QImage::Format_ARGB8565_Premultiplied;
}

void tst_QPainter::drawScaledImageRoundedRect()
{
    QFETCH(int, imageType);

    //setup image
    const int radius = 10;
    QImage rectImage(81, 81, (QImage::Format)imageType);
    rectImage.fill(0);
    QPainter rp(&rectImage);
    rp.setRenderHint(QPainter::Antialiasing);
    rp.setPen(Qt::black);
    rp.setBrush(Qt::red);
    rp.drawRoundedRect(QRectF(.5, .5, 80, 80), radius, radius);

    //setup surface
    QImage surface(400, 400, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);
    p.scale(3, 3);

    QBENCHMARK {
        p.drawImage(0,0, rectImage);
    }
}

void tst_QPainter::drawTransformedImageRoundedRect_data()
{
    QTest::addColumn<int>("imageType");

    QTest::newRow("imagetype=ARGB32_Pre") << (int)QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("imagetype=ARGB8565_Pre") << (int)QImage::Format_ARGB8565_Premultiplied;
}

void tst_QPainter::drawTransformedImageRoundedRect()
{
    QFETCH(int, imageType);

    //setup image
    const int radius = 10;
    QImage rectImage(81, 81, (QImage::Format)imageType);
    rectImage.fill(0);
    QPainter rp(&rectImage);
    rp.setRenderHint(QPainter::Antialiasing);
    rp.setPen(Qt::black);
    rp.setBrush(Qt::red);
    rp.drawRoundedRect(QRectF(.5, .5, 80, 80), radius, radius);

    //setup surface
    QImage surface(400, 400, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    QBENCHMARK {
        p.setWorldTransform(QTransform(0.956957, 0, 0.000704124, 0, 1, 0, 16.141, 0, 0.735953));
        p.drawImage(100,100, rectImage);
    }
}

//code from QDeclarativeRectangle for drawing rounded rects
void tst_QPainter::drawBorderPixmapRoundedRect()
{
    //setup image
    const int pw = 1;
    const int radius = 10;
    QImage rectImage(radius*2 + 3 + pw*2, radius*2 + 3 + pw*2, QImage::Format_ARGB32_Premultiplied);
    rectImage.fill(0);
    QPainter rp(&rectImage);
    rp.setRenderHint(QPainter::Antialiasing);
    rp.setPen(Qt::black);
    rp.setBrush(Qt::red);
    if (pw%2)
        rp.drawRoundedRect(QRectF(qreal(pw)/2+1, qreal(pw)/2+1, rectImage.width()-(pw+1), rectImage.height()-(pw+1)), radius, radius);
    else
        rp.drawRoundedRect(QRectF(qreal(pw)/2, qreal(pw)/2, rectImage.width()-pw, rectImage.height()-pw), radius, radius);
    QPixmap rectPixmap = rasterPixmap(rectImage);

    //setup surface
    QImage surface(100, 100, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    QBENCHMARK {
        const int pw = 2;
        int width = 80;
        int height = 80;

        int xOffset = (rectPixmap.width()-1)/2;
        int yOffset = (rectPixmap.height()-1)/2;
        Q_ASSERT(rectPixmap.width() == 2*xOffset + 1);
        Q_ASSERT(rectPixmap.height() == 2*yOffset + 1);

        QMargins margins(xOffset, yOffset, xOffset, yOffset);
        QTileRules rules(Qt::StretchTile, Qt::StretchTile);
        //NOTE: even though our item may have qreal-based width and height, qDrawBorderPixmap only supports QRects
        qDrawBorderPixmap(&p, QRect(-pw/2, -pw/2, width+pw, height+pw), margins, rectPixmap, rectPixmap.rect(), margins, rules);
    }
}

void tst_QPainter::drawScaledBorderPixmapRoundedRect_data()
{
    QTest::addColumn<float>("scale");
    QTest::addColumn<int>("imageType");

    for (float i = 0; i < 3; i += .1f)
        QTest::newRow(QString(QLatin1String("scale=%1; imagetype=ARGB32_Pre")).arg(i).toLatin1()) << i << (int)QImage::Format_ARGB32_Premultiplied;
    //for (float i = 0; i < 3; i += .1)
    //    QTest::newRow(QString(QLatin1String("scale=%1; imagetype=ARGB8565_Pre")).arg(i).toLatin1()) << i << (int)QImage::Format_ARGB8565_Premultiplied;
}

//code from QDeclarativeRectangle for drawing rounded rects
void tst_QPainter::drawScaledBorderPixmapRoundedRect()
{
    QFETCH(float, scale);
    QFETCH(int, imageType);

    //setup image
    const int pw = 1;
    const int radius = 10;
    QImage rectImage(radius*2 + 3 + pw*2, radius*2 + 3 + pw*2, (QImage::Format)imageType);
    rectImage.fill(0);
    QPainter rp(&rectImage);
    rp.setRenderHint(QPainter::Antialiasing);
    rp.setPen(Qt::black);
    rp.setBrush(Qt::red);
    if (pw%2)
        rp.drawRoundedRect(QRectF(qreal(pw)/2+1, qreal(pw)/2+1, rectImage.width()-(pw+1), rectImage.height()-(pw+1)), radius, radius);
    else
        rp.drawRoundedRect(QRectF(qreal(pw)/2, qreal(pw)/2, rectImage.width()-pw, rectImage.height()-pw), radius, radius);

    QPixmap rectPixmap = rasterPixmap(rectImage);

    //setup surface
    QImage surface(400, 400, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);
    p.scale(scale, scale);

    QBENCHMARK {
        const int pw = 2;
        int width = 80;
        int height = 80;

        int xOffset = (rectPixmap.width()-1)/2;
        int yOffset = (rectPixmap.height()-1)/2;
        Q_ASSERT(rectPixmap.width() == 2*xOffset + 1);
        Q_ASSERT(rectPixmap.height() == 2*yOffset + 1);

        QMargins margins(xOffset, yOffset, xOffset, yOffset);
        QTileRules rules(Qt::StretchTile, Qt::StretchTile);
        qDrawBorderPixmap(&p, QRect(-pw/2, -pw/2, width+pw, height+pw), margins, rectPixmap, rectPixmap.rect(), margins, rules);
    }
}

void tst_QPainter::drawTransformedBorderPixmapRoundedRect_data()
{
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<int>("imageType");

    for (float angle = 0; angle < 360; angle += 10)
        QTest::newRow(QString(QLatin1String("angle=%1; imagetype=ARGB32_Pre")).arg(angle).toLatin1()) << transformForAngle(angle) << (int)QImage::Format_ARGB32_Premultiplied;
    //for (float angle = 0; angle < 360; angle += 10)
    //    QTest::newRow(QString(QLatin1String("angle=%1; imagetype=ARGB8565_Pre")).arg(angle).toLatin1()) << transformForAngle(angle) << (int)QImage::Format_ARGB8565_Premultiplied;

}

//code from QDeclarativeRectangle for drawing rounded rects
void tst_QPainter::drawTransformedBorderPixmapRoundedRect()
{
    QFETCH(QTransform, transform);
    QFETCH(int, imageType);

    //setup image
    const int pw = 1;
    const int radius = 10;
    QImage rectImage(radius*2 + 3 + pw*2, radius*2 + 3 + pw*2, (QImage::Format)imageType);
    rectImage.fill(0);
    QPainter rp(&rectImage);
    rp.setRenderHint(QPainter::Antialiasing);
    rp.setPen(Qt::black);
    rp.setBrush(Qt::red);
    if (pw%2)
        rp.drawRoundedRect(QRectF(qreal(pw)/2+1, qreal(pw)/2+1, rectImage.width()-(pw+1), rectImage.height()-(pw+1)), radius, radius);
    else
        rp.drawRoundedRect(QRectF(qreal(pw)/2, qreal(pw)/2, rectImage.width()-pw, rectImage.height()-pw), radius, radius);

    QPixmap rectPixmap = rasterPixmap(rectImage);

    //setup surface
    QImage surface(400, 400, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    QBENCHMARK {
        p.setWorldTransform(transform);
        const int pw = 2;
        int width = 80;
        int height = 80;

        int xOffset = (rectPixmap.width()-1)/2;
        int yOffset = (rectPixmap.height()-1)/2;
        Q_ASSERT(rectPixmap.width() == 2*xOffset + 1);
        Q_ASSERT(rectPixmap.height() == 2*yOffset + 1);

        QMargins margins(xOffset, yOffset, xOffset, yOffset);
        QTileRules rules(Qt::StretchTile, Qt::StretchTile);
        qDrawBorderPixmap(&p, QRect(-pw/2, -pw/2, width+pw, height+pw), margins, rectPixmap, rectPixmap.rect(), margins, rules);
    }
}

void tst_QPainter::drawTransformedTransparentImage_data()
{
    QTest::addColumn<int>("imageType");

    QTest::newRow("imagetype=ARGB32_Pre") << (int)QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("imagetype=ARGB8565_Pre") << (int)QImage::Format_ARGB8565_Premultiplied;
}

void tst_QPainter::drawTransformedTransparentImage()
{
    QFETCH(int, imageType);

    //setup image
    QImage transImage(200, 200, (QImage::Format)imageType);
    transImage.fill(0);

    //setup surface
    QImage surface(200, 200, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    QBENCHMARK {
        p.setWorldTransform(QTransform(0.956957, 0, 0.000704124, 0, 1, 0, 16.141, 0, 0.735953));
        p.drawImage(0,0, transImage);
    }
}

void tst_QPainter::drawTransformedSemiTransparentImage_data()
{
    QTest::addColumn<int>("imageType");

    QTest::newRow("imagetype=ARGB32_Pre") << (int)QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("imagetype=ARGB8565_Pre") << (int)QImage::Format_ARGB8565_Premultiplied;
}

void tst_QPainter::drawTransformedSemiTransparentImage()
{
    QFETCH(int, imageType);

    //setup image
    QImage transImage(200, 200, (QImage::Format)imageType);
    transImage.fill(QColor(0,0,0, 128).rgba());

    //setup surface
    QImage surface(200, 200, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    QBENCHMARK {
        p.setWorldTransform(QTransform(0.956957, 0, 0.000704124, 0, 1, 0, 16.141, 0, 0.735953));
        p.drawImage(0,0, transImage);
    }
}

void tst_QPainter::drawTransformedFilledImage_data()
{
    QTest::addColumn<int>("imageType");

    QTest::newRow("imagetype=ARGB32_Pre") << (int)QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("imagetype=ARGB8565_Pre") << (int)QImage::Format_ARGB8565_Premultiplied;
}

void tst_QPainter::drawTransformedFilledImage()
{
    QFETCH(int, imageType);

    //setup image
    QImage filledImage(200, 200, (QImage::Format)imageType);
    filledImage.fill(QColor(0,0,0).rgb());

    //setup surface
    QImage surface(200, 200, QImage::Format_RGB16);
    surface.fill(QColor(255,255,255).rgb());
    QPainter p(&surface);

    QBENCHMARK {
        p.setWorldTransform(QTransform(0.956957, 0, 0.000704124, 0, 1, 0, 16.141, 0, 0.735953));
        p.drawImage(0,0, filledImage);
    }
}


QTEST_MAIN(tst_QPainter)

#include "tst_qpainter.moc"
