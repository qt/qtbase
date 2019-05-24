/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QWINDOWSDIRECT2DPAINTENGINE_H
#define QWINDOWSDIRECT2DPAINTENGINE_H

#include <QtCore/qscopedpointer.h>
#include <QtGui/private/qpaintengineex_p.h>

struct ID2D1Geometry;

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPaintEnginePrivate;
class QWindowsDirect2DBitmap;

class QWindowsDirect2DPaintEngine : public QPaintEngineEx
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DPaintEngine)
    friend class QWindowsDirect2DPaintEngineSuspenderImpl;
    friend class QWindowsDirect2DPaintEngineSuspenderPrivate;
public:
    enum Flag {
        NoFlag = 0,
        TranslucentTopLevelWindow = 1,
        EmulateComposition = 2,
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QWindowsDirect2DPaintEngine(QWindowsDirect2DBitmap *bitmap, Flags flags);

    bool begin(QPaintDevice *pdev) override;
    bool end() override;

    Type type() const override;

    void setState(QPainterState *s) override;

    void draw(const QVectorPath &path) override;

    void fill(const QVectorPath &path, const QBrush &brush) override;
    void fill(ID2D1Geometry *geometry, const QBrush &brush);

    void stroke(const QVectorPath &path, const QPen &pen) override;
    void stroke(ID2D1Geometry *geometry, const QPen &pen);

    void clip(const QVectorPath &path, Qt::ClipOperation op) override;

    void clipEnabledChanged() override;
    void penChanged() override;
    void brushChanged() override;
    void brushOriginChanged() override;
    void opacityChanged() override;
    void compositionModeChanged() override;
    void renderHintsChanged() override;
    void transformChanged() override;

    void fillRect(const QRectF &rect, const QBrush &brush) override;

    void drawRects(const QRect *rects, int rectCount) override;
    void drawRects(const QRectF *rects, int rectCount) override;

    void drawEllipse(const QRectF &r) override;
    void drawEllipse(const QRect &r) override;

    void drawImage(const QRectF &rectangle, const QImage &image, const QRectF &sr, Qt::ImageConversionFlags flags = Qt::AutoColor) override;
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;

    void drawStaticTextItem(QStaticTextItem *staticTextItem) override;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) override;

private:
    void ensureBrush();
    void ensureBrush(const QBrush &brush);
    void ensurePen();
    void ensurePen(const QPen &pen);

    void rasterFill(const QVectorPath &path, const QBrush &brush);

    enum EmulationType { PenEmulation, BrushEmulation };
    bool emulationRequired(EmulationType type) const;

    bool antiAliasingEnabled() const;
    void adjustForAliasing(QRectF *rect);
    void adjustForAliasing(QPointF *point);

    void suspend();
    void resume();
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowsDirect2DPaintEngine::Flags)

class QWindowsDirect2DPaintEngineSuspenderPrivate;
class QWindowsDirect2DPaintEngineSuspender
{
    Q_DISABLE_COPY_MOVE(QWindowsDirect2DPaintEngineSuspender)
    Q_DECLARE_PRIVATE(QWindowsDirect2DPaintEngineSuspender)
    QScopedPointer<QWindowsDirect2DPaintEngineSuspenderPrivate> d_ptr;
public:
    QWindowsDirect2DPaintEngineSuspender(QWindowsDirect2DPaintEngine *engine);
    ~QWindowsDirect2DPaintEngineSuspender();
    void resume();
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DPAINTENGINE_H
