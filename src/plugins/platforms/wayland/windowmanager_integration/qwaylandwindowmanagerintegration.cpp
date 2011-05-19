/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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

void QWaylandWindowManagerIntegration::wlHandleListenerGlobal(wl_display *display, uint32_t id, const char *interface,
                                                              uint32_t version, void *data)
{
    if (strcmp(interface, "wl_windowmanager") == 0) {
        QWaylandWindowManagerIntegration *integration = static_cast<QWaylandWindowManagerIntegration *>(data);
        integration->mWaylandWindowManager = wl_windowmanager_create(display,id, version);
    }
}

void QWaylandWindowManagerIntegration::mapClientToProcess(long long processId)
{
    wl_windowmanager_map_client_to_process(mWaylandWindowManager, (uint32_t) processId);
}

