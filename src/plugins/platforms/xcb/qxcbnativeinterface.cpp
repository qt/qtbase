/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbnativeinterface.h"

#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbintegration.h"
#include "qxcbsystemtraytracker.h"

#include <private/qguiapplication_p.h>
#include <QtCore/QMap>

#include <QtCore/QDebug>

#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>

#include <QtPlatformHeaders/qxcbwindowfunctions.h>
#include <QtPlatformHeaders/qxcbintegrationfunctions.h>

#ifndef QT_NO_DBUS
#include "QtPlatformSupport/private/qdbusmenuconnection_p.h"
#endif

#ifdef XCB_USE_XLIB
#  include <X11/Xlib.h>
#else
#  include <stdio.h>
#endif

#include <algorithm>

#include "qxcbnativeinterfacehandler.h"

QT_BEGIN_NAMESPACE

// return QXcbNativeInterface::ResourceType for the key.
static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match QXcbNativeInterface::ResourceType
        QByteArrayLiteral("display"),
        QByteArrayLiteral("connection"), QByteArrayLiteral("screen"),
        QByteArrayLiteral("apptime"),
        QByteArrayLiteral("appusertime"), QByteArrayLiteral("hintstyle"),
        QByteArrayLiteral("startupid"), QByteArrayLiteral("traywindow"),
        QByteArrayLiteral("gettimestamp"), QByteArrayLiteral("x11screen"),
        QByteArrayLiteral("rootwindow"),
        QByteArrayLiteral("subpixeltype"), QByteArrayLiteral("antialiasingEnabled"),
        QByteArrayLiteral("nofonthinting"),
        QByteArrayLiteral("atspibus")
    };
    const QByteArray *end = names + sizeof(names) / sizeof(names[0]);
    const QByteArray *result = std::find(names, end, key);
    return int(result - names);
}

QXcbNativeInterface::QXcbNativeInterface() :
    m_genericEventFilterType(QByteArrayLiteral("xcb_generic_event_t")),
    m_sysTraySelectionAtom(XCB_ATOM_NONE)
{
}

void QXcbNativeInterface::beep() // For QApplication::beep()
{
    QScreen *priScreen = QGuiApplication::primaryScreen();
    if (!priScreen)
        return;
    QPlatformScreen *screen = priScreen->handle();
    if (!screen)
        return;
    xcb_connection_t *connection = static_cast<QXcbScreen *>(screen)->xcb_connection();
    xcb_bell(connection, 0);
}

static inline QXcbSystemTrayTracker *systemTrayTracker(const QScreen *s)
{
    if (!s)
        return Q_NULLPTR;

    return static_cast<const QXcbScreen *>(s->handle())->connection()->systemTrayTracker();
}

bool QXcbNativeInterface::systemTrayAvailable(const QScreen *screen) const
{
    return systemTrayTracker(screen);
}

bool QXcbNativeInterface::requestSystemTrayWindowDock(const QWindow *window)
{
    return QXcbWindow::requestSystemTrayWindowDockStatic(window);
}

QRect QXcbNativeInterface::systemTrayWindowGlobalGeometry(const QWindow *window)
{
    return QXcbWindow::systemTrayWindowGlobalGeometryStatic(window);
}

xcb_window_t QXcbNativeInterface::locateSystemTray(xcb_connection_t *conn, const QXcbScreen *screen)
{
    if (m_sysTraySelectionAtom == XCB_ATOM_NONE) {
        const QByteArray net_sys_tray = QString::fromLatin1("_NET_SYSTEM_TRAY_S%1").arg(screen->screenNumber()).toLatin1();
        xcb_intern_atom_cookie_t intern_c =
            xcb_intern_atom_unchecked(conn, true, net_sys_tray.length(), net_sys_tray);

        xcb_intern_atom_reply_t *intern_r = xcb_intern_atom_reply(conn, intern_c, 0);

        if (!intern_r)
            return XCB_WINDOW_NONE;

        m_sysTraySelectionAtom = intern_r->atom;
        free(intern_r);
    }

    xcb_get_selection_owner_cookie_t sel_owner_c = xcb_get_selection_owner_unchecked(conn, m_sysTraySelectionAtom);
    xcb_get_selection_owner_reply_t *sel_owner_r = xcb_get_selection_owner_reply(conn, sel_owner_c, 0);

    if (!sel_owner_r)
        return XCB_WINDOW_NONE;

    xcb_window_t selection_window = sel_owner_r->owner;
    free(sel_owner_r);

    return selection_window;
}

