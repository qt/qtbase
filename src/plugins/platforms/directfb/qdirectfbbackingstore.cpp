// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdirectfbbackingstore.h"
#include "qdirectfbintegration.h"
#include "qdirectfbblitter.h"
#include "qdirectfbconvenience.h"
#include "qdirectfbwindow.h"
#include <private/qpixmap_blitter_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QDirectFbBackingStore::QDirectFbBackingStore(QWindow *window)
    : QPlatformBackingStore(window), m_pixmap(0), m_pmdata(0)
{
    IDirectFBWindow *dfbWindow = static_cast<QDirectFbWindow *>(window->handle())->dfbWindow();
    dfbWindow->GetSurface(dfbWindow, m_dfbSurface.outPtr());

//WRONGSIZE
    QDirectFbBlitter *blitter = new QDirectFbBlitter(window->size(), m_dfbSurface.data());
    m_pmdata = new QDirectFbBlitterPlatformPixmap;
    m_pmdata->setBlittable(blitter);
    m_pixmap.reset(new QPixmap(m_pmdata));
}

QPaintDevice *QDirectFbBackingStore::paintDevice()
{
    return m_pixmap.data();
}

void QDirectFbBackingStore::flush(QWindow *, const QRegion &region, const QPoint &offset)
{
    m_pmdata->blittable()->unlock();

    for (const QRect &rect : region) {
        DFBRegion dfbReg(rect.x() + offset.x(),rect.y() + offset.y(),rect.right() + offset.x(),rect.bottom() + offset.y());
        m_dfbSurface->Flip(m_dfbSurface.data(), &dfbReg, DFBSurfaceFlipFlags(DSFLIP_BLIT|DSFLIP_ONSYNC));
    }
}

void QDirectFbBackingStore::resize(const QSize &size, const QRegion& reg)
{
    Q_UNUSED(reg);

    if ((m_pmdata->width() == size.width()) &&
        (m_pmdata->height() == size.height()))
        return;

    QDirectFbBlitter *blitter = new QDirectFbBlitter(size, m_dfbSurface.data());
    m_pmdata->setBlittable(blitter);
}

static inline void scrollSurface(IDirectFBSurface *surface, const QRect &r, int dx, int dy)
{
    const DFBRectangle rect(r.x(), r.y(), r.width(), r.height());
    surface->Blit(surface, surface, &rect, r.x() + dx, r.y() + dy);
    const DFBRegion region(rect.x + dx, rect.y + dy, r.right() + dx, r.bottom() + dy);
    surface->Flip(surface, &region, DFBSurfaceFlipFlags(DSFLIP_BLIT));
}

bool QDirectFbBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    m_pmdata->blittable()->unlock();

    if (!m_dfbSurface || area.isEmpty())
        return false;
    m_dfbSurface->SetBlittingFlags(m_dfbSurface.data(), DSBLIT_NOFX);
    if (area.rectCount() == 1) {
        scrollSurface(m_dfbSurface.data(), area.boundingRect(), dx, dy);
    } else {
        for (const QRect &rect : area)
            scrollSurface(m_dfbSurface.data(), rect, dx, dy);
    }
    return true;
}

QImage QDirectFbBackingStore::toImage() const
{
    return m_pixmap.data()->toImage();
}

QT_END_NAMESPACE
