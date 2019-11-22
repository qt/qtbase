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

#include "qxcbnativeinterface.h"

#include "qxcbcursor.h"
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
#include <QtPlatformHeaders/qxcbscreenfunctions.h>

#include <stdio.h>

#include <algorithm>

#include "qxcbnativeinterfacehandler.h"

#if QT_CONFIG(vulkan)
#include "qxcbvulkanwindow.h"
#endif

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
        QByteArrayLiteral("subpixeltype"), QByteArrayLiteral("antialiasingenabled"),
        QByteArrayLiteral("atspibus"),
        QByteArrayLiteral("compositingenabled"),
        QByteArrayLiteral("vksurface"),
        QByteArrayLiteral("generatepeekerid"),
        QByteArrayLiteral("removepeekerid"),
        QByteArrayLiteral("peekeventqueue")
    };
    const QByteArray *end = names + sizeof(names) / sizeof(names[0]);
    const QByteArray *result = std::find(names, end, key);
    return int(result - names);
}

QXcbNativeInterface::QXcbNativeInterface()
{
}

static inline QXcbSystemTrayTracker *systemTrayTracker(const QScreen *s)
{
    if (!s)
        return nullptr;

    return static_cast<const QXcbScreen *>(s->handle())->connection()->systemTrayTracker();
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
        qWarning("nativeResourceForScreen: null screen");
        return nullptr;
    }

    QByteArray lowerCaseResource = resourceString.toLower();
    void *result = handlerNativeResourceForScreen(lowerCaseResource, screen);
    if (result)
        return result;

    const QXcbScreen *xcbScreen = static_cast<QXcbScreen *>(screen->handle());
    switch (resourceType(lowerCaseResource)) {
    case Display:
#if QT_CONFIG(xcb_xlib)
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
    case RootWindow:
        result = reinterpret_cast<void *>(xcbScreen->root());
        break;
    case CompositingEnabled:
        if (QXcbVirtualDesktop *vd = xcbScreen->virtualDesktop())
            result = vd->compositingActive() ? this : nullptr;
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
#if QT_CONFIG(vulkan)
    case VkSurface:
        if (window->surfaceType() == QSurface::VulkanSurface && window->handle()) {
            // return a pointer to the VkSurfaceKHR value, not the value itself
            result = static_cast<QXcbVulkanWindow *>(window->handle())->surface();
        }
        break;
#endif
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

#ifndef QT_NO_CURSOR
void *QXcbNativeInterface::nativeResourceForCursor(const QByteArray &resource, const QCursor &cursor)
{
    if (resource == QByteArrayLiteral("xcbcursor")) {
        if (const QScreen *primaryScreen = QGuiApplication::primaryScreen()) {
            if (const QPlatformCursor *pCursor= primaryScreen->handle()->cursor()) {
                xcb_cursor_t xcbCursor = static_cast<const QXcbCursor *>(pCursor)->xcbCursor(cursor);
                return reinterpret_cast<void *>(quintptr(xcbCursor));
            }
        }
    }
    return nullptr;
}
#endif // !QT_NO_CURSOR

QPlatformNativeInterface::NativeResourceForIntegrationFunction QXcbNativeInterface::nativeResourceFunctionForIntegration(const QByteArray &resource)
{
    const QByteArray lowerCaseResource = resource.toLower();
    QPlatformNativeInterface::NativeResourceForIntegrationFunction func = handlerNativeResourceFunctionForIntegration(lowerCaseResource);
    if (func)
        return func;

    if (lowerCaseResource == "setstartupid")
        return NativeResourceForIntegrationFunction(reinterpret_cast<void *>(setStartupId));
    if (lowerCaseResource == "generatepeekerid")
        return NativeResourceForIntegrationFunction(reinterpret_cast<void *>(generatePeekerId));
    if (lowerCaseResource == "removepeekerid")
        return NativeResourceForIntegrationFunction(reinterpret_cast<void *>(removePeekerId));
    if (lowerCaseResource == "peekeventqueue")
        return NativeResourceForIntegrationFunction(reinterpret_cast<void *>(peekEventQueue));

    return nullptr;
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbNativeInterface::nativeResourceFunctionForContext(const QByteArray &resource)
{
    const QByteArray lowerCaseResource = resource.toLower();
    QPlatformNativeInterface::NativeResourceForContextFunction func = handlerNativeResourceFunctionForContext(lowerCaseResource);
    if (func)
        return func;
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForScreenFunction QXcbNativeInterface::nativeResourceFunctionForScreen(const QByteArray &resource)
{
    const QByteArray lowerCaseResource = resource.toLower();
    NativeResourceForScreenFunction func = handlerNativeResourceFunctionForScreen(lowerCaseResource);
    if (func)
        return func;

    if (lowerCaseResource == "setapptime")
        return NativeResourceForScreenFunction(reinterpret_cast<void *>(setAppTime));
    else if (lowerCaseResource == "setappusertime")
        return NativeResourceForScreenFunction(reinterpret_cast<void *>(setAppUserTime));
    return nullptr;
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
    NativeResourceForBackingStoreFunction func = handlerNativeResourceFunctionForBackingStore(lowerCaseResource);
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

    if (function == QXcbWindowFunctions::setWmWindowRoleIdentifier())
        return QFunctionPointer(QXcbWindowFunctions::SetWmWindowRole(QXcbWindow::setWmWindowRoleStatic));

    if (function == QXcbWindowFunctions::setWmWindowIconTextIdentifier())
        return QFunctionPointer(QXcbWindowFunctions::SetWmWindowIconText(QXcbWindow::setWindowIconTextStatic));

    if (function == QXcbWindowFunctions::visualIdIdentifier()) {
        return QFunctionPointer(QXcbWindowFunctions::VisualId(QXcbWindow::visualIdStatic));
    }

    if (function == QXcbScreenFunctions::virtualDesktopNumberIdentifier())
        return QFunctionPointer(QXcbScreenFunctions::VirtualDesktopNumber(reinterpret_cast<void *>(QXcbScreen::virtualDesktopNumberStatic)));

    return nullptr;
}

void *QXcbNativeInterface::appTime(const QXcbScreen *screen)
{
    if (!screen)
        return nullptr;

    return reinterpret_cast<void *>(quintptr(screen->connection()->time()));
}

void *QXcbNativeInterface::appUserTime(const QXcbScreen *screen)
{
    if (!screen)
        return nullptr;

    return reinterpret_cast<void *>(quintptr(screen->connection()->netWmUserTime()));
}

void *QXcbNativeInterface::getTimestamp(const QXcbScreen *screen)
{
    if (!screen)
        return nullptr;

    return reinterpret_cast<void *>(quintptr(screen->connection()->getTimestamp()));
}

void *QXcbNativeInterface::startupId()
{
    QXcbIntegration* integration = QXcbIntegration::instance();
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return reinterpret_cast<void *>(const_cast<char *>(defaultConnection->startupId().constData()));
    return nullptr;
}

void *QXcbNativeInterface::x11Screen()
{
    QXcbIntegration *integration = QXcbIntegration::instance();
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return reinterpret_cast<void *>(defaultConnection->primaryScreenNumber());
    return nullptr;
}

void *QXcbNativeInterface::rootWindow()
{
    QXcbIntegration *integration = QXcbIntegration::instance();
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return reinterpret_cast<void *>(defaultConnection->rootWindow());
    return nullptr;
}

void *QXcbNativeInterface::display()
{
#if QT_CONFIG(xcb_xlib)
    QXcbIntegration *integration = QXcbIntegration::instance();
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return defaultConnection->xlib_display();
#endif
    return nullptr;
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
        auto atspiBusAtom = defaultConnection->atom(QXcbAtom::AT_SPI_BUS);
        auto reply = Q_XCB_REPLY(xcb_get_property, defaultConnection->xcb_connection(),
                                     false, defaultConnection->rootWindow(),
                                     atspiBusAtom, XCB_ATOM_STRING, 0, 128);
        if (!reply)
            return nullptr;

        char *data = (char *)xcb_get_property_value(reply.get());
        int length = xcb_get_property_value_length(reply.get());
        return new QByteArray(data, length);
    }

    return nullptr;
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

qint32 QXcbNativeInterface::generatePeekerId()
{
    QXcbIntegration *integration = QXcbIntegration::instance();
    return integration->defaultConnection()->eventQueue()->generatePeekerId();
}

bool QXcbNativeInterface::removePeekerId(qint32 peekerId)
{
    QXcbIntegration *integration = QXcbIntegration::instance();
    return integration->defaultConnection()->eventQueue()->removePeekerId(peekerId);
}

bool QXcbNativeInterface::peekEventQueue(QXcbEventQueue::PeekerCallback peeker, void *peekerData,
                                         QXcbEventQueue::PeekOptions option, qint32 peekerId)
{
    QXcbIntegration *integration = QXcbIntegration::instance();
    return integration->defaultConnection()->eventQueue()->peekEventQueue(peeker, peekerData, option, peekerId);
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
        screen = static_cast<QXcbScreen *>(qs ? qs->handle() : nullptr);
    } else {
        QScreen *qs = QGuiApplication::primaryScreen();
        screen = static_cast<QXcbScreen *>(qs ? qs->handle() : nullptr);
    }
    return screen;
}

void *QXcbNativeInterface::displayForWindow(QWindow *window)
{
#if QT_CONFIG(xcb_xlib)
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen ? screen->connection()->xlib_display() : nullptr;
#else
    Q_UNUSED(window);
    return nullptr;
#endif
}

void *QXcbNativeInterface::connectionForWindow(QWindow *window)
{
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen ? screen->xcb_connection() : nullptr;
}

void *QXcbNativeInterface::screenForWindow(QWindow *window)
{
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen ? screen->screen() : nullptr;
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
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbNativeInterface::handlerNativeResourceFunctionForContext(const QByteArray &resource) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        NativeResourceForContextFunction result = handler->nativeResourceFunctionForContext(resource);
        if (result)
            return result;
    }
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForScreenFunction QXcbNativeInterface::handlerNativeResourceFunctionForScreen(const QByteArray &resource) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        NativeResourceForScreenFunction result = handler->nativeResourceFunctionForScreen(resource);
        if (result)
            return result;
    }
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForWindowFunction QXcbNativeInterface::handlerNativeResourceFunctionForWindow(const QByteArray &resource) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        NativeResourceForWindowFunction result = handler->nativeResourceFunctionForWindow(resource);
        if (result)
            return result;
    }
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForBackingStoreFunction QXcbNativeInterface::handlerNativeResourceFunctionForBackingStore(const QByteArray &resource) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        NativeResourceForBackingStoreFunction result = handler->nativeResourceFunctionForBackingStore(resource);
        if (result)
            return result;
    }
    return nullptr;
}

