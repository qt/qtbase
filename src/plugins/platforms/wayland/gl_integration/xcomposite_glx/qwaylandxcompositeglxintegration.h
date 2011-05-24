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

#ifndef QWAYLANDXCOMPOSITEGLXINTEGRATION_H
#define QWAYLANDXCOMPOSITEGLXINTEGRATION_H

#include "gl_integration/qwaylandglintegration.h"
#include "wayland-client.h"

#include <QtCore/QTextStream>
#include <QtCore/QDataStream>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>
#include <QtGui/QWidget>

#include <X11/Xlib.h>

struct wl_xcomposite;

class QWaylandXCompositeGLXIntegration : public QWaylandGLIntegration
{
public:
    QWaylandXCompositeGLXIntegration(QWaylandDisplay * waylandDispaly);
    ~QWaylandXCompositeGLXIntegration();

    void initialize();

    QWaylandWindow *createEglWindow(QWidget *widget);

    QWaylandDisplay *waylandDisplay() const;
    struct wl_xcomposite *waylandXComposite() const;

    Display *xDisplay() const;
    int screen() const;
    Window rootWindow() const;

private:
    QWaylandDisplay *mWaylandDisplay;
    struct wl_xcomposite *mWaylandComposite;

    Display *mDisplay;
    int mScreen;
    Window mRootWindow;

    static void wlDisplayHandleGlobal(struct wl_display *display, uint32_t id,
                             const char *interface, uint32_t version, void *data);

    static const struct wl_xcomposite_listener xcomposite_listener;
    static void rootInformation(void *data,
                 struct wl_xcomposite *xcomposite,
                 const char *display_name,
                 uint32_t root_window);
};

#endif // QWAYLANDXCOMPOSITEGLXINTEGRATION_H
