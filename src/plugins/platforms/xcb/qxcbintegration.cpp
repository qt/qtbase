/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbintegration.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbcursor.h"
#include "qxcbkeyboard.h"
#include "qxcbbackingstore.h"
#include "qxcbnativeinterface.h"
#include "qxcbclipboard.h"
#include "qxcbdrag.h"

#include <xcb/xcb.h>

#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>

#include <stdio.h>

//this has to be included before egl, since egl pulls in X headers
#include <QtGui/private/qguiapplication_p.h>

#ifdef XCB_USE_EGL
#include <EGL/egl.h>
#endif

#ifdef XCB_USE_XLIB
#include <X11/Xlib.h>
#endif

#include <qpa/qplatforminputcontextfactory_p.h>
#include <private/qgenericunixthemes_p.h>
#include <qpa/qplatforminputcontext.h>

#if defined(XCB_USE_GLX)
#include "qglxintegration.h"
#elif defined(XCB_USE_EGL)
#include "qxcbeglsurface.h"
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#endif

#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#ifndef QT_NO_ACCESSIBILITY
#include <qpa/qplatformaccessibility.h>
#ifndef QT_NO_ACCESSIBILITY_ATSPI_BRIDGE
#include "../../../platformsupport/linuxaccessibility/bridge_p.h"
#endif
#endif

QT_BEGIN_NAMESPACE

QXcbIntegration::QXcbIntegration(const QStringList &parameters)
    : m_eventDispatcher(createUnixEventDispatcher())
    ,  m_services(new QGenericUnixServices)
{
    QGuiApplicationPrivate::instance()->setEventDispatcher(m_eventDispatcher);

#ifdef XCB_USE_XLIB
    XInitThreads();
#endif
    m_nativeInterface.reset(new QXcbNativeInterface);

    m_connections << new QXcbConnection(m_nativeInterface.data());

    for (int i = 0; i < parameters.size() - 1; i += 2) {
#ifdef Q_XCB_DEBUG
        qDebug() << "QXcbIntegration: Connecting to additional display: " << parameters.at(i) << parameters.at(i+1);
#endif
        QString display = parameters.at(i) + ':' + parameters.at(i+1);
        m_connections << new QXcbConnection(m_nativeInterface.data(), display.toLatin1().constData());
    }

    m_fontDatabase.reset(new QGenericUnixFontDatabase());
    m_inputContext.reset(QPlatformInputContextFactory::create());
#if !defined(QT_NO_ACCESSIBILITY) && !defined(QT_NO_ACCESSIBILITY_ATSPI_BRIDGE)
    m_accessibility.reset(new QSpiAccessibleBridge());
#endif
}

QXcbIntegration::~QXcbIntegration()
{
#if !defined(QT_NO_OPENGL) && defined(XCB_USE_GLX)
    qDeleteAll(m_defaultContextInfos);
#endif
    qDeleteAll(m_connections);
}

QPlatformWindow *QXcbIntegration::createPlatformWindow(QWindow *window) const
{
    return new QXcbWindow(window);
}

#if defined(XCB_USE_EGL)
class QEGLXcbPlatformContext : public QEGLPlatformContext
{
public:
    QEGLXcbPlatformContext(const QSurfaceFormat &glFormat, QPlatformOpenGLContext *share,
                           EGLDisplay display, QXcbConnection *c)
        : QEGLPlatformContext(glFormat, share, display)
        , m_connection(c)
    {
        Q_XCB_NOOP(m_connection);
    }

    void swapBuffers(QPlatformSurface *surface)
    {
        Q_XCB_NOOP(m_connection);
        QEGLPlatformContext::swapBuffers(surface);
        Q_XCB_NOOP(m_connection);
    }

    bool makeCurrent(QPlatformSurface *surface)
    {
        Q_XCB_NOOP(m_connection);
        bool ret = QEGLPlatformContext::makeCurrent(surface);
        Q_XCB_NOOP(m_connection);
        return ret;
    }

