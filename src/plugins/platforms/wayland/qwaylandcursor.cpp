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

#include "qwaylandcursor.h"

#include "qwaylanddisplay.h"
#include "qwaylandinputdevice.h"
#include "qwaylandshmsurface.h"
#include "qwaylandscreen.h"

#include <QtGui/QImageReader>

#define DATADIR "/usr/share"

static const struct pointer_image {
    const char *filename;
    int hotspot_x, hotspot_y;
} pointer_images[] = {
    /* FIXME: Half of these are wrong... */
    /* Qt::ArrowCursor */
    { DATADIR "/wayland/left_ptr.png",			10,  5 },
    /* Qt::UpArrowCursor */
    { DATADIR "/wayland/top_side.png",			18,  8 },
    /* Qt::CrossCursor */
    { DATADIR "/wayland/top_side.png",			18,  8 },
    /* Qt::WaitCursor */
    { DATADIR "/wayland/top_side.png",			18,  8 },
    /* Qt::IBeamCursor */
    { DATADIR "/wayland/xterm.png",			15, 15 },
    /* Qt::SizeVerCursor */
    { DATADIR "/wayland/top_side.png",			18,  8 },
    /* Qt::SizeHorCursor */
    { DATADIR "/wayland/bottom_left_corner.png",	 6, 30 },
    /* Qt::SizeBDiagCursor */
    { DATADIR "/wayland/bottom_right_corner.png",	28, 28 },
    /* Qt::SizeFDiagCursor */
    { DATADIR "/wayland/bottom_side.png",		16, 20 },
    /* Qt::SizeAllCursor */
    { DATADIR "/wayland/left_side.png",			10, 20 },
    /* Qt::BlankCursor */
    { DATADIR "/wayland/right_side.png",		30, 19 },
    /* Qt::SplitVCursor */
    { DATADIR "/wayland/sb_v_double_arrow.png",		15, 15 },
    /* Qt::SplitHCursor */
    { DATADIR "/wayland/sb_h_double_arrow.png",		15, 15 },
    /* Qt::PointingHandCursor */
    { DATADIR "/wayland/hand2.png",			14,  8 },
    /* Qt::ForbiddenCursor */
    { DATADIR "/wayland/top_right_corner.png",		26,  8 },
    /* Qt::WhatsThisCursor */
    { DATADIR "/wayland/top_right_corner.png",		26,  8 },
    /* Qt::BusyCursor */
    { DATADIR "/wayland/top_right_corner.png",		26,  8 },
    /* Qt::OpenHandCursor */
    { DATADIR "/wayland/hand1.png",			18, 11 },
    /* Qt::ClosedHandCursor */
    { DATADIR "/wayland/grabbing.png",			20, 17 },
    /* Qt::DragCopyCursor */
    { DATADIR "/wayland/dnd-copy.png",			13, 13 },
    /* Qt::DragMoveCursor */
    { DATADIR "/wayland/dnd-move.png",			13, 13 },
    /* Qt::DragLinkCursor */
    { DATADIR "/wayland/dnd-link.png",			13, 13 },
};

QWaylandCursor::QWaylandCursor(QWaylandScreen *screen)
    : QPlatformCursor(screen)
    , mBuffer(0)
    , mDisplay(screen->display())
{
}

void QWaylandCursor::changeCursor(QCursor *cursor, QWidget *widget)
{
    const struct pointer_image *p;

    if (widget == NULL)
        return;

    p = NULL;

    switch (cursor->shape()) {
    case Qt::ArrowCursor:
        p = &pointer_images[cursor->shape()];
        break;
    case Qt::UpArrowCursor:
    case Qt::CrossCursor:
    case Qt::WaitCursor:
        break;
    case Qt::IBeamCursor:
        p = &pointer_images[cursor->shape()];
        break;
    case Qt::SizeVerCursor:	/* 5 */
    case Qt::SizeHorCursor:
    case Qt::SizeBDiagCursor:
    case Qt::SizeFDiagCursor:
    case Qt::SizeAllCursor:
    case Qt::BlankCursor:	/* 10 */
        break;
    case Qt::SplitVCursor:
    case Qt::SplitHCursor:
    case Qt::PointingHandCursor:
        p = &pointer_images[cursor->shape()];
        break;
    case Qt::ForbiddenCursor:
    case Qt::WhatsThisCursor:	/* 15 */
    case Qt::BusyCursor:
        break;
    case Qt::OpenHandCursor:
    case Qt::ClosedHandCursor:
    case Qt::DragCopyCursor:
    case Qt::DragMoveCursor: /* 20 */
    case Qt::DragLinkCursor:
        p = &pointer_images[cursor->shape()];
        break;

    default:
    case Qt::BitmapCursor:
        break;
    }

    if (!p) {
        p = &pointer_images[0];
        qWarning("unhandled cursor %d", cursor->shape());
    }

    QImageReader reader(p->filename);

    if (!reader.canRead())
        return;

    if (mBuffer == NULL || mBuffer->size() != reader.size()) {
        if (mBuffer)
            delete mBuffer;

        mBuffer = new QWaylandShmBuffer(mDisplay, reader.size(),
                                        QImage::Format_ARGB32);
    }

    reader.read(mBuffer->image());

    mDisplay->setCursor(mBuffer, p->hotspot_x, p->hotspot_y);
}

void QWaylandDisplay::setCursor(QWaylandBuffer *buffer, int32_t x, int32_t y)
{
    /* Qt doesn't tell us which input device we should set the cursor
     * for, so set it for all devices. */
    for (int i = 0; i < mInputDevices.count(); i++) {
        QWaylandInputDevice *inputDevice = mInputDevices.at(i);
        inputDevice->attach(buffer, x, y);
    }
}
