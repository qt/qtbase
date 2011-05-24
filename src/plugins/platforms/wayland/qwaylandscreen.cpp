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

#include "qwaylandscreen.h"

#include "qwaylanddisplay.h"
#include "qwaylandcursor.h"

QWaylandScreen::QWaylandScreen(QWaylandDisplay *waylandDisplay, struct wl_output *output, QRect geometry)
    : QPlatformScreen()
    , mWaylandDisplay(waylandDisplay)
    , mOutput(output)
    , mGeometry(geometry)
    , mDepth(32)
    , mFormat(QImage::Format_ARGB32_Premultiplied)
    , mWaylandCursor(new QWaylandCursor(this))
{
    moveToThread(waylandDisplay->thread());
}

QWaylandScreen::~QWaylandScreen()
{
    delete mWaylandCursor;
}

QWaylandDisplay * QWaylandScreen::display() const
{
    return mWaylandDisplay;
}

QRect QWaylandScreen::geometry() const
{
    return mGeometry;
}

int QWaylandScreen::depth() const
{
    return mDepth;
}

QImage::Format QWaylandScreen::format() const
{
    return mFormat;
}

QWaylandScreen * QWaylandScreen::waylandScreenFromWidget(QWidget *widget)
{
    QPlatformScreen *platformScreen = QPlatformScreen::platformScreenForWidget(widget);
    return static_cast<QWaylandScreen *>(platformScreen);
}

wl_visual * QWaylandScreen::visual() const
{
    struct wl_visual *visual;

    switch (format()) {
    case QImage::Format_ARGB32:
        visual = mWaylandDisplay->argbVisual();
        break;
    case QImage::Format_ARGB32_Premultiplied:
        visual = mWaylandDisplay->argbPremultipliedVisual();
        break;
    default:
        qDebug("unsupported buffer format %d requested\n", format());
        visual = mWaylandDisplay->argbVisual();
        break;
    }
    return visual;
}
