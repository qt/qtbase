// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
#include "qxcbscrollingdevice_p.h"

QT_BEGIN_NAMESPACE

QXcbScrollingDevicePrivate::QXcbScrollingDevicePrivate(const QString &name, qint64 id, QInputDevice::Capabilities caps,
                                   int buttonCount, const QString &seatName)
    : QPointingDevicePrivate(name, id, QInputDevice::DeviceType::Mouse, QPointingDevice::PointerType::Generic,
                             caps, 1, buttonCount, seatName)
{
}

QT_END_NAMESPACE

#include "moc_qxcbscrollingdevice_p.cpp"
