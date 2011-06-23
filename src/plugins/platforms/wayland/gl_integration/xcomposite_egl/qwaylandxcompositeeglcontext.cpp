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

#include "qwaylandxcompositeeglcontext.h"

#include "qwaylandxcompositeeglwindow.h"
#include "qwaylandxcompositebuffer.h"

#include "wayland-xcomposite-client-protocol.h"
#include <QtCore/QDebug>

#include "qeglconvenience.h"
#include "qxlibeglintegration.h"

#include <X11/extensions/Xcomposite.h>

QWaylandXCompositeEGLContext::QWaylandXCompositeEGLContext(QWaylandXCompositeEGLIntegration *glxIntegration, QWaylandXCompositeEGLWindow *window)
    : QPlatformGLContext()
    , mEglIntegration(glxIntegration)
    , mWindow(window)
    , mBuffer(0)
    , mXWindow(0)
    , mConfig(q_configFromQPlatformWindowFormat(glxIntegration->eglDisplay(),window->widget()->platformWindowFormat(),true,EGL_WINDOW_BIT))
    , mWaitingForSync(false)
{
    QVector<EGLint> eglContextAttrs;
    eglContextAttrs.append(EGL_CONTEXT_CLIENT_VERSION); eglContextAttrs.append(2);
    eglContextAttrs.append(EGL_NONE);

    mContext = eglCreateContext(glxIntegration->eglDisplay(),mConfig,EGL_NO_CONTEXT,eglContextAttrs.constData());
    if (mContext == EGL_NO_CONTEXT) {
        qFatal("failed to find context");
    }

    geometryChanged();
}

void QWaylandXCompositeEGLContext::makeCurrent()
{
    QPlatformGLContext::makeCurrent();

    eglMakeCurrent(mEglIntegration->eglDisplay(),mEglWindowSurface,mEglWindowSurface,mContext);
}

void QWaylandXCompositeEGLContext::doneCurrent()
{
    QPlatformGLContext::doneCurrent();
    eglMakeCurrent(mEglIntegration->eglDisplay(),EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
}

void QWaylandXCompositeEGLContext::swapBuffers()
{
    QSize size = mWindow->geometry().size();

    eglSwapBuffers(mEglIntegration->eglDisplay(),mEglWindowSurface);
    mWindow->damage(QRect(QPoint(0,0),size));
    mWindow->waitForFrameSync();
}

void * QWaylandXCompositeEGLContext::getProcAddress(const QString &procName)
{
    return (void *)eglGetProcAddress(qPrintable(procName));
}

QPlatformWindowFormat QWaylandXCompositeEGLContext::platformWindowFormat() const
{
    return qt_qPlatformWindowFormatFromConfig(mEglIntegration->eglDisplay(),mConfig);
}

void QWaylandXCompositeEGLContext::sync_function(void *data)
{
    QWaylandXCompositeEGLContext *that = static_cast<QWaylandXCompositeEGLContext *>(data);
    that->mWaitingForSync = false;
}

void QWaylandXCompositeEGLContext::geometryChanged()
{
    QSize size(mWindow->geometry().size());
    if (size.isEmpty()) {
        //QGLWidget wants a context for a window without geometry
        size = QSize(1,1);
    }

    delete mBuffer;
    //XFreePixmap deletes the glxPixmap as well
    if (mXWindow) {
        XDestroyWindow(mEglIntegration->xDisplay(),mXWindow);
    }

    VisualID visualId = QXlibEglIntegration::getCompatibleVisualId(mEglIntegration->xDisplay(),mEglIntegration->eglDisplay(),mConfig);

    XVisualInfo visualInfoTemplate;
    memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
    visualInfoTemplate.visualid = visualId;

    int matchingCount = 0;
    XVisualInfo *visualInfo = XGetVisualInfo(mEglIntegration->xDisplay(), VisualIDMask, &visualInfoTemplate, &matchingCount);

    Colormap cmap = XCreateColormap(mEglIntegration->xDisplay(),mEglIntegration->rootWindow(),visualInfo->visual,AllocNone);

    XSetWindowAttributes a;
    a.colormap = cmap;
    mXWindow = XCreateWindow(mEglIntegration->xDisplay(), mEglIntegration->rootWindow(),0, 0, size.width(), size.height(),
                             0, visualInfo->depth, InputOutput, visualInfo->visual,
                             CWColormap, &a);

    XCompositeRedirectWindow(mEglIntegration->xDisplay(), mXWindow, CompositeRedirectManual);
    XMapWindow(mEglIntegration->xDisplay(), mXWindow);

    mEglWindowSurface = eglCreateWindowSurface(mEglIntegration->eglDisplay(),mConfig,mXWindow,0);
    if (mEglWindowSurface == EGL_NO_SURFACE) {
        qFatal("Could not make eglsurface");
    }

    XSync(mEglIntegration->xDisplay(),False);
    mBuffer = new QWaylandXCompositeBuffer(mEglIntegration->waylandXComposite(),
                                           (uint32_t)mXWindow,
                                           size,
                                           mEglIntegration->waylandDisplay()->argbVisual());
    mWindow->attach(mBuffer);
    wl_display_sync_callback(mEglIntegration->waylandDisplay()->wl_display(),
                             QWaylandXCompositeEGLContext::sync_function,
                             this);

    mWaitingForSync = true;
    wl_display_sync(mEglIntegration->waylandDisplay()->wl_display(),0);
    mEglIntegration->waylandDisplay()->flushRequests();
    while (mWaitingForSync) {
        mEglIntegration->waylandDisplay()->readEvents();
    }
}
