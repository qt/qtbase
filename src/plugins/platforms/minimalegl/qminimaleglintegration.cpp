// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qminimaleglintegration.h"

#include "qminimaleglwindow.h"
#ifndef QT_NO_OPENGL
# include "qminimaleglbackingstore.h"
#endif
#include <QtGui/private/qgenericunixfontdatabase_p.h>

#if defined(Q_OS_UNIX)
#  include <QtGui/private/qgenericunixeventdispatcher_p.h>
#elif defined(Q_OS_WIN)
#  include <QtGui/private/qwindowsguieventdispatcher_p.h>
#endif

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <qpa/qwindowsysteminterface.h>

// this is where EGL headers are pulled in, make sure it is last
#include "qminimaleglscreen.h"

QT_BEGIN_NAMESPACE

QMinimalEglIntegration::QMinimalEglIntegration()
    : mFontDb(new QGenericUnixFontDatabase()), mScreen(new QMinimalEglScreen(EGL_DEFAULT_DISPLAY))
{
    QWindowSystemInterface::handleScreenAdded(mScreen);

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QMinimalEglIntegration\n");
#endif
}

QMinimalEglIntegration::~QMinimalEglIntegration()
{
    QWindowSystemInterface::handleScreenRemoved(mScreen);
    delete mFontDb;
}

bool QMinimalEglIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QMinimalEglIntegration::createPlatformWindow(QWindow *window) const
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QMinimalEglIntegration::createPlatformWindow %p\n",window);
#endif
    QPlatformWindow *w = new QMinimalEglWindow(window);
    w->requestActivateWindow();
    return w;
}


QPlatformBackingStore *QMinimalEglIntegration::createPlatformBackingStore(QWindow *window) const
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QMinimalEglIntegration::createWindowSurface %p\n", window);
#endif
#ifndef QT_NO_OPENGL
    return new QMinimalEglBackingStore(window);
#else
    Q_UNUSED(window);
    return nullptr;
#endif
}
#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QMinimalEglIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return static_cast<QMinimalEglScreen *>(context->screen()->handle())->platformContext();
}
#endif

QPlatformFontDatabase *QMinimalEglIntegration::fontDatabase() const
{
    return mFontDb;
}

QAbstractEventDispatcher *QMinimalEglIntegration::createEventDispatcher() const
{
#if defined(Q_OS_UNIX)
    return createUnixEventDispatcher();
#elif defined(Q_OS_WIN)
    return new QWindowsGuiEventDispatcher;
#else
    return nullptr;
#endif
}

QVariant QMinimalEglIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    if (hint == QPlatformIntegration::ShowIsFullScreen)
        return true;

    return QPlatformIntegration::styleHint(hint);
}

QT_END_NAMESPACE