bool QXcbNativeInterface::systrayVisualHasAlphaChannel()
{
    return QXcbConnection::xEmbedSystemTrayVisualHasAlphaChannel();
}

void QXcbNativeInterface::setParentRelativeBackPixmap(QWindow *window)
{
    QXcbWindow::setParentRelativeBackPixmapStatic(window);
}

void *QXcbNativeInterface::nativeResourceForIntegration(const QByteArray &resourceString)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    void *result = handlerNativeResourceForIntegration(lowerCaseResource);
    if (result)
        return result;

    switch (resourceType(lowerCaseResource)) {
    case StartupId:
        result = startupId();
        break;
    case X11Screen:
        result = x11Screen();
        break;
    case RootWindow:
        result = rootWindow();
        break;
    case Display:
        result = display();
        break;
    case AtspiBus:
        result = atspiBus();
        break;
    case Connection:
        result = connection();
        break;
    default:
        break;
    }

    return result;
}

void *QXcbNativeInterface::nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    void *result = handlerNativeResourceForContext(lowerCaseResource, context);
    return result;
}

void *QXcbNativeInterface::nativeResourceForScreen(const QByteArray &resourceString, QScreen *screen)
{
    if (!screen) {
        qWarning() << "nativeResourceForScreen: null screen";
        return Q_NULLPTR;
    }

    QByteArray lowerCaseResource = resourceString.toLower();
    void *result = handlerNativeResourceForScreen(lowerCaseResource, screen);
    if (result)
        return result;

    const QXcbScreen *xcbScreen = static_cast<QXcbScreen *>(screen->handle());
    switch (resourceType(lowerCaseResource)) {
    case Display:
#ifdef XCB_USE_XLIB
        result = xcbScreen->connection()->xlib_display();
#endif
        break;
    case AppTime:
        result = appTime(xcbScreen);
        break;
    case AppUserTime:
        result = appUserTime(xcbScreen);
        break;
    case ScreenHintStyle:
        result = reinterpret_cast<void *>(xcbScreen->hintStyle() + 1);
        break;
    case ScreenSubpixelType:
        result = reinterpret_cast<void *>(xcbScreen->subpixelType() + 1);
        break;
    case ScreenAntialiasingEnabled:
        result = reinterpret_cast<void *>(xcbScreen->antialiasingEnabled() + 1);
        break;
    case TrayWindow:
        if (QXcbSystemTrayTracker *s = systemTrayTracker(screen))
            result = (void *)quintptr(s->trayWindow());
        break;
    case GetTimestamp:
        result = getTimestamp(xcbScreen);
        break;
    case NoFontHinting:
        result = xcbScreen->noFontHinting() ? this : 0; //qboolptr...
        break;
    case RootWindow:
        result = reinterpret_cast<void *>(xcbScreen->root());
        break;
    default:
        break;
    }
    return result;
}

void *QXcbNativeInterface::nativeResourceForWindow(const QByteArray &resourceString, QWindow *window)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    void *result = handlerNativeResourceForWindow(lowerCaseResource, window);
    if (result)
        return result;

    switch (resourceType(lowerCaseResource)) {
    case Display:
        result = displayForWindow(window);
        break;
    case Connection:
        result = connectionForWindow(window);
        break;
    case Screen:
        result = screenForWindow(window);
        break;
    default:
        break;
    }

    return result;
}

void *QXcbNativeInterface::nativeResourceForBackingStore(const QByteArray &resourceString, QBackingStore *backingStore)
{
    const QByteArray lowerCaseResource = resourceString.toLower();
    void *result = handlerNativeResourceForBackingStore(lowerCaseResource,backingStore);
    return result;
}


