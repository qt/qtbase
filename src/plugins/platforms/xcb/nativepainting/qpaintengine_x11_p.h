/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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

    bool begin(QPaintDevice *pdev) override;
    bool end() override;

    void updateState(const QPaintEngineState &state) override;

    void updatePen(const QPen &pen);
    void updateBrush(const QBrush &brush, const QPointF &pt);
    void updateRenderHints(QPainter::RenderHints hints);
    void updateFont(const QFont &font);
    void updateMatrix(const QTransform &matrix);
    void updateClipRegion_dev(const QRegion &region, Qt::ClipOperation op);

    void drawLines(const QLine *lines, int lineCount) override;
    void drawLines(const QLineF *lines, int lineCount) override;

    void drawRects(const QRect *rects, int rectCount) override;
    void drawRects(const QRectF *rects, int rectCount) override;

    void drawPoints(const QPoint *points, int pointCount) override;
    void drawPoints(const QPointF *points, int pointCount) override;

    void drawEllipse(const QRect &r) override;
    void drawEllipse(const QRectF &r) override;

    virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
    inline void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode) override
        { QPaintEngine::drawPolygon(points, pointCount, mode); }

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
    void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s) override;
    void drawPath(const QPainterPath &path) override;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) override;
    void drawImage(const QRectF &r, const QImage &img, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor) override;

    virtual Drawable handle() const;
    inline Type type() const override { return QPaintEngine::X11; }

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
    Q_DISABLE_COPY_MOVE(QX11PaintEngine)
};

QT_END_NAMESPACE

#endif // QPAINTENGINE_X11_H
