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

#include "qxcbdrag.h"
#include <xcb/xcb.h>
#include "qxcbconnection.h"
#include "qxcbclipboard.h"
#include "qxcbmime.h"
#include "qxcbwindow.h"
#include "qwindow.h"
#include <private/qdnd_p.h>
#include <qdebug.h>
#include <qevent.h>
#include <qguiapplication.h>
#include <qrect.h>

QT_BEGIN_NAMESPACE

#define DND_DEBUG
#ifdef DND_DEBUG
#define DEBUG qDebug
#else
#define DEBUG if(0) qDebug
#endif

#ifdef DND_DEBUG
#define DNDDEBUG qDebug()
#else
#define DNDDEBUG if(0) qDebug()
#endif

const int xdnd_version = 5;

static inline xcb_window_t xcb_window(QWindow *w)
{
    return static_cast<QXcbWindow *>(w->handle())->xcb_window();
}

class QDropData : public QXcbMime
{
public:
    QDropData(QXcbDrag *d);
    ~QDropData();

protected:
    bool hasFormat_sys(const QString &mimeType) const;
    QStringList formats_sys() const;
    QVariant retrieveData_sys(const QString &mimeType, QVariant::Type type) const;

    QVariant xdndObtainData(const QByteArray &format, QVariant::Type requestedType) const;

    QXcbDrag *drag;
};


QXcbDrag::QXcbDrag(QXcbConnection *c)
{
    m_connection = c;
    dropData = new QDropData(this);

    startDrag(); // init variables
}

QXcbDrag::~QXcbDrag()
{
    delete dropData;
}


QMimeData *QXcbDrag::platformDropData()
{
    return dropData;
}

void QXcbDrag::startDrag()
{
    currentWindow.clear();

    xdnd_dragsource = XCB_NONE;
    last_target_accepted_action = Qt::IgnoreAction;

    waiting_for_status = false;
    current_proxy_target = XCB_NONE;
}

void QXcbDrag::cancel()
{
    //###
}

void QXcbDrag::move(const QMouseEvent *me)
{
    // ###
}

void QXcbDrag::drop(const QMouseEvent *me)
{
    // ###
}

#define ATOM(x) connection()->atom(QXcbAtom::x)

Qt::DropAction QXcbDrag::toDropAction(xcb_atom_t atom) const
{
    if (atom == ATOM(XdndActionCopy) || atom == 0)
        return Qt::CopyAction;
    if (atom == ATOM(XdndActionLink))
        return Qt::LinkAction;
    if (atom == ATOM(XdndActionMove))
        return Qt::MoveAction;
    return Qt::CopyAction;
}

xcb_atom_t QXcbDrag::toXdndAction(Qt::DropAction a) const
{
    switch (a) {
    case Qt::CopyAction:
        return ATOM(XdndActionCopy);
    case Qt::LinkAction:
        return ATOM(XdndActionLink);
    case Qt::MoveAction:
    case Qt::TargetMoveAction:
        return ATOM(XdndActionMove);
    case Qt::IgnoreAction:
        return XCB_NONE;
    default:
        return ATOM(XdndActionCopy);
    }
}

#undef ATOM

#if 0
static int findXdndDropTransactionByWindow(Window window)
{
    int at = -1;
    for (int i = 0; i < X11->dndDropTransactions.count(); ++i) {
        const QXdndDropTransaction &t = X11->dndDropTransactions.at(i);
        if (t.target == window || t.proxy_target == window) {
            at = i;
            break;
        }
    }
    return at;
}

static int findXdndDropTransactionByTime(Time timestamp)
{
    int at = -1;
    for (int i = 0; i < X11->dndDropTransactions.count(); ++i) {
        const QXdndDropTransaction &t = X11->dndDropTransactions.at(i);
        if (t.timestamp == timestamp) {
            at = i;
            break;
        }
    }
    return at;
}

// timer used to discard old XdndDrop transactions
static int transaction_expiry_timer = -1;
enum { XdndDropTransactionTimeout = 5000 }; // 5 seconds

static void restartXdndDropExpiryTimer()
{
    if (transaction_expiry_timer != -1)
        QDragManager::self()->killTimer(transaction_expiry_timer);
    transaction_expiry_timer = QDragManager::self()->startTimer(XdndDropTransactionTimeout);
}


// find an ancestor with XdndAware on it
static Window findXdndAwareParent(Window window)
{
    Window target = 0;
    forever {
        // check if window has XdndAware
        Atom type = 0;
        int f;
        unsigned long n, a;
        unsigned char *data = 0;
        if (XGetWindowProperty(X11->display, window, ATOM(XdndAware), 0, 0, False,
                               AnyPropertyType, &type, &f,&n,&a,&data) == Success) {
	    if (data)
                XFree(data);
	    if (type) {
                target = window;
                break;
            }
        }

        // try window's parent
        Window root;
        Window parent;
        Window *children;
        uint unused;
        if (!XQueryTree(X11->display, window, &root, &parent, &children, &unused))
            break;
        if (children)
            XFree(children);
        if (window == root)
            break;
        window = parent;
    }
    return target;
}




// clean up the stuff used.
static void qt_xdnd_cleanup();

static void qt_xdnd_send_leave();


// timer used when target wants "continuous" move messages (eg. scroll)
static int heartbeat = -1;
// top-level window we sent position to last.
static Window qt_xdnd_current_target;
// window to send events to (always valid if qt_xdnd_current_target)
static Window current_proxy_target;

// widget we forwarded position to last, and local position
static QPointer<QWidget> currentWindow;
static QPoint currentPosition;
// timestamp from the XdndPosition and XdndDrop
static Time target_time;
// screen number containing the pointer... -1 means default
static int qt_xdnd_current_screen = -1;
// state of dragging... true if dragging, false if not
bool qt_xdnd_dragging = false;

static bool waiting_for_status = false;

// used to preset each new QDragMoveEvent

// Shift/Ctrl handling, and final drop status
static Qt::DropAction global_accepted_action = Qt::CopyAction;
static Qt::DropActions possible_actions = Qt::IgnoreAction;

// for embedding only
static QWidget* current_embedding_widget  = 0;
static xcb_client_message_event_t last_enter_event;

// cursors
static QCursor *noDropCursor = 0;
static QCursor *moveCursor = 0;
static QCursor *copyCursor = 0;
static QCursor *linkCursor = 0;


class QExtraWidget : public QWidget
{
    Q_DECLARE_PRIVATE(QWidget)
public:
    inline QWExtra* extraData();
    inline QTLWExtra* topData();
};

inline QWExtra* QExtraWidget::extraData() { return d_func()->extraData(); }
inline QTLWExtra* QExtraWidget::topData() { return d_func()->topData(); }



void QX11Data::xdndSetup() {
    QCursorData::initialize();
    qAddPostRoutine(qt_xdnd_cleanup);
}


void qt_xdnd_cleanup()
{
    delete noDropCursor;
    noDropCursor = 0;
    delete copyCursor;
    copyCursor = 0;
    delete moveCursor;
    moveCursor = 0;
    delete linkCursor;
    linkCursor = 0;
    delete defaultPm;
    defaultPm = 0;
    delete xdnd_data.desktop_proxy;
    xdnd_data.desktop_proxy = 0;
    delete xdnd_data.deco;
    xdnd_data.deco = 0;
}


static QWidget *find_child(QWidget *tlw, QPoint & p)
{
    QWidget *widget = tlw;

    p = widget->mapFromGlobal(p);
    bool done = false;
    while (!done) {
        done = true;
        if (((QExtraWidget*)widget)->extraData() &&
             ((QExtraWidget*)widget)->extraData()->xDndProxy != 0)
            break; // stop searching for widgets under the mouse cursor if found widget is a proxy.
        QObjectList children = widget->children();
        if (!children.isEmpty()) {
            for(int i = children.size(); i > 0;) {
                --i;
                QWidget *w = qobject_cast<QWidget *>(children.at(i));
                if (!w)
                    continue;
                if (w->testAttribute(Qt::WA_TransparentForMouseEvents))
                    continue;
                if (w->isVisible() &&
                     w->geometry().contains(p) &&
                     !w->isWindow()) {
                    widget = w;
                    done = false;
                    p = widget->mapFromParent(p);
                    break;
                }
            }
        }
    }
    return widget;
}


static bool checkEmbedded(QWidget* w, const XEvent* xe)
{
    if (!w)
        return false;

    if (current_embedding_widget != 0 && current_embedding_widget != w) {
        qt_xdnd_current_target = ((QExtraWidget*)current_embedding_widget)->extraData()->xDndProxy;
        current_proxy_target = qt_xdnd_current_target;
        qt_xdnd_send_leave();
        qt_xdnd_current_target = 0;
        current_proxy_target = 0;
        current_embedding_widget = 0;
    }

    QWExtra* extra = ((QExtraWidget*)w)->extraData();
    if (extra && extra->xDndProxy != 0) {

        if (current_embedding_widget != w) {

            last_enter_event.xany.window = extra->xDndProxy;
            XSendEvent(X11->display, extra->xDndProxy, False, NoEventMask, &last_enter_event);
            current_embedding_widget = w;
        }

        ((XEvent*)xe)->xany.window = extra->xDndProxy;
        XSendEvent(X11->display, extra->xDndProxy, False, NoEventMask, (XEvent*)xe);
        if (currentWindow != w) {
            currentWindow = w;
        }
        return true;
    }
    current_embedding_widget = 0;
    return false;
}
#endif


void QXcbDrag::handleEnter(QWindow *window, const xcb_client_message_event_t *event)
{
    Q_UNUSED(window);
    DEBUG() << "handleEnter" << window;

//    motifdnd_active = false;
//    last_enter_event.xclient = xe->xclient;

    int version = (int)(event->data.data32[1] >> 24);
    if (version > xdnd_version)
        return;

    xdnd_dragsource = event->data.data32[0];

    if (event->data.data32[1] & 1) {
        // get the types from XdndTypeList
        xcb_get_property_cookie_t cookie = xcb_get_property(connection()->xcb_connection(), false, xdnd_dragsource,
                                                            connection()->atom(QXcbAtom::XdndTypelist), QXcbAtom::XA_ATOM,
                                                            0, xdnd_max_type);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(connection()->xcb_connection(), cookie, 0);
        if (reply && reply->type != XCB_NONE && reply->format == 32) {
            int length = xcb_get_property_value_length(reply) / 4;
            if (length > xdnd_max_type)
                length = xdnd_max_type;

            xcb_atom_t *atoms = (xcb_atom_t *)xcb_get_property_value(reply);
            for (int i = 0; i < length; ++i)
                xdnd_types.append(atoms[i]);
        }
        free(reply);
    } else {
        // get the types from the message
        for(int i = 2; i < 5; i++) {
            if (event->data.data32[i])
                xdnd_types.append(event->data.data32[i]);
        }
    }
    for(int i = 0; i < xdnd_types.length(); ++i)
        DEBUG() << "    " << connection()->atomName(xdnd_types.at(i));
}

