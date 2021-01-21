/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qminimaleglintegration.h"

#include "qminimaleglwindow.h"
#ifndef QT_NO_OPENGL
# include "qminimaleglbackingstore.h"
#endif
#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>

#if defined(Q_OS_UNIX)
#  include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#elif defined(Q_OS_WINRT)
#  include <QtCore/private/qeventdispatcher_winrt_p.h>
#  include <QtGui/qpa/qwindowsysteminterface.h>
#elif defined(Q_OS_WIN)
#  include <QtEventDispatcherSupport/private/qwindowsguieventdispatcher_p.h>
#endif

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <qpa/qwindowsysteminterface.h>

// this is where EGL headers are pulled in, make sure it is last
#include "qminimaleglscreen.h"

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WINRT
namespace {
class QWinRTEventDispatcher : public QEventDispatcherWinRT {
public:
    QWinRTEventDispatcher() {}

protected:
    bool hasPendingEvents() override
    {
        return QEventDispatcherWinRT::hasPendingEvents() || QWindowSystemInterface::windowSystemEventsQueued();
    }

    bool sendPostedEvents(QEventLoop::ProcessEventsFlags flags)
    {
        bool didProcess = QEventDispatcherWinRT::sendPostedEvents(flags);
        if (!(flags & QEventLoop::ExcludeUserInputEvents))
            didProcess |= QWindowSystemInterface::sendWindowSystemEvents(flags);
        return didProcess;
    }
};
} // anonymous namespace
#endif // Q_OS_WINRT

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
#elif defined(Q_OS_WINRT)
    return new QWinRTEventDispatcher;
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
