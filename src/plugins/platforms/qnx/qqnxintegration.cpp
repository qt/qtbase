/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxglobal.h"

#include "qqnxintegration.h"
#if defined(QQNX_SCREENEVENTTHREAD)
#include "qqnxscreeneventthread.h"
#endif
#include "qqnxnativeinterface.h"
#include "qqnxrasterbackingstore.h"
#include "qqnxscreen.h"
#include "qqnxscreeneventhandler.h"
#include "qqnxwindow.h"
#include "qqnxnavigatoreventhandler.h"
#include "qqnxabstractnavigator.h"
#include "qqnxabstractvirtualkeyboard.h"
#include "qqnxservices.h"

#include "qqnxrasterwindow.h"
#if !defined(QT_NO_OPENGL)
#include "qqnxeglwindow.h"
#endif

#if defined(QQNX_PPS)
#include "qqnxnavigatorpps.h"
#include "qqnxnavigatoreventnotifier.h"
#include "qqnxvirtualkeyboardpps.h"
#endif

#if defined(QQNX_PPS)
#  include "qqnxbuttoneventnotifier.h"
#  include "qqnxclipboard.h"

#  if defined(QQNX_IMF)
#    include "qqnxinputcontext_imf.h"
#  else
#    include "qqnxinputcontext_noimf.h"
#  endif
#endif

#include "private/qgenericunixfontdatabase_p.h"
#include "private/qgenericunixeventdispatcher_p.h"

#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtGui/private/qguiapplication_p.h>

#if !defined(QT_NO_OPENGL)
#include "qqnxglcontext.h"
#include <QtGui/QOpenGLContext>
#endif

#include <private/qsimpledrag_p.h>

#include <QtCore/QDebug>

#include <errno.h>

#if defined(QQNXINTEGRATION_DEBUG)
#define qIntegrationDebug qDebug
#else
#define qIntegrationDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxWindowMapper QQnxIntegration::ms_windowMapper;
QMutex QQnxIntegration::ms_windowMapperMutex;

static inline QQnxIntegration::Options parseOptions(const QStringList &paramList)
{
    QQnxIntegration::Options options = QQnxIntegration::NoOptions;
    if (!paramList.contains(QLatin1String("no-fullscreen"))) {
        options |= QQnxIntegration::FullScreenApplication;
    }

    if (paramList.contains(QLatin1String("flush-screen-context"))) {
        options |= QQnxIntegration::AlwaysFlushScreenContext;
    }

    if (paramList.contains(QLatin1String("rootwindow"))) {
        options |= QQnxIntegration::RootWindow;
    }

    return options;
}

QQnxIntegration::QQnxIntegration(const QStringList &paramList)
    : QPlatformIntegration()
#if defined(QQNX_SCREENEVENTTHREAD)
    , m_screenEventThread(0)
#endif
    , m_navigatorEventHandler(new QQnxNavigatorEventHandler())
    , m_virtualKeyboard(0)
#if defined(QQNX_PPS)
    , m_navigatorEventNotifier(0)
    , m_inputContext(0)
    , m_buttonsNotifier(new QQnxButtonEventNotifier())
#endif
    , m_services(0)
    , m_fontDatabase(new QGenericUnixFontDatabase())
    , m_eventDispatcher(createUnixEventDispatcher())
    , m_nativeInterface(new QQnxNativeInterface(this))
    , m_screenEventHandler(new QQnxScreenEventHandler(this))
#if !defined(QT_NO_CLIPBOARD)
    , m_clipboard(0)
#endif
    , m_navigator(0)
#if !defined(QT_NO_DRAGANDDROP)
    , m_drag(new QSimpleDrag())
