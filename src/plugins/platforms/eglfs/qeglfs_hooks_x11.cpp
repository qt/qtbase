/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the qmake spec of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfs_hooks.h"

#include <X11/Xlib.h>

QEglFSHooks platform_hooks;
static Display *display = 0;

void QEglFSHooks::platformInit()
{
    display = XOpenDisplay(NULL);
    if (!display)
        qFatal("Could not open display");
}

void QEglFSHooks::platformDestroy()
{
    XCloseDisplay(display);
}

EGLNativeDisplayType QEglFSHooks::platformDisplay() const
{
    return display;
}

QSize QEglFSHooks::screenSize() const
{
    QList<QByteArray> env = qgetenv("EGLFS_X11_SIZE").split('x');
    if (env.length() != 2)
        return QSize(640, 480);
    return QSize(env.at(0).toInt(), env.at(1).toInt());
}

EGLNativeWindowType QEglFSHooks::createNativeWindow(const QSize &size)
{
    Window root = DefaultRootWindow(display);
    XSetWindowAttributes swa;
    memset(&swa, 0, sizeof(swa));
    Window win  = XCreateWindow(display, root, 0, 0, size.width(), size.height(), 0, CopyFromParent,
                                InputOutput, CopyFromParent, CWEventMask, &swa);
    XMapWindow(display, win);
    XStoreName(display, win, "EGLFS");
    return win;
}

void QEglFSHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    XDestroyWindow(display, window);
}

bool QEglFSHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    return false;
}

