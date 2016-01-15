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
