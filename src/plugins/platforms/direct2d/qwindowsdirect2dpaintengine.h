/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSDIRECT2DPAINTENGINE_H
#define QWINDOWSDIRECT2DPAINTENGINE_H

#include <QtCore/QScopedPointer>
#include <QtGui/QPaintEngine>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPaintEnginePrivate;
class QWindowsDirect2DBitmap;

class QWindowsDirect2DPaintEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DPaintEngine)

public:
    QWindowsDirect2DPaintEngine(QWindowsDirect2DBitmap *bitmap);

    bool begin(QPaintDevice *pdev) Q_DECL_OVERRIDE;
    bool end() Q_DECL_OVERRIDE;

    void updateState(const QPaintEngineState &state) Q_DECL_OVERRIDE;

    Type type() const Q_DECL_OVERRIDE;

    void drawEllipse(const QRectF &rect) Q_DECL_OVERRIDE;
    void drawEllipse(const QRect &rect) Q_DECL_OVERRIDE;
    void drawImage(const QRectF &rectangle, const QImage &image, const QRectF &sr, Qt::ImageConversionFlags flags = Qt::AutoColor) Q_DECL_OVERRIDE;
    void drawLines(const QLineF *lines, int lineCount) Q_DECL_OVERRIDE;
    void drawLines(const QLine *lines, int lineCount) Q_DECL_OVERRIDE;
    void drawPath(const QPainterPath &path) Q_DECL_OVERRIDE;
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) Q_DECL_OVERRIDE;
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) Q_DECL_OVERRIDE;
    void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode) Q_DECL_OVERRIDE;
    void drawRects(const QRectF *rects, int rectCount) Q_DECL_OVERRIDE;
    void drawRects(const QRect *rects, int rectCount) Q_DECL_OVERRIDE;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) Q_DECL_OVERRIDE;
    void drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &p) Q_DECL_OVERRIDE;

private:
    QScopedPointer<QWindowsDirect2DPaintEnginePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DPAINTENGINE_H
