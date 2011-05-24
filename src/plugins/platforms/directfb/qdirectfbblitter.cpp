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

#include "qdirectfbblitter.h"
#include "qdirectfbconvenience.h"

#include <QtGui/private/qpixmap_blitter_p.h>

#include <QDebug>

#include <directfb.h>

QDirectFbBlitter::QDirectFbBlitter(const QSize &rect, IDirectFBSurface *surface)
        : QBlittable(rect, QBlittable::Capabilities(QBlittable::SolidRectCapability
                                                          |QBlittable::SourcePixmapCapability
                                                          |QBlittable::SourceOverPixmapCapability
                                                          |QBlittable::SourceOverScaledPixmapCapability))
{
    if (surface) {
        m_surface = surface;
    } else {
        DFBSurfaceDescription surfaceDesc;
        memset(&surfaceDesc,0,sizeof(DFBSurfaceDescription));
        surfaceDesc.width = rect.width();
        surfaceDesc.height = rect.height();
        surfaceDesc.caps = DSCAPS_PREMULTIPLIED;
        surfaceDesc.pixelformat = DSPF_ARGB;
        surfaceDesc.flags = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_CAPS | DSDESC_PIXELFORMAT);

        IDirectFB *dfb = QDirectFbConvenience::dfbInterface();
        dfb->CreateSurface(dfb,&surfaceDesc, &m_surface);
        m_surface->Clear(m_surface,0,0,0,0);
    }

}

QDirectFbBlitter::~QDirectFbBlitter()
{
    unlock();
    m_surface->Release(m_surface);
}

void QDirectFbBlitter::fillRect(const QRectF &rect, const QColor &color)
{
    m_surface->SetColor(m_surface, color.red(), color.green(), color.blue(), color.alpha());
//    When the blitter api supports non opaque blits, also remember to change
//    qpixmap_blitter.cpp::fill
//    DFBSurfaceDrawingFlags drawingFlags = color.alpha() ? DSDRAW_BLEND : DSDRAW_NOFX;
//    m_surface->SetDrawingFlags(m_surface, drawingFlags);
    m_surface->SetDrawingFlags(m_surface, DSDRAW_NOFX);
    m_surface->FillRectangle(m_surface, rect.x(), rect.y(),
                              rect.width(), rect.height());
}

void QDirectFbBlitter::drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &srcRect)
{
    QPixmapData *data = pixmap.pixmapData();
    Q_ASSERT(data->width() && data->height());
    Q_ASSERT(data->classId() == QPixmapData::BlitterClass);
    QBlittablePixmapData *blitPm = static_cast<QBlittablePixmapData*>(data);
    QDirectFbBlitter *dfbBlitter = static_cast<QDirectFbBlitter *>(blitPm->blittable());
    dfbBlitter->unlock();

    IDirectFBSurface *s = dfbBlitter->m_surface;

    DFBSurfaceBlittingFlags blittingFlags = DSBLIT_NOFX;
    DFBSurfacePorterDuffRule porterDuff = DSPD_SRC;
    if (pixmap.hasAlpha()) {
        blittingFlags = DSBLIT_BLEND_ALPHACHANNEL;
        porterDuff = DSPD_SRC_OVER;
    }

    m_surface->SetBlittingFlags(m_surface, DFBSurfaceBlittingFlags(blittingFlags));
    m_surface->SetPorterDuff(m_surface,porterDuff);
    m_surface->SetDstBlendFunction(m_surface,DSBF_INVSRCALPHA);

    const DFBRectangle sRect = { srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height() };

    DFBResult result;
    if (rect.width() == srcRect.width() && rect.height() == srcRect.height())
        result = m_surface->Blit(m_surface, s, &sRect, rect.x(), rect.y());
    else {
        const DFBRectangle dRect = { rect.x(), rect.y(), rect.width(), rect.height() };
        result = m_surface->StretchBlit(m_surface, s, &sRect, &dRect);
    }
    if (result != DFB_OK)
        DirectFBError("QDirectFBBlitter::drawPixmap()", result);
}

QImage *QDirectFbBlitter::doLock()
{
    Q_ASSERT(m_surface);
    Q_ASSERT(size().isValid());

    void *mem;
    int bpl;
    const DFBResult result = m_surface->Lock(m_surface, DFBSurfaceLockFlags(DSLF_WRITE|DSLF_READ), static_cast<void**>(&mem), &bpl);
    if (result == DFB_OK) {
        DFBSurfacePixelFormat dfbFormat;
        DFBSurfaceCapabilities dfbCaps;
        m_surface->GetPixelFormat(m_surface,&dfbFormat);
        m_surface->GetCapabilities(m_surface,&dfbCaps);
        QImage::Format format = QDirectFbConvenience::imageFormatFromSurfaceFormat(dfbFormat, dfbCaps);
        int w, h;
        m_surface->GetSize(m_surface,&w,&h);
        m_image = QImage(static_cast<uchar *>(mem),w,h,bpl,format);
    } else {
        DirectFBError("Failed to lock image", result);
    }

    return &m_image;
}

void QDirectFbBlitter::doUnlock()
{
    m_surface->Unlock(m_surface);
}
