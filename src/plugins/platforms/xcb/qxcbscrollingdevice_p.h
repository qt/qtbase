/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
