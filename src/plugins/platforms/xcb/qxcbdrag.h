/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QXCBDRAG_H
#define QXCBDRAG_H

#include <qplatformdrag_qpa.h>
#include <qxcbobject.h>
#include <xcb/xcb.h>
#include <qlist.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsharedpointer.h>
#include <qvector.h>

QT_BEGIN_NAMESPACE

class QMouseEvent;
class QWindow;
class QXcbConnection;
class QXcbWindow;
class QDropData;
class QXcbScreen;
class QDrag;

class QXcbDrag : public QObject, public QXcbObject, public QPlatformDrag
{
public:
    QXcbDrag(QXcbConnection *c);
    ~QXcbDrag();

    virtual QMimeData *platformDropData();

//    virtual Qt::DropAction drag(QDrag *);

    virtual void startDrag();
    virtual void cancel();
    virtual void move(const QMouseEvent *me);
    virtual void drop(const QMouseEvent *me);
    void endDrag();

    void handleEnter(QWindow *window, const xcb_client_message_event_t *event);
    void handlePosition(QWindow *w, const xcb_client_message_event_t *event, bool passive);
    void handleLeave(QWindow *w, const xcb_client_message_event_t *event, bool /*passive*/);
    void handleDrop(QWindow *, const xcb_client_message_event_t *event, bool passive);

    void handleStatus(const xcb_client_message_event_t *event, bool passive);
    void handleSelectionRequest(const xcb_selection_request_event_t *event);
    void handleFinished(const xcb_client_message_event_t *event, bool passive);

    bool dndEnable(QXcbWindow *win, bool on);

protected:
    void timerEvent(QTimerEvent* e);

private:
    friend class QDropData;

    void init();

    void handle_xdnd_position(QWindow *w, const xcb_client_message_event_t *event, bool passive);
    void handle_xdnd_status(const xcb_client_message_event_t *event, bool);
    void send_leave();

    Qt::DropAction toDropAction(xcb_atom_t atom) const;
    xcb_atom_t toXdndAction(Qt::DropAction a) const;

    QWeakPointer<QWindow> currentWindow;
    QPoint currentPosition;

    QDropData *dropData;

    QWindow *desktop_proxy;

    xcb_atom_t xdnd_dragsource;

    // the types in this drop. 100 is no good, but at least it's big.
    enum { xdnd_max_type = 100 };
    QList<xcb_atom_t> xdnd_types;

    xcb_timestamp_t target_time;
    xcb_timestamp_t source_time;
    Qt::DropAction last_target_accepted_action;

    // rectangle in which the answer will be the same
    QRect source_sameanswer;
    bool waiting_for_status;

    // top-level window we sent position to last.
    xcb_window_t current_target;
    // window to send events to (always valid if current_target)
    xcb_window_t current_proxy_target;

    QXcbScreen *current_screen;

    int heartbeat;
    bool xdnd_dragging;

    QVector<xcb_atom_t> drag_types;

    struct Transaction
    {
        xcb_timestamp_t timestamp;
        xcb_window_t target;
        xcb_window_t proxy_target;
        QWindow *targetWindow;
//        QWidget *embedding_widget;
        QDrag *object;
    };
    QList<Transaction> transactions;

    int transaction_expiry_timer;
    void restartDropExpiryTimer();
    int findTransactionByWindow(xcb_window_t window);
    int findTransactionByTime(xcb_timestamp_t timestamp);
    xcb_window_t findRealWindow(const QPoint & pos, xcb_window_t w, int md);
};

QT_END_NAMESPACE

#endif
