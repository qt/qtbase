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

#include "qdirectfbscreen.h"
#include "qdirectfbpaintdevice.h"
#include "qdirectfbpaintengine.h"

#ifndef QT_NO_QWS_DIRECTFB

QT_BEGIN_NAMESPACE

QDirectFBPaintDevice::QDirectFBPaintDevice(QDirectFBScreen *scr)
    : QCustomRasterPaintDevice(0), dfbSurface(0), screen(scr),
      bpl(-1), lockFlgs(DFBSurfaceLockFlags(0)), mem(0), engine(0), imageFormat(QImage::Format_Invalid)
{
#ifdef QT_DIRECTFB_SUBSURFACE
    subSurface = 0;
    syncPending = false;
#endif
}

QDirectFBPaintDevice::~QDirectFBPaintDevice()
{
    if (QDirectFBScreen::instance()) {
        unlockSurface();
#ifdef QT_DIRECTFB_SUBSURFACE
        releaseSubSurface();
#endif
        if (dfbSurface) {
            screen->releaseDFBSurface(dfbSurface);
        }
    }
    delete engine;
}

IDirectFBSurface *QDirectFBPaintDevice::directFBSurface() const
{
    return dfbSurface;
}

bool QDirectFBPaintDevice::lockSurface(DFBSurfaceLockFlags lockFlags)
{
    if (lockFlgs && (lockFlags & ~lockFlgs))
        unlockSurface();
    if (!mem) {
        Q_ASSERT(dfbSurface);
#ifdef QT_DIRECTFB_SUBSURFACE
        if (!subSurface) {
            DFBResult result;
            subSurface = screen->getSubSurface(dfbSurface, QRect(), QDirectFBScreen::TrackSurface, &result);
            if (result != DFB_OK || !subSurface) {
                DirectFBError("Couldn't create sub surface", result);
                return false;
            }
        }
        IDirectFBSurface *surface = subSurface;
#else
        IDirectFBSurface *surface = dfbSurface;
#endif
        Q_ASSERT(surface);
        mem = QDirectFBScreen::lockSurface(surface, lockFlags, &bpl);
        lockFlgs = lockFlags;
        Q_ASSERT(mem);
        Q_ASSERT(bpl > 0);
        const QSize s = size();
        lockedImage = QImage(mem, s.width(), s.height(), bpl,
                             QDirectFBScreen::getImageFormat(dfbSurface));
        return true;
    }
#ifdef QT_DIRECTFB_SUBSURFACE
    if (syncPending) {
        syncPending = false;
        screen->waitIdle();
    }
#endif
    return false;
}

void QDirectFBPaintDevice::unlockSurface()
{
    if (QDirectFBScreen::instance() && lockFlgs) {
#ifdef QT_DIRECTFB_SUBSURFACE
        IDirectFBSurface *surface = subSurface;
#else
        IDirectFBSurface *surface = dfbSurface;
#endif
        if (surface) {
            surface->Unlock(surface);
            lockFlgs = static_cast<DFBSurfaceLockFlags>(0);
            mem = 0;
        }
    }
}

void *QDirectFBPaintDevice::memory() const
{
    return mem;
}

QImage::Format QDirectFBPaintDevice::format() const
{
    return imageFormat;
}

int QDirectFBPaintDevice::bytesPerLine() const
{
    Q_ASSERT(!mem || bpl != -1);
    return bpl;
}

QSize QDirectFBPaintDevice::size() const
{
    int w, h;
    dfbSurface->GetSize(dfbSurface, &w, &h);
    return QSize(w, h);
}

int QDirectFBPaintDevice::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    if (!dfbSurface)
        return 0;

    switch (metric) {
    case QPaintDevice::PdmWidth:
    case QPaintDevice::PdmHeight:
        return (metric == PdmWidth ? size().width() : size().height());
    case QPaintDevice::PdmWidthMM:
        return (size().width() * 1000) / dotsPerMeterX();
    case QPaintDevice::PdmHeightMM:
        return (size().height() * 1000) / dotsPerMeterY();
    case QPaintDevice::PdmPhysicalDpiX:
    case QPaintDevice::PdmDpiX:
        return (dotsPerMeterX() * 254) / 10000; // 0.0254 meters-per-inch
    case QPaintDevice::PdmPhysicalDpiY:
    case QPaintDevice::PdmDpiY:
        return (dotsPerMeterY() * 254) / 10000; // 0.0254 meters-per-inch
    case QPaintDevice::PdmDepth:
        return QDirectFBScreen::depth(imageFormat);
    case QPaintDevice::PdmNumColors: {
        if (!lockedImage.isNull())
            return lockedImage.colorCount();

        DFBResult result;
        IDirectFBPalette *palette = 0;
        unsigned int numColors = 0;

        result = dfbSurface->GetPalette(dfbSurface, &palette);
        if ((result != DFB_OK) || !palette)
            return 0;

        result = palette->GetSize(palette, &numColors);
        palette->Release(palette);
        if (result != DFB_OK)
            return 0;

        return numColors;
    }
    default:
        qCritical("QDirectFBPaintDevice::metric(): Unhandled metric!");
        return 0;
    }
}

QPaintEngine *QDirectFBPaintDevice::paintEngine() const
{
    return engine;
}

#ifdef QT_DIRECTFB_SUBSURFACE
void QDirectFBPaintDevice::releaseSubSurface()
{
    Q_ASSERT(QDirectFBScreen::instance());
    if (subSurface) {
        unlockSurface();
        screen->releaseDFBSurface(subSurface);
        subSurface = 0;
    }
}
#endif

QT_END_NAMESPACE

#endif // QT_NO_QWS_DIRECTFB
