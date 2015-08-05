/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platformservices.h"

#include <QUrl>

#include <ubuntu/application/url_dispatcher/service.h>
#include <ubuntu/application/url_dispatcher/session.h>

bool UbuntuPlatformServices::openUrl(const QUrl &url)
{
    return callDispatcher(url);
}

bool UbuntuPlatformServices::openDocument(const QUrl &url)
{
    return callDispatcher(url);
}

bool UbuntuPlatformServices::callDispatcher(const QUrl &url)
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
