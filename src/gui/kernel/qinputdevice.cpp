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

#include "qinputdevice.h"
#include "qinputdevice_p.h"
#include "qpointingdevice.h"
#include "qwindowsysteminterface_p.h"
#include <QCoreApplication>
#include <QDebug>
#include <QMutex>
#include <QScreen>

QT_BEGIN_NAMESPACE

/*!
    \class QInputDevice
    \brief The QInputDevice class describes a device from which a QInputEvent originates.
    \since 6.0
    \inmodule QtGui

    Each QInputEvent contains a QInputDevice pointer to allow accessing
    device-specific properties like type, capabilities and seat. It is the
    responsibility of the platform or generic plug-ins to discover, create and
    register an instance of this class corresponding to each available input
    device, via QWindowSystemInterface::registerInputDevice(), before
    generating any input event referring to that device.

    Applications do not need to instantiate this class, but can read the
    instances pointed to by QInputEvent::device() and QInputDevice::devices().
*/

/*!
    \enum QInputDevice::Capability

    Indicates what kind of information the input device or its driver can
    provide.

    \value None
           No information about input device capabilities available.

    \value Position
           Indicates that position information is available, meaning that the
           position() family of functions in the touch points return valid points.

    \value Area
           Indicates that touch area information is available, meaning that
           QEventPoint::ellipseDiameters() in the touch points return valid
           values.

    \value Pressure
           Indicates that pressure information is available, meaning that
           QEventPoint::pressure() returns a valid value.

    \value Velocity
           Indicates that velocity information is available, meaning that
           QEventPoint::velocity() returns a valid vector.

    \value NormalizedPosition
           Indicates that the normalized position is available, meaning that
           QEventPoint::globalPosition() returns a valid value.

    \value MouseEmulation
           Indicates that the device synthesizes mouse events.

    \value Scroll
           Indicates that the device has a scroll capability.

    \value PixelScroll \since 6.2
           Indicates that the device (usually a
           \l {QInputDevice::DeviceType::TouchPad}{touchpad})
           scrolls with \l {QWheelEvent::pixelDelta()}{pixel precision}.

    \value Hover
           Indicates that the device has a hover capability.

    \value Rotation
           Indicates that \l {QEventPoint::}{rotation} information is available.

    \value XTilt
           Indicates that \l {QTabletEvent::xTilt()}{tilt} information is
           available for the X-axis.

    \value YTilt
           Indicates that \l {QTabletEvent::yTilt()}{tilt} information is
           available for the Y-axis.

    \value TangentialPressure
           Indicates that \l {QTabletEvent::tangentialPressure()}
           {tangential pressure} information is  available.

    \value ZPosition
           Indicates that position information for the \l {QTabletEvent::z()}
           {Z-axis} is available.

    \value All
*/

/*!
    Creates a new invalid input device instance as a child of \a parent.
*/
QInputDevice::QInputDevice(QObject *parent)
    : QObject(*(new QInputDevicePrivate(QString(), -1, QInputDevice::DeviceType::Unknown)), parent)
{
}

QInputDevice::~QInputDevice()
{
    QInputDevicePrivate::unregisterDevice(this);
}

/*!
    Creates a new input device instance. The given \a name is normally a
    manufacturer-assigned model name if available, or something else
    identifiable; \a id is a platform-specific number that will be unique per
    device (for example the xinput ID on X11); \a type identifies what kind of
    device. On window systems that are capable of handling input from multiple
    users or sets of input devices at the same time (such as Wayland or X11),
    \a seatName identifies the name of the set of devices that will be used
    together. If the device is a child or slave device (for example one of
    several mice that can take turns moving the "core pointer"), the master
    device should be given as the \a parent.

    The platform plugin creates, registers and continues to own each device
    instance; usually \a parent should be given for memory management purposes
    even if there is no master for a particular device.

    By default, capabilities() are \c None.
*/
QInputDevice::QInputDevice(const QString &name, qint64 id, QInputDevice::DeviceType type,
                           const QString &seatName, QObject *parent)
    : QObject(*new QInputDevicePrivate(name, id, type, QInputDevice::Capability::None, seatName), parent)
{
}

/*!
    \internal
*/
QInputDevice::QInputDevice(QInputDevicePrivate &d, QObject *parent)
    : QObject(d, parent)
{
}

/*!
    Returns the region within the \l{QScreen::availableVirtualGeometry}{virtual desktop}
    that this device can access.

    For example a \l {QInputDevice::DeviceType}{TouchScreen} input
    device is fixed in place upon a single physical screen, and usually
    calibrated so that this area is the same as QScreen::geometry(); whereas a
    \l {QInputDevice::DeviceType}{Mouse} can probably access all screens
    on the virtual desktop. A Wacom graphics tablet may be configured in a way
    that it's mapped to all screens, or only to the screen where the user
    prefers to create drawings, or to the window in which drawing occurs.
    A \l {QInputDevice::DeviceType}{Stylus} device that is integrated
    with a touchscreen may be physically limited to that screen.

    If the returned rectangle is \l {QRect::isNull()}{null}, it means this device
    can access the entire virtual desktop.
*/
QRect QInputDevice::availableVirtualGeometry() const
{
    Q_D(const QInputDevice);
    return d->availableVirtualGeometry;
}

