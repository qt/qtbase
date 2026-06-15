// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosdeviceinfo_p.h"
#include <QtCore/private/qohoslogger_p.h>

QT_BEGIN_NAMESPACE

namespace QOhosDeviceInfo {

namespace {

bool isSupportedDeviceType(RecognizedDeviceType recognizedDeviceType)
{
    switch (recognizedDeviceType) {
    case RecognizedDeviceType::_2in1:
    case RecognizedDeviceType::tablet:
    case RecognizedDeviceType::phone:
        return true;
    }
    return false;
}

}

static QMap<Type, QVariant> s_deviceInfo = {};

void init(QMap<Type, QVariant> devinfo)
{
    s_deviceInfo = std::move(devinfo);
}

QVariant getProperty(Type prop)
{
    auto result = s_deviceInfo.value(prop);
    if (!result.isValid()) {
        qOhosWarning(QtForOhos)
                << "QOhosDeviceInfo::getProperty cannot obtain value from DeviceInfo map";
    }
    return result;
}

QOhosOptional<RecognizedDeviceType> tryGetRecognizedDeviceType()
{
    static const QMap<QString, RecognizedDeviceType> deviceTypeNameMapping = {
        {QStringLiteral("2in1"), RecognizedDeviceType::_2in1},
        {QStringLiteral("tablet"), RecognizedDeviceType::tablet},
        {QStringLiteral("phone"), RecognizedDeviceType::phone},
    };

    auto deviceTypeName = getProperty(Type::deviceType).toString();
    auto deviceTypeIt = deviceTypeNameMapping.constFind(deviceTypeName);
    auto deviceType = deviceTypeIt != deviceTypeNameMapping.constEnd()
        ? makeQOhosOptional(deviceTypeIt.value())
        : makeEmptyQOhosOptional();

    return deviceType;
}

bool isTablet()
{
    return tryGetRecognizedDeviceType() == QOhosDeviceInfo::RecognizedDeviceType::tablet;
}

bool is2in1()
{
    return tryGetRecognizedDeviceType() == QOhosDeviceInfo::RecognizedDeviceType::_2in1;
}

bool isPhone()
{
    return tryGetRecognizedDeviceType() == RecognizedDeviceType::phone;
}

int sdkApiVersion()
{
    return getProperty(Type::sdkApiVersion).toInt();
}

bool isCurrentDeviceSupported()
{
    auto deviceType = tryGetRecognizedDeviceType();
    return deviceType.has_value() && isSupportedDeviceType(deviceType.value());
}

}

QT_END_NAMESPACE
