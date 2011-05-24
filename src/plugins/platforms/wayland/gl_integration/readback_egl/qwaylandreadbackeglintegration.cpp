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

#include "qwaylandreadbackeglintegration.h"

#include <QDebug>

#include "qwaylandreadbackeglwindow.h"

QWaylandReadbackEglIntegration::QWaylandReadbackEglIntegration(QWaylandDisplay *display)
    : QWaylandGLIntegration()
    , mWaylandDisplay(display)
{
    qDebug() << "Using Readback-EGL";
    char *display_name = getenv("DISPLAY");
    mDisplay = XOpenDisplay(display_name);
    mScreen = XDefaultScreen(mDisplay);
    mRootWindow = XDefaultRootWindow(mDisplay);
    XSync(mDisplay, False);
}

QWaylandReadbackEglIntegration::~QWaylandReadbackEglIntegration()
{
    XCloseDisplay(mDisplay);
}


QWaylandGLIntegration *QWaylandGLIntegration::createGLIntegration(QWaylandDisplay *waylandDisplay)
{
    return new QWaylandReadbackEglIntegration(waylandDisplay);
}

void QWaylandReadbackEglIntegration::initialize()
{
    eglBindAPI(EGL_OPENGL_ES_API);
    mEglDisplay = eglGetDisplay(mDisplay);
    EGLint major, minor;
    EGLBoolean initialized = eglInitialize(mEglDisplay,&major,&minor);
    if (initialized) {
        qDebug() << "EGL initialized successfully" << major << "," << minor;
    } else {
        qDebug() << "EGL could not initialized. All EGL and GL operations will fail";
    }
}

QWaylandWindow * QWaylandReadbackEglIntegration::createEglWindow(QWidget *widget)
{
    return new QWaylandReadbackEglWindow(widget,this);
}

EGLDisplay QWaylandReadbackEglIntegration::eglDisplay()
{
    return mEglDisplay;
}

Window QWaylandReadbackEglIntegration::rootWindow() const
{
    return mRootWindow;
}

int QWaylandReadbackEglIntegration::depth() const
{
    return XDefaultDepth(mDisplay,mScreen);
}

Display * QWaylandReadbackEglIntegration::xDisplay() const
{
    return mDisplay;
}

QWaylandDisplay * QWaylandReadbackEglIntegration::waylandDisplay() const
{
    return mWaylandDisplay;
}
