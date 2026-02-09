// Copyright (C) 2024 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWAYLANDSESSIONMANAGER_H
#define QWAYLANDSESSIONMANAGER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QT_NO_SESSIONMANAGER

#include <QtGui/qpa/qplatformsessionmanager.h>
#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtWaylandClient/private/qwayland-xx-session-management-v1.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandWindow;
class QWaylandSession;
class QWaylandSessionManager;


class Q_WAYLANDCLIENT_EXPORT QWaylandSession : public QObject, public QtWayland::xx_session_v1
{
    Q_OBJECT
public:
    QWaylandSession(QWaylandSessionManager *sessionManager);
    ~QWaylandSession();

protected:
    void xx_session_v1_created(const QString &id) override;
    void xx_session_v1_restored() override;
    void xx_session_v1_replaced() override;
private:
    QWaylandSessionManager *mSessionManager;
};

class Q_WAYLANDCLIENT_EXPORT QWaylandSessionManager : public QObject, public QPlatformSessionManager
{
    Q_OBJECT
public:
    static QWaylandSessionManager *instance();
    QWaylandSessionManager(QWaylandDisplay *display, const QString &id);

    QWaylandSession* session() const;
private:
    void setSessionId(const QString &id);
    void startSession();

    QWaylandDisplay *mDisplay = nullptr;
    QScopedPointer<QWaylandSession> mSession;
    friend class QWaylandSession;
};

}

QT_END_NAMESPACE

#endif
#endif