void QXcbDrag::handle_xdnd_position(QWindow *w, const xcb_client_message_event_t *e, bool passive)
{
    QPoint p((e->data.data32[2] & 0xffff0000) >> 16, e->data.data32[2] & 0x0000ffff);
    Q_ASSERT(w);
    QRect geometry = w->geometry();

    p -= geometry.topLeft();

    // ####
//    if (!passive && checkEmbedded(w, e))
//        return;

    if (!w || (/*!w->acceptDrops() &&*/ (w->windowType() == Qt::Desktop)))
        return;

    if (e->data.data32[0] != xdnd_dragsource) {
        DEBUG("xdnd drag position from unexpected source (%x not %x)", e->data.data32[0], xdnd_dragsource);
        return;
    }

    // timestamp from the source
    if (e->data.data32[3] != 0)
        target_time /*= X11->userTime*/ = e->data.data32[3];

    QDragManager *manager = QDragManager::self();
    QMimeData *dropData = manager->dropData();

    xcb_client_message_event_t response;
    response.response_type = XCB_CLIENT_MESSAGE;
    response.window = xdnd_dragsource;
    response.format = 32;
    response.type = connection()->atom(QXcbAtom::XdndStatus);
    response.data.data32[0] = xcb_window(w);
    response.data.data32[1] = 0; // flags
    response.data.data32[2] = 0; // x, y
    response.data.data32[3] = 0; // w, h
    response.data.data32[4] = 0; // action

    if (!passive) { // otherwise just reject
        QRect answerRect(p + geometry.topLeft(), QSize(1,1));

        if (manager->object) {
            manager->possible_actions = manager->dragPrivate()->possible_actions;
        } else {
            manager->possible_actions = Qt::DropActions(toDropAction(e->data.data32[4]));
//             possible_actions |= Qt::CopyAction;
        }
        QDragMoveEvent me(p, manager->possible_actions, dropData,
                          QGuiApplication::mouseButtons(), QGuiApplication::keyboardModifiers());

        Qt::DropAction accepted_action = Qt::IgnoreAction;

        currentPosition = p;

        if (w != currentWindow.data()) {
            if (currentWindow) {
                QDragLeaveEvent e;
                QGuiApplication::sendEvent(currentWindow.data(), &e);
            }
            currentWindow = w;

            last_target_accepted_action = Qt::IgnoreAction;
            QDragEnterEvent de(p, manager->possible_actions, dropData,
                               QGuiApplication::mouseButtons(), QGuiApplication::keyboardModifiers());
            QGuiApplication::sendEvent(w, &de);
            if (de.isAccepted() && de.dropAction() != Qt::IgnoreAction)
                last_target_accepted_action = de.dropAction();
        }

        DEBUG() << "qt_handle_xdnd_position action=" << connection()->atomName(e->data.data32[4]);

        if (last_target_accepted_action != Qt::IgnoreAction) {
            me.setDropAction(last_target_accepted_action);
            me.accept();
        }
        QGuiApplication::sendEvent(w, &me);
        if (me.isAccepted()) {
            response.data.data32[1] = 1; // yes
            accepted_action = me.dropAction();
            last_target_accepted_action = accepted_action;
        } else {
            response.data.data32[0] = 0;
            last_target_accepted_action = Qt::IgnoreAction;
        }
        answerRect = me.answerRect().translated(geometry.topLeft()).intersected(geometry);

        if (answerRect.left() < 0)
            answerRect.setLeft(0);
        if (answerRect.right() > 4096)
            answerRect.setRight(4096);
        if (answerRect.top() < 0)
            answerRect.setTop(0);
        if (answerRect.bottom() > 4096)
            answerRect.setBottom(4096);
        if (answerRect.width() < 0)
            answerRect.setWidth(0);
        if (answerRect.height() < 0)
            answerRect.setHeight(0);

        response.data.data32[2] = (answerRect.x() << 16) + answerRect.y();
        response.data.data32[3] = (answerRect.width() << 16) + answerRect.height();
        response.data.data32[4] = toXdndAction(accepted_action);
    }

    // reset
    target_time = XCB_CURRENT_TIME;

    QXcbWindow *source = connection()->platformWindowFromId(xdnd_dragsource);
    if (source && (source->window()->windowType() == Qt::Desktop) /*&& !source->acceptDrops()*/)
        source = 0;

    DEBUG() << "sending XdndStatus";
    if (source)
        handle_xdnd_status(source->window(), &response, passive);
    else
        Q_XCB_CALL(xcb_send_event(connection()->xcb_connection(), false, xdnd_dragsource,
                                  XCB_EVENT_MASK_NO_EVENT, (const char *)&response));
}

namespace
{
    class ClientMessageScanner {
    public:
        ClientMessageScanner(xcb_atom_t a) : atom(a) {}
        xcb_atom_t atom;
        bool check(xcb_generic_event_t *event) const {
            if (!event)
                return false;
            if ((event->response_type & 0x7f) != XCB_CLIENT_MESSAGE)
                return false;
            return ((xcb_client_message_event_t *)event)->type == atom;
        }
    };
}

void QXcbDrag::handlePosition(QWindow * w, const xcb_client_message_event_t *event, bool passive)
{
    xcb_client_message_event_t *lastEvent = const_cast<xcb_client_message_event_t *>(event);
    xcb_generic_event_t *nextEvent;
    ClientMessageScanner scanner(connection()->atom(QXcbAtom::XdndPosition));
    while ((nextEvent = connection()->checkEvent(scanner))) {
        if (lastEvent != event)
            free(lastEvent);
        lastEvent = (xcb_client_message_event_t *)nextEvent;
    }

    handle_xdnd_position(w, lastEvent, passive);
    if (lastEvent != event)
        free(lastEvent);
}

void QXcbDrag::handle_xdnd_status(QWindow *, const xcb_client_message_event_t *event, bool)
{
    // ignore late status messages
    if (event->data.data32[0] && event->data.data32[0] != current_proxy_target)
        return;

    Qt::DropAction newAction = (event->data.data32[1] & 0x1) ? toDropAction(event->data.data32[4]) : Qt::IgnoreAction;

    if ((event->data.data32[1] & 2) == 0) {
        QPoint p((event->data.data32[2] & 0xffff0000) >> 16, event->data.data32[2] & 0x0000ffff);
        QSize s((event->data.data32[3] & 0xffff0000) >> 16, event->data.data32[3] & 0x0000ffff);
        source_sameanswer = QRect(p, s);
    } else {
        source_sameanswer = QRect();
    }
    QDragManager *manager = QDragManager::self();
    manager->willDrop = (event->data.data32[1] & 0x1);
    if (manager->global_accepted_action != newAction) {
        manager->global_accepted_action = newAction;
        manager->emitActionChanged(newAction);
    }
    manager->updateCursor();
    waiting_for_status = false;
}

void QXcbDrag::handleStatus(QWindow *w, const xcb_client_message_event_t *event, bool passive)
{
    DEBUG("xdndHandleStatus");
    xcb_client_message_event_t *lastEvent = const_cast<xcb_client_message_event_t *>(event);
    xcb_generic_event_t *nextEvent;
    ClientMessageScanner scanner(connection()->atom(QXcbAtom::XdndStatus));
    while ((nextEvent = connection()->checkEvent(scanner))) {
        if (lastEvent != event)
            free(lastEvent);
        lastEvent = (xcb_client_message_event_t *)nextEvent;
    }

    handle_xdnd_status(w, lastEvent, passive);
    if (lastEvent != event)
        free(lastEvent);
    DEBUG("xdndHandleStatus end");
}

void QXcbDrag::handleLeave(QWindow *w, const xcb_client_message_event_t *event, bool /*passive*/)
{
    DEBUG("xdnd leave");
    if (!currentWindow || w != currentWindow.data())
        return; // sanity

    // ###
//    if (checkEmbedded(current_embedding_widget, event)) {
//        current_embedding_widget = 0;
//        currentWindow.clear();
//        return;
//    }

    if (event->data.data32[0] != xdnd_dragsource) {
        // This often happens - leave other-process window quickly
        DEBUG("xdnd drag leave from unexpected source (%x not %x", event->data.data32[0], xdnd_dragsource);
    }

    QDragLeaveEvent e;
    QGuiApplication::sendEvent(currentWindow.data(), &e);

    xdnd_dragsource = 0;
    xdnd_types.clear();
    currentWindow.clear();
}

