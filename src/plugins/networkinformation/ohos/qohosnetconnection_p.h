// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSNETCONNECTION_P_H
#define QOHOSNETCONNECTION_P_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <optional>
#include <qohosenums.h>

QT_BEGIN_NAMESPACE

namespace QtOhosNetConnection {

using NetBearType = QtOhos::enums::ohos::net::connection::NetBearType;
using NetCap = QtOhos::enums::ohos::net::connection::NetCap;

enum class NetworkReachability
{
    Disconnected,
    Local,
    Site,
    Online,
};

struct NetState
{
    NetworkReachability reachability = NetworkReachability::Disconnected;
    bool behindCaptivePortal = false;
    bool metered = false;
    std::optional<NetBearType> transport;
};

QOhosSupplier<NetState> makeOhosNetStateDataSource(QOhosConsumer<NetState> stateChangeConsumer);

}

QT_END_NAMESPACE

#endif // QOHOSNETCONNECTION_P_H
