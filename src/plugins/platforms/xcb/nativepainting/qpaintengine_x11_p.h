/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QPAINTENGINE_X11_H
#define QPAINTENGINE_X11_H

#include <QtGui/QPaintEngine>

typedef unsigned long XID;
typedef XID Drawable;
typedef struct _XGC *GC;

QT_BEGIN_NAMESPACE

extern "C" {
Drawable qt_x11Handle(const QPaintDevice *pd);
GC qt_x11_get_pen_gc(QPainter *);
GC qt_x11_get_brush_gc(QPainter *);
}

class QX11PaintEnginePrivate;
class QX11PaintEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QX11PaintEngine)
public:
    QX11PaintEngine();
    ~QX11PaintEngine();

    bool begin(QPaintDevice *pdev) Q_DECL_OVERRIDE;
    bool end() Q_DECL_OVERRIDE;

    void updateState(const QPaintEngineState &state) Q_DECL_OVERRIDE;

    void updatePen(const QPen &pen);
    void updateBrush(const QBrush &brush, const QPointF &pt);
    void updateRenderHints(QPainter::RenderHints hints);
    void updateFont(const QFont &font);
    void updateMatrix(const QTransform &matrix);
    void updateClipRegion_dev(const QRegion &region, Qt::ClipOperation op);

    void drawLines(const QLine *lines, int lineCount) Q_DECL_OVERRIDE;
    void drawLines(const QLineF *lines, int lineCount) Q_DECL_OVERRIDE;

    void drawRects(const QRect *rects, int rectCount) Q_DECL_OVERRIDE;
    void drawRects(const QRectF *rects, int rectCount) Q_DECL_OVERRIDE;

    void drawPoints(const QPoint *points, int pointCount) Q_DECL_OVERRIDE;
    void drawPoints(const QPointF *points, int pointCount) Q_DECL_OVERRIDE;

    void drawEllipse(const QRect &r) Q_DECL_OVERRIDE;
    void drawEllipse(const QRectF &r) Q_DECL_OVERRIDE;

    virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) Q_DECL_OVERRIDE;
    inline void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode) Q_DECL_OVERRIDE
        { QPaintEngine::drawPolygon(points, pointCount, mode); }

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) Q_DECL_OVERRIDE;
    void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s) Q_DECL_OVERRIDE;
    void drawPath(const QPainterPath &path) Q_DECL_OVERRIDE;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) Q_DECL_OVERRIDE;
    void drawImage(const QRectF &r, const QImage &img, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor) Q_DECL_OVERRIDE;

    virtual Drawable handle() const;
    inline Type type() const Q_DECL_OVERRIDE { return QPaintEngine::X11; }

    QPainter::RenderHints supportedRenderHints() const;

protected:
    QX11PaintEngine(QX11PaintEnginePrivate &dptr);

#if QT_CONFIG(fontconfig)
    void drawFreetype(const QPointF &p, const QTextItemInt &ti);
    bool drawCachedGlyphs(const QTransform &transform, const QTextItemInt &ti);
#endif // QT_CONFIG(fontconfig)

    friend class QPixmap;
    friend class QFontEngineBox;
    friend GC qt_x11_get_pen_gc(QPainter *);
    friend GC qt_x11_get_brush_gc(QPainter *);

private:
    Q_DISABLE_COPY(QX11PaintEngine)
};

QT_END_NAMESPACE

#endif // QPAINTENGINE_X11_H