#if 0
void qt_xdnd_send_leave()
{
    if (!qt_xdnd_current_target)
        return;

    QDragManager *manager = QDragManager::self();

    XClientMessageEvent leave;
    leave.type = ClientMessage;
    leave.window = qt_xdnd_current_target;
    leave.format = 32;
    leave.message_type = ATOM(XdndLeave);
    leave.data.l[0] = manager->dragPrivate()->source->effectiveWinId();
    leave.data.l[1] = 0; // flags
    leave.data.l[2] = 0; // x, y
    leave.data.l[3] = 0; // w, h
    leave.data.l[4] = 0; // just null

    QWidget * w = QWidget::find(current_proxy_target);

    if (w && (w->windowType() == Qt::Desktop) && !w->acceptDrops())
        w = 0;

    if (w)
        X11->xdndHandleLeave(w, (const XEvent *)&leave, false);
    else
        XSendEvent(X11->display, current_proxy_target, False,
                    NoEventMask, (XEvent*)&leave);

    // reset the drag manager state
    manager->willDrop = false;
    if (global_accepted_action != Qt::IgnoreAction)
        manager->emitActionChanged(Qt::IgnoreAction);
    global_accepted_action = Qt::IgnoreAction;
    manager->updateCursor();
    qt_xdnd_current_target = 0;
    current_proxy_target = 0;
    qt_xdnd_source_current_time = 0;
    waiting_for_status = false;
}

// TODO: remove and use QApplication::currentKeyboardModifiers() in Qt 4.8.
static Qt::KeyboardModifiers currentKeyboardModifiers()
{
    Window root;
    Window child;
    int root_x, root_y, win_x, win_y;
    uint keybstate;
    for (int i = 0; i < ScreenCount(X11->display); ++i) {
        if (XQueryPointer(X11->display, QX11Info::appRootWindow(i), &root, &child,
                          &root_x, &root_y, &win_x, &win_y, &keybstate))
            return X11->translateModifiers(keybstate & 0x00ff);
    }
    return 0;
}
#endif

void QXcbDrag::handleDrop(QWindow *, const xcb_client_message_event_t *event, bool passive)
{
    DEBUG("xdndHandleDrop");
    if (!currentWindow) {
        xdnd_dragsource = 0;
        return; // sanity
    }

    // ###
//    if (!passive && checkEmbedded(currentWindow, xe)){
//        current_embedding_widget = 0;
//        xdnd_dragsource = 0;
//        currentWindow = 0;
//        return;
//    }
    const uint32_t *l = event->data.data32;

    QDragManager *manager = QDragManager::self();
    DEBUG("xdnd drop");

    if (l[0] != xdnd_dragsource) {
        DEBUG("xdnd drop from unexpected source (%x not %x", l[0], xdnd_dragsource);
        return;
    }

    // update the "user time" from the timestamp in the event.
    if (l[2] != 0)
        target_time = /*X11->userTime =*/ l[2];

    if (!passive) {
        // this could be a same-application drop, just proxied due to
        // some XEMBEDding, so try to find the real QMimeData used
        // based on the timestamp for this drop.
        QMimeData *dropData = 0;
        // ###
//        int at = findXdndDropTransactionByTime(target_time);
//        if (at != -1)
//            dropData = QDragManager::dragPrivate(X11->dndDropTransactions.at(at).object)->data;
        // if we can't find it, then use the data in the drag manager
        if (!dropData)
            dropData = manager->dropData();

        // Drop coming from another app? Update keyboard modifiers.
//        if (!qt_xdnd_dragging) {
//            QApplicationPrivate::modifier_buttons = currentKeyboardModifiers();
//        }

        QDropEvent de(currentPosition, manager->possible_actions, dropData,
                      QGuiApplication::mouseButtons(), QGuiApplication::keyboardModifiers());
        QGuiApplication::sendEvent(currentWindow.data(), &de);
        if (!de.isAccepted()) {
            // Ignore a failed drag
            manager->global_accepted_action = Qt::IgnoreAction;
        } else {
            manager->global_accepted_action = de.dropAction();
        }
        xcb_client_message_event_t finished;
        finished.type = XCB_CLIENT_MESSAGE;
        finished.window = xdnd_dragsource;
        finished.format = 32;
        finished.type = connection()->atom(QXcbAtom::XdndFinished);
        DNDDEBUG << "xdndHandleDrop"
             << "currentWindow" << currentWindow
             << (currentWindow ? xcb_window(currentWindow.data()) : 0);
        finished.data.data32[0] = currentWindow ? xcb_window(currentWindow.data()) : XCB_NONE;
        finished.data.data32[1] = de.isAccepted() ? 1 : 0; // flags
        finished.data.data32[2] = toXdndAction(manager->global_accepted_action);
        Q_XCB_CALL(xcb_send_event(connection()->xcb_connection(), false, xdnd_dragsource,
                       XCB_EVENT_MASK_NO_EVENT, (char *)&finished));
    } else {
        QDragLeaveEvent e;
        QGuiApplication::sendEvent(currentWindow.data(), &e);
    }
    xdnd_dragsource = 0;
    currentWindow.clear();
    waiting_for_status = false;

    // reset
    target_time = XCB_CURRENT_TIME;
}

#if 0

void QX11Data::xdndHandleFinished(QWidget *, const XEvent * xe, bool passive)
{
    DEBUG("xdndHandleFinished");
    const unsigned long *l = (const unsigned long *)xe->xclient.data.l;

    DNDDEBUG << "xdndHandleFinished, l[0]" << l[0]
             << "qt_xdnd_current_target" << qt_xdnd_current_target
             << "qt_xdnd_current_proxy_targe" << current_proxy_target;

    if (l[0]) {
        int at = findXdndDropTransactionByWindow(l[0]);
        if (at != -1) {
            restartXdndDropExpiryTimer();

            QXdndDropTransaction t = X11->dndDropTransactions.takeAt(at);
            QDragManager *manager = QDragManager::self();

            Window target = qt_xdnd_current_target;
            Window proxy_target = current_proxy_target;
            QWidget *embedding_widget = current_embedding_widget;
            QDrag *currentObject = manager->object;

            qt_xdnd_current_target = t.target;
            current_proxy_target = t.proxy_target;
            current_embedding_widget = t.embedding_widget;
            manager->object = t.object;

            if (!passive)
                (void) checkEmbedded(currentWindow, xe);

            current_embedding_widget = 0;
            qt_xdnd_current_target = 0;
            current_proxy_target = 0;

            if (t.object)
                t.object->deleteLater();

            qt_xdnd_current_target = target;
            current_proxy_target = proxy_target;
            current_embedding_widget = embedding_widget;
            manager->object = currentObject;
        }
    }
    waiting_for_status = false;
}


void QDragManager::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == heartbeat && source_sameanswer.isNull()) {
        move(QCursor::pos());
    } else if (e->timerId() == transaction_expiry_timer) {
        for (int i = 0; i < X11->dndDropTransactions.count(); ++i) {
            const QXdndDropTransaction &t = X11->dndDropTransactions.at(i);
            if (t.targetWidget) {
                // dnd within the same process, don't delete these
                continue;
            }
            t.object->deleteLater();
            X11->dndDropTransactions.removeAt(i--);
        }

        killTimer(transaction_expiry_timer);
        transaction_expiry_timer = -1;
    }
}

