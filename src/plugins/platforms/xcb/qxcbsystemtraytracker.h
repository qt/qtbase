/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QXCBSYSTEMTRAYTRACKER_H
#define QXCBSYSTEMTRAYTRACKER_H

#include "qxcbconnection.h"

#include <xcb/xcb.h>

QT_BEGIN_NAMESPACE

class QXcbConnection;
class QScreen;

class QXcbSystemTrayTracker : public QObject, public QXcbWindowEventListener
{
    Q_OBJECT
public:
    static QXcbSystemTrayTracker *create(QXcbConnection *connection);

    xcb_window_t trayWindow();
    void requestSystemTrayWindowDock(xcb_window_t window) const;

    void notifyManagerClientMessageEvent(const xcb_client_message_event_t *);

    void handleDestroyNotifyEvent(const xcb_destroy_notify_event_t *) override;

    xcb_visualid_t visualId();
signals:
    void systemTrayWindowChanged(QScreen *screen);

private:
    explicit QXcbSystemTrayTracker(QXcbConnection *connection,
                                   xcb_atom_t trayAtom,
                                   xcb_atom_t selection);
    static xcb_window_t locateTrayWindow(const QXcbConnection *connection, xcb_atom_t selection);
    void emitSystemTrayWindowChanged();
    xcb_visualid_t netSystemTrayVisual();

    const xcb_atom_t m_selection;
    const xcb_atom_t m_trayAtom;
    QXcbConnection *m_connection;
    xcb_window_t m_trayWindow = 0;
};

QT_END_NAMESPACE

#endif // QXCBSYSTEMTRAYTRACKER_H
