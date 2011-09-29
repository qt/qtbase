/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <private/qt_mac_p.h>
#include "qcoreapplication.h"
#include <qlibrary.h>

QT_BEGIN_NAMESPACE

QRegion::QRegionData QRegion::shared_empty = { Q_BASIC_ATOMIC_INITIALIZER(1), 0 };


#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
OSStatus QRegion::shape2QRegionHelper(int inMessage, HIShapeRef,
                                      const CGRect *inRect, void *inRefcon)
{
    QRegion *region = static_cast<QRegion *>(inRefcon);
    if (!region)
        return paramErr;

    switch (inMessage) {
    case kHIShapeEnumerateRect:
        *region += QRect(inRect->origin.x, inRect->origin.y,
                         inRect->size.width, inRect->size.height);
        break;
    case kHIShapeEnumerateInit:
        // Assume the region is already setup correctly
    case kHIShapeEnumerateTerminate:
    default:
        break;
    }
    return noErr;
}
#endif

/*!
    \internal
     Create's a mutable shape, it's the caller's responsibility to release.
     WARNING: this function clamps the coordinates to SHRT_MIN/MAX on 10.4 and below.
*/
HIMutableShapeRef QRegion::toHIMutableShape() const
{
    HIMutableShapeRef shape = HIShapeCreateMutable();
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
        if (d->qt_rgn && d->qt_rgn->numRects) {
            int n = d->qt_rgn->numRects;
            const QRect *qt_r = (n == 1) ? &d->qt_rgn->extents : d->qt_rgn->rects.constData();
            while (n--) {
                CGRect cgRect = CGRectMake(qt_r->x(), qt_r->y(), qt_r->width(), qt_r->height());
                HIShapeUnionWithRect(shape, &cgRect);
                ++qt_r;
            }
        }
    } else
#endif
    {
    }
    return shape;
}



QRegion QRegion::fromHIShapeRef(HIShapeRef shape)
{
    QRegion returnRegion;
    returnRegion.detach();
    // Begin gratuitous #if-defery
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
# ifndef Q_WS_MAC64
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
# endif
        HIShapeEnumerate(shape, kHIShapeParseFromTopLeft, shape2QRegionHelper, &returnRegion);
# ifndef Q_WS_MAC64
    } else
# endif
#endif
    {
    }
    return returnRegion;
}

QT_END_NAMESPACE