bool QDragManager::eventFilter(QObject * o, QEvent * e)
{
    if (beingCancelled) {
        if (e->type() == QEvent::KeyRelease && ((QKeyEvent*)e)->key() == Qt::Key_Escape) {
            qApp->removeEventFilter(this);
            Q_ASSERT(object == 0);
            beingCancelled = false;
            eventLoop->exit();
            return true; // block the key release
        }
        return false;
    }

    Q_ASSERT(object != 0);

    if (!o->isWidgetType())
        return false;

    if (e->type() == QEvent::MouseMove) {
        QMouseEvent* me = (QMouseEvent *)e;
        move(me->globalPos());
        return true;
    } else if (e->type() == QEvent::MouseButtonRelease) {
        DEBUG("pre drop");
        qApp->removeEventFilter(this);
        if (willDrop)
            drop();
        else
            cancel();
        DEBUG("drop, resetting object");
        beingCancelled = false;
        eventLoop->exit();
        return true;
    }

    if (e->type() == QEvent::ShortcutOverride) {
        // prevent accelerators from firing while dragging
        e->accept();
        return true;
    }

    if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = ((QKeyEvent*)e);
        if (ke->key() == Qt::Key_Escape && e->type() == QEvent::KeyPress) {
            cancel();
            qApp->removeEventFilter(this);
            beingCancelled = false;
            eventLoop->exit();
        } else {
            source_sameanswer = QRect(); // force move
            move(QCursor::pos());
        }
        return true; // Eat all key events
    }

    // ### We bind modality to widgets, so we have to do this
    // ###  "manually".
    // DnD is modal - eat all other interactive events
    switch (e->type()) {
      case QEvent::MouseButtonPress:
      case QEvent::MouseButtonRelease:
      case QEvent::MouseButtonDblClick:
      case QEvent::MouseMove:
      case QEvent::KeyPress:
      case QEvent::KeyRelease:
      case QEvent::Wheel:
      case QEvent::ShortcutOverride:
        return true;
      default:
        return false;
    }
}

void QDragManager::updateCursor()
{
    if (!noDropCursor) {
#ifndef QT_NO_CURSOR
        noDropCursor = new QCursor(Qt::ForbiddenCursor);
        moveCursor = new QCursor(Qt::DragMoveCursor);
        copyCursor = new QCursor(Qt::DragCopyCursor);
        linkCursor = new QCursor(Qt::DragLinkCursor);
#endif
    }

    QCursor *c;
    if (willDrop) {
        if (global_accepted_action == Qt::CopyAction) {
            c = copyCursor;
        } else if (global_accepted_action == Qt::LinkAction) {
            c = linkCursor;
        } else {
            c = moveCursor;
        }
        if (xdnd_data.deco) {
            xdnd_data.deco->show();
            xdnd_data.deco->raise();
        }
    } else {
        c = noDropCursor;
        //if (qt_xdnd_deco)
        //    qt_xdnd_deco->hide();
    }
#ifndef QT_NO_CURSOR
    if (c)
        qApp->changeOverrideCursor(*c);
#endif
}


void QDragManager::cancel(bool deleteSource)
{
    DEBUG("QDragManager::cancel");
    Q_ASSERT(heartbeat != -1);
    killTimer(heartbeat);
    heartbeat = -1;
    beingCancelled = true;
    qt_xdnd_dragging = false;

    if (qt_xdnd_current_target)
        qt_xdnd_send_leave();

#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        QApplication::restoreOverrideCursor();
        restoreCursor = false;
    }
#endif

    if (deleteSource && object)
        object->deleteLater();
    object = 0;
    qDeleteInEventHandler(xdnd_data.deco);
    xdnd_data.deco = 0;

    global_accepted_action = Qt::IgnoreAction;
}

static
Window findRealWindow(const QPoint & pos, Window w, int md)
{
    if (xdnd_data.deco && w == xdnd_data.deco->effectiveWinId())
        return 0;

    if (md) {
        X11->ignoreBadwindow();
        XWindowAttributes attr;
        XGetWindowAttributes(X11->display, w, &attr);
        if (X11->badwindow())
            return 0;

        if (attr.map_state == IsViewable
            && QRect(attr.x,attr.y,attr.width,attr.height).contains(pos)) {
            {
                Atom   type = XNone;
                int f;
                unsigned long n, a;
                unsigned char *data;

                XGetWindowProperty(X11->display, w, ATOM(XdndAware), 0, 0, False,
                                   AnyPropertyType, &type, &f,&n,&a,&data);
                if (data) XFree(data);
                if (type)
                    return w;
            }

            Window r, p;
            Window* c;
            uint nc;
            if (XQueryTree(X11->display, w, &r, &p, &c, &nc)) {
                r=0;
                for (uint i=nc; !r && i--;) {
                    r = findRealWindow(pos-QPoint(attr.x,attr.y),
                                        c[i], md-1);
                }
                XFree(c);
                if (r)
                    return r;

                // We didn't find a client window!  Just use the
                // innermost window.
            }

            // No children!
            return w;
        }
    }
    return 0;
}

