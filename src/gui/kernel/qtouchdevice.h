/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
****************************************************************************/

#ifndef QTOUCHDEVICE_H
#define QTOUCHDEVICE_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QTouchDevicePrivate;

class Q_GUI_EXPORT QTouchDevice
{
    Q_GADGET
public:
    enum DeviceType {
        TouchScreen,
        TouchPad
    };
    Q_ENUM(DeviceType)

    enum CapabilityFlag {
        Position = 0x0001,
        Area = 0x0002,
        Pressure = 0x0004,
        Velocity = 0x0008,
        RawPositions = 0x0010,
        NormalizedPosition = 0x0020,
        MouseEmulation = 0x0040
    };
    Q_FLAG(CapabilityFlag)
    Q_DECLARE_FLAGS(Capabilities, CapabilityFlag)

    QTouchDevice();
    ~QTouchDevice();

    static QList<const QTouchDevice *> devices();

    QString name() const;
    DeviceType type() const;
    Capabilities capabilities() const;
    int maximumTouchPoints() const;

    void setName(const QString &name);
    void setType(DeviceType devType);
    void setCapabilities(Capabilities caps);
    void setMaximumTouchPoints(int max);

private:
    QTouchDevicePrivate *d;
    friend class QTouchDevicePrivate;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QTouchDevice::Capabilities)

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QTouchDevice *);
#endif

QT_END_NAMESPACE

#endif // QTOUCHDEVICE_H
