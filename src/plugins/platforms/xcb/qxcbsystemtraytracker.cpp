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
        return nullptr;
    const QByteArray netSysTray = QByteArrayLiteral("_NET_SYSTEM_TRAY_S") + QByteArray::number(connection->primaryScreenNumber());
    const xcb_atom_t selection = connection->internAtom(netSysTray.constData());
    if (!selection)
        return nullptr;

    return new QXcbSystemTrayTracker(connection, trayAtom, selection);
}

QXcbSystemTrayTracker::QXcbSystemTrayTracker(QXcbConnection *connection,
                                             xcb_atom_t trayAtom,
                                             xcb_atom_t selection)
    : QObject(connection)
    , m_selection(selection)
    , m_trayAtom(trayAtom)
    , m_connection(connection)
{
}

xcb_window_t QXcbSystemTrayTracker::locateTrayWindow(const QXcbConnection *connection, xcb_atom_t selection)
{
    auto reply = Q_XCB_REPLY(xcb_get_selection_owner, connection->xcb_connection(), selection);
    if (!reply)
        return 0;
    return reply->owner;
}

// Request a window to be docked on the tray.
void QXcbSystemTrayTracker::requestSystemTrayWindowDock(xcb_window_t window) const
{
    xcb_client_message_event_t trayRequest;
    trayRequest.response_type = XCB_CLIENT_MESSAGE;
    trayRequest.format = 32;
    trayRequest.sequence = 0;
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
            xcb_change_window_attributes(m_connection->xcb_connection(), m_trayWindow, mask, &value);
        }
    }
    return m_trayWindow;
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

xcb_visualid_t QXcbSystemTrayTracker::visualId()
{
    xcb_visualid_t visual = netSystemTrayVisual();
    if (visual == XCB_NONE)
        visual = m_connection->primaryScreen()->screen()->root_visual;
    return visual;
}

xcb_visualid_t QXcbSystemTrayTracker::netSystemTrayVisual()
{
    if (m_trayWindow == XCB_WINDOW_NONE)
        return XCB_NONE;

    xcb_atom_t tray_atom = m_connection->atom(QXcbAtom::_NET_SYSTEM_TRAY_VISUAL);

    // Get the xcb property for the _NET_SYSTEM_TRAY_VISUAL atom
    auto systray_atom_reply = Q_XCB_REPLY_UNCHECKED(xcb_get_property, m_connection->xcb_connection(),
                                                    false, m_trayWindow,
                                                    tray_atom, XCB_ATOM_VISUALID, 0, 1);
    if (!systray_atom_reply)
        return XCB_NONE;

    xcb_visualid_t systrayVisualId = XCB_NONE;
    if (systray_atom_reply->value_len > 0 && xcb_get_property_value_length(systray_atom_reply.get()) > 0) {
        xcb_visualid_t * vids = (uint32_t *)xcb_get_property_value(systray_atom_reply.get());
        systrayVisualId = vids[0];
    }

    return systrayVisualId;
}

QT_END_NAMESPACE