void QDragManager::move(const QPoint & globalPos)
{
#ifdef QT_NO_CURSOR
    Q_UNUSED(globalPos);
    return;
#else
    DEBUG() << "QDragManager::move enter";
    if (!object) {
        // perhaps the target crashed?
        return;
    }

    int screen = QCursor::x11Screen();
    if ((qt_xdnd_current_screen == -1 && screen != X11->defaultScreen) || (screen != qt_xdnd_current_screen)) {
        // recreate the pixmap on the new screen...
        delete xdnd_data.deco;
        QWidget* parent = object->source()->window()->x11Info().screen() == screen
            ? object->source()->window() : QApplication::desktop()->screen(screen);
        xdnd_data.deco = new QShapedPixmapWidget(parent);
        if (!QWidget::mouseGrabber()) {
            updatePixmap();
            xdnd_data.deco->grabMouse();
        }
    }
    xdnd_data.deco->move(QCursor::pos() - xdnd_data.deco->pm_hot);

    if (source_sameanswer.contains(globalPos) && source_sameanswer.isValid())
        return;

    qt_xdnd_current_screen = screen;
    Window rootwin = QX11Info::appRootWindow(qt_xdnd_current_screen);
    Window target = 0;
    int lx = 0, ly = 0;
    if (!XTranslateCoordinates(X11->display, rootwin, rootwin, globalPos.x(), globalPos.y(), &lx, &ly, &target))
        // some weird error...
        return;

    if (target == rootwin) {
        // Ok.
    } else if (target) {
        //me
        Window src = rootwin;
        while (target != 0) {
            DNDDEBUG << "checking target for XdndAware" << QWidget::find(target) << target;
            int lx2, ly2;
            Window t;
            // translate coordinates
            if (!XTranslateCoordinates(X11->display, src, target, lx, ly, &lx2, &ly2, &t)) {
                target = 0;
                break;
            }
            lx = lx2;
            ly = ly2;
            src = target;

	    // check if it has XdndAware
	    Atom type = 0;
	    int f;
	    unsigned long n, a;
	    unsigned char *data = 0;
	    XGetWindowProperty(X11->display, target, ATOM(XdndAware), 0, 0, False,
                               AnyPropertyType, &type, &f,&n,&a,&data);
	    if (data)
                XFree(data);
	    if (type) {
                DNDDEBUG << "Found XdndAware on " << QWidget::find(target) << target;
                break;
            }

            // find child at the coordinates
            if (!XTranslateCoordinates(X11->display, src, src, lx, ly, &lx2, &ly2, &target)) {
                target = 0;
                break;
            }
        }
        if (xdnd_data.deco && (!target || target == xdnd_data.deco->effectiveWinId())) {
            DNDDEBUG << "need to find real window";
            target = findRealWindow(globalPos, rootwin, 6);
            DNDDEBUG << "real window found" << QWidget::find(target) << target;
        }
    }

    QWidget* w;
    if (target) {
        w = QWidget::find((WId)target);
        if (w && (w->windowType() == Qt::Desktop) && !w->acceptDrops())
            w = 0;
    } else {
        w = 0;
        target = rootwin;
    }

    DNDDEBUG << "and the final target is " << QWidget::find(target) << target;
    DNDDEBUG << "the widget w is" << w;

    WId proxy_target = xdndProxy(target);
    if (!proxy_target)
        proxy_target = target;
    int target_version = 1;

    if (proxy_target) {
        Atom   type = XNone;
        int r, f;
        unsigned long n, a;
        unsigned char *retval;
        X11->ignoreBadwindow();
        r = XGetWindowProperty(X11->display, proxy_target, ATOM(XdndAware), 0,
                               1, False, AnyPropertyType, &type, &f,&n,&a,&retval);
        int *tv = (int *)retval;
        if (r != Success || X11->badwindow()) {
            target = 0;
        } else {
            target_version = qMin(xdnd_version,tv ? *tv : 1);
            if (tv)
                XFree(tv);
//             if (!(!X11->badwindow() && type))
//                 target = 0;
        }
    }

    if (target != qt_xdnd_current_target) {
        if (qt_xdnd_current_target)
            qt_xdnd_send_leave();

        qt_xdnd_current_target = target;
        current_proxy_target = proxy_target;
        if (target) {
            QVector<Atom> types;
            int flags = target_version << 24;
            QStringList fmts = QInternalMimeData::formatsHelper(dragPrivate()->data);
            for (int i = 0; i < fmts.size(); ++i) {
                QList<Atom> atoms = X11->xdndMimeAtomsForFormat(fmts.at(i));
                for (int j = 0; j < atoms.size(); ++j) {
                    if (!types.contains(atoms.at(j)))
                        types.append(atoms.at(j));
                }
            }
            if (types.size() > 3) {
                XChangeProperty(X11->display,
                                dragPrivate()->source->effectiveWinId(), ATOM(XdndTypelist),
                                XA_ATOM, 32, PropModeReplace,
                                (unsigned char *)types.data(),
                                types.size());
                flags |= 0x0001;
            }
            XClientMessageEvent enter;
            enter.type = ClientMessage;
            enter.window = target;
            enter.format = 32;
            enter.message_type = ATOM(XdndEnter);
            enter.data.l[0] = dragPrivate()->source->effectiveWinId();
            enter.data.l[1] = flags;
            enter.data.l[2] = types.size()>0 ? types.at(0) : 0;
            enter.data.l[3] = types.size()>1 ? types.at(1) : 0;
            enter.data.l[4] = types.size()>2 ? types.at(2) : 0;
            // provisionally set the rectangle to 5x5 pixels...
            source_sameanswer = QRect(globalPos.x() - 2,
                                              globalPos.y() -2 , 5, 5);

            DEBUG("sending Xdnd enter");
            if (w)
                X11->xdndHandleEnter(w, (const XEvent *)&enter, false);
            else if (target)
                XSendEvent(X11->display, proxy_target, False, NoEventMask, (XEvent*)&enter);
            waiting_for_status = false;
        }
    }
    if (waiting_for_status)
        return;

    if (target) {
        waiting_for_status = true;

        XClientMessageEvent move;
        move.type = ClientMessage;
        move.window = target;
        move.format = 32;
        move.message_type = ATOM(XdndPosition);
        move.window = target;
        move.data.l[0] = dragPrivate()->source->effectiveWinId();
        move.data.l[1] = 0; // flags
        move.data.l[2] = (globalPos.x() << 16) + globalPos.y();
        move.data.l[3] = X11->time;
        move.data.l[4] = qtaction_to_xdndaction(defaultAction(dragPrivate()->possible_actions, QApplication::keyboardModifiers()));
        DEBUG("sending Xdnd position");

        qt_xdnd_source_current_time = X11->time;

        if (w)
            handle_xdnd_position(w, (const XEvent *)&move, false);
        else
            XSendEvent(X11->display, proxy_target, False, NoEventMask,
                       (XEvent*)&move);
    } else {
        if (willDrop) {
            willDrop = false;
            updateCursor();
        }
    }
    DEBUG() << "QDragManager::move leave";
#endif
}


void QDragManager::drop()
{
    Q_ASSERT(heartbeat != -1);
    killTimer(heartbeat);
    heartbeat = -1;
    qt_xdnd_dragging = false;

    if (!qt_xdnd_current_target)
        return;

    qDeleteInEventHandler(xdnd_data.deco);
    xdnd_data.deco = 0;

    XClientMessageEvent drop;
    drop.type = ClientMessage;
    drop.window = qt_xdnd_current_target;
    drop.format = 32;
    drop.message_type = ATOM(XdndDrop);
    drop.data.l[0] = dragPrivate()->source->effectiveWinId();
    drop.data.l[1] = 0; // flags
    drop.data.l[2] = X11->time;

    drop.data.l[3] = 0;
    drop.data.l[4] = 0;

    QWidget * w = QWidget::find(current_proxy_target);

    if (w && (w->windowType() == Qt::Desktop) && !w->acceptDrops())
        w = 0;

    QXdndDropTransaction t = {
        X11->time,
        qt_xdnd_current_target,
        current_proxy_target,
        w,
        current_embedding_widget,
        object
    };
    X11->dndDropTransactions.append(t);
    restartXdndDropExpiryTimer();

    if (w)
        X11->xdndHandleDrop(w, (const XEvent *)&drop, false);
    else
        XSendEvent(X11->display, current_proxy_target, False,
                   NoEventMask, (XEvent*)&drop);

    qt_xdnd_current_target = 0;
    current_proxy_target = 0;
    qt_xdnd_source_current_time = 0;
    current_embedding_widget = 0;
    object = 0;

#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        QApplication::restoreOverrideCursor();
        restoreCursor = false;
    }
