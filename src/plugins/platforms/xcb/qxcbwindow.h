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

#ifndef QXCBWINDOW_H
#define QXCBWINDOW_H

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QImage>

#include <xcb/xcb.h>
#include <xcb/sync.h>

#include "qxcbobject.h"

#include <QtPlatformHeaders/qxcbwindowfunctions.h>

QT_BEGIN_NAMESPACE

class QXcbScreen;
class QXcbSyncWindowRequest;
class QIcon;

class Q_XCB_EXPORT QXcbWindow : public QXcbObject, public QXcbWindowEventListener, public QPlatformWindow
{
public:
    enum NetWmState {
        NetWmStateAbove = 0x1,
        NetWmStateBelow = 0x2,
        NetWmStateFullScreen = 0x4,
        NetWmStateMaximizedHorz = 0x8,
        NetWmStateMaximizedVert = 0x10,
        NetWmStateModal = 0x20,
        NetWmStateStaysOnTop = 0x40,
        NetWmStateDemandsAttention = 0x80
    };

    Q_DECLARE_FLAGS(NetWmStates, NetWmState)

    QXcbWindow(QWindow *window);
    ~QXcbWindow();

    void setGeometry(const QRect &rect) Q_DECL_OVERRIDE;

    QMargins frameMargins() const Q_DECL_OVERRIDE;

    void setVisible(bool visible) Q_DECL_OVERRIDE;
    void setWindowFlags(Qt::WindowFlags flags) Q_DECL_OVERRIDE;
    void setWindowState(Qt::WindowState state) Q_DECL_OVERRIDE;
    WId winId() const Q_DECL_OVERRIDE;
    void setParent(const QPlatformWindow *window) Q_DECL_OVERRIDE;

    bool isExposed() const Q_DECL_OVERRIDE;
    bool isEmbedded(const QPlatformWindow *parentWindow = 0) const Q_DECL_OVERRIDE;
    QPoint mapToGlobal(const QPoint &pos) const Q_DECL_OVERRIDE;
    QPoint mapFromGlobal(const QPoint &pos) const Q_DECL_OVERRIDE;

    void setWindowTitle(const QString &title) Q_DECL_OVERRIDE;
    void setWindowIconText(const QString &title);
    void setWindowIcon(const QIcon &icon) Q_DECL_OVERRIDE;
    void raise() Q_DECL_OVERRIDE;
    void lower() Q_DECL_OVERRIDE;
    void propagateSizeHints() Q_DECL_OVERRIDE;

    void requestActivateWindow() Q_DECL_OVERRIDE;

    bool setKeyboardGrabEnabled(bool grab) Q_DECL_OVERRIDE;
    bool setMouseGrabEnabled(bool grab) Q_DECL_OVERRIDE;

    void setCursor(xcb_cursor_t cursor);

    QSurfaceFormat format() const Q_DECL_OVERRIDE;

    void windowEvent(QEvent *event) Q_DECL_OVERRIDE;

    bool startSystemResize(const QPoint &pos, Qt::Corner corner) Q_DECL_OVERRIDE;

    void setOpacity(qreal level) Q_DECL_OVERRIDE;
    void setMask(const QRegion &region) Q_DECL_OVERRIDE;

    void setAlertState(bool enabled) Q_DECL_OVERRIDE;
    bool isAlertState() const Q_DECL_OVERRIDE { return m_alertState; }

    xcb_window_t xcb_window() const { return m_window; }
    uint depth() const { return m_depth; }
    QImage::Format imageFormat() const { return m_imageFormat; }
    bool imageNeedsRgbSwap() const { return m_imageRgbSwap; }

    bool handleGenericEvent(xcb_generic_event_t *event, long *result)  Q_DECL_OVERRIDE;

