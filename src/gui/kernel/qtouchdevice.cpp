/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qtouchdevice.h"
#include "qtouchdevice_p.h"
#include <QList>
#include <QMutex>
#include <QCoreApplication>

#include <private/qdebug_p.h>
#include <private/qlocking_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QTouchDevice
    \brief The QTouchDevice class describes the device from which touch events originate.
    \since 5.0
    \ingroup touch
    \inmodule QtGui

    Each QTouchEvent contains a QTouchDevice pointer to allow accessing
    device-specific properties like type and capabilities. It is the
    responsibility of the platform or generic plug-ins to register the
    available touch devices via QWindowSystemInterface before generating any
    touch events. Applications do not need to instantiate this class, they
    should just access the global instances pointed to by QTouchEvent::device().
*/

/*! \enum QTouchDevice::DeviceType

    This enum represents the type of device that generated a QTouchEvent.

    \value TouchScreen In this type of device, the touch surface and display are integrated. This
                       means the surface and display typically have the same size, such that there
                       is a direct relationship between the touch points' physical positions and the
                       coordinate reported by QTouchEvent::TouchPoint. As a result, Qt allows the
                       user to interact directly with multiple QWidgets and QGraphicsItems at the
                       same time.

    \value TouchPad In this type of device, the touch surface is separate from the display. There
                    is not a direct relationship between the physical touch location and the
                    on-screen coordinates. Instead, they are calculated relative to the current
                    mouse position, and the user must use the touch-pad to move this reference
                    point. Unlike touch-screens, Qt allows users to only interact with a single
                    QWidget or QGraphicsItem at a time.
*/

/*! \enum QTouchDevice::CapabilityFlag

    This enum is used with QTouchDevice::capabilities() to indicate what kind of information the
    touch device or its driver can provide.

    \value Position Indicates that position information is available, meaning
                    that the pos() family of functions in the touch points return valid points.

    \value Area Indicates that touch area information is available, meaning that the rect() family
                of functions in the touch points return valid rectangles.

    \value Pressure Indicates that pressure information is available, meaning that pressure()
                    returns a valid value.

    \value Velocity Indicates that velocity information is available, meaning that velocity()
                    returns a valid vector.

    \value RawPositions Indicates that the list returned by QTouchEvent::TouchPoint::rawScreenPositions()
                        may contain one or more positions for each touch point. This is relevant when
                        the touch input gets filtered or corrected on driver level.

    \value NormalizedPosition Indicates that the normalized position is available, meaning that normalizedPos()
                              returns a valid value.

    \value MouseEmulation Indicates that the device synthesizes mouse events.
                          This enum value has been introduced in Qt 5.5.
*/

/*!
  Creates a new touch device instance.
  By default the name is empty, the only capability is Position and type is TouchScreen.
  */
QTouchDevice::QTouchDevice()
    : d(new QTouchDevicePrivate)
{
}

/*!
  Destroys a touch device instance.
  */
QTouchDevice::~QTouchDevice()
{
    delete d;
}

/*!
    Returns the touch device type.
*/
QTouchDevice::DeviceType QTouchDevice::type() const
{
    return d->type;
}

/*!
    Returns the touch device capabilities.
  */
QTouchDevice::Capabilities QTouchDevice::capabilities() const
{
    return d->caps;
}

/*!
    Returns the maximum number of simultaneous touch points (fingers) that
    can be detected.
    \since 5.2
  */
int QTouchDevice::maximumTouchPoints() const
{
    return d->maxTouchPoints;
}

/*!
    Returns the touch device name.

    This string may often be empty. It is however useful for systems that have
    more than one touch input device because there it can be used to
    differentiate between the devices (i.e. to tell from which device a
    QTouchEvent originates from).
*/
QString QTouchDevice::name() const
{
    return d->name;
}

/*!
  Sets the device type \a devType.
  */
void QTouchDevice::setType(DeviceType devType)
{
    d->type = devType;
}

/*!
  Sets the capabilities \a caps supported by the device and its driver.
  */
void QTouchDevice::setCapabilities(Capabilities caps)
{
    d->caps = caps;
}

/*!
  Sets the maximum number of simultaneous touchpoints \a max
  supported by the device and its driver.
  */
void QTouchDevice::setMaximumTouchPoints(int max)
{
    d->maxTouchPoints = max;
}

/*!
  Sets the \a name (a unique identifier) for the device. In most systems it is
  enough to leave this unset and keep the default empty name. This identifier
  becomes important when having multiple touch devices and a need to
  differentiate between them.
  */
void QTouchDevice::setName(const QString &name)
{
    d->name = name;
}

static QBasicMutex devicesMutex;

struct TouchDevices {
    TouchDevices();
    QList<const QTouchDevice *> list;
};
Q_GLOBAL_STATIC(TouchDevices, deviceList)

TouchDevices::TouchDevices()
{
    qAddPostRoutine([]{
        const auto locker = qt_scoped_lock(devicesMutex);
        qDeleteAll(qExchange(deviceList->list, {}));
    });
}

/*!
  Returns a list of all registered devices.

  \note The returned list cannot be used to add new devices. To add a simulated
  touch screen for an autotest, QTest::createTouchDevice() can be used.
  To add real touch screens to QPA plugins, the private
  \c QWindowSystemInterface::registerTouchDevice() function can be used.
  */
QList<const QTouchDevice *> QTouchDevice::devices()
{
    const auto locker = qt_scoped_lock(devicesMutex);
    return deviceList->list;
}

/*!
  \internal
  */
bool QTouchDevicePrivate::isRegistered(const QTouchDevice *dev)
{
    const auto locker = qt_scoped_lock(devicesMutex);
    return deviceList->list.contains(dev);
}

const QTouchDevice *QTouchDevicePrivate::deviceById(quint8 id)
{
    const auto locker = qt_scoped_lock(devicesMutex);
    for (const QTouchDevice *dev : qAsConst(deviceList->list))
        if (QTouchDevicePrivate::get(const_cast<QTouchDevice *>(dev))->id == id)
            return dev;
    return nullptr;
}

/*!
  \internal
  */
void QTouchDevicePrivate::registerDevice(const QTouchDevice *dev)
{
    const auto locker = qt_scoped_lock(devicesMutex);
    deviceList->list.append(dev);
}

/*!
  \internal
  */
void QTouchDevicePrivate::unregisterDevice(const QTouchDevice *dev)
{
    const auto locker = qt_scoped_lock(devicesMutex);
    deviceList->list.removeOne(dev);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QTouchDevice *device)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();
    debug << "QTouchDevice(";
    if (device) {
        debug << '"' << device->name() << "\", type=";
        QtDebugUtils::formatQEnum(debug, device->type());
        debug << ", capabilities=";
        QtDebugUtils::formatQFlags(debug, device->capabilities());
        debug << ", maximumTouchPoints=" << device->maximumTouchPoints();
    } else {
        debug << '0';
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