#endif
}



bool QX11Data::xdndHandleBadwindow()
{
    if (qt_xdnd_current_target) {
        QDragManager *manager = QDragManager::self();
        if (manager->object) {
            qt_xdnd_current_target = 0;
            current_proxy_target = 0;
            manager->object->deleteLater();
            manager->object = 0;
            delete xdnd_data.deco;
            xdnd_data.deco = 0;
            return true;
        }
    }
    if (xdnd_dragsource) {
        xdnd_dragsource = 0;
        if (currentWindow) {
            QApplication::postEvent(currentWindow, new QDragLeaveEvent);
            currentWindow = 0;
        }
        return true;
    }
    return false;
}

void QX11Data::xdndHandleSelectionRequest(const XSelectionRequestEvent * req)
{
    if (!req)
        return;
    XEvent evt;
    evt.xselection.type = SelectionNotify;
    evt.xselection.display = req->display;
    evt.xselection.requestor = req->requestor;
    evt.xselection.selection = req->selection;
    evt.xselection.target = XNone;
    evt.xselection.property = XNone;
    evt.xselection.time = req->time;

    QDragManager *manager = QDragManager::self();
    QDrag *currentObject = manager->object;

    // which transaction do we use? (note: -2 means use current manager->object)
    int at = -1;

    // figure out which data the requestor is really interested in
    if (manager->object && req->time == qt_xdnd_source_current_time) {
        // requestor wants the current drag data
        at = -2;
    } else {
        // if someone has requested data in response to XdndDrop, find the corresponding transaction. the
        // spec says to call XConvertSelection() using the timestamp from the XdndDrop
        at = findXdndDropTransactionByTime(req->time);
        if (at == -1) {
            // no dice, perhaps the client was nice enough to use the same window id in XConvertSelection()
            // that we sent the XdndDrop event to.
            at = findXdndDropTransactionByWindow(req->requestor);
        }
        if (at == -1 && req->time == CurrentTime) {
            // previous Qt versions always requested the data on a child of the target window
            // using CurrentTime... but it could be asking for either drop data or the current drag's data
            Window target = findXdndAwareParent(req->requestor);
            if (target) {
                if (qt_xdnd_current_target && qt_xdnd_current_target == target)
                    at = -2;
                else
                    at = findXdndDropTransactionByWindow(target);
            }
        }
    }
    if (at >= 0) {
        restartXdndDropExpiryTimer();

        // use the drag object from an XdndDrop tansaction
        manager->object = X11->dndDropTransactions.at(at).object;
    } else if (at != -2) {
        // no transaction found, we'll have to reject the request
        manager->object = 0;
    }
    if (manager->object) {
        Atom atomFormat = req->target;
        int dataFormat = 0;
        QByteArray data;
        if (X11->xdndMimeDataForAtom(req->target, manager->dragPrivate()->data,
                                     &data, &atomFormat, &dataFormat)) {
            int dataSize = data.size() / (dataFormat / 8);
            XChangeProperty (X11->display, req->requestor, req->property,
                             atomFormat, dataFormat, PropModeReplace,
                             (unsigned char *)data.data(), dataSize);
            evt.xselection.property = req->property;
            evt.xselection.target = atomFormat;
        }
    }

    // reset manager->object in case we modified it above
    manager->object = currentObject;

    // ### this can die if req->requestor crashes at the wrong
    // ### moment
    XSendEvent(X11->display, req->requestor, False, 0, &evt);
}



/*
  Enable drag and drop for widget w by installing the proper
  properties on w's toplevel widget.
*/
bool QX11Data::dndEnable(QWidget* w, bool on)
{
    w = w->window();

    if (bool(((QExtraWidget*)w)->topData()->dnd) == on)
        return true; // been there, done that
    ((QExtraWidget*)w)->topData()->dnd = on ? 1 : 0;

    motifdndEnable(w, on);
    return xdndEnable(w, on);
}

Qt::DropAction QDragManager::drag(QDrag * o)
{
    if (object == o || !o || !o->d_func()->source)
        return Qt::IgnoreAction;

    if (object) {
        cancel();
        qApp->removeEventFilter(this);
        beingCancelled = false;
    }

    if (object) {
        // the last drag and drop operation hasn't finished, so we are going to wait
        // for one second to see if it does... if the finish message comes after this,
        // then we could still have problems, but this is highly unlikely
        QApplication::flush();

        QElapsedTimer timer;
        timer.start();
        do {
            XEvent event;
            if (XCheckTypedEvent(X11->display, ClientMessage, &event))
                qApp->x11ProcessEvent(&event);

            // sleep 50 ms, so we don't use up CPU cycles all the time.
            struct timeval usleep_tv;
            usleep_tv.tv_sec = 0;
            usleep_tv.tv_usec = 50000;
            select(0, 0, 0, 0, &usleep_tv);
        } while (object && timer.hasExpired(1000));
    }

    object = o;
    object->d_func()->target = 0;
    xdnd_data.deco = new QShapedPixmapWidget(object->source()->window());

    willDrop = false;

    updatePixmap();

    qApp->installEventFilter(this);
    XSetSelectionOwner(X11->display, ATOM(XdndSelection), dragPrivate()->source->window()->internalWinId(), X11->time);
    global_accepted_action = Qt::CopyAction;
    source_sameanswer = QRect();
#ifndef QT_NO_CURSOR
    // set the override cursor (must be done here, since it is updated
    // in the call to move() below)
    qApp->setOverrideCursor(Qt::ArrowCursor);
    restoreCursor = true;
#endif
    move(QCursor::pos());
    heartbeat = startTimer(200);

    qt_xdnd_dragging = true;

    if (!QWidget::mouseGrabber())
        xdnd_data.deco->grabMouse();

    eventLoop = new QEventLoop;
    (void) eventLoop->exec();
    delete eventLoop;
    eventLoop = 0;

#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        qApp->restoreOverrideCursor();
        restoreCursor = false;
    }
#endif

    // delete cursors as they may be different next drag.
    delete noDropCursor;
    noDropCursor = 0;
    delete copyCursor;
    copyCursor = 0;
    delete moveCursor;
    moveCursor = 0;
    delete linkCursor;
    linkCursor = 0;

    delete xdnd_data.deco;
    xdnd_data.deco = 0;
    if (heartbeat != -1)
        killTimer(heartbeat);
    heartbeat = -1;
    qt_xdnd_current_screen = -1;
    qt_xdnd_dragging = false;

    return global_accepted_action;
    // object persists until we get an xdnd_finish message
}