#endif
{
    ms_options = parseOptions(paramList);
    qIntegrationDebug();
    // Open connection to QNX composition manager
    Q_SCREEN_CRITICALERROR(screen_create_context(&ms_screenContext, SCREEN_APPLICATION_CONTEXT),
                           "Failed to create screen context");

#if defined(QQNX_PPS)
    // Create/start navigator event notifier
    m_navigatorEventNotifier = new QQnxNavigatorEventNotifier(m_navigatorEventHandler);

    // delay invocation of start() to the time the event loop is up and running
    // needed to have the QThread internals of the main thread properly initialized
    QMetaObject::invokeMethod(m_navigatorEventNotifier, "start", Qt::QueuedConnection);
#endif

#if !defined(QT_NO_OPENGL)
    // Initialize global OpenGL resources
    QQnxGLContext::initializeContext();
#endif

    // Create/start event thread
#if defined(QQNX_SCREENEVENTTHREAD)
    m_screenEventThread = new QQnxScreenEventThread(ms_screenContext, m_screenEventHandler);
    m_screenEventThread->start();
#endif

#if defined(QQNX_PPS)
    // Create/start the keyboard class.
    m_virtualKeyboard = new QQnxVirtualKeyboardPps();

    // delay invocation of start() to the time the event loop is up and running
    // needed to have the QThread internals of the main thread properly initialized
    QMetaObject::invokeMethod(m_virtualKeyboard, "start", Qt::QueuedConnection);
#endif

#if defined(QQNX_PPS)
    m_navigator = new QQnxNavigatorPps();
#endif

    // Create services handling class
    if (m_navigator)
        m_services = new QQnxServices(m_navigator);

    createDisplays();

    if (m_virtualKeyboard) {
        // TODO check if we need to do this for all screens or only the primary one
        QObject::connect(m_virtualKeyboard, SIGNAL(heightChanged(int)),
                         primaryDisplay(), SLOT(keyboardHeightChanged(int)));

#if defined(QQNX_PPS)
        // Set up the input context
        m_inputContext = new QQnxInputContext(this, *m_virtualKeyboard);
#if defined(QQNX_IMF)
        m_screenEventHandler->addScreenEventFilter(m_inputContext);
#endif
#endif
    }

#if defined(QQNX_PPS)
    // delay invocation of start() to the time the event loop is up and running
    // needed to have the QThread internals of the main thread properly initialized
    QMetaObject::invokeMethod(m_buttonsNotifier, "start", Qt::QueuedConnection);
#endif
}

QQnxIntegration::~QQnxIntegration()
{
    qIntegrationDebug("platform plugin shutdown begin");
    delete m_nativeInterface;

#if !defined(QT_NO_DRAGANDDROP)
    // Destroy the drag object
    delete m_drag;
#endif

#if !defined(QT_NO_CLIPBOARD)
    // Delete the clipboard
    delete m_clipboard;
#endif

    // Stop/destroy navigator event notifier
#if defined(QQNX_PPS)
    delete m_navigatorEventNotifier;
#endif
    delete m_navigatorEventHandler;

#if defined(QQNX_SCREENEVENTTHREAD)
    // Stop/destroy screen event thread
    delete m_screenEventThread;
#endif

    // In case the event-dispatcher was never transferred to QCoreApplication
    delete m_eventDispatcher;

    delete m_screenEventHandler;

    // Destroy all displays
    destroyDisplays();

    // Close connection to QNX composition manager
    screen_destroy_context(ms_screenContext);

#if !defined(QT_NO_OPENGL)
    // Cleanup global OpenGL resources
    QQnxGLContext::shutdownContext();
#endif

#if defined(QQNX_PPS)
    // Destroy the hardware button notifier
    delete m_buttonsNotifier;

    // Destroy input context
    delete m_inputContext;
#endif

    // Destroy the keyboard class.
    delete m_virtualKeyboard;

    // Destroy services class
    delete m_services;

    // Destroy navigator interface
    delete m_navigator;

    qIntegrationDebug("platform plugin shutdown end");
}