/*!
    Returns the device name.

    This string may be empty. It is however useful on systems that have
    multiple input devices: it can be used to differentiate from which device a
    QPointerEvent originates.
*/
QString QInputDevice::name() const
{
    Q_D(const QInputDevice);
    return d->name;
}

/*!
    Returns the device type.
*/
QInputDevice::DeviceType QInputDevice::type() const
{
    Q_D(const QInputDevice);
    return d->deviceType;
}

/*!
    Returns the device capabilities.
*/
QInputDevice::Capabilities QInputDevice::capabilities() const
{
    Q_D(const QInputDevice);
    return QInputDevice::Capabilities(d->capabilities);
}

/*!
    Returns whether the device capabilities include the given \a capability.
*/
bool QInputDevice::hasCapability(QInputDevice::Capability capability) const
{
    return capabilities().testFlag(capability);
}

/*!
    Returns the platform specific system ID (for example xinput ID on the X11 platform).

    All platforms are expected to provide a unique system ID for each device.
*/
qint64 QInputDevice::systemId() const
{
    Q_D(const QInputDevice);
    return d->systemId;
}

/*!
    Returns the seat with which the device is associated, if known; otherwise empty.

    Devices that are intended to be used together by one user may be configured
    to have the same seat name. That is only possible on Wayland and X11
    platforms so far.
*/
QString QInputDevice::seatName() const
{
    Q_D(const QInputDevice);
    return d->seatName;
}

using InputDevicesList = QList<const QInputDevice *>;
Q_GLOBAL_STATIC(InputDevicesList, deviceList)
static QBasicMutex devicesMutex;

/*!
    Returns a list of all registered input devices (keyboards and pointing devices).

    \note The returned list cannot be used to add new devices. To add a simulated
    touch screen for an autotest, QTest::createTouchDevice() can be used.
    Platform plugins should call QWindowSystemInterface::registerInputDevice()
    to add devices as they are discovered.
*/
QList<const QInputDevice *> QInputDevice::devices()
{
    QMutexLocker lock(&devicesMutex);
    return *deviceList();
}

/*!
    Returns the core or master keyboard on the given seat \a seatName.
*/
const QInputDevice *QInputDevice::primaryKeyboard(const QString& seatName)
{
    QMutexLocker locker(&devicesMutex);
    InputDevicesList v = *deviceList();
    locker.unlock();
    const QInputDevice *ret = nullptr;
    for (const QInputDevice *d : v) {
        if (d->type() == DeviceType::Keyboard && d->seatName() == seatName) {
            // the master keyboard's parent is not another input device
            if (!d->parent() || !qobject_cast<const QInputDevice *>(d->parent()))
                return d;
            if (!ret)
                ret = d;
        }
    }
    if (!ret) {
        qCDebug(lcQpaInputDevices) << "no keyboards registered for seat" << seatName
                                   << "The platform plugin should have provided one via "
                                      "QWindowSystemInterface::registerInputDevice(). Creating a default one for now.";
        ret = new QInputDevice(QLatin1String("core keyboard"), 0, DeviceType::Keyboard, seatName, QCoreApplication::instance());
        QInputDevicePrivate::registerDevice(ret);
        return ret;
    }
    qWarning() << "core keyboard ambiguous for seat" << seatName;
    return ret;
}

QInputDevicePrivate::~QInputDevicePrivate()
    = default;

/*!
    \internal
    Checks whether a matching device is already registered
    (via operator==, not pointer equality).
*/
bool QInputDevicePrivate::isRegistered(const QInputDevice *dev)
{
    if (!dev)
        return false;
    QMutexLocker locker(&devicesMutex);
    InputDevicesList v = *deviceList();
    for (const QInputDevice *d : v)
        if (d && *d == *dev)
            return true;
    return false;
}

/*!
    \internal
    Find the device with the given \a systemId (for example the xinput
    device ID on X11), which is expected to be unique if nonzero.

    If the \a systemId is not unique, this function returns the first one found.

    \note Use QInputDevicePrivate::queryTabletDevice() if the device is a
    tablet or a tablet stylus; in that case, \a id is not unique.
*/
const QInputDevice *QInputDevicePrivate::fromId(qint64 systemId)
{
    QMutexLocker locker(&devicesMutex);
    for (const QInputDevice *dev : *deviceList()) {
        if (dev->systemId() == systemId)
            return dev;
    }
    return nullptr;
}

void QInputDevicePrivate::registerDevice(const QInputDevice *dev)
{
    QMutexLocker lock(&devicesMutex);
    deviceList()->append(dev);
}

/*!
    \internal
*/
void QInputDevicePrivate::unregisterDevice(const QInputDevice *dev)
{
    QMutexLocker lock(&devicesMutex);
    deviceList()->removeOne(dev);
}

bool QInputDevice::operator==(const QInputDevice &other) const
{
    return systemId() == other.systemId();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QInputDevice *device)
{
    const QInputDevicePrivate *d = QInputDevicePrivate::get(device);
    if (d->pointingDeviceType)
        return operator<<(debug, static_cast<const QPointingDevice *>(device));
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();
    debug << "QInputDevice(";
    if (device) {
        debug << '"' << device->name() << "\", type=" << device->type()
              << Qt::hex << ", ID=" << device->systemId() << ", seat='" << device->seatName() << "'";
    } else {
        debug << '0';
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