#endif


static xcb_window_t xdndProxy(QXcbConnection *c, xcb_window_t w)
{
    xcb_window_t proxy = XCB_NONE;

    xcb_get_property_cookie_t cookie = xcb_get_property(c->xcb_connection(), false, w, c->atom(QXcbAtom::XdndProxy),
                                                        QXcbAtom::XA_WINDOW, 0, 1);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(c->xcb_connection(), cookie, 0);

    if (reply && reply->type == QXcbAtom::XA_WINDOW)
        proxy = *((xcb_window_t *)xcb_get_property_value(reply));
    free(reply);

    if (proxy == XCB_NONE)
        return proxy;

    // exists and is real?
    cookie = xcb_get_property(c->xcb_connection(), false, proxy, c->atom(QXcbAtom::XdndProxy),
                                                        QXcbAtom::XA_WINDOW, 0, 1);
    reply = xcb_get_property_reply(c->xcb_connection(), cookie, 0);

    if (reply && reply->type == QXcbAtom::XA_WINDOW) {
        xcb_window_t p = *((xcb_window_t *)xcb_get_property_value(reply));
        if (proxy != p)
            proxy = 0;
    } else {
        proxy = 0;
    }

    free(reply);

    return proxy;
}

bool QXcbDrag::dndEnable(QXcbWindow *w, bool on)
{
    DNDDEBUG << "xdndEnable" << w << on;
    if (on) {
        QXcbWindow *xdnd_widget = 0;
        if ((w->window()->windowType() == Qt::Desktop)) {
            if (desktop_proxy) // *WE* already have one.
                return false;

            xcb_grab_server(connection()->xcb_connection());

            // As per Xdnd4, use XdndProxy
            xcb_window_t proxy_id = xdndProxy(connection(), w->xcb_window());

            if (!proxy_id) {
                desktop_proxy = new QWindow;
                xdnd_widget = static_cast<QXcbWindow *>(desktop_proxy->handle());
                proxy_id = xdnd_widget->xcb_window();
                xcb_atom_t xdnd_proxy = connection()->atom(QXcbAtom::XdndProxy);
                xcb_change_property(connection()->xcb_connection(), XCB_PROP_MODE_REPLACE, w->xcb_window(), xdnd_proxy,
                                    QXcbAtom::XA_WINDOW, 32, 1, &proxy_id);
                xcb_change_property(connection()->xcb_connection(), XCB_PROP_MODE_REPLACE, proxy_id, xdnd_proxy,
                                    QXcbAtom::XA_WINDOW, 32, 1, &proxy_id);
            }

            xcb_ungrab_server(connection()->xcb_connection());
        } else {
            xdnd_widget = w;
        }
        if (xdnd_widget) {
            DNDDEBUG << "setting XdndAware for" << xdnd_widget << xdnd_widget->xcb_window();
            xcb_atom_t atm = xdnd_version;
            xcb_change_property(connection()->xcb_connection(), XCB_PROP_MODE_REPLACE, xdnd_widget->xcb_window(),
                                connection()->atom(QXcbAtom::XdndAware), QXcbAtom::XA_ATOM, 32, 1, &atm);
            return true;
        } else {
            return false;
        }
    } else {
        if ((w->window()->windowType() == Qt::Desktop)) {
            xcb_delete_property(connection()->xcb_connection(), w->xcb_window(), connection()->atom(QXcbAtom::XdndProxy));
            delete desktop_proxy;
            desktop_proxy = 0;
        } else {
            DNDDEBUG << "not deleting XDndAware";
        }
        return true;
    }
}




QDropData::QDropData(QXcbDrag *d)
    : QXcbMime(),
      drag(d)
{
}

QDropData::~QDropData()
{
}

QVariant QDropData::retrieveData_sys(const QString &mimetype, QVariant::Type requestedType) const
{
    QByteArray mime = mimetype.toLatin1();
    QVariant data = /*X11->motifdnd_active
                      ? X11->motifdndObtainData(mime)
                      :*/ xdndObtainData(mime, requestedType);
    return data;
}

QVariant QDropData::xdndObtainData(const QByteArray &format, QVariant::Type requestedType) const
{
    QByteArray result;

    QDragManager *manager = QDragManager::self();
    QXcbConnection *c = drag->connection();
    QXcbWindow *xcb_window = c->platformWindowFromId(drag->xdnd_dragsource);
    if (xcb_window && manager->object && xcb_window->window()->windowType() != Qt::Desktop) {
        QDragPrivate *o = manager->dragPrivate();
        if (o->data->hasFormat(QLatin1String(format)))
            result = o->data->data(QLatin1String(format));
        return result;
    }

    QList<xcb_atom_t> atoms = drag->xdnd_types;
    QByteArray encoding;
    xcb_atom_t a = mimeAtomForFormat(c, QLatin1String(format), requestedType, atoms, &encoding);
    if (a == XCB_NONE)
        return result;

    if (c->clipboard()->getSelectionOwner(drag->connection()->atom(QXcbAtom::XdndSelection)) == XCB_NONE)
        return result; // should never happen?

    QWindow* tw = drag->currentWindow.data();
    if (!drag->currentWindow || (drag->currentWindow.data()->windowType() == Qt::Desktop))
        tw = new QWindow;
    xcb_window_t win = ::xcb_window(tw);

    xcb_atom_t xdnd_selection = c->atom(QXcbAtom::XdndSelection);
    result = c->clipboard()->getSelection(win, xdnd_selection, a, xdnd_selection);

    if (!drag->currentWindow || (drag->currentWindow.data()->windowType() == Qt::Desktop))
        delete tw;

    return mimeConvertToFormat(c, a, result, QLatin1String(format), requestedType, encoding);
}


bool QDropData::hasFormat_sys(const QString &format) const
{
    return formats().contains(format);
}

QStringList QDropData::formats_sys() const
{
    QStringList formats;
//    if (X11->motifdnd_active) {
//        int i = 0;
//        QByteArray fmt;
//        while (!(fmt = X11->motifdndFormat(i)).isEmpty()) {
//            formats.append(QLatin1String(fmt));
//            ++i;
//        }
//    } else {
        for (int i = 0; i < drag->xdnd_types.size(); ++i) {
            QString f = mimeAtomToString(drag->connection(), drag->xdnd_types.at(i));
            if (!formats.contains(f))
                formats.append(f);
        }
//    }
    return formats;
}

QT_END_NAMESPACE