QPlatformNativeInterface::NativeResourceForIntegrationFunction QXcbNativeInterface::nativeResourceFunctionForIntegration(const QByteArray &resource)
{
    const QByteArray lowerCaseResource = resource.toLower();
    QPlatformNativeInterface::NativeResourceForIntegrationFunction func = handlerNativeResourceFunctionForIntegration(lowerCaseResource);
    if (func)
        return func;

    if (lowerCaseResource == "setstartupid")
        return NativeResourceForIntegrationFunction(setStartupId);
    return 0;
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbNativeInterface::nativeResourceFunctionForContext(const QByteArray &resource)
{
    const QByteArray lowerCaseResource = resource.toLower();
    QPlatformNativeInterface::NativeResourceForContextFunction func = handlerNativeResourceFunctionForContext(lowerCaseResource);
    if (func)
        return func;
    return Q_NULLPTR;
}

QPlatformNativeInterface::NativeResourceForScreenFunction QXcbNativeInterface::nativeResourceFunctionForScreen(const QByteArray &resource)
{
    const QByteArray lowerCaseResource = resource.toLower();
    NativeResourceForScreenFunction func = handlerNativeResourceFunctionForScreen(lowerCaseResource);
    if (func)
        return func;

    if (lowerCaseResource == "setapptime")
        return NativeResourceForScreenFunction(setAppTime);
    else if (lowerCaseResource == "setappusertime")
        return NativeResourceForScreenFunction(setAppUserTime);
    return 0;
}

QPlatformNativeInterface::NativeResourceForWindowFunction QXcbNativeInterface::nativeResourceFunctionForWindow(const QByteArray &resource)
{
    const QByteArray lowerCaseResource = resource.toLower();
    NativeResourceForWindowFunction func = handlerNativeResourceFunctionForWindow(lowerCaseResource);
    return func;
}

QPlatformNativeInterface::NativeResourceForBackingStoreFunction QXcbNativeInterface::nativeResourceFunctionForBackingStore(const QByteArray &resource)
{
    const QByteArray lowerCaseResource = resource.toLower();
    NativeResourceForBackingStoreFunction func = handlerNativeResourceFunctionForBackingStore(resource);
    return func;
}

QFunctionPointer QXcbNativeInterface::platformFunction(const QByteArray &function) const
{
    const QByteArray lowerCaseFunction = function.toLower();
    QFunctionPointer func = handlerPlatformFunction(lowerCaseFunction);
    if (func)
        return func;

    //case sensitive
    if (function == QXcbWindowFunctions::setWmWindowTypeIdentifier())
        return QFunctionPointer(QXcbWindowFunctions::SetWmWindowType(QXcbWindow::setWmWindowTypeStatic));

    if (function == QXcbWindowFunctions::setWmWindowIconTextIdentifier())
        return QFunctionPointer(QXcbWindowFunctions::SetWmWindowIconText(QXcbWindow::setWindowIconTextStatic));

    if (function == QXcbWindowFunctions::setParentRelativeBackPixmapIdentifier())
        return QFunctionPointer(QXcbWindowFunctions::SetParentRelativeBackPixmap(QXcbWindow::setParentRelativeBackPixmapStatic));

    if (function == QXcbWindowFunctions::requestSystemTrayWindowDockIdentifier())
        return QFunctionPointer(QXcbWindowFunctions::RequestSystemTrayWindowDock(QXcbWindow::requestSystemTrayWindowDockStatic));

    if (function == QXcbWindowFunctions::systemTrayWindowGlobalGeometryIdentifier())
        return QFunctionPointer(QXcbWindowFunctions::SystemTrayWindowGlobalGeometry(QXcbWindow::systemTrayWindowGlobalGeometryStatic));

    if (function == QXcbIntegrationFunctions::xEmbedSystemTrayVisualHasAlphaChannelIdentifier())
        return QFunctionPointer(QXcbIntegrationFunctions::XEmbedSystemTrayVisualHasAlphaChannel(QXcbConnection::xEmbedSystemTrayVisualHasAlphaChannel));

    if (function == QXcbWindowFunctions::visualIdIdentifier()) {
        return QFunctionPointer(QXcbWindowFunctions::VisualId(QXcbWindow::visualIdStatic));
    }
    return Q_NULLPTR;
}

void *QXcbNativeInterface::appTime(const QXcbScreen *screen)
{
    if (!screen)
        return Q_NULLPTR;

    return reinterpret_cast<void *>(quintptr(screen->connection()->time()));
}

void *QXcbNativeInterface::appUserTime(const QXcbScreen *screen)
{
    if (!screen)
        return Q_NULLPTR;

    return reinterpret_cast<void *>(quintptr(screen->connection()->netWmUserTime()));
}

void *QXcbNativeInterface::getTimestamp(const QXcbScreen *screen)
{
    if (!screen)
        return Q_NULLPTR;

    return reinterpret_cast<void *>(quintptr(screen->connection()->getTimestamp()));
}

void *QXcbNativeInterface::startupId()
{
    QXcbIntegration* integration = QXcbIntegration::instance();
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return reinterpret_cast<void *>(const_cast<char *>(defaultConnection->startupId().constData()));
    return 0;
}

void *QXcbNativeInterface::x11Screen()
{
    QXcbIntegration *integration = QXcbIntegration::instance();
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return reinterpret_cast<void *>(defaultConnection->primaryScreenNumber());
    return 0;
}

void *QXcbNativeInterface::rootWindow()
{
    QXcbIntegration *integration = QXcbIntegration::instance();
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return reinterpret_cast<void *>(defaultConnection->rootWindow());
    return 0;
}

void *QXcbNativeInterface::display()
{
#ifdef XCB_USE_XLIB
    QXcbIntegration *integration = QXcbIntegration::instance();
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return defaultConnection->xlib_display();
#endif
    return Q_NULLPTR;
}

void *QXcbNativeInterface::connection()
{
    QXcbIntegration *integration = QXcbIntegration::instance();
    return integration->defaultConnection()->xcb_connection();
}

void *QXcbNativeInterface::atspiBus()
{
    QXcbIntegration *integration = static_cast<QXcbIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection) {
        xcb_atom_t atspiBusAtom = defaultConnection->internAtom("AT_SPI_BUS");
        xcb_get_property_cookie_t cookie = Q_XCB_CALL(xcb_get_property(defaultConnection->xcb_connection(), false,
                                                            defaultConnection->rootWindow(),
                                                            atspiBusAtom,
                                                            XCB_ATOM_STRING, 0, 128));
        xcb_get_property_reply_t *reply = Q_XCB_CALL(xcb_get_property_reply(defaultConnection->xcb_connection(), cookie, 0));
        Q_ASSERT(!reply->bytes_after);
        char *data = (char *)xcb_get_property_value(reply);
        int length = xcb_get_property_value_length(reply);
        QByteArray *busAddress = new QByteArray(data, length);
        free(reply);
        return busAddress;
    }
    return 0;
}