QFunctionPointer QXcbNativeInterface::handlerPlatformFunction(const QByteArray &function) const
{
    for (int i = 0; i < m_handlers.size(); i++) {
        QXcbNativeInterfaceHandler *handler = m_handlers.at(i);
        QFunctionPointer func = handler->platformFunction(function);
        if (func)
            return func;
    }
    return nullptr;
}

void *QXcbNativeInterface::handlerNativeResourceForIntegration(const QByteArray &resource) const
{
    NativeResourceForIntegrationFunction func = handlerNativeResourceFunctionForIntegration(resource);
    if (func)
        return func();
    return nullptr;
}

void *QXcbNativeInterface::handlerNativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) const
{
    NativeResourceForContextFunction func = handlerNativeResourceFunctionForContext(resource);
    if (func)
        return func(context);
    return nullptr;
}

void *QXcbNativeInterface::handlerNativeResourceForScreen(const QByteArray &resource, QScreen *screen) const
{
    NativeResourceForScreenFunction func = handlerNativeResourceFunctionForScreen(resource);
    if (func)
        return func(screen);
    return nullptr;
}

void *QXcbNativeInterface::handlerNativeResourceForWindow(const QByteArray &resource, QWindow *window) const
{
    NativeResourceForWindowFunction func = handlerNativeResourceFunctionForWindow(resource);
    if (func)
        return func(window);
    return nullptr;
}

