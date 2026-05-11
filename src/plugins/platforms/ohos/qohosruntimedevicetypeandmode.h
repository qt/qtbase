// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSRUNTIMEDEVICETYPEANDMODE_H
#define QOHOSRUNTIMEDEVICETYPEANDMODE_H

#include <QtCore/qglobal.h>
#include <type_traits>

QT_BEGIN_NAMESPACE

enum class QOhosRuntimeDeviceTypeAndMode
{
    _2in1 = 1 << 0,
    HandheldDeviceFullScreen = 1 << 1,
    HandheldDeviceWindowPcMode = 1 << 2,
};

QOhosRuntimeDeviceTypeAndMode queryQOhosRuntimeDeviceAndMode();
bool isHandheldDeviceType();

constexpr QOhosRuntimeDeviceTypeAndMode operator|(
    QOhosRuntimeDeviceTypeAndMode lhs, QOhosRuntimeDeviceTypeAndMode rhs)
{
    return static_cast<QOhosRuntimeDeviceTypeAndMode>(
        std::underlying_type_t<QOhosRuntimeDeviceTypeAndMode>(lhs)
        | std::underlying_type_t<QOhosRuntimeDeviceTypeAndMode>(rhs));
}

QT_END_NAMESPACE

#endif