bool QQnxIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    qIntegrationDebug();
    switch (cap) {
    case MultipleWindows:
    case ThreadedPixmaps:
        return true;
#if !defined(QT_NO_OPENGL)
    case OpenGL:
    case ThreadedOpenGL:
    case BufferQueueingOpenGL:
        return true;
#endif
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QQnxIntegration::createPlatformWindow(QWindow *window) const
{
    qIntegrationDebug();
    QSurface::SurfaceType surfaceType = window->surfaceType();
    const bool needRootWindow = options() & RootWindow;
    switch (surfaceType) {
    case QSurface::RasterSurface:
        return new QQnxRasterWindow(window, ms_screenContext, needRootWindow);
#if !defined(QT_NO_OPENGL)
    case QSurface::OpenGLSurface:
        return new QQnxEglWindow(window, ms_screenContext, needRootWindow);
#endif
    default:
        qFatal("QQnxWindow: unsupported window API");
    }
    return 0;
}

QPlatformBackingStore *QQnxIntegration::createPlatformBackingStore(QWindow *window) const
{
    qIntegrationDebug();
    return new QQnxRasterBackingStore(window);
}

#if !defined(QT_NO_OPENGL)
QPlatformOpenGLContext *QQnxIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    qIntegrationDebug();
    return new QQnxGLContext(context);
}
#endif

#if defined(QQNX_PPS)
QPlatformInputContext *QQnxIntegration::inputContext() const
{
    qIntegrationDebug();
    return m_inputContext;
}
#endif

void QQnxIntegration::moveToScreen(QWindow *window, int screen)
{
    qIntegrationDebug() << "w =" << window << ", s =" << screen;

    // get platform window used by widget
    QQnxWindow *platformWindow = static_cast<QQnxWindow *>(window->handle());

    // lookup platform screen by index
    QQnxScreen *platformScreen = m_screens.at(screen);

    // move the platform window to the platform screen
    platformWindow->setScreen(platformScreen);
}

QAbstractEventDispatcher *QQnxIntegration::createEventDispatcher() const
{
    qIntegrationDebug();

    // We transfer ownersip of the event-dispatcher to QtCoreApplication
    QAbstractEventDispatcher *eventDispatcher = m_eventDispatcher;
    m_eventDispatcher = 0;

    return eventDispatcher;
}

QPlatformNativeInterface *QQnxIntegration::nativeInterface() const
{
    return m_nativeInterface;
}

#if !defined(QT_NO_CLIPBOARD)
QPlatformClipboard *QQnxIntegration::clipboard() const
{
    qIntegrationDebug();

#if defined(QQNX_PPS)
    if (!m_clipboard)
        m_clipboard = new QQnxClipboard;
#endif
    return m_clipboard;
}
#endif

#if !defined(QT_NO_DRAGANDDROP)
QPlatformDrag *QQnxIntegration::drag() const
{
    return m_drag;
}
#endif

QVariant QQnxIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    qIntegrationDebug();
    if ((hint == ShowIsFullScreen) && (ms_options & FullScreenApplication))
        return true;

    return QPlatformIntegration::styleHint(hint);
}

QPlatformServices * QQnxIntegration::services() const
{
    return m_services;
}

QWindow *QQnxIntegration::window(screen_window_t qnxWindow)
{
    qIntegrationDebug();
    QMutexLocker locker(&ms_windowMapperMutex);
    Q_UNUSED(locker);
    return ms_windowMapper.value(qnxWindow, 0);
}

void QQnxIntegration::addWindow(screen_window_t qnxWindow, QWindow *window)
{
    qIntegrationDebug();
    QMutexLocker locker(&ms_windowMapperMutex);
    Q_UNUSED(locker);
    ms_windowMapper.insert(qnxWindow, window);
}

void QQnxIntegration::removeWindow(screen_window_t qnxWindow)
{
    qIntegrationDebug();
    QMutexLocker locker(&ms_windowMapperMutex);
    Q_UNUSED(locker);
    ms_windowMapper.remove(qnxWindow);
}

