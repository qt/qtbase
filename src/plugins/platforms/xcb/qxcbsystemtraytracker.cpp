/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qxcbsystemtraytracker.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"

#include <QtCore/QDebug>
#include <QtCore/QRect>
#include <QtGui/QScreen>

#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

enum {
    SystemTrayRequestDock = 0,
    SystemTrayBeginMessage = 1,
    SystemTrayCancelMessage = 2
};

// QXcbSystemTrayTracker provides API for accessing the tray window and tracks
// its lifecyle by listening for its destruction and recreation.
// See http://standards.freedesktop.org/systemtray-spec/systemtray-spec-latest.html

QXcbSystemTrayTracker *QXcbSystemTrayTracker::create(QXcbConnection *connection)
{
    // Selection, tray atoms for GNOME, NET WM Specification
    const xcb_atom_t trayAtom = connection->atom(QXcbAtom::_NET_SYSTEM_TRAY_OPCODE);
    if (!trayAtom)
        return 0;
    const QByteArray netSysTray = QByteArrayLiteral("_NET_SYSTEM_TRAY_S") + QByteArray::number(connection->primaryScreenNumber());
    const xcb_atom_t selection = connection->internAtom(netSysTray.constData());
    if (!selection)
        return 0;

    return new QXcbSystemTrayTracker(connection, trayAtom, selection);
}

QXcbSystemTrayTracker::QXcbSystemTrayTracker(QXcbConnection *connection,
                                             xcb_atom_t trayAtom,
                                             xcb_atom_t selection)
    : QObject(connection)
    , m_selection(selection)
    , m_trayAtom(trayAtom)
    , m_connection(connection)
    , m_trayWindow(0)
{
}

xcb_window_t QXcbSystemTrayTracker::locateTrayWindow(const QXcbConnection *connection, xcb_atom_t selection)
{
    xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(connection->xcb_connection(), selection);
    xcb_get_selection_owner_reply_t *reply = xcb_get_selection_owner_reply(connection->xcb_connection(), cookie, 0);
    if (!reply)
        return 0;
    const xcb_window_t result = reply->owner;
    free(reply);
    return result;
}

// API for QPlatformNativeInterface/QPlatformSystemTrayIcon: Request a window
// to be docked on the tray.
void QXcbSystemTrayTracker::requestSystemTrayWindowDock(xcb_window_t window) const
{
    xcb_client_message_event_t trayRequest;
    memset(&trayRequest, 0, sizeof(trayRequest));
    trayRequest.response_type = XCB_CLIENT_MESSAGE;
    trayRequest.format = 32;
    trayRequest.window = m_trayWindow;
    trayRequest.type = m_trayAtom;
    trayRequest.data.data32[0] = XCB_CURRENT_TIME;
    trayRequest.data.data32[1] = SystemTrayRequestDock;
    trayRequest.data.data32[2] = window;
    xcb_send_event(m_connection->xcb_connection(), 0, m_trayWindow, XCB_EVENT_MASK_NO_EVENT, (const char *)&trayRequest);
}

// API for QPlatformNativeInterface/QPlatformSystemTrayIcon: Return tray window.
xcb_window_t QXcbSystemTrayTracker::trayWindow()
{
    if (!m_trayWindow) {
        m_trayWindow = QXcbSystemTrayTracker::locateTrayWindow(m_connection, m_selection);
        if (m_trayWindow) { // Listen for DestroyNotify on tray.
            m_connection->addWindowEventListener(m_trayWindow, this);
            const quint32 mask = XCB_CW_EVENT_MASK;
            const quint32 value = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            Q_XCB_CALL2(xcb_change_window_attributes(m_connection->xcb_connection(), m_trayWindow, mask, &value), m_connection);
        }
    }
    return m_trayWindow;
}

// API for QPlatformNativeInterface/QPlatformSystemTrayIcon: Return the geometry of a
// a window parented on the tray. Determines the global geometry via XCB since mapToGlobal
// does not work for the QWindow parented on the tray.
QRect QXcbSystemTrayTracker::systemTrayWindowGlobalGeometry(xcb_window_t window) const
{

    xcb_connection_t *conn = m_connection->xcb_connection();
    xcb_get_geometry_reply_t *geomReply =
        xcb_get_geometry_reply(conn, xcb_get_geometry(conn, window), 0);
    if (!geomReply)
        return QRect();

    xcb_translate_coordinates_reply_t *translateReply =
        xcb_translate_coordinates_reply(conn, xcb_translate_coordinates(conn, window, m_connection->rootWindow(), 0, 0), 0);
    if (!translateReply) {
        free(geomReply);
        return QRect();
    }

    const QRect result(QPoint(translateReply->dst_x, translateReply->dst_y), QSize(geomReply->width, geomReply->height));
    free(translateReply);
    return result;
}

inline void QXcbSystemTrayTracker::emitSystemTrayWindowChanged()
{
    if (const QPlatformScreen *ps = m_connection->primaryScreen())
        emit systemTrayWindowChanged(ps->screen());
}

// Client messages with the "MANAGER" atom on the root window indicate creation of a new tray.
void QXcbSystemTrayTracker::notifyManagerClientMessageEvent(const xcb_client_message_event_t *t)
{
    if (t->data.data32[1] == m_selection)
        emitSystemTrayWindowChanged();
}

// Listen for destruction of the tray.
void QXcbSystemTrayTracker::handleDestroyNotifyEvent(const xcb_destroy_notify_event_t *event)
{
    if (event->window == m_trayWindow) {
        m_connection->removeWindowEventListener(m_trayWindow);
        m_trayWindow = XCB_WINDOW_NONE;
        emitSystemTrayWindowChanged();
    }
}

bool QXcbSystemTrayTracker::visualHasAlphaChannel()
{
    if (m_trayWindow == XCB_WINDOW_NONE)
        return false;

    xcb_atom_t tray_atom = m_connection->atom(QXcbAtom::_NET_SYSTEM_TRAY_VISUAL);

    // Get the xcb property for the _NET_SYSTEM_TRAY_VISUAL atom
    xcb_get_property_cookie_t systray_atom_cookie;
    xcb_get_property_reply_t *systray_atom_reply;

    systray_atom_cookie = xcb_get_property_unchecked(m_connection->xcb_connection(), false, m_trayWindow,
                                                    tray_atom, XCB_ATOM_VISUALID, 0, 1);
    systray_atom_reply = xcb_get_property_reply(m_connection->xcb_connection(), systray_atom_cookie, 0);

    if (!systray_atom_reply)
        return false;

    xcb_visualid_t systrayVisualId = XCB_NONE;
    if (systray_atom_reply->value_len > 0 && xcb_get_property_value_length(systray_atom_reply) > 0) {
        xcb_visualid_t * vids = (uint32_t *)xcb_get_property_value(systray_atom_reply);
        systrayVisualId = vids[0];
    }

    free(systray_atom_reply);

    if (systrayVisualId != XCB_NONE) {
        quint8 depth = m_connection->primaryScreen()->depthOfVisual(systrayVisualId);
        return depth == 32;
    }

    return false;
}

QT_END_NAMESPACE
