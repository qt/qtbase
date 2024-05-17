// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBWINDOW_H
#define QXCBWINDOW_H

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QImage>

#include <xcb/xcb.h>
#include <xcb/sync.h>

#include "qxcbobject.h"

QT_BEGIN_NAMESPACE

class QXcbScreen;
class QXcbSyncWindowRequest;
class QIcon;

class Q_XCB_EXPORT QXcbWindow : public QObject, public QXcbObject, public QXcbWindowEventListener, public QPlatformWindow
                              , public QNativeInterface::Private::QXcbWindow
{
    Q_OBJECT
public:
    enum NetWmState {
        NetWmStateAbove = 0x1,
        NetWmStateBelow = 0x2,
        NetWmStateFullScreen = 0x4,
        NetWmStateMaximizedHorz = 0x8,
        NetWmStateMaximizedVert = 0x10,
        NetWmStateModal = 0x20,
        NetWmStateStaysOnTop = 0x40,
        NetWmStateDemandsAttention = 0x80,
        NetWmStateHidden = 0x100
    };

    Q_DECLARE_FLAGS(NetWmStates, NetWmState)

    enum RecreationReason {
        RecreationNotNeeded = 0,
        WindowStaysOnTopHintChanged = 0x1,
        WindowStaysOnBottomHintChanged = 0x2
    };
    Q_DECLARE_FLAGS(RecreationReasons, RecreationReason)

    QXcbWindow(QWindow *window);
    ~QXcbWindow();

    void setGeometry(const QRect &rect) override;

    QMargins frameMargins() const override;

    void setVisible(bool visible) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    void setWindowState(Qt::WindowStates state) override;
    WId winId() const override;
    void setParent(const QPlatformWindow *window) override;

    bool isExposed() const override;
    bool isEmbedded() const override;
    QPoint mapToGlobal(const QPoint &pos) const override;
    QPoint mapFromGlobal(const QPoint &pos) const override;

    void setWindowTitle(const QString &title) override;
    void setWindowIconText(const QString &title) override;
    void setWindowIcon(const QIcon &icon) override;
    void raise() override;
    void lower() override;
    void propagateSizeHints() override;

    void requestActivateWindow() override;

    bool setKeyboardGrabEnabled(bool grab) override;
    bool setMouseGrabEnabled(bool grab) override;

    QSurfaceFormat format() const override;

    bool windowEvent(QEvent *event) override;

    bool startSystemResize(Qt::Edges edges) override;
    bool startSystemMove() override;

    void setOpacity(qreal level) override;
    void setMask(const QRegion &region) override;

    void setAlertState(bool enabled) override;
    bool isAlertState() const override { return m_alertState; }

    xcb_window_t xcb_window() const { return m_window; }
    uint depth() const { return m_depth; }
    QImage::Format imageFormat() const { return m_imageFormat; }
    bool imageNeedsRgbSwap() const { return m_imageRgbSwap; }

    bool handleNativeEvent(xcb_generic_event_t *event)  override;

    void handleExposeEvent(const xcb_expose_event_t *event) override;
    void handleClientMessageEvent(const xcb_client_message_event_t *event) override;
    void handleConfigureNotifyEvent(const xcb_configure_notify_event_t *event) override;
    void handleMapNotifyEvent(const xcb_map_notify_event_t *event) override;
    void handleUnmapNotifyEvent(const xcb_unmap_notify_event_t *event) override;
    void handleButtonPressEvent(const xcb_button_press_event_t *event) override;
    void handleButtonReleaseEvent(const xcb_button_release_event_t *event) override;
    void handleMotionNotifyEvent(const xcb_motion_notify_event_t *event) override;

    void handleEnterNotifyEvent(const xcb_enter_notify_event_t *event) override;
    void handleLeaveNotifyEvent(const xcb_leave_notify_event_t *event) override;
    void handleFocusInEvent(const xcb_focus_in_event_t *event) override;
    void handleFocusOutEvent(const xcb_focus_out_event_t *event) override;
    void handlePropertyNotifyEvent(const xcb_property_notify_event_t *event) override;
    void handleXIMouseEvent(xcb_ge_event_t *, Qt::MouseEventSource source = Qt::MouseEventNotSynthesized) override;
    void handleXIEnterLeave(xcb_ge_event_t *) override;

    QXcbWindow *toWindow() override;

    void handleMouseEvent(xcb_timestamp_t time, const QPoint &local, const QPoint &global,
                          Qt::KeyboardModifiers modifiers, QEvent::Type type, Qt::MouseEventSource source);

    void updateNetWmUserTime(xcb_timestamp_t timestamp);
    void updateWmTransientFor();
    void registerWmTransientForChild(QXcbWindow *);

    WindowTypes wmWindowTypes() const;
    void setWmWindowType(WindowTypes types, Qt::WindowFlags flags);
    void setWindowType(WindowTypes windowTypes) override { setWmWindowType(windowTypes, window()->flags()); }
    void setWindowRole(const QString &role) override;

    void setParentRelativeBackPixmap();
    bool requestSystemTrayWindowDock();
    uint visualId() const override;

    bool needsSync() const;

    void postSyncWindowRequest();
    void clearSyncWindowRequest() { m_pendingSyncRequest = nullptr; }

    QXcbScreen *xcbScreen() const;

    QPoint lastPointerPosition() const { return m_lastPointerPosition; }
    QPoint lastPointerGlobalPosition() const { return m_lastPointerGlobalPosition; }

    bool startSystemMoveResize(const QPoint &pos, int edges);
    void doStartSystemMoveResize(const QPoint &globalPos, int edges);

    static bool isTrayIconWindow(QWindow *window)
    {
        return window->objectName() == QLatin1StringView("QSystemTrayIconSysWindow");
    }

    virtual void create();
    virtual void destroy();

    static void setWindowTitle(const QXcbConnection *conn, xcb_window_t window, const QString &title);
    static QString windowTitle(const QXcbConnection *conn, xcb_window_t window);

    int swapInterval() const { return m_swapInterval; }
    void setSwapInterval(int swapInterval) { m_swapInterval = swapInterval; }

public Q_SLOTS:
    void updateSyncRequestCounter();

protected:
    virtual void resolveFormat(const QSurfaceFormat &format) { m_format = format; }
    virtual const xcb_visualtype_t *createVisual();
    void setImageFormatForVisual(const xcb_visualtype_t *visual);

    QXcbScreen *parentScreen();
    QXcbScreen *initialScreen() const;

    void setNetWmState(bool set, xcb_atom_t one, xcb_atom_t two = 0);
    void setNetWmState(Qt::WindowFlags flags);
    void setNetWmState(Qt::WindowStates state);
    void setNetWmStateOnUnmappedWindow();
    NetWmStates netWmStates();

    void setMotifWmHints(Qt::WindowFlags flags);

    void setTransparentForMouseEvents(bool transparent);
    void updateDoesNotAcceptFocus(bool doesNotAcceptFocus);

    void sendXEmbedMessage(xcb_window_t window, quint32 message,
                           quint32 detail = 0, quint32 data1 = 0, quint32 data2 = 0);
    void handleXEmbedMessage(const xcb_client_message_event_t *event);

    void show();
    void hide();

    bool relayFocusToModalWindow() const;
    void doFocusIn();
    void doFocusOut();

    void handleButtonPressEvent(int event_x, int event_y, int root_x, int root_y,
                                int detail, Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp,
                                QEvent::Type type, Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);

    void handleButtonReleaseEvent(int event_x, int event_y, int root_x, int root_y,
                                  int detail, Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp,
                                  QEvent::Type type, Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);

    void handleMotionNotifyEvent(int event_x, int event_y, int root_x, int root_y,
                                 Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp,
                                 QEvent::Type type, Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);

    void handleEnterNotifyEvent(int event_x, int event_y, int root_x, int root_y,
                                quint8 mode, quint8 detail, xcb_timestamp_t timestamp);

    void handleLeaveNotifyEvent(int root_x, int root_y,
                                quint8 mode, quint8 detail, xcb_timestamp_t timestamp);

    xcb_window_t m_window = 0;

    uint m_depth = 0;
    QImage::Format m_imageFormat = QImage::Format_ARGB32_Premultiplied;
    bool m_imageRgbSwap = false;

    xcb_sync_int64_t m_syncValue;
    xcb_sync_counter_t m_syncCounter = 0;

    Qt::WindowStates m_windowState = Qt::WindowNoState;

    QMutex m_mappedMutex;
    bool m_mapped = false;
    bool m_transparent = false;
    bool m_deferredActivation = false;
    bool m_embedded = false;
    bool m_alertState = false;
    bool m_minimized = false;
    bool m_trayIconWindow = false;
    bool m_ignorePressedWindowOnMouseLeave = false;
    xcb_window_t m_netWmUserTimeWindow = XCB_NONE;

    QSurfaceFormat m_format;

    mutable bool m_dirtyFrameMargins = false;
    mutable QMargins m_frameMargins;

    QRegion m_exposeRegion;
    QSize m_oldWindowSize;
    QPoint m_lastPointerPosition;
    QPoint m_lastPointerGlobalPosition;

    xcb_visualid_t m_visualId = 0;
    // Last sent state. Initialized to an invalid state, on purpose.
    Qt::WindowStates m_lastWindowStateEvent = Qt::WindowActive;

    enum SyncState {
        NoSyncNeeded,
        SyncReceived,
        SyncAndConfigureReceived
    };
    SyncState m_syncState = NoSyncNeeded;

    QXcbSyncWindowRequest *m_pendingSyncRequest = nullptr;
    int m_swapInterval = -1;

    qreal m_sizeHintsScaleFactor = 1.0;

    RecreationReasons m_recreationReasons = RecreationNotNeeded;

    QList<QPointer<QXcbWindow>> m_wmTransientForChildren;
};

class QXcbForeignWindow : public QXcbWindow
{
public:
    QXcbForeignWindow(QWindow *window, WId nativeHandle);
    ~QXcbForeignWindow();
    bool isForeignWindow() const override { return true; }

protected:
    void create() override {} // No-op
};

QList<xcb_rectangle_t> qRegionToXcbRectangleList(const QRegion &region);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QXcbWindow*)

#endif
