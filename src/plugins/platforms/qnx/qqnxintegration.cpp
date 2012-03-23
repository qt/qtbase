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

#include "qqnxintegration.h"
#include "qqnxeventthread.h"
#include "qqnxglbackingstore.h"
#include "qqnxglcontext.h"
#include "qqnxnavigatoreventhandler.h"
#include "qqnxrasterbackingstore.h"
#include "qqnxscreen.h"
#include "qqnxwindow.h"
#include "qqnxvirtualkeyboard.h"
#include "qqnxclipboard.h"
#include "qqnxglcontext.h"
#include "qqnxservices.h"

#if defined(QQnx_IMF)
#include "qqnxinputcontext_imf.h"
#else
#include "qqnxinputcontext_noimf.h"
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

QQnxWindowMapper QQnxIntegration::ms_windowMapper;
QMutex QQnxIntegration::ms_windowMapperMutex;

QQnxIntegration::QQnxIntegration()
    : QPlatformIntegration()
    , m_eventThread(0)
    , m_navigatorEventHandler(0)
    , m_virtualKeyboard(0)
    , m_inputContext(0)
    , m_fontDatabase(new QGenericUnixFontDatabase())
    , m_paintUsingOpenGL(false)
    , m_eventDispatcher(createUnixEventDispatcher())
    , m_services(0)
#ifndef QT_NO_CLIPBOARD
    , m_clipboard(0)
#endif
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // Open connection to QNX composition manager
    errno = 0;
    int result = screen_create_context(&m_screenContext, SCREEN_APPLICATION_CONTEXT);
    if (result != 0) {
        qFatal("QQnx: failed to connect to composition manager, errno=%d", errno);
    }

    // Create displays for all possible screens (which may not be attached)
    QQnxScreen::createDisplays(m_screenContext);
    Q_FOREACH (QPlatformScreen *screen, QQnxScreen::screens()) {
        screenAdded(screen);
    }

    // Initialize global OpenGL resources
    QQnxGLContext::initialize();

    // Create/start event thread
    m_eventThread = new QQnxEventThread(m_screenContext);
    m_eventThread->start();

    // Create/start navigator event handler
    // Not on BlackBerry, it has specialised event dispatcher which also handles navigator events
#ifndef Q_OS_BLACKBERRY
    m_navigatorEventHandler = new QQnxNavigatorEventHandler(*QQnxScreen::primaryDisplay());

    // delay invocation of start() to the time the event loop is up and running
    // needed to have the QThread internals of the main thread properly initialized
    QMetaObject::invokeMethod(m_navigatorEventHandler, "start", Qt::QueuedConnection);
#endif

    // Create/start the keyboard class.
    m_virtualKeyboard = new QQnxVirtualKeyboard();

    // delay invocation of start() to the time the event loop is up and running
    // needed to have the QThread internals of the main thread properly initialized
    QMetaObject::invokeMethod(m_virtualKeyboard, "start", Qt::QueuedConnection);

    // TODO check if we need to do this for all screens or only the primary one
    QObject::connect(m_virtualKeyboard, SIGNAL(heightChanged(int)),
                     QQnxScreen::primaryDisplay(), SLOT(keyboardHeightChanged(int)));

    // Set up the input context
    m_inputContext = new QQnxInputContext(*m_virtualKeyboard);

    // Create services handling class
#ifdef Q_OS_BLACKBERRY
    m_services = new QQnxServices;
#endif
}

QQnxIntegration::~QQnxIntegration()
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << "QQnx: platform plugin shutdown begin";
#endif

    // Destroy input context
    delete m_inputContext;

    // Destroy the keyboard class.
    delete m_virtualKeyboard;

#ifndef QT_NO_CLIPBOARD
    // Delete the clipboard
    delete m_clipboard;
#endif

    // Stop/destroy event thread
    delete m_eventThread;

    // Stop/destroy navigator thread
    delete m_navigatorEventHandler;

    // Destroy all displays
    QQnxScreen::destroyDisplays();

    // Close connection to QNX composition manager
    screen_destroy_context(m_screenContext);

    // Cleanup global OpenGL resources
    QQnxGLContext::shutdown();

    // Destroy services class
#ifdef Q_OS_BLACKBERRY
    delete m_services;
#endif

#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << "QQnx: platform plugin shutdown end";
#endif
}

bool QQnxIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
#if defined(QQNXINTEGRATION_DEBUG)
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

QPlatformWindow *QQnxIntegration::createPlatformWindow(QWindow *window) const
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // New windows are created on the primary display.
    return new QQnxWindow(window, m_screenContext);
}

QPlatformBackingStore *QQnxIntegration::createPlatformBackingStore(QWindow *window) const
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    if (paintUsingOpenGL())
        return new QQnxGLBackingStore(window);
    else
        return new QQnxRasterBackingStore(window);
}

QPlatformOpenGLContext *QQnxIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    return new QQnxGLContext(context);
}

QPlatformInputContext *QQnxIntegration::inputContext() const
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    return m_inputContext;
}

void QQnxIntegration::moveToScreen(QWindow *window, int screen)
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << "QQnxIntegration::moveToScreen - w=" << window << ", s=" << screen;
#endif

    // get platform window used by widget
    QQnxWindow *platformWindow = static_cast<QQnxWindow *>(window->handle());

    // lookup platform screen by index
    QQnxScreen *platformScreen = static_cast<QQnxScreen*>(QQnxScreen::screens().at(screen));

    // move the platform window to the platform screen
    platformWindow->setScreen(platformScreen);
}

QList<QPlatformScreen *> QQnxIntegration::screens() const
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    return QQnxScreen::screens();
}

QAbstractEventDispatcher *QQnxIntegration::guiThreadEventDispatcher() const
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    return m_eventDispatcher;
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QQnxIntegration::clipboard() const
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    if (!m_clipboard) {
        m_clipboard = new QQnxClipboard;
    }
    return m_clipboard;
}
#endif

QVariant QQnxIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    if (hint == ShowIsFullScreen)
        return true;

    return QPlatformIntegration::styleHint(hint);
}

QPlatformServices * QQnxIntegration::services() const
{
    return m_services;
}

QWindow *QQnxIntegration::window(screen_window_t qnxWindow)
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    QMutexLocker locker(&ms_windowMapperMutex);
    Q_UNUSED(locker);
    return ms_windowMapper.value(qnxWindow, 0);
}

void QQnxIntegration::addWindow(screen_window_t qnxWindow, QWindow *window)
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    QMutexLocker locker(&ms_windowMapperMutex);
    Q_UNUSED(locker);
    ms_windowMapper.insert(qnxWindow, window);
}

void QQnxIntegration::removeWindow(screen_window_t qnxWindow)
{
#if defined(QQNXINTEGRATION_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    QMutexLocker locker(&ms_windowMapperMutex);
    Q_UNUSED(locker);
    ms_windowMapper.remove(qnxWindow);
}

QT_END_NAMESPACE