void QXcbNativeInterface::setAppTime(QScreen* screen, xcb_timestamp_t time)
{
    if (screen) {
        static_cast<QXcbScreen *>(screen->handle())->connection()->setTime(time);
    }
}

void QXcbNativeInterface::setAppUserTime(QScreen* screen, xcb_timestamp_t time)
{
    if (screen) {
        static_cast<QXcbScreen *>(screen->handle())->connection()->setNetWmUserTime(time);
    }
}

void QXcbNativeInterface::setStartupId(const char *data)
{
    QByteArray startupId(data);
    QXcbIntegration *integration = QXcbIntegration::instance();
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        defaultConnection->setStartupId(startupId);
}

QXcbScreen *QXcbNativeInterface::qPlatformScreenForWindow(QWindow *window)
{
    QXcbScreen *screen;
    if (window) {
        QScreen *qs = window->screen();
        screen = static_cast<QXcbScreen *>(qs ? qs->handle() : Q_NULLPTR);
    } else {
        QScreen *qs = QGuiApplication::primaryScreen();
        screen = static_cast<QXcbScreen *>(qs ? qs->handle() : Q_NULLPTR);
    }
    return screen;
}

void *QXcbNativeInterface::displayForWindow(QWindow *window)
{
#if defined(XCB_USE_XLIB)
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen ? screen->connection()->xlib_display() : Q_NULLPTR;
#else
    Q_UNUSED(window);
    return Q_NULLPTR;
#endif
}

