/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwaylandwindowmanagerintegration.h"
#include "qwaylandwindowmanager-client-protocol.h"

#include <stdint.h>

QWaylandWindowManagerIntegration *QWaylandWindowManagerIntegration::createIntegration(QWaylandDisplay *waylandDisplay)
{
    return new QWaylandWindowManagerIntegration(waylandDisplay);
}

QWaylandWindowManagerIntegration::QWaylandWindowManagerIntegration(QWaylandDisplay *waylandDisplay)
    : mWaylandDisplay(waylandDisplay)
    , mWaylandWindowManager(0)
{
    wl_display_add_global_listener(mWaylandDisplay->wl_display(),
                                   QWaylandWindowManagerIntegration::wlHandleListenerGlobal,
                                   this);
}

QWaylandWindowManagerIntegration::~QWaylandWindowManagerIntegration()
{

}

struct wl_windowmanager *QWaylandWindowManagerIntegration::windowManager() const
{
    return mWaylandWindowManager;
}

void QWaylandWindowManagerIntegration::wlHandleListenerGlobal(wl_display *display, uint32_t id, const char *interface, uint32_t version, void *data)
{
    if (strcmp(interface, "wl_windowmanager") == 0) {
        QWaylandWindowManagerIntegration *integration = static_cast<QWaylandWindowManagerIntegration *>(data);
        integration->mWaylandWindowManager = wl_windowmanager_create(display, id);
    }
}

void QWaylandWindowManagerIntegration::mapClientToProcess(long long processId)
{
    if (mWaylandWindowManager)
        wl_windowmanager_map_client_to_process(mWaylandWindowManager, (uint32_t) processId);
}

void QWaylandWindowManagerIntegration::authenticateWithToken(const QByteArray &token)
{
    QByteArray authToken = token;
    if (authToken.isEmpty())
        authToken = qgetenv("WL_AUTHENTICATION_TOKEN");
    if (mWaylandWindowManager)
        wl_windowmanager_authenticate_with_token(mWaylandWindowManager, authToken.constData());
}
