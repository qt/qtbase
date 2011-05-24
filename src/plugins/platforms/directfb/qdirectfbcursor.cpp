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

#include "qdirectfbcursor.h"
#include "qdirectfbconvenience.h"


QDirectFBCursor::QDirectFBCursor(QPlatformScreen* screen) :
        QPlatformCursor(screen), surface(0)
{
    QDirectFbConvenience::dfbInterface()->GetDisplayLayer(QDirectFbConvenience::dfbInterface(),DLID_PRIMARY, &m_layer);
    image = new QPlatformCursorImage(0, 0, 0, 0, 0, 0);
}

void QDirectFBCursor::changeCursor(QCursor * cursor, QWidget * widget)
{
    Q_UNUSED(widget);
    int xSpot;
    int ySpot;
    QPixmap map;

    if (cursor->shape() != Qt::BitmapCursor) {
        image->set(cursor->shape());
        xSpot = image->hotspot().x();
        ySpot = image->hotspot().y();
        QImage *i = image->image();
        map = QPixmap::fromImage(*i);
    } else {
        QPoint point = cursor->hotSpot();
        xSpot = point.x();
        ySpot = point.y();
        map = cursor->pixmap();
    }

    IDirectFBSurface *surface = QDirectFbConvenience::dfbSurfaceForPixmapData(map.pixmapData());

    if (m_layer->SetCooperativeLevel(m_layer, DLSCL_ADMINISTRATIVE) != DFB_OK) {
        return;
    }
    m_layer->SetCursorShape( m_layer, surface, xSpot, ySpot);
    m_layer->SetCooperativeLevel(m_layer, DLSCL_SHARED);
}
