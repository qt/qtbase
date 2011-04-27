/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPAINTENGINE_DIRECTFB_P_H
#define QPAINTENGINE_DIRECTFB_P_H

#include <QtGui/qpaintengine.h>
#include <private/qpaintengine_raster_p.h>

#ifndef QT_NO_QWS_DIRECTFB

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QDirectFBPaintEnginePrivate;

class QDirectFBPaintEngine : public QRasterPaintEngine
{
    Q_DECLARE_PRIVATE(QDirectFBPaintEngine)
public:
    QDirectFBPaintEngine(QPaintDevice *device);
    virtual ~QDirectFBPaintEngine();

    virtual bool begin(QPaintDevice *device);
    virtual bool end();

    virtual void drawRects(const QRect  *rects, int rectCount);
    virtual void drawRects(const QRectF *rects, int rectCount);

    virtual void fillRect(const QRectF &r, const QBrush &brush);
    virtual void fillRect(const QRectF &r, const QColor &color);

    virtual void drawLines(const QLine *line, int lineCount);
    virtual void drawLines(const QLineF *line, int lineCount);

    virtual void drawImage(const QPointF &p, const QImage &img);
    virtual void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                           Qt::ImageConversionFlags falgs = Qt::AutoColor);

    virtual void drawPixmap(const QPointF &p, const QPixmap &pm);
    virtual void drawPixmap(const QRectF &r, const QPixmap &pixmap, const QRectF &sr);
    virtual void drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &sr);

    virtual void drawBufferSpan(const uint *buffer, int bufsize,
                                int x, int y, int length, uint const_alpha);

    virtual void stroke(const QVectorPath &path, const QPen &pen);
    virtual void drawPath(const QPainterPath &path);
    virtual void drawPoints(const QPointF *points, int pointCount);
    virtual void drawPoints(const QPoint *points, int pointCount);
    virtual void drawEllipse(const QRectF &rect);
    virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    virtual void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode);
    virtual void drawTextItem(const QPointF &p, const QTextItem &textItem);
    virtual void fill(const QVectorPath &path, const QBrush &brush);
    virtual void drawRoundedRect(const QRectF &rect, qreal xrad, qreal yrad, Qt::SizeMode mode);

    virtual void clipEnabledChanged();
    virtual void penChanged();
    virtual void opacityChanged();
    virtual void compositionModeChanged();
    virtual void renderHintsChanged();
    virtual void transformChanged();

    virtual void setState(QPainterState *state);

    virtual void clip(const QVectorPath &path, Qt::ClipOperation op);
    virtual void clip(const QRegion &region, Qt::ClipOperation op);
    virtual void clip(const QRect &rect, Qt::ClipOperation op);

    virtual void drawStaticTextItem(QStaticTextItem *item);

    static void initImageCache(int size);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QWS_DIRECTFB

#endif // QPAINTENGINE_DIRECTFB_P_H
