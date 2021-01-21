/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qsharednetworksession_p.h"
#include "qbearerengine_p.h"
#include <QThreadStorage>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

QThreadStorage<QSharedNetworkSessionManager *> tls;

inline QSharedNetworkSessionManager* sharedNetworkSessionManager()
{
    QSharedNetworkSessionManager* rv = tls.localData();
    if (!rv) {
        rv = new QSharedNetworkSessionManager;
        tls.setLocalData(rv);
    }
    return rv;
}

struct DeleteLater {
    void operator()(QObject* obj) const
    {
        obj->deleteLater();
    }
};

template <typename Container>
static void maybe_prune_expired(Container &c)
{
    if (c.size() > 16) {
        for (auto it = c.cbegin(), end = c.cend(); it != end; /*erasing*/) {
            if (!it->second.lock())
                it = c.erase(it);
            else
                ++it;
        }
    }
}

QSharedPointer<QNetworkSession> QSharedNetworkSessionManager::getSession(const QNetworkConfiguration &config)
{
    QSharedNetworkSessionManager *m = sharedNetworkSessionManager();
    maybe_prune_expired(m->sessions);
    auto &entry = m->sessions[config];
    //if already have a session, return it
    if (auto p = entry.toStrongRef())
        return p;
    //otherwise make one
    QSharedPointer<QNetworkSession> session(new QNetworkSession(config), DeleteLater{});
    entry = session;
    return session;
}

void QSharedNetworkSessionManager::setSession(const QNetworkConfiguration &config, QSharedPointer<QNetworkSession> session)
{
    QSharedNetworkSessionManager *m = sharedNetworkSessionManager();
    m->sessions[config] = std::move(session);
}

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT
