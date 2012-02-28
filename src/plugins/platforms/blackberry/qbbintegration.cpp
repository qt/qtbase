/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qbbintegration.h"
#include "qbbeventthread.h"
#include "qbbglbackingstore.h"
#include "qbbglcontext.h"
#include "qbbnavigatorthread.h"
#include "qbbrasterbackingstore.h"
#include "qbbscreen.h"
#include "qbbwindow.h"
#include "qbbvirtualkeyboard.h"
#include "qbbclipboard.h"
#include "qbbglcontext.h"

#if defined(QBB_IMF)
#include "qbbinputcontext_imf.h"
#else
#include "qbbinputcontext_noimf.h"
#endif

#include "private/qgenericunixfontdatabase_p.h"
#include "private/qgenericunixeventdispatcher_p.h"

#include <QtGui/QPlatformWindow>
#include <QtGui/QWindowSystemInterface>
#include <QtGui/QOpenGLContext>

#include <QtCore/QDebug>
#include <QtCore/QHash>

#include <errno.h>

QT_BEGIN_NAMESPACE

QBBWindowMapper QBBIntegration::ms_windowMapper;
QMutex QBBIntegration::ms_windowMapperMutex;

QBBIntegration::QBBIntegration()
    : QPlatformIntegration()
    , m_eventThread(0)
    , m_navigatorThread(0)
    , m_inputContext(0)
    , m_fontDatabase(new QGenericUnixFontDatabase())
    , m_paintUsingOpenGL(false)
    , m_eventDispatcher(createUnixEventDispatcher())
#ifndef QT_NO_CLIPBOARD
    , m_clipboard(0)
#endif
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // Open connection to QNX composition manager
    errno = 0;
    int result = screen_create_context(&m_screenContext, SCREEN_APPLICATION_CONTEXT);
    if (result != 0) {
        qFatal("QBB: failed to connect to composition manager, errno=%d", errno);
    }

    // Create displays for all possible screens (which may not be attached)
    QBBScreen::createDisplays(m_screenContext);
    Q_FOREACH (QPlatformScreen *screen, QBBScreen::screens()) {
        screenAdded(screen);
    }

    // Initialize global OpenGL resources
    QBBGLContext::initialize();

    // Create/start event thread
    m_eventThread = new QBBEventThread(m_screenContext, *QBBScreen::primaryDisplay());
    m_eventThread->start();

    // Create/start navigator thread
    m_navigatorThread = new QBBNavigatorThread(*QBBScreen::primaryDisplay());
    m_navigatorThread->start();

    // Create/start the keyboard class.
    QBBVirtualKeyboard::instance();

    // Set up the input context
    m_inputContext = new QBBInputContext;
}

QBBIntegration::~QBBIntegration()
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << "QBB: platform plugin shutdown begin";
#endif
    // Destroy the keyboard class.
    QBBVirtualKeyboard::destroy();

#ifndef QT_NO_CLIPBOARD
    // Delete the clipboard
    delete m_clipboard;
#endif

    // Stop/destroy event thread
    delete m_eventThread;

    // Stop/destroy navigator thread
    delete m_navigatorThread;

    // Destroy all displays
    QBBScreen::destroyDisplays();

    // Close connection to QNX composition manager
    screen_destroy_context(m_screenContext);

    // Cleanup global OpenGL resources
    QBBGLContext::shutdown();

#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << "QBB: platform plugin shutdown end";
#endif
}

bool QBBIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    switch (cap) {
    case ThreadedPixmaps: return true;
#if defined(QT_OPENGL_ES)
    case OpenGL:
        return true;
#endif
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QBBIntegration::createPlatformWindow(QWindow *window) const
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // New windows are created on the primary display.
    return new QBBWindow(window, m_screenContext);
}

QPlatformBackingStore *QBBIntegration::createPlatformBackingStore(QWindow *window) const
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    if (paintUsingOpenGL())
        return new QBBGLBackingStore(window);
    else
        return new QBBRasterBackingStore(window);
}

QPlatformOpenGLContext *QBBIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    return new QBBGLContext(context);
}

QPlatformInputContext *QBBIntegration::inputContext() const
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    return m_inputContext;
}

void QBBIntegration::moveToScreen(QWindow *window, int screen)
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << "QBBIntegration::moveToScreen - w=" << window << ", s=" << screen;
#endif

    // get platform window used by widget
    QBBWindow *platformWindow = static_cast<QBBWindow *>(window->handle());

    // lookup platform screen by index
    QBBScreen *platformScreen = static_cast<QBBScreen*>(QBBScreen::screens().at(screen));

    // move the platform window to the platform screen
    platformWindow->setScreen(platformScreen);
}

QList<QPlatformScreen *> QBBIntegration::screens() const
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    return QBBScreen::screens();
}

QAbstractEventDispatcher *QBBIntegration::guiThreadEventDispatcher() const
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    return m_eventDispatcher;
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QBBIntegration::clipboard() const
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    if (!m_clipboard) {
        m_clipboard = new QBBClipboard;
    }
    return m_clipboard;
}
#endif

QVariant QBBIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    if (hint == ShowIsFullScreen)
        return true;

    return QPlatformIntegration::styleHint(hint);
}

QWindow *QBBIntegration::window(screen_window_t qnxWindow)
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    QMutexLocker locker(&ms_windowMapperMutex);
    Q_UNUSED(locker);
    return ms_windowMapper.value(qnxWindow, 0);
}

void QBBIntegration::addWindow(screen_window_t qnxWindow, QWindow *window)
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    QMutexLocker locker(&ms_windowMapperMutex);
    Q_UNUSED(locker);
    ms_windowMapper.insert(qnxWindow, window);
}

void QBBIntegration::removeWindow(screen_window_t qnxWindow)
{
#if defined(QBBINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    QMutexLocker locker(&ms_windowMapperMutex);
    Q_UNUSED(locker);
    ms_windowMapper.remove(qnxWindow);
}

QT_END_NAMESPACE