void *QXcbNativeInterface::connectionForWindow(QWindow *window)
{
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen ? screen->xcb_connection() : Q_NULLPTR;
}

void *QXcbNativeInterface::screenForWindow(QWindow *window)
{
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen ? screen->screen() : Q_NULLPTR;
}

void QXcbNativeInterface::addHandler(QXcbNativeInterfaceHandler *handler)
{
    m_handlers.removeAll(handler);
    m_handlers.prepend(handler);
}

void QXcbNativeInterface::removeHandler(QXcbNativeInterfaceHandler *handler)
{
    m_handlers.removeAll(handler);
}

QPlatformNativeInterface::NativeResourceForIntegrationFunction QXcbNativeInterface::handlerNativeResourceFunctionForIntegration(const QByteArray &resource) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        NativeResourceForIntegrationFunction result = handler->nativeResourceFunctionForIntegration(resource);
        if (result)
            return result;
    }
    return Q_NULLPTR;
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbNativeInterface::handlerNativeResourceFunctionForContext(const QByteArray &resource) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        NativeResourceForContextFunction result = handler->nativeResourceFunctionForContext(resource);
        if (result)
            return result;
    }
    return Q_NULLPTR;
}

QPlatformNativeInterface::NativeResourceForScreenFunction QXcbNativeInterface::handlerNativeResourceFunctionForScreen(const QByteArray &resource) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        NativeResourceForScreenFunction result = handler->nativeResourceFunctionForScreen(resource);
        if (result)
            return result;
    }
    return Q_NULLPTR;
}

QPlatformNativeInterface::NativeResourceForWindowFunction QXcbNativeInterface::handlerNativeResourceFunctionForWindow(const QByteArray &resource) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        NativeResourceForWindowFunction result = handler->nativeResourceFunctionForWindow(resource);
        if (result)
            return result;
    }
    return Q_NULLPTR;
}

QPlatformNativeInterface::NativeResourceForBackingStoreFunction QXcbNativeInterface::handlerNativeResourceFunctionForBackingStore(const QByteArray &resource) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        NativeResourceForBackingStoreFunction result = handler->nativeResourceFunctionForBackingStore(resource);
        if (result)
            return result;
    }
    return Q_NULLPTR;
}

QFunctionPointer QXcbNativeInterface::handlerPlatformFunction(const QByteArray &function) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        QFunctionPointer func = handler->platformFunction(function);
        if (func)
            return func;
    }
    return Q_NULLPTR;
}

void *QXcbNativeInterface::handlerNativeResourceForIntegration(const QByteArray &resource) const
{
    NativeResourceForIntegrationFunction func = handlerNativeResourceFunctionForIntegration(resource);
    if (func)
        return func();
    return Q_NULLPTR;
}

void *QXcbNativeInterface::handlerNativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) const
{
    NativeResourceForContextFunction func = handlerNativeResourceFunctionForContext(resource);
    if (func)
        return func(context);
    return Q_NULLPTR;
}

void *QXcbNativeInterface::handlerNativeResourceForScreen(const QByteArray &resource, QScreen *screen) const
{
    NativeResourceForScreenFunction func = handlerNativeResourceFunctionForScreen(resource);
    if (func)
        return func(screen);
    return Q_NULLPTR;
}

void *QXcbNativeInterface::handlerNativeResourceForWindow(const QByteArray &resource, QWindow *window) const
{
    NativeResourceForWindowFunction func = handlerNativeResourceFunctionForWindow(resource);
    if (func)
        return func(window);
    return Q_NULLPTR;
}

void *QXcbNativeInterface::handlerNativeResourceForBackingStore(const QByteArray &resource, QBackingStore *backingStore) const
{
    NativeResourceForBackingStoreFunction func = handlerNativeResourceFunctionForBackingStore(resource);
    if (func)
        return func(backingStore);
    return Q_NULLPTR;
}

QT_END_NAMESPACE
