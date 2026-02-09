// Copyright (C) 2024 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandsessionmanager_p.h"

#ifndef QT_NO_SESSIONMANAGER

#include "qwaylanddisplay_p.h"
#include "qwaylandwindow_p.h"

#include <private/qsessionmanager_p.h>
#include <private/qguiapplication_p.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandSessionManager::QWaylandSessionManager(QWaylandDisplay *display, const QString &id)
    : QObject(nullptr)
    , QPlatformSessionManager(id, QString())
    , mDisplay(display)
{
    if (!display->xxSessionManager())
        return;

    // The protocol also exposes a way of supporting crash handling to expose later
    startSession();
}

QWaylandSession *QWaylandSessionManager::session() const
{
    return mSession.data();
}

QWaylandSessionManager *QWaylandSessionManager::instance()
{
    auto *qGuiAppPriv = QGuiApplicationPrivate::instance();
    auto *managerPrivate = static_cast<QSessionManagerPrivate*>(QObjectPrivate::get(qGuiAppPriv->session_manager));
    return static_cast<QWaylandSessionManager *>(managerPrivate->platformSessionManager);
}

void QWaylandSessionManager::setSessionId(const QString &id)
{
    m_sessionId = id;
}

void QWaylandSessionManager::startSession()
{
    QtWayland::xx_session_manager_v1::reason restoreReason = QtWayland::xx_session_manager_v1::reason_launch;
    if (!sessionId().isEmpty()) {
        restoreReason = QtWayland::xx_session_manager_v1::reason_session_restore;
    }
    mSession.reset(new QWaylandSession(this));
    mSession->init(mDisplay->xxSessionManager()->get_session(restoreReason, sessionId()));
    mDisplay->forceRoundTrip();
}

QWaylandSession::QWaylandSession(QWaylandSessionManager *sessionManager)
    : mSessionManager(sessionManager)
{
}

QWaylandSession::~QWaylandSession() {
    // There's also remove which is another dtor
    // depending on whether we're meant to clean up server side or not
    // we might need to expose that later
    destroy();
}

void QWaylandSession::xx_session_v1_created(const QString &id) {
    qCDebug(lcQpaWayland) << "Session created" << id;
    mSessionManager->setSessionId(id);
}

void QWaylandSession::xx_session_v1_restored() {
    qCDebug(lcQpaWayland) << "Session restored";
    // session Id won't have change, do nothing
}

void QWaylandSession::xx_session_v1_replaced() {
    qCDebug(lcQpaWayland) << "Session replaced";
    mSessionManager->setSessionId(QString());
}

}

QT_END_NAMESPACE

#endif
