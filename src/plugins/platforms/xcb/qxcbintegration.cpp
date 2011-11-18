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

#include "qxcbintegration.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbbackingstore.h"
#include "qxcbnativeinterface.h"
#include "qxcbclipboard.h"
#include "qxcbdrag.h"

#include <xcb/xcb.h>

#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>

#include <stdio.h>

//this has to be included before egl, since egl pulls in X headers
#include <QtGui/private/qguiapplication_p.h>

#ifdef XCB_USE_EGL
#include <EGL/egl.h>
#endif

#ifdef XCB_USE_XLIB
#include <X11/Xlib.h>
#endif

#include <private/qplatforminputcontextfactory_qpa_p.h>
#include <qplatforminputcontext_qpa.h>

#if defined(XCB_USE_GLX)
#include "qglxintegration.h"
#elif defined(XCB_USE_EGL)
#include "qxcbeglsurface.h"
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#endif

#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <QtGui/QPlatformAccessibility>

QT_BEGIN_NAMESPACE

QXcbIntegration::QXcbIntegration(const QStringList &parameters)
    : m_eventDispatcher(createUnixEventDispatcher())
{
    QGuiApplicationPrivate::instance()->setEventDispatcher(m_eventDispatcher);

#ifdef XCB_USE_XLIB
    XInitThreads();
#endif

    m_connections << new QXcbConnection;

    for (int i = 0; i < parameters.size() - 1; i += 2) {
        qDebug() << parameters.at(i) << parameters.at(i+1);
        QString display = parameters.at(i) + ':' + parameters.at(i+1);
        m_connections << new QXcbConnection(display.toAscii().constData());
    }

    foreach (QXcbConnection *connection, m_connections)
        foreach (QXcbScreen *screen, connection->screens())
            screenAdded(screen);

    m_fontDatabase = new QGenericUnixFontDatabase();
    m_nativeInterface = new QXcbNativeInterface;

    m_inputContext = QPlatformInputContextFactory::create();

    m_accessibility = new QPlatformAccessibility();
}

QXcbIntegration::~QXcbIntegration()
{
    qDeleteAll(m_connections);
    delete m_accessibility;
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

QPlatformOpenGLContext *QXcbIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(context->screen()->handle());
#if defined(XCB_USE_GLX)
    return new QGLXContext(screen, context->format(), context->shareHandle());
#elif defined(XCB_USE_EGL)
    return new QEGLXcbPlatformContext(context->format(), context->shareHandle(),
        screen->connection()->egl_display(), screen->connection());
#elif defined(XCB_USE_DRI2)
    return new QDri2Context(context->format(), context->shareHandle());
#endif
    qWarning("Cannot create platform GL context, none of GLX, EGL, DRI2 is enabled");
    return 0;
}

QPlatformBackingStore *QXcbIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QXcbBackingStore(window);
}

bool QXcbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL:
#ifdef XCB_POLL_FOR_QUEUED_EVENT
        return true;
#else
        return false;
#endif
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
    return m_fontDatabase;
}

QPlatformNativeInterface * QXcbIntegration::nativeInterface() const
{
    return m_nativeInterface;
}

QPlatformClipboard *QXcbIntegration::clipboard() const
{
    return m_connections.at(0)->clipboard();
}

QPlatformDrag *QXcbIntegration::drag() const
{
    return m_connections.at(0)->drag();
}

QPlatformInputContext *QXcbIntegration::inputContext() const
{
    return m_inputContext;
}

QPlatformAccessibility *QXcbIntegration::accessibility() const
{
    return m_accessibility;
}

QT_END_NAMESPACE
