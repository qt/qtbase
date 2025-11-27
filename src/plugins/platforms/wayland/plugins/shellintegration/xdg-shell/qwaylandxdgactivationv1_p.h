// Copyright (C) 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDXDGACTIVATIONV1_P_H
#define QWAYLANDXDGACTIVATIONV1_P_H

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

#include <QObject>
#include "qwayland-xdg-activation-v1.h"

#include <QtWaylandClient/qtwaylandclientglobal.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandSurface;

class Q_WAYLANDCLIENT_EXPORT QWaylandXdgActivationTokenV1
    : public QObject,
      public QtWayland::xdg_activation_token_v1
{
    Q_OBJECT
public:
    ~QWaylandXdgActivationTokenV1() override;
    void xdg_activation_token_v1_done(const QString &token) override { Q_EMIT done(token); }

Q_SIGNALS:
    void done(const QString &token);
};

class Q_WAYLANDCLIENT_EXPORT QWaylandXdgActivationV1 : public QtWayland::xdg_activation_v1
{
public:
    QWaylandXdgActivationV1(struct ::wl_registry *registry, uint32_t id, uint32_t availableVersion);
    ~QWaylandXdgActivationV1() override;

    QWaylandXdgActivationTokenV1 *requestXdgActivationToken(QWaylandDisplay *display,
                                                            struct ::wl_surface *surface,
                                                            std::optional<uint32_t> serial,
                                                            const QString &app_id);
};

} // namespace QtWaylandClient

QT_END_NAMESPACE
#endif // QWAYLANDXDGACTIVATIONV1_P_H
