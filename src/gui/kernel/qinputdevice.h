/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QINPUTDEVICE_H
#define QINPUTDEVICE_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>
#include <QtGui/qscreen.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QInputDevicePrivate;

class Q_GUI_EXPORT QInputDevice : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QInputDevice)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(DeviceType type READ type CONSTANT)
    Q_PROPERTY(Capabilities capabilities READ capabilities CONSTANT)
    Q_PROPERTY(qint64 systemId READ systemId CONSTANT)
    Q_PROPERTY(QString seatName READ seatName CONSTANT)
    Q_PROPERTY(QRect availableVirtualGeometry READ availableVirtualGeometry NOTIFY availableVirtualGeometryChanged)

public:
    enum class DeviceType {
        Unknown = 0x0000,
        Mouse = 0x0001,
        TouchScreen = 0x0002,
        TouchPad = 0x0004,
        Puck = 0x0008,
        Stylus = 0x0010,
        Airbrush = 0x0020,
        Keyboard = 0x1000,
        AllDevices = 0x7FFFFFFF
    };
    Q_DECLARE_FLAGS(DeviceTypes, DeviceType)
    Q_FLAG(DeviceTypes)

    enum class Capability {
        None = 0,
        Position = 0x0001,
        Area = 0x0002,
        Pressure = 0x0004,
        Velocity = 0x0008,
        NormalizedPosition = 0x0020,
        MouseEmulation = 0x0040,
        Scroll      = 0x0100,
        Hover       = 0x0200,
        Rotation    = 0x0400,
        XTilt       = 0x0800,
        YTilt       = 0x1000,
        TangentialPressure = 0x2000,
        ZPosition   = 0x4000,
        All = 0x7FFFFFFF
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)
    Q_FLAG(Capabilities)

    QInputDevice(QObject *parent = nullptr);
    ~QInputDevice();
    QInputDevice(const QString &name, qint64 systemId, DeviceType type,
                 const QString &seatName = QString(), QObject *parent = nullptr);

    QString name() const;
    DeviceType type() const;
    Capabilities capabilities() const;
    bool hasCapability(Capability cap) const;
    qint64 systemId() const;
    QString seatName() const;
    QRect availableVirtualGeometry() const;

    static QList<const QInputDevice *> devices();
    static const QInputDevice *primaryKeyboard(const QString& seatName = QString());

    bool operator==(const QInputDevice &other) const;

Q_SIGNALS:
    void availableVirtualGeometryChanged(QRect area);

protected:
    QInputDevice(QInputDevicePrivate &d, QObject *parent);

    Q_DISABLE_COPY_MOVE(QInputDevice)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QInputDevice::DeviceTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QInputDevice::Capabilities)

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QInputDevice *);
#endif

QT_END_NAMESPACE

#endif // QINPUTDEVICE_H
