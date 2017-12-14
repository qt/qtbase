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

#ifndef QXCBNATIVEINTERFACE_H
#define QXCBNATIVEINTERFACE_H

#include <qpa/qplatformnativeinterface.h>
#include <xcb/xcb.h>

#include <QtCore/QRect>

#include "qxcbexport.h"
#include "qxcbconnection.h"

QT_BEGIN_NAMESPACE

class QXcbScreen;
class QXcbNativeInterfaceHandler;

class Q_XCB_EXPORT QXcbNativeInterface : public QPlatformNativeInterface
{
    Q_OBJECT
public:
    enum ResourceType {
        Display,
        Connection,
        Screen,
        AppTime,
        AppUserTime,
        ScreenHintStyle,
        StartupId,
        TrayWindow,
        GetTimestamp,
        X11Screen,
        RootWindow,
        ScreenSubpixelType,
        ScreenAntialiasingEnabled,
        AtspiBus,
        CompositingEnabled,
        VkSurface,
        GeneratePeekerId,
        RemovePeekerId,
        PeekEventQueue
    };

    QXcbNativeInterface();

    void *nativeResourceForIntegration(const QByteArray &resource) override;
    void *nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context) override;
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) override;
    void *nativeResourceForWindow(const QByteArray &resourceString, QWindow *window) override;
    void *nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *backingStore) override;
#ifndef QT_NO_CURSOR
    void *nativeResourceForCursor(const QByteArray &resource, const QCursor &cursor) override;
#endif

    NativeResourceForIntegrationFunction nativeResourceFunctionForIntegration(const QByteArray &resource) override;
    NativeResourceForContextFunction nativeResourceFunctionForContext(const QByteArray &resource) override;
    NativeResourceForScreenFunction nativeResourceFunctionForScreen(const QByteArray &resource) override;
    NativeResourceForWindowFunction nativeResourceFunctionForWindow(const QByteArray &resource) override;
    NativeResourceForBackingStoreFunction nativeResourceFunctionForBackingStore(const QByteArray &resource) override;

    QFunctionPointer platformFunction(const QByteArray &function) const override;

    inline const QByteArray &genericEventFilterType() const { return m_genericEventFilterType; }

    void *displayForWindow(QWindow *window);
    void *connectionForWindow(QWindow *window);
    void *screenForWindow(QWindow *window);
    void *appTime(const QXcbScreen *screen);
    void *appUserTime(const QXcbScreen *screen);
    void *getTimestamp(const QXcbScreen *screen);
    void *startupId();
    void *x11Screen();
    void *rootWindow();
    void *display();
    void *atspiBus();
    void *connection();
    static void setStartupId(const char *);
    static void setAppTime(QScreen *screen, xcb_timestamp_t time);
    static void setAppUserTime(QScreen *screen, xcb_timestamp_t time);

    static qint32 generatePeekerId();
    static bool removePeekerId(qint32 peekerId);
    static bool peekEventQueue(QXcbConnection::PeekerCallback peeker, void *peekerData = nullptr,
                               QXcbConnection::PeekOptions option = QXcbConnection::PeekDefault,
                               qint32 peekerId = -1);

    Q_INVOKABLE bool systemTrayAvailable(const QScreen *screen) const;
    Q_INVOKABLE void setParentRelativeBackPixmap(QWindow *window);
    Q_INVOKABLE bool systrayVisualHasAlphaChannel();
    Q_INVOKABLE bool requestSystemTrayWindowDock(const QWindow *window);
    Q_INVOKABLE QRect systemTrayWindowGlobalGeometry(const QWindow *window);
    Q_INVOKABLE QString dumpConnectionNativeWindows(const QXcbConnection *connection, WId root) const;
    Q_INVOKABLE QString dumpNativeWindows(WId root = 0) const;

    void addHandler(QXcbNativeInterfaceHandler *handler);
    void removeHandler(QXcbNativeInterfaceHandler *handler);
signals:
    void systemTrayWindowChanged(QScreen *screen);

private:
    xcb_window_t locateSystemTray(xcb_connection_t *conn, const QXcbScreen *screen);

    const QByteArray m_genericEventFilterType;

    xcb_atom_t m_sysTraySelectionAtom = XCB_ATOM_NONE;

    static QXcbScreen *qPlatformScreenForWindow(QWindow *window);

    QList<QXcbNativeInterfaceHandler *> m_handlers;
    NativeResourceForIntegrationFunction handlerNativeResourceFunctionForIntegration(const QByteArray &resource) const;
    NativeResourceForContextFunction handlerNativeResourceFunctionForContext(const QByteArray &resource) const;
    NativeResourceForScreenFunction handlerNativeResourceFunctionForScreen(const QByteArray &resource) const;
    NativeResourceForWindowFunction handlerNativeResourceFunctionForWindow(const QByteArray &resource) const;
    NativeResourceForBackingStoreFunction handlerNativeResourceFunctionForBackingStore(const QByteArray &resource) const;
    QFunctionPointer handlerPlatformFunction(const QByteArray &function) const;
    void *handlerNativeResourceForIntegration(const QByteArray &resource) const;
    void *handlerNativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) const;
    void *handlerNativeResourceForScreen(const QByteArray &resource, QScreen *screen) const;
    void *handlerNativeResourceForWindow(const QByteArray &resource, QWindow *window) const;
    void *handlerNativeResourceForBackingStore(const QByteArray &resource, QBackingStore *backingStore) const;
};

QT_END_NAMESPACE

#endif // QXCBNATIVEINTERFACE_H
