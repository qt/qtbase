// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBSCROLLINGDEVICE_P_H
#define QXCBSCROLLINGDEVICE_P_H

#include <QtGui/private/qpointingdevice_p.h>

QT_BEGIN_NAMESPACE

class QXcbScrollingDevicePrivate : public QPointingDevicePrivate
{
    Q_DECLARE_PUBLIC(QPointingDevice)
public:
    QXcbScrollingDevicePrivate(const QString &name, qint64 id, QPointingDevice::Capabilities caps,
                     int buttonCount = 3, const QString &seatName = QString());

    // scrolling-related data
    int verticalIndex = 0;
    int horizontalIndex = 0;
    double verticalIncrement = 0;
    double horizontalIncrement = 0;
    Qt::Orientations orientations;
    Qt::Orientations legacyOrientations;
    mutable QPointF lastScrollPosition;
    // end of scrolling-related data
};

class QXcbScrollingDevice : public QPointingDevice
{
    Q_OBJECT
public:
    QXcbScrollingDevice(QXcbScrollingDevicePrivate &d, QObject *parent)
        : QPointingDevice(d, parent) {}

    QXcbScrollingDevice(const QString &name, qint64 deviceId, Capabilities caps, int buttonCount,
                        const QString &seatName = QString(), QObject *parent = nullptr)
        : QPointingDevice(*new QXcbScrollingDevicePrivate(name, deviceId, caps, buttonCount, seatName), parent)
    {
    }

    inline static QXcbScrollingDevicePrivate *get(QXcbScrollingDevice *q)
    {
        return static_cast<QXcbScrollingDevicePrivate *>(QObjectPrivate::get(q));
    }

    inline static const QXcbScrollingDevicePrivate *get(const QXcbScrollingDevice *q)
    {
        return static_cast<const QXcbScrollingDevicePrivate *>(QObjectPrivate::get(q));
    }

};

QT_END_NAMESPACE

#endif // QXCBSCROLLINGDEVICE_P_H
