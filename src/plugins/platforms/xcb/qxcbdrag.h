/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include <qlist.h>
#include <qplatformdrag_qpa.h>
#include <qnamespace.h>
#include <xcb/xcb.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsharedpointer.h>

QT_BEGIN_NAMESPACE

class QMouseEvent;
class QWindow;
class QXcbConnection;
class QXcbWindow;
class QDropData;

class QXcbDrag : public QPlatformDrag
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

    void handleEnter(QWindow *window, const xcb_client_message_event_t *event);
    void handlePosition(QWindow *w, const xcb_client_message_event_t *event, bool passive);
    void handleStatus(QWindow *w, const xcb_client_message_event_t *event, bool passive);
    void handleLeave(QWindow *w, const xcb_client_message_event_t *event, bool /*passive*/);
    void handleDrop(QWindow *, const xcb_client_message_event_t *event, bool passive);

    bool dndEnable(QXcbWindow *win, bool on);

    QXcbConnection *connection() const { return m_connection; }

private:
    friend class QDropData;

    void handle_xdnd_position(QWindow *w, const xcb_client_message_event_t *event, bool passive);
    void handle_xdnd_status(QWindow *, const xcb_client_message_event_t *event, bool);

    Qt::DropAction toDropAction(xcb_atom_t atom) const;
    xcb_atom_t toXdndAction(Qt::DropAction a) const;

    QWeakPointer<QWindow> currentWindow;
    QPoint currentPosition;

    QXcbConnection *m_connection;
    QDropData *dropData;

    QWindow *desktop_proxy;

    xcb_atom_t xdnd_dragsource;

    // the types in this drop. 100 is no good, but at least it's big.
    enum { xdnd_max_type = 100 };
    QList<xcb_atom_t> xdnd_types;

    xcb_timestamp_t target_time;
    Qt::DropAction last_target_accepted_action;

    // rectangle in which the answer will be the same
    QRect source_sameanswer;
    bool waiting_for_status;

    // window to send events to (always valid if current_target)
    xcb_window_t current_proxy_target;
};

QT_END_NAMESPACE

#endif
