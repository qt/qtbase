// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdirectfbcursor.h"
#include "qdirectfbconvenience.h"

QT_BEGIN_NAMESPACE

QDirectFBCursor::QDirectFBCursor(QPlatformScreen *screen)
    : m_screen(screen)
{
#ifndef QT_NO_CURSOR
    m_image.reset(new QPlatformCursorImage(0, 0, 0, 0, 0, 0));
#endif
}

#ifndef QT_NO_CURSOR
void QDirectFBCursor::changeCursor(QCursor *cursor, QWindow *)
{
    int xSpot;
    int ySpot;
    QPixmap map;

    const Qt::CursorShape newShape = cursor ? cursor->shape() : Qt::ArrowCursor;
    if (newShape != Qt::BitmapCursor) {
        m_image->set(newShape);
        xSpot = m_image->hotspot().x();
        ySpot = m_image->hotspot().y();
        QImage *i = m_image->image();
        map = QPixmap::fromImage(*i);
    } else {
        QPoint point = cursor->hotSpot();
        xSpot = point.x();
        ySpot = point.y();
        map = cursor->pixmap();
    }

    DFBResult res;
    IDirectFBDisplayLayer *layer = toDfbLayer(m_screen);
    IDirectFBSurface* surface(QDirectFbConvenience::dfbSurfaceForPlatformPixmap(map.handle()));

    res = layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);
    if (res != DFB_OK) {
        DirectFBError("Failed to set DLSCL_ADMINISTRATIVE", res);
        return;
    }

    layer->SetCursorShape(layer, surface, xSpot, ySpot);
    layer->SetCooperativeLevel(layer, DLSCL_SHARED);
}
#endif

QT_END_NAMESPACE
