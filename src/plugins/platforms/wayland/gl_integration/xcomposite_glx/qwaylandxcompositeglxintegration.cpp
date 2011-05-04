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

#include "qwaylandxcompositeglxintegration.h"

#include "qwaylandxcompositeglxwindow.h"

#include <QtCore/QDebug>

#include "wayland-xcomposite-client-protocol.h"

QWaylandGLIntegration * QWaylandGLIntegration::createGLIntegration(QWaylandDisplay *waylandDisplay)
{
    return new QWaylandXCompositeGLXIntegration(waylandDisplay);
}

QWaylandXCompositeGLXIntegration::QWaylandXCompositeGLXIntegration(QWaylandDisplay * waylandDispaly)
    : QWaylandGLIntegration()
    , mWaylandDisplay(waylandDispaly)
{
    qDebug() << "Using XComposite-GLX";
    wl_display_add_global_listener(waylandDispaly->wl_display(), QWaylandXCompositeGLXIntegration::wlDisplayHandleGlobal,
                                   this);
}

QWaylandXCompositeGLXIntegration::~QWaylandXCompositeGLXIntegration()
{
    XCloseDisplay(mDisplay);
}

void QWaylandXCompositeGLXIntegration::initialize()
{
}

QWaylandWindow * QWaylandXCompositeGLXIntegration::createEglWindow(QWindow *window)
{
    return new QWaylandXCompositeGLXWindow(window,this);
}

Display * QWaylandXCompositeGLXIntegration::xDisplay() const
{
    return mDisplay;
}

int QWaylandXCompositeGLXIntegration::screen() const
{
    return mScreen;
}

Window QWaylandXCompositeGLXIntegration::rootWindow() const
{
    return mRootWindow;
}

QWaylandDisplay * QWaylandXCompositeGLXIntegration::waylandDisplay() const
{
    return mWaylandDisplay;
}
wl_xcomposite * QWaylandXCompositeGLXIntegration::waylandXComposite() const
{
    return mWaylandComposite;
}

const struct wl_xcomposite_listener QWaylandXCompositeGLXIntegration::xcomposite_listener = {
    QWaylandXCompositeGLXIntegration::rootInformation
};

void QWaylandXCompositeGLXIntegration::wlDisplayHandleGlobal(wl_display *display, uint32_t id, const char *interface, uint32_t version, void *data)
{
    Q_UNUSED(version);
    if (strcmp(interface, "wl_xcomposite") == 0) {
        QWaylandXCompositeGLXIntegration *integration = static_cast<QWaylandXCompositeGLXIntegration *>(data);
        integration->mWaylandComposite = wl_xcomposite_create(display,id,1);
        wl_xcomposite_add_listener(integration->mWaylandComposite,&xcomposite_listener,integration);
    }

}

void QWaylandXCompositeGLXIntegration::rootInformation(void *data, wl_xcomposite *xcomposite, const char *display_name, uint32_t root_window)
{
    Q_UNUSED(xcomposite);
    QWaylandXCompositeGLXIntegration *integration = static_cast<QWaylandXCompositeGLXIntegration *>(data);

    integration->mDisplay = XOpenDisplay(display_name);
    integration->mRootWindow = (Window) root_window;
    integration->mScreen = XDefaultScreen(integration->mDisplay);
}

