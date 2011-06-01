/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPAINTENGINE_VG_P_H
#define QPAINTENGINE_VG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qpaintengineex_p.h>
#include <QtGui/private/qtextureglyphcache_p.h>

QT_BEGIN_NAMESPACE

struct QFixedPoint;
class QVGPaintEnginePrivate;
class QPixmapData;
class QVGEGLWindowSurfacePrivate;

class Q_OPENVG_EXPORT QVGPainterState : public QPainterState
{
public:
    QVGPainterState(QVGPainterState& other);
    QVGPainterState();
    ~QVGPainterState();

    bool isNew;
    QRegion clipRegion;
    QPaintEngine::DirtyFlags savedDirty;
};

class Q_OPENVG_EXPORT QVGPaintEngine : public QPaintEngineEx
{
    Q_DECLARE_PRIVATE(QVGPaintEngine)
public:
    QVGPaintEngine();
    ~QVGPaintEngine();

    Type type() const { return OpenVG; }

    QPainterState *createState(QPainterState *orig) const;

    bool begin(QPaintDevice *pdev);
    bool end();

    void draw(const QVectorPath &path);
    void fill(const QVectorPath &path, const QBrush &brush);
    void stroke(const QVectorPath &path, const QPen &pen);

    void clip(const QVectorPath &path, Qt::ClipOperation op);
    void clip(const QRect &rect, Qt::ClipOperation op);
    void clip(const QRegion &region, Qt::ClipOperation op);
    void clip(const QPainterPath &path, Qt::ClipOperation op);

    void clipEnabledChanged();
    void penChanged();
    void brushChanged();
    void brushOriginChanged();
    void opacityChanged();
    void compositionModeChanged();
    void renderHintsChanged();
    void transformChanged();

    void fillRect(const QRectF &rect, const QBrush &brush);
    void fillRect(const QRectF &rect, const QColor &color);

    void drawRoundedRect(const QRectF &rect, qreal xrad, qreal yrad, Qt::SizeMode mode);

    void drawRects(const QRect *rects, int rectCount);
    void drawRects(const QRectF *rects, int rectCount);

    void drawLines(const QLine *lines, int lineCount);
    void drawLines(const QLineF *lines, int lineCount);

    void drawEllipse(const QRectF &r);
    void drawEllipse(const QRect &r);

    void drawPath(const QPainterPath &path);

    void drawPoints(const QPointF *points, int pointCount);
    void drawPoints(const QPoint *points, int pointCount);

    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode);

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    void drawPixmap(const QPointF &pos, const QPixmap &pm);

    void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor);
    void drawImage(const QPointF &pos, const QImage &image);

    void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s);

    void drawPixmapFragments(const QPainter::PixmapFragment *drawingData, int dataCount, const QPixmap &pixmap,
                             QFlags<QPainter::PixmapFragmentHint> hints);

    void drawTextItem(const QPointF &p, const QTextItem &textItem);
    void drawStaticTextItem(QStaticTextItem *staticTextItem);
    bool drawCachedGlyphs(int numGlyphs, const glyph_t *glyphs, const QFont &font,
                          QFontEngine *fontEngine, const QPointF &p,
                          const QFixedPoint *positions);

    void setState(QPainterState *s);
    QVGPainterState *state() { return static_cast<QVGPainterState *>(QPaintEngineEx::state()); }
    const QVGPainterState *state() const { return static_cast<const QVGPainterState *>(QPaintEngineEx::state()); }

    void beginNativePainting();
    void endNativePainting();

    QPixmapFilter *pixmapFilter(int type, const QPixmapFilter *prototype);

    QVGPaintEnginePrivate *vgPrivate() { Q_D(QVGPaintEngine); return d; }

    void fillRegion(const QRegion& region, const QColor& color, const QSize& surfaceSize);
    bool supportsTransformations(qreal, const QTransform &) const { return true; }

protected:
    QVGPaintEngine(QVGPaintEnginePrivate &data);

private:
    void restoreState(QPaintEngine::DirtyFlags dirty);
    void updateScissor();
    QRegion defaultClipRegion();
    bool isDefaultClipRegion(const QRegion& region);
    bool isDefaultClipRect(const QRect& rect);
    bool clearRect(const QRectF &rect, const QColor &color);
    bool canVgWritePixels(const QImage &image) const;
};

QT_END_NAMESPACE

#endif