    void doneCurrent()
    {
        Q_XCB_NOOP(m_connection);
        QEGLPlatformContext::doneCurrent();
        Q_XCB_NOOP(m_connection);
    }

    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface)
    {
        return static_cast<QXcbWindow *>(surface)->eglSurface()->surface();
    }

private:
    QXcbConnection *m_connection;
};
#endif

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QXcbIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(context->screen()->handle());
#if defined(XCB_USE_GLX)
    QOpenGLDefaultContextInfo *defaultContextInfo;
    if (m_defaultContextInfos.contains(screen)) {
        defaultContextInfo = m_defaultContextInfos.value(screen);
    } else {
        defaultContextInfo = QOpenGLDefaultContextInfo::create(screen);
        m_defaultContextInfos.insert(screen, defaultContextInfo);
    }
    return new QGLXContext(screen, context->format(), context->shareHandle(), defaultContextInfo);
#elif defined(XCB_USE_EGL)
    return new QEGLXcbPlatformContext(context->format(), context->shareHandle(),
        screen->connection()->egl_display(), screen->connection());
#else
    Q_UNUSED(screen);
    qWarning("QXcbIntegration: Cannot create platform OpenGL context, neither GLX nor EGL are enabled");
    return 0;
#endif
}
#endif

QPlatformBackingStore *QXcbIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QXcbBackingStore(window);
}

bool QXcbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
#if defined(XCB_USE_GLX)
    case OpenGL: return m_connections.at(0)->hasGLX();
#elif defined(XCB_USE_EGL)
    case OpenGL: return true;
#else
    case OpenGL: return false;
#endif
    case ThreadedOpenGL: return m_connections.at(0)->supportsThreadedRendering();
    case WindowMasks: return true;
    case MultipleWindows: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QAbstractEventDispatcher *QXcbIntegration::guiThreadEventDispatcher() const
{
    return m_eventDispatcher;
}

void QXcbIntegration::moveToScreen(QWindow *window, int screen)
{
    Q_UNUSED(window);
    Q_UNUSED(screen);
}

QPlatformFontDatabase *QXcbIntegration::fontDatabase() const
{
    return m_fontDatabase.data();
}

QPlatformNativeInterface * QXcbIntegration::nativeInterface() const
{
    return m_nativeInterface.data();
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QXcbIntegration::clipboard() const
{
    return m_connections.at(0)->clipboard();
}
#endif

#ifndef QT_NO_DRAGANDDROP
QPlatformDrag *QXcbIntegration::drag() const
{
    return m_connections.at(0)->drag();
}
#endif

QPlatformInputContext *QXcbIntegration::inputContext() const
{
    return m_inputContext.data();
}

#ifndef QT_NO_ACCESSIBILITY
QPlatformAccessibility *QXcbIntegration::accessibility() const
{
    return m_accessibility.data();
}
#endif

QPlatformServices *QXcbIntegration::services() const
{
    return m_services.data();
}

Qt::KeyboardModifiers QXcbIntegration::queryKeyboardModifiers() const
{
    int keybMask = 0;
    QXcbConnection *conn = m_connections.at(0);
    QXcbCursor::queryPointer(conn, 0, 0, &keybMask);
    return conn->keyboard()->translateModifiers(keybMask);
}

QStringList QXcbIntegration::themeNames() const
{
    return QGenericUnixTheme::themeNames();
}

QPlatformTheme *QXcbIntegration::createPlatformTheme(const QString &name) const
{
    return QGenericUnixTheme::createUnixTheme(name);
}

/*!
  Called by QXcbConnection prior to a QQnxScreen being deleted.

  Destroys and cleans up any default OpenGL context info for this screen.
*/
void QXcbIntegration::removeDefaultOpenGLContextInfo(QXcbScreen *screen)
{
#if !defined(QT_NO_OPENGL) && defined(XCB_USE_GLX)
    if (!m_defaultContextInfos.contains(screen))
        return;
    QOpenGLDefaultContextInfo* info = m_defaultContextInfos.take(screen);
    delete info;
#else
    Q_UNUSED(screen);
#endif
}

QT_END_NAMESPACE
