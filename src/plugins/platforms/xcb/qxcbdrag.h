/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QXCBDRAG_H
#define QXCBDRAG_H

#include <qpa/qplatformdrag.h>
#include <private/qsimpledrag_p.h>
#include <qxcbobject.h>
#include <xcb/xcb.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsharedpointer.h>
#include <qpointer.h>
#include <qvector.h>
#include <qdatetime.h>
#include <qpixmap.h>
#include <qbackingstore.h>

#include <QtCore/QDebug>

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
    Qt::DropActions toDropActions(const QVector<xcb_atom_t> &atoms) const;
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
    QVector<xcb_atom_t> xdnd_types;

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
    enum { XdndDropTransactionTimeout = 600000 };
    int cleanup_timer;

    QVector<xcb_atom_t> drag_types;

    QVector<xcb_atom_t> current_actions;
    QVector<xcb_atom_t> drop_actions;

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
    QVector<Transaction> transactions;

    int transaction_expiry_timer;
    void restartDropExpiryTimer();
    int findTransactionByWindow(xcb_window_t window);
    int findTransactionByTime(xcb_timestamp_t timestamp);
    xcb_window_t findRealWindow(const QPoint & pos, xcb_window_t w, int md, bool ignoreNonXdndAwareWindows);
};
Q_DECLARE_TYPEINFO(QXcbDrag::Transaction, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif
