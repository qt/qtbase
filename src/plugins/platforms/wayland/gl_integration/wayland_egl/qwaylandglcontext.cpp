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

#include "qwaylandglcontext.h"

#include "qwaylanddisplay.h"
#include "qwaylandwindow.h"

#include "../../../eglconvenience/qeglconvenience.h"

#include <QtGui/QPlatformGLContext>
#include <QtGui/QPlatformWindowFormat>
#include <QtCore/QMutex>

QWaylandGLContext::QWaylandGLContext(EGLDisplay eglDisplay, const QPlatformWindowFormat &format)
    : QPlatformGLContext()
    , mEglDisplay(eglDisplay)
    , mSurface(EGL_NO_SURFACE)
    , mConfig(q_configFromQPlatformWindowFormat(mEglDisplay,format,true))
    , mFormat(qt_qPlatformWindowFormatFromConfig(mEglDisplay,mConfig))
{
    QPlatformGLContext *sharePlatformContext = 0;
    sharePlatformContext = format.sharedGLContext();
    mFormat.setSharedContext(sharePlatformContext);
    EGLContext shareEGLContext = EGL_NO_CONTEXT;
    if (sharePlatformContext)
        shareEGLContext = static_cast<const QWaylandGLContext*>(sharePlatformContext)->mContext;

    eglBindAPI(EGL_OPENGL_ES_API);

    QVector<EGLint> eglContextAttrs;
    eglContextAttrs.append(EGL_CONTEXT_CLIENT_VERSION);
    eglContextAttrs.append(2);
    eglContextAttrs.append(EGL_NONE);

    mContext = eglCreateContext(mEglDisplay, mConfig,
                                shareEGLContext, eglContextAttrs.constData());
}

QWaylandGLContext::QWaylandGLContext()
    : QPlatformGLContext()
    , mEglDisplay(0)
    , mContext(EGL_NO_CONTEXT)
    , mSurface(EGL_NO_SURFACE)
    , mConfig(0)
{ }

QWaylandGLContext::~QWaylandGLContext()
{
    eglDestroyContext(mEglDisplay,mContext);
}

void QWaylandGLContext::makeCurrent()
{
    QPlatformGLContext::makeCurrent();
    if (mSurface == EGL_NO_SURFACE) {
        qWarning("makeCurrent with EGL_NO_SURFACE");
    }
    eglMakeCurrent(mEglDisplay, mSurface, mSurface, mContext);
}

void QWaylandGLContext::doneCurrent()
{
    QPlatformGLContext::doneCurrent();
    eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void QWaylandGLContext::swapBuffers()
{
    eglSwapBuffers(mEglDisplay,mSurface);
}

void *QWaylandGLContext::getProcAddress(const QString &string)
{
    return (void *) eglGetProcAddress(string.toLatin1().data());
}

void QWaylandGLContext::setEglSurface(EGLSurface surface)
{
    bool wasCurrent = false;
    if (QPlatformGLContext::currentContext() == this) {
        wasCurrent = true;
        doneCurrent();
    }
    mSurface = surface;
    if (wasCurrent) {
        makeCurrent();
    }
}

EGLConfig QWaylandGLContext::eglConfig() const
{
    return mConfig;
}

