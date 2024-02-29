// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <qpa/qplatformdrag.h>
#include <private/qsimpledrag_p.h>
#include <xcb/xcb.h>
#include <qbackingstore.h>
#include <qdatetime.h>
#include <qlist.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qpointer.h>
#include <qrect.h>
#include <qxcbobject.h>

QT_REQUIRE_CONFIG(draganddrop);

QT_BEGIN_NAMESPACE

class QWindow;
class QPlatformWindow;
class QXcbConnection;
class QXcbWindow;
class QXcbDropData;
class QXcbScreen;
class QDrag;
class QShapedPixmapWindow;

class QXcbDrag : public QXcbObject, public QBasicDrag, public QXcbWindowEventListener
{
public:
    QXcbDrag(QXcbConnection *c);
    ~QXcbDrag();

    bool eventFilter(QObject *o, QEvent *e) override;

    void startDrag() override;
    void cancel() override;
    void move(const QPoint &globalPos, Qt::MouseButtons b, Qt::KeyboardModifiers mods) override;
    void drop(const QPoint &globalPos, Qt::MouseButtons b, Qt::KeyboardModifiers mods) override;
    void endDrag() override;

    Qt::DropAction defaultAction(Qt::DropActions possibleActions, Qt::KeyboardModifiers modifiers) const override;

    void handlePropertyNotifyEvent(const xcb_property_notify_event_t *event) override;

    void handleEnter(QPlatformWindow *window, const xcb_client_message_event_t *event, xcb_window_t proxy = 0);
    void handlePosition(QPlatformWindow *w, const xcb_client_message_event_t *event);
    void handleLeave(QPlatformWindow *w, const xcb_client_message_event_t *event);
    void handleDrop(QPlatformWindow *, const xcb_client_message_event_t *event,
                    Qt::MouseButtons b = { }, Qt::KeyboardModifiers mods = { });

    void handleStatus(const xcb_client_message_event_t *event);
    void handleSelectionRequest(const xcb_selection_request_event_t *event);
    void handleFinished(const xcb_client_message_event_t *event);

    bool dndEnable(QXcbWindow *win, bool on);
    bool ownsDragObject() const override;

    void updatePixmap();
    xcb_timestamp_t targetTime() { return target_time; }

protected:
    void timerEvent(QTimerEvent* e) override;

    bool findXdndAwareTarget(const QPoint &globalPos, xcb_window_t *target_out);

private:
    friend class QXcbDropData;

    void init();

    void handle_xdnd_position(QPlatformWindow *w, const xcb_client_message_event_t *event,
                              Qt::MouseButtons b = { }, Qt::KeyboardModifiers mods = { });
    void handle_xdnd_status(const xcb_client_message_event_t *event);
    void send_leave();

    Qt::DropAction toDropAction(xcb_atom_t atom) const;
    Qt::DropActions toDropActions(const QList<xcb_atom_t> &atoms) const;
    xcb_atom_t toXdndAction(Qt::DropAction a) const;

    void readActionList();
    void setActionList(Qt::DropAction requestedAction, Qt::DropActions supportedActions);
    void startListeningForActionListChanges();
    void stopListeningForActionListChanges();

    QPointer<QWindow> initiatorWindow;
    QPointer<QWindow> currentWindow;
    QPoint currentPosition;

    QXcbDropData *m_dropData;
    Qt::DropAction accepted_drop_action;

    QWindow *desktop_proxy;

    xcb_atom_t xdnd_dragsource;

    // the types in this drop. 100 is no good, but at least it's big.
    enum { xdnd_max_type = 100 };
    QList<xcb_atom_t> xdnd_types;

    // timestamp from XdndPosition and XdndDroptime for retrieving the data
    xcb_timestamp_t target_time;
    xcb_timestamp_t source_time;

    // rectangle in which the answer will be the same
    QRect source_sameanswer;
    bool waiting_for_status;

    // helpers for setting executed drop action outside application
    bool dropped;
    bool canceled;

    // A window from Unity DnD Manager, which does not respect the XDnD spec
    xcb_window_t xdndCollectionWindow = XCB_NONE;

    // top-level window we sent position to last.
    xcb_window_t current_target;
    // window to send events to (always valid if current_target)
    xcb_window_t current_proxy_target;

    QXcbVirtualDesktop *current_virtual_desktop;

    // 10 minute timer used to discard old XdndDrop transactions
    static constexpr std::chrono::minutes XdndDropTransactionTimeout{10};
    QBasicTimer cleanup_timer;

    QList<xcb_atom_t> drag_types;

    QList<xcb_atom_t> current_actions;
    QList<xcb_atom_t> drop_actions;

    struct Transaction
    {
        xcb_timestamp_t timestamp;
        xcb_window_t target;
        xcb_window_t proxy_target;
        QPlatformWindow *targetWindow;
//        QWidget *embedding_widget;
        QPointer<QDrag> drag;
        QTime time;
    };
    friend class QTypeInfo<Transaction>;
    QList<Transaction> transactions;

    int transaction_expiry_timer;
    void restartDropExpiryTimer();
    int findTransactionByWindow(xcb_window_t window);
    int findTransactionByTime(xcb_timestamp_t timestamp);
    xcb_window_t findRealWindow(const QPoint & pos, xcb_window_t w, int md, bool ignoreNonXdndAwareWindows);
};
Q_DECLARE_TYPEINFO(QXcbDrag::Transaction, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE
