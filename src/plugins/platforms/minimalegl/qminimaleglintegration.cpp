/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qminimaleglintegration.h"

#include "qminimaleglwindow.h"
#include "qminimaleglbackingstore.h"

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>

#if defined(Q_OS_UNIX)
#  include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#elif defined(Q_OS_WINRT)
#  include <QtCore/private/qeventdispatcher_winrt_p.h>
#  include <QtGui/qpa/qwindowsysteminterface.h>
#elif defined(Q_OS_WIN)
#  include <QtPlatformSupport/private/qwindowsguieventdispatcher_p.h>
#endif

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WINRT
namespace {
class QWinRTEventDispatcher : public QEventDispatcherWinRT {
public:
    QWinRTEventDispatcher() {}

protected:
    bool hasPendingEvents() Q_DECL_OVERRIDE
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
    screenAdded(mScreen);

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QMinimalEglIntegration\n");
#endif
}

QMinimalEglIntegration::~QMinimalEglIntegration()
{
    destroyScreen(mScreen);
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
    return new QMinimalEglBackingStore(window);
}

QPlatformOpenGLContext *QMinimalEglIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return static_cast<QMinimalEglScreen *>(context->screen()->handle())->platformContext();
}

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
    return Q_NULLPTR;
#endif
}

QVariant QMinimalEglIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    if (hint == QPlatformIntegration::ShowIsFullScreen)
        return true;

    return QPlatformIntegration::styleHint(hint);
}

QT_END_NAMESPACE
