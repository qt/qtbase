// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSNETWORKINFORMATIONENUMS_H
#define QOHOSNETWORKINFORMATIONENUMS_H

#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>
#include <array>
#include <info/application_target_sdk_version.h>

QT_BEGIN_NAMESPACE

namespace QtOhosNetworkInformation {

namespace enums {

namespace ohos {

namespace net {

namespace connection {

enum class NetBearType {
    BEARER_BLUETOOTH,
    BEARER_CELLULAR,
    BEARER_ETHERNET,
    BEARER_VPN,
    BEARER_WIFI,
};

enum class NetCap {
    NET_CAPABILITY_CHECKING_CONNECTIVITY,
    NET_CAPABILITY_INTERNET,
    NET_CAPABILITY_MMS,
    NET_CAPABILITY_NOT_METERED,
    NET_CAPABILITY_NOT_VPN,
    NET_CAPABILITY_PORTAL,
    NET_CAPABILITY_VALIDATED,
};

}

}

}

}

}

namespace QtOhos {

template<typename Enum>
struct OhosEnumMeta;

template<>
struct OhosEnumMeta<QtOhosNetworkInformation::enums::ohos::net::connection::NetBearType>
{
    using Enum = QtOhosNetworkInformation::enums::ohos::net::connection::NetBearType;
    static constexpr const char *fullTypeName = "@ohos.net.connection.NetBearType";
    static constexpr std::array<std::pair<Enum, const char *>, 5> enumeratorsNames = {{
        {Enum::BEARER_BLUETOOTH, "BEARER_BLUETOOTH"},
        {Enum::BEARER_CELLULAR, "BEARER_CELLULAR"},
        {Enum::BEARER_ETHERNET, "BEARER_ETHERNET"},
        {Enum::BEARER_VPN, "BEARER_VPN"},
        {Enum::BEARER_WIFI, "BEARER_WIFI"},
    }};
};

template<>
struct OhosEnumMeta<QtOhosNetworkInformation::enums::ohos::net::connection::NetCap>
{
    using Enum = QtOhosNetworkInformation::enums::ohos::net::connection::NetCap;
    static constexpr const char *fullTypeName = "@ohos.net.connection.NetCap";
    static constexpr std::array<std::pair<Enum, const char *>, 7> enumeratorsNames = {{
        {Enum::NET_CAPABILITY_CHECKING_CONNECTIVITY, "NET_CAPABILITY_CHECKING_CONNECTIVITY"},
        {Enum::NET_CAPABILITY_INTERNET, "NET_CAPABILITY_INTERNET"},
        {Enum::NET_CAPABILITY_MMS, "NET_CAPABILITY_MMS"},
        {Enum::NET_CAPABILITY_NOT_METERED, "NET_CAPABILITY_NOT_METERED"},
        {Enum::NET_CAPABILITY_NOT_VPN, "NET_CAPABILITY_NOT_VPN"},
        {Enum::NET_CAPABILITY_PORTAL, "NET_CAPABILITY_PORTAL"},
        {Enum::NET_CAPABILITY_VALIDATED, "NET_CAPABILITY_VALIDATED"},
    }};
};

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosNetworkInformation::enums::ohos::net::connection::NetBearType));
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhosNetworkInformation::enums::ohos::net::connection::NetCap));

#endif
