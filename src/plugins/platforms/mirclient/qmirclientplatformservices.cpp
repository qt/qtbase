/****************************************************************************
**
** Copyright (C) 2014 Canonical, Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qmirclientplatformservices.h"

#include <QUrl>

#include <ubuntu/application/url_dispatcher/service.h>
#include <ubuntu/application/url_dispatcher/session.h>

bool QMirClientPlatformServices::openUrl(const QUrl &url)
{
    return callDispatcher(url);
}

bool QMirClientPlatformServices::openDocument(const QUrl &url)
{
    return callDispatcher(url);
}

bool QMirClientPlatformServices::callDispatcher(const QUrl &url)
{
    UAUrlDispatcherSession* session = ua_url_dispatcher_session();
    if (!session)
        return false;

    ua_url_dispatcher_session_open(session, url.toEncoded().constData(), NULL, NULL);

    free(session);

    // We are returning true here because the other option
    // is spawning a nested event loop and wait for the
    // callback. But there is no guarantee on how fast
    // the callback is going to be so we prefer to avoid the
    // nested event loop. Long term plan is improve Qt API
    // to support an async openUrl
    return true;
}
