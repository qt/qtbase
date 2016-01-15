/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <private/qpaintengine_preview_p.h>
#include <private/qpainter_p.h>
#include <private/qpaintengine_p.h>
#include <private/qpicture_p.h>

#include <QtPrintSupport/qprintengine.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpicture.h>

#ifndef QT_NO_PRINTPREVIEWWIDGET
QT_BEGIN_NAMESPACE

class QPreviewPaintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QPreviewPaintEngine)
public:
    QPreviewPaintEnginePrivate() : state(QPrinter::Idle) {}
    ~QPreviewPaintEnginePrivate() {}

    QList<const QPicture *> pages;
    QPaintEngine *engine;
    QPainter *painter;
    QPrinter::PrinterState state;

    QPaintEngine *proxy_paint_engine;
    QPrintEngine *proxy_print_engine;
};


QPreviewPaintEngine::QPreviewPaintEngine()
    : QPaintEngine(*(new QPreviewPaintEnginePrivate), PaintEngineFeatures(AllFeatures & ~ObjectBoundingModeGradients))
{
    Q_D(QPreviewPaintEngine);
    d->proxy_print_engine = 0;
    d->proxy_paint_engine = 0;
}

QPreviewPaintEngine::~QPreviewPaintEngine()
{
    Q_D(QPreviewPaintEngine);

    qDeleteAll(d->pages);
}

bool QPreviewPaintEngine::begin(QPaintDevice *)
{
    Q_D(QPreviewPaintEngine);

    qDeleteAll(d->pages);
    d->pages.clear();

    QPicture *page = new QPicture;
    page->d_func()->in_memory_only = true;
    d->painter = new QPainter(page);
    d->engine = d->painter->paintEngine();
    *d->painter->d_func()->state = *painter()->d_func()->state;
    d->pages.append(page);
    d->state = QPrinter::Active;
    return true;
}

bool QPreviewPaintEngine::end()
{
    Q_D(QPreviewPaintEngine);

    delete d->painter;
    d->painter = 0;
    d->engine = 0;
    d->state = QPrinter::Idle;
    return true;
}

void QPreviewPaintEngine::updateState(const QPaintEngineState &state)
{
    Q_D(QPreviewPaintEngine);
    d->engine->updateState(state);
}

void QPreviewPaintEngine::drawPath(const QPainterPath &path)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawPath(path);
}

void QPreviewPaintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawPolygon(points, pointCount, mode);
}

void QPreviewPaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawTextItem(p, textItem);
}

void QPreviewPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawPixmap(r, pm, sr);
}

void QPreviewPaintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &p)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawTiledPixmap(r, pm, p);
}

bool QPreviewPaintEngine::newPage()
{
    Q_D(QPreviewPaintEngine);

    QPicture *page = new QPicture;
    page->d_func()->in_memory_only = true;
    QPainter *tmp_painter = new QPainter(page);
    QPaintEngine *tmp_engine = tmp_painter->paintEngine();

    // copy the painter state from the original painter
    Q_ASSERT(painter()->d_func()->state && tmp_painter->d_func()->state);
    *tmp_painter->d_func()->state = *painter()->d_func()->state;

    // composition modes aren't supported on a QPrinter and yields a
    // warning, so ignore it for now
    tmp_engine->setDirty(DirtyFlags(AllDirty & ~DirtyCompositionMode));
    tmp_engine->syncState();

    delete d->painter;
    d->painter = tmp_painter;
    d->pages.append(page);
    d->engine = tmp_engine;
    return true;
}

bool QPreviewPaintEngine::abort()
{
    Q_D(QPreviewPaintEngine);
    end();
    qDeleteAll(d->pages);
    d->state = QPrinter::Aborted;

    return true;
}

QList<const QPicture *> QPreviewPaintEngine::pages()
{
    Q_D(QPreviewPaintEngine);
    return d->pages;
}

void QPreviewPaintEngine::setProxyEngines(QPrintEngine *printEngine, QPaintEngine *paintEngine)
{
    Q_D(QPreviewPaintEngine);
    d->proxy_print_engine = printEngine;
    d->proxy_paint_engine = paintEngine;
}

void QPreviewPaintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    Q_D(QPreviewPaintEngine);
    d->proxy_print_engine->setProperty(key, value);
}

QVariant QPreviewPaintEngine::property(PrintEnginePropertyKey key) const
{
    Q_D(const QPreviewPaintEngine);
    return d->proxy_print_engine->property(key);
}

int QPreviewPaintEngine::metric(QPaintDevice::PaintDeviceMetric id) const
{
    Q_D(const QPreviewPaintEngine);
    return d->proxy_print_engine->metric(id);
}

QPrinter::PrinterState QPreviewPaintEngine::printerState() const
{
    Q_D(const QPreviewPaintEngine);
    return d->state;
}

QT_END_NAMESPACE

#endif
