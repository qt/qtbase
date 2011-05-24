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

#include "qdirectfbwindowsurface.h"
#include "qdirectfbintegration.h"
#include "qdirectfbblitter.h"
#include "qdirectfbconvenience.h"
#include <private/qpixmap_blitter_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QDirectFbWindowSurface::QDirectFbWindowSurface(QWidget *window, WId wId)
    : QWindowSurface(window), m_pixmap(0), m_pmdata(0), m_dfbSurface(0)
{

    IDirectFBDisplayLayer *layer = QDirectFbConvenience::dfbDisplayLayer();

    DFBWindowID id(wId);
    IDirectFBWindow *dfbWindow;

    layer->GetWindow(layer,id,&dfbWindow);

    dfbWindow->GetSurface(dfbWindow,&m_dfbSurface);
//WRONGSIZE
    QDirectFbBlitter *blitter = new QDirectFbBlitter(window->rect().size(), m_dfbSurface);
    m_pmdata = new QDirectFbBlitterPixmapData;
    m_pmdata->setBlittable(blitter);
    m_pixmap = new QPixmap(m_pmdata);
}

QDirectFbWindowSurface::~QDirectFbWindowSurface()
{
    delete m_pixmap;
}

QPaintDevice *QDirectFbWindowSurface::paintDevice()
{
    return m_pixmap;
}

void QDirectFbWindowSurface::flush(QWidget *widget, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(widget);
    m_pmdata->blittable()->unlock();

    QVector<QRect> rects = region.rects();
    for (int i = 0 ; i < rects.size(); i++) {
        const QRect rect = rects.at(i);
        DFBRegion dfbReg = { rect.x() + offset.x(),rect.y() + offset.y(),rect.right() + offset.x(),rect.bottom() + offset.y()};
        m_dfbSurface->Flip(m_dfbSurface, &dfbReg, DFBSurfaceFlipFlags(DSFLIP_BLIT|DSFLIP_ONSYNC));
    }
}

void QDirectFbWindowSurface::resize(const QSize &size)
{
    QWindowSurface::resize(size);

    //Have to add 1 ref ass it will be removed by deleting the old blitter in setBlittable
    m_dfbSurface->AddRef(m_dfbSurface);
    QDirectFbBlitter *blitter = new QDirectFbBlitter(size,m_dfbSurface);
    m_pmdata->setBlittable(blitter);
}

static inline void scrollSurface(IDirectFBSurface *surface, const QRect &r, int dx, int dy)
{
    const DFBRectangle rect = { r.x(), r.y(), r.width(), r.height() };
    surface->Blit(surface, surface, &rect, r.x() + dx, r.y() + dy);
    const DFBRegion region = { rect.x + dx, rect.y + dy, r.right() + dx, r.bottom() + dy };
    surface->Flip(surface, &region, DFBSurfaceFlipFlags(DSFLIP_BLIT));
}

bool QDirectFbWindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    m_pmdata->blittable()->unlock();

    if (!m_dfbSurface || area.isEmpty())
        return false;
    m_dfbSurface->SetBlittingFlags(m_dfbSurface, DSBLIT_NOFX);
    if (area.rectCount() == 1) {
        scrollSurface(m_dfbSurface, area.boundingRect(), dx, dy);
    } else {
        const QVector<QRect> rects = area.rects();
        const int n = rects.size();
        for (int i=0; i<n; ++i) {
            scrollSurface(m_dfbSurface, rects.at(i), dx, dy);
        }
    }
    return true;
}

void QDirectFbWindowSurface::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);
}

void QDirectFbWindowSurface::endPaint(const QRegion &region)
{
    Q_UNUSED(region);
}

QT_END_NAMESPACE