    void handleExposeEvent(const xcb_expose_event_t *event) Q_DECL_OVERRIDE;
    void handleClientMessageEvent(const xcb_client_message_event_t *event) Q_DECL_OVERRIDE;
    void handleConfigureNotifyEvent(const xcb_configure_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleMapNotifyEvent(const xcb_map_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleUnmapNotifyEvent(const xcb_unmap_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleButtonPressEvent(const xcb_button_press_event_t *event) Q_DECL_OVERRIDE;
    void handleButtonReleaseEvent(const xcb_button_release_event_t *event) Q_DECL_OVERRIDE;
    void handleMotionNotifyEvent(const xcb_motion_notify_event_t *event) Q_DECL_OVERRIDE;

    void handleEnterNotifyEvent(const xcb_enter_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleLeaveNotifyEvent(const xcb_leave_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleFocusInEvent(const xcb_focus_in_event_t *event) Q_DECL_OVERRIDE;
    void handleFocusOutEvent(const xcb_focus_out_event_t *event) Q_DECL_OVERRIDE;
    void handlePropertyNotifyEvent(const xcb_property_notify_event_t *event) Q_DECL_OVERRIDE;
#ifdef XCB_USE_XINPUT22
    void handleXIMouseEvent(xcb_ge_event_t *, Qt::MouseEventSource source = Qt::MouseEventNotSynthesized) Q_DECL_OVERRIDE;
    void handleXIEnterLeave(xcb_ge_event_t *) Q_DECL_OVERRIDE;
#endif

    QXcbWindow *toWindow() Q_DECL_OVERRIDE;

    void handleMouseEvent(xcb_timestamp_t time, const QPoint &local, const QPoint &global,
                          Qt::KeyboardModifiers modifiers, Qt::MouseEventSource source);

    void updateNetWmUserTime(xcb_timestamp_t timestamp);

    static void setWmWindowTypeStatic(QWindow *window, QXcbWindowFunctions::WmWindowTypes windowTypes);
    static uint visualIdStatic(QWindow *window);

    QXcbWindowFunctions::WmWindowTypes wmWindowTypes() const;
    void setWmWindowType(QXcbWindowFunctions::WmWindowTypes types, Qt::WindowFlags flags);

    static void setWindowIconTextStatic(QWindow *window, const QString &text);

    static void setParentRelativeBackPixmapStatic(QWindow *window);
    void setParentRelativeBackPixmap();

    static bool requestSystemTrayWindowDockStatic(const QWindow *window);
    bool requestSystemTrayWindowDock() const;

    static QRect systemTrayWindowGlobalGeometryStatic(const QWindow *window);
    QRect systemTrayWindowGlobalGeometry() const;
    uint visualId() const;

    bool needsSync() const;

    void postSyncWindowRequest();
    void clearSyncWindowRequest() { m_pendingSyncRequest = 0; }

    QXcbScreen *xcbScreen() const;

    virtual void create();
    virtual void destroy();

public Q_SLOTS:
    void updateSyncRequestCounter();

protected:
    virtual void resolveFormat(const QSurfaceFormat &format) { m_format = format; }
    virtual const xcb_visualtype_t *createVisual();

    QXcbScreen *parentScreen();

    void changeNetWmState(bool set, xcb_atom_t one, xcb_atom_t two = 0);
    NetWmStates netWmStates();
    void setNetWmStates(NetWmStates);

    void setMotifWindowFlags(Qt::WindowFlags flags);
    void setNetWmStateWindowFlags(Qt::WindowFlags flags);

    void updateMotifWmHintsBeforeMap();
    void updateNetWmStateBeforeMap();

    void setTransparentForMouseEvents(bool transparent);
    void updateDoesNotAcceptFocus(bool doesNotAcceptFocus);

    QRect windowToWmGeometry(QRect r) const;
    void sendXEmbedMessage(xcb_window_t window, quint32 message,
                           quint32 detail = 0, quint32 data1 = 0, quint32 data2 = 0);
    void handleXEmbedMessage(const xcb_client_message_event_t *event);

    void show();
    void hide();

    bool relayFocusToModalWindow() const;
    void doFocusIn();
    void doFocusOut();

    bool compressExposeEvent(QRegion &exposeRegion);

    void handleButtonPressEvent(int event_x, int event_y, int root_x, int root_y,
                                int detail, Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp, Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);

    void handleButtonReleaseEvent(int event_x, int event_y, int root_x, int root_y,
                                  int detail, Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp, Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);

    void handleMotionNotifyEvent(int event_x, int event_y, int root_x, int root_y,
                                 Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp, Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);

    void handleEnterNotifyEvent(int event_x, int event_y, int root_x, int root_y,
                                quint8 mode, quint8 detail, xcb_timestamp_t timestamp);

    void handleLeaveNotifyEvent(int root_x, int root_y,
                                quint8 mode, quint8 detail, xcb_timestamp_t timestamp);

    xcb_window_t m_window;

    uint m_depth;
    QImage::Format m_imageFormat;
    bool m_imageRgbSwap;

    xcb_sync_int64_t m_syncValue;
    xcb_sync_counter_t m_syncCounter;

    Qt::WindowState m_windowState;

    xcb_gravity_t m_gravity;

    bool m_mapped;
    bool m_transparent;
    bool m_usingSyncProtocol;
    bool m_deferredActivation;
    bool m_embedded;
    bool m_alertState;
    xcb_window_t m_netWmUserTimeWindow;

    QSurfaceFormat m_format;

    mutable bool m_dirtyFrameMargins;
    mutable QMargins m_frameMargins;

    QRegion m_exposeRegion;

    xcb_visualid_t m_visualId;
    int m_lastWindowStateEvent;

    enum SyncState {
        NoSyncNeeded,
        SyncReceived,
        SyncAndConfigureReceived
    };
    SyncState m_syncState;

    QXcbSyncWindowRequest *m_pendingSyncRequest;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QXcbWindow*)

#endif
