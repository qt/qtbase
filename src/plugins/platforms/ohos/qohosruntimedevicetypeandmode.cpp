// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosruntimedevicetypeandmode.h>

#include <QtCore/private/qohoslogger_p.h>
#include <qohosdeviceinfo_p.h>
#include <qohossettings.h>

QT_BEGIN_NAMESPACE

QOhosRuntimeDeviceTypeAndMode queryQOhosRuntimeDeviceAndMode()
{
    if (QOhosDeviceInfo::is2in1()) {
        return QOhosRuntimeDeviceTypeAndMode::_2in1;
    } else if (isHandheldDeviceType()) {
        return QOhosSettings::instance().isWindowPcModeEnabled()
            ? QOhosRuntimeDeviceTypeAndMode::HandheldDeviceWindowPcMode
            : QOhosRuntimeDeviceTypeAndMode::HandheldDeviceFullScreen;
    } else {
        qCCritical(QtForOhos)
            << Q_FUNC_INFO << "Failed to determine valid runtimeDeviceTypeAndMode as this is unknown device type. Assuming 2in1.";
        return QOhosRuntimeDeviceTypeAndMode::_2in1;
    }
}

bool isHandheldDeviceType()
{
    return QOhosDeviceInfo::isPhone() || QOhosDeviceInfo::isTablet();
}

QT_END_NAMESPACE