void *QXcbNativeInterface::handlerNativeResourceForBackingStore(const QByteArray &resource, QBackingStore *backingStore) const
{
    NativeResourceForBackingStoreFunction func = handlerNativeResourceFunctionForBackingStore(resource);
    if (func)
        return func(backingStore);
    return nullptr;
}

static void dumpNativeWindowsRecursion(const QXcbConnection *connection, xcb_window_t window,
                                       int level, QTextStream &str)
{
    if (level)
        str << QByteArray(2 * level, ' ');

    xcb_connection_t *conn = connection->xcb_connection();
    auto geomReply = Q_XCB_REPLY(xcb_get_geometry, conn, window);
    if (!geomReply)
        return;
    const QRect geom(geomReply->x, geomReply->y, geomReply->width, geomReply->height);
    if (!geom.isValid() || (geom.width() <= 3 && geom.height() <= 3))
        return; // Skip helper/dummy windows.
    str << "0x";
    const int oldFieldWidth = str.fieldWidth();
    const QChar oldPadChar =str.padChar();
    str.setFieldWidth(8);
    str.setPadChar(QLatin1Char('0'));
    str << Qt::hex << window;
    str.setFieldWidth(oldFieldWidth);
    str.setPadChar(oldPadChar);
    str << Qt::dec << " \""
        << QXcbWindow::windowTitle(connection, window) << "\" "
        << geom.width() << 'x' << geom.height() << Qt::forcesign << geom.x() << geom.y()
        << Qt::noforcesign << '\n';

    auto reply = Q_XCB_REPLY(xcb_query_tree, conn, window);
    if (reply) {
        const int count = xcb_query_tree_children_length(reply.get());
        const xcb_window_t *children = xcb_query_tree_children(reply.get());
        for (int i = 0; i < count; ++i)
            dumpNativeWindowsRecursion(connection, children[i], level + 1, str);
    }
}

QString QXcbNativeInterface::dumpConnectionNativeWindows(const QXcbConnection *connection, WId root) const
{
    QString result;
    QTextStream str(&result);
    if (root) {
        dumpNativeWindowsRecursion(connection, xcb_window_t(root), 0, str);
    } else {
        for (const QXcbScreen *screen : connection->screens()) {
            str << "Screen: \"" << screen->name() << "\"\n";
            dumpNativeWindowsRecursion(connection, screen->root(), 0, str);
            str << '\n';
        }
    }
    return result;
}

QString QXcbNativeInterface::dumpNativeWindows(WId root) const
{
    return dumpConnectionNativeWindows(QXcbIntegration::instance()->defaultConnection(), root);
}

QT_END_NAMESPACE
