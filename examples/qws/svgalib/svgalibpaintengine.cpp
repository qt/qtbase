/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "svgalibpaintengine.h"

#include <QColor>
#include <vga.h>
#include <vgagl.h>

SvgalibPaintEngine::SvgalibPaintEngine(QPaintDevice *device)
    : QRasterPaintEngine(device)
{
}

SvgalibPaintEngine::~SvgalibPaintEngine()
{
}

//! [0]
bool SvgalibPaintEngine::begin(QPaintDevice *dev)
{
    device = dev;
    pen = Qt::NoPen;
    simplePen = true;
    brush = Qt::NoBrush;
    simpleBrush = true;
    matrix = QTransform();
    simpleMatrix = true;
    setClip(QRect(0, 0, device->width(), device->height()));
    opaque = true;
    aliased = true;
    sourceOver = true;

    return QRasterPaintEngine::begin(dev);
}
//! [0]

//! [1]
bool SvgalibPaintEngine::end()
{
    gl_setclippingwindow(0, 0, device->width() - 1, device->height() - 1);
    return QRasterPaintEngine::end();
}
//! [1]

//! [2]
void SvgalibPaintEngine::updateState()
{
    QRasterPaintEngineState *s = state();

    if (s->dirty & DirtyTransform) {
        matrix = s->matrix;
        simpleMatrix = (matrix.m12() == 0 && matrix.m21() == 0);
    }

    if (s->dirty & DirtyPen) {
        pen = s->pen;
        simplePen = (pen.width() == 0 || pen.widthF() <= 1)
                    && (pen.style() == Qt::NoPen || pen.style() == Qt::SolidLine)
                    && (pen.color().alpha() == 255);
    }

    if (s->dirty & DirtyBrush) {
        brush = s->brush;
        simpleBrush = (brush.style() == Qt::SolidPattern
                       || brush.style() == Qt::NoBrush)
                      && (brush.color().alpha() == 255);
    }

    if (s->dirty & DirtyClipRegion)
        setClip(s->clipRegion);

    if (s->dirty & DirtyClipEnabled) {
        clipEnabled = s->isClipEnabled();
        updateClip();
    }

    if (s->dirty & DirtyClipPath) {
        setClip(QRegion());
        simpleClip = false;
    }

    if (s->dirty & DirtyCompositionMode) {
        const QPainter::CompositionMode m = s->composition_mode;
        sourceOver = (m == QPainter::CompositionMode_SourceOver);
    }

    if (s->dirty & DirtyOpacity)
        opaque = (s->opacity == 256);

    if (s->dirty & DirtyHints)
        aliased = !(s->flags.antialiased);
}
//! [2]

//! [3]
void SvgalibPaintEngine::setClip(const QRegion &region)
{
    if (region.isEmpty())
        clip = QRect(0, 0, device->width(), device->height());
    else
        clip = matrix.map(region) & QRect(0, 0, device->width(), device->height());
    clipEnabled = true;
    updateClip();
}
//! [3]

//! [4]
void SvgalibPaintEngine::updateClip()
{
    QRegion clipRegion = QRect(0, 0, device->width(), device->height());

    if (!systemClip().isEmpty())
        clipRegion &= systemClip();
    if (clipEnabled)
        clipRegion &= clip;

    simpleClip = (clipRegion.rects().size() <= 1);

    const QRect r = clipRegion.boundingRect();
    gl_setclippingwindow(r.left(), r.top(),
                         r.x() + r.width(),
                         r.y() + r.height());
}
//! [4]

//! [5]
void SvgalibPaintEngine::drawRects(const QRect *rects, int rectCount)
{
    const bool canAccelerate = simplePen && simpleBrush && simpleMatrix
                               && simpleClip && opaque && aliased
                               && sourceOver;
    if (!canAccelerate) {
        QRasterPaintEngine::drawRects(rects, rectCount);
        return;
    }

    for (int i = 0; i < rectCount; ++i) {
        const QRect r = matrix.mapRect(rects[i]);
        if (brush != Qt::NoBrush) {
            gl_fillbox(r.left(), r.top(), r.width(), r.height(),
                       brush.color().rgba());
        }
        if (pen != Qt::NoPen) {
            const int c = pen.color().rgba();
            gl_hline(r.left(), r.top(), r.right(), c);
            gl_hline(r.left(), r.bottom(), r.right(), c);
            gl_line(r.left(), r.top(), r.left(), r.bottom(), c);
            gl_line(r.right(), r.top(), r.right(), r.bottom(), c);
        }
    }
}
//! [5]
