/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DRAGANDDROP

class QWindow;
class QPlatformWindow;
class QXcbConnection;
class QXcbWindow;
class QXcbDropData;
class QXcbScreen;
class QDrag;
class QShapedPixmapWindow;

class QXcbDrag : public QXcbObject, public QBasicDrag
{
public:
    QXcbDrag(QXcbConnection *c);
    ~QXcbDrag();

    virtual QMimeData *platformDropData() Q_DECL_OVERRIDE;

    void startDrag() Q_DECL_OVERRIDE;
    void cancel() Q_DECL_OVERRIDE;
    void move(const QPoint &globalPos) Q_DECL_OVERRIDE;
    void drop(const QPoint &globalPos) Q_DECL_OVERRIDE;
    void endDrag() Q_DECL_OVERRIDE;

    void handleEnter(QPlatformWindow *window, const xcb_client_message_event_t *event, xcb_window_t proxy = 0);
    void handlePosition(QPlatformWindow *w, const xcb_client_message_event_t *event);
    void handleLeave(QPlatformWindow *w, const xcb_client_message_event_t *event);
    void handleDrop(QPlatformWindow *, const xcb_client_message_event_t *event);

    void handleStatus(const xcb_client_message_event_t *event);
    void handleSelectionRequest(const xcb_selection_request_event_t *event);
    void handleFinished(const xcb_client_message_event_t *event);

    bool dndEnable(QXcbWindow *win, bool on);
    bool ownsDragObject() const Q_DECL_OVERRIDE;

    void updatePixmap();
    xcb_timestamp_t targetTime() { return target_time; }

protected:
    void timerEvent(QTimerEvent* e) Q_DECL_OVERRIDE;

private:
    friend class QXcbDropData;

    void init();

    void handle_xdnd_position(QPlatformWindow *w, const xcb_client_message_event_t *event);
    void handle_xdnd_status(const xcb_client_message_event_t *event);
    void send_leave();

    Qt::DropAction toDropAction(xcb_atom_t atom) const;
    xcb_atom_t toXdndAction(Qt::DropAction a) const;

    QPointer<QWindow> currentWindow;
    QPoint currentPosition;

    QXcbDropData *dropData;
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

    // top-level window we sent position to last.
    xcb_window_t current_target;
    // window to send events to (always valid if current_target)
    xcb_window_t current_proxy_target;

    QXcbVirtualDesktop *current_virtual_desktop;

    // 10 minute timer used to discard old XdndDrop transactions
    enum { XdndDropTransactionTimeout = 600000 };
    int cleanup_timer;

    QVector<xcb_atom_t> drag_types;

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
    QVector<Transaction> transactions;

    int transaction_expiry_timer;
    void restartDropExpiryTimer();
    int findTransactionByWindow(xcb_window_t window);
    int findTransactionByTime(xcb_timestamp_t timestamp);
    xcb_window_t findRealWindow(const QPoint & pos, xcb_window_t w, int md, bool ignoreNonXdndAwareWindows);
};

#endif // QT_NO_DRAGANDDROP

QT_END_NAMESPACE

#endif
