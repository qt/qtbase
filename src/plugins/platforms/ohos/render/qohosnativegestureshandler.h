// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSNATIVEGESTURESHANDLER_H
#define QOHOSNATIVEGESTURESHANDLER_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qpoint.h>
#include <QtGui/qinputdevice.h>
#include <QtGui/qwindow.h>
#include <chrono>
#include <cstdint>
#include <functional>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

struct QOhosNativeGestureEvent
{
    std::chrono::steady_clock::time_point timestamp;
    std::int64_t gestureTimestamp;
    qreal value;
    QPointF localPosition;
    QPointF globalPosition;
    Qt::NativeGestureType gestureType;
    QInputDevice::DeviceType deviceType;
};

QOhosConsumer<const QOhosNativeGestureEvent &> makeQOhosNativeGesturesHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    std::function<void(QOhosNativeGestureEvent &)> optQtThreadEventTransformer = nullptr);

QT_END_NAMESPACE

#endif
