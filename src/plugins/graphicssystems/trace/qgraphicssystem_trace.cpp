/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qgraphicssystem_trace_p.h"
#include <private/qpixmap_raster_p.h>
#include <private/qpaintbuffer_p.h>
#include <private/qwindowsurface_raster_p.h>

#include <QFile>
#include <QPainter>
#include <QtDebug>

QT_BEGIN_NAMESPACE

class QTraceWindowSurface : public QRasterWindowSurface
{
public:
    QTraceWindowSurface(QWidget *widget);
    ~QTraceWindowSurface();

    QPaintDevice *paintDevice();
    void beginPaint(const QRegion &rgn);
    void endPaint(const QRegion &rgn);

    bool scroll(const QRegion &area, int dx, int dy);

private:
    QPaintBuffer *buffer;
    QList<QRegion> updates;

    qulonglong winId;
};

QTraceWindowSurface::QTraceWindowSurface(QWidget *widget)
    : QRasterWindowSurface(widget)
    , buffer(0)
    , winId(0)
{
}

QTraceWindowSurface::~QTraceWindowSurface()
{
    if (buffer) {
        QFile outputFile(QString(QLatin1String("qtgraphics-%0.trace")).arg(winId));
        if (outputFile.open(QIODevice::WriteOnly)) {
            QDataStream out(&outputFile);
            out.setFloatingPointPrecision(QDataStream::SinglePrecision);

            out.writeBytes("qttraceV2", 9);

            uint version = 1;

            out << version << *buffer << updates;
        }
        delete buffer;
    }
}

QPaintDevice *QTraceWindowSurface::paintDevice()
{
    if (!buffer) {
        buffer = new QPaintBuffer;
#ifdef Q_WS_QPA
        buffer->setBoundingRect(QRect(QPoint(), size()));
#else
        buffer->setBoundingRect(geometry());
#endif
    }
    return buffer;
}

void QTraceWindowSurface::beginPaint(const QRegion &rgn)
{
    // ensure paint buffer is created
    paintDevice();
    buffer->beginNewFrame();

    QRasterWindowSurface::beginPaint(rgn);
}

void QTraceWindowSurface::endPaint(const QRegion &rgn)
{
    QPainter p(QRasterWindowSurface::paintDevice());
    buffer->draw(&p, buffer->numFrames()-1);
    p.end();

    winId = (qulonglong)window()->winId();

    updates << rgn;

    QRasterWindowSurface::endPaint(rgn);
}

bool QTraceWindowSurface::scroll(const QRegion &, int, int)
{
    // TODO: scrolling should also be streamed and replayed
    // to test scrolling performance
    return false;
}

QTraceGraphicsSystem::QTraceGraphicsSystem()
{
}

QPixmapData *QTraceGraphicsSystem::createPixmapData(QPixmapData::PixelType type) const
{
    return new QRasterPixmapData(type);
}

QWindowSurface *QTraceGraphicsSystem::createWindowSurface(QWidget *widget) const
{
    return new QTraceWindowSurface(widget);
}

QT_END_NAMESPACE