void QQnxIntegration::createDisplays()
{
    qIntegrationDebug();
    // Query number of displays
    int displayCount = 0;
    int result = screen_get_context_property_iv(ms_screenContext, SCREEN_PROPERTY_DISPLAY_COUNT,
                                                &displayCount);
    Q_SCREEN_CRITICALERROR(result, "Failed to query display count");

    if (Q_UNLIKELY(displayCount < 1)) {
        // Never happens, even if there's no display, libscreen returns 1
        qFatal("QQnxIntegration: displayCount=%d", displayCount);
    }

    // Get all displays
    screen_display_t *displays = (screen_display_t *)alloca(sizeof(screen_display_t) * displayCount);
    result = screen_get_context_property_pv(ms_screenContext, SCREEN_PROPERTY_DISPLAYS,
                                            (void **)displays);
    Q_SCREEN_CRITICALERROR(result, "Failed to query displays");

    // If it's primary, we create a QScreen for it even if it's not attached
    // since Qt will dereference QGuiApplication::primaryScreen()
    createDisplay(displays[0], /*isPrimary=*/true);

    for (int i=1; i<displayCount; i++) {
        int isAttached = 1;
        result = screen_get_display_property_iv(displays[i], SCREEN_PROPERTY_ATTACHED,
                                                &isAttached);
        Q_SCREEN_CHECKERROR(result, "Failed to query display attachment");

        if (!isAttached) {
            qIntegrationDebug("Skipping non-attached display %d", i);
            continue;
        }

        qIntegrationDebug("Creating screen for display %d", i);
        createDisplay(displays[i], /*isPrimary=*/false);
    } // of displays iteration
}

void QQnxIntegration::createDisplay(screen_display_t display, bool isPrimary)
{
    QQnxScreen *screen = new QQnxScreen(ms_screenContext, display, isPrimary);
    m_screens.append(screen);
    screenAdded(screen);
    screen->adjustOrientation();

    QObject::connect(m_screenEventHandler, SIGNAL(newWindowCreated(void*)),
                     screen, SLOT(newWindowCreated(void*)));
    QObject::connect(m_screenEventHandler, SIGNAL(windowClosed(void*)),
                     screen, SLOT(windowClosed(void*)));

    QObject::connect(m_navigatorEventHandler, SIGNAL(rotationChanged(int)), screen, SLOT(setRotation(int)));
    QObject::connect(m_navigatorEventHandler, SIGNAL(windowGroupActivated(QByteArray)), screen, SLOT(activateWindowGroup(QByteArray)));
    QObject::connect(m_navigatorEventHandler, SIGNAL(windowGroupDeactivated(QByteArray)), screen, SLOT(deactivateWindowGroup(QByteArray)));
    QObject::connect(m_navigatorEventHandler, SIGNAL(windowGroupStateChanged(QByteArray,Qt::WindowState)),
            screen, SLOT(windowGroupStateChanged(QByteArray,Qt::WindowState)));
}

void QQnxIntegration::removeDisplay(QQnxScreen *screen)
{
    Q_CHECK_PTR(screen);
    Q_ASSERT(m_screens.contains(screen));
    m_screens.removeAll(screen);
    destroyScreen(screen);
}

void QQnxIntegration::destroyDisplays()
{
    qIntegrationDebug();
    Q_FOREACH (QQnxScreen *screen, m_screens) {
        QPlatformIntegration::destroyScreen(screen);
    }
    m_screens.clear();
}

QQnxScreen *QQnxIntegration::screenForNative(screen_display_t qnxScreen) const
{
    Q_FOREACH (QQnxScreen *screen, m_screens) {
        if (screen->nativeDisplay() == qnxScreen)
            return screen;
    }

    return 0;
}

QQnxScreen *QQnxIntegration::primaryDisplay() const
{
    return m_screens.first();
}

QQnxIntegration::Options QQnxIntegration::options()
{
    return ms_options;
}

screen_context_t QQnxIntegration::screenContext()
{
    return ms_screenContext;
}

QQnxNavigatorEventHandler *QQnxIntegration::navigatorEventHandler()
{
    return m_navigatorEventHandler;
}

screen_context_t QQnxIntegration::ms_screenContext = 0;

QQnxIntegration::Options QQnxIntegration::ms_options = 0;

bool QQnxIntegration::supportsNavigatorEvents() const
{
    // If QQNX_PPS is defined then we have navigator
    return m_navigator != 0;
}

QT_END_NAMESPACE
