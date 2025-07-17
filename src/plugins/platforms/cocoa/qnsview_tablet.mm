// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// This file is included from qnsview.mm, and only used to organize the code

#ifndef QT_NO_TABLETEVENT

#include <QtGui/qpointingdevice.h>
#include <QtCore/private/qflatmap_p.h>

using QCocoaTabletDeviceMap = QFlatMap<qint64, const QPointingDevice*>;
Q_GLOBAL_STATIC(QCocoaTabletDeviceMap, devicesInProximity)

@implementation QNSView (Tablet)

- (bool)handleTabletEvent:(NSEvent *)theEvent
{
    static bool ignoreButtonMapping = qEnvironmentVariableIsSet("QT_MAC_TABLET_IGNORE_BUTTON_MAPPING");

    if (!m_platformWindow)
        return false;

    NSEventType eventType = [theEvent type];
    if (eventType != NSEventTypeTabletPoint && [theEvent subtype] != NSEventSubtypeTabletPoint)
        return false; // Not a tablet event.

    ulong timestamp = [theEvent timestamp] * 1000;

    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    nativeDrag->setLastInputEvent(theEvent, self);

    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint: &windowPoint andScreenPoint: &screenPoint];

    // We use devicesInProximity because deviceID is typically 0,
    // so QInputDevicePrivate::fromId() won't work.
    const auto deviceId = theEvent.deviceID;
    const auto *device = devicesInProximity->value(deviceId);
    if (!device && deviceId == 0) {
        // Application started up with stylus in proximity already, so we missed the proximity event?
        // Create a generic tablet device for now.
        device = tabletToolInstance(theEvent);
        devicesInProximity->insert(deviceId, device);
    }

    if (Q_UNLIKELY(!device))
        return false;

    bool down = (eventType != NSEventTypeMouseMoved);

    qreal pressure;
    if (down) {
        pressure = [theEvent pressure];
    } else {
        pressure = 0.0;
    }

    NSPoint tilt = [theEvent tilt];
    int xTilt = qRound(tilt.x * 60.0);
    int yTilt = qRound(tilt.y * -60.0);
    qreal tangentialPressure = 0;
    qreal rotation = 0;
    int z = 0;
    if (device->hasCapability(QInputDevice::Capability::ZPosition))
        z = [theEvent absoluteZ];

    if (device->hasCapability(QInputDevice::Capability::TangentialPressure))
        tangentialPressure = ([theEvent tangentialPressure] * 2.0) - 1.0;

    rotation = 360.0 - [theEvent rotation];
    if (rotation > 180.0)
        rotation -= 360.0;

    Qt::KeyboardModifiers keyboardModifiers = QAppleKeyMapper::fromCocoaModifiers(theEvent.modifierFlags);
    Qt::MouseButtons buttons = ignoreButtonMapping ? static_cast<Qt::MouseButtons>(static_cast<uint>([theEvent buttonMask])) : m_buttons;

    QWindowSystemInterface::handleTabletEvent(m_platformWindow->window(), timestamp, device, windowPoint, screenPoint,
                                              buttons, pressure, xTilt, yTilt, tangentialPressure, rotation, z,  keyboardModifiers);
    return true;
}

- (void)tabletPoint:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return [super tabletPoint:theEvent];

    [self handleTabletEvent: theEvent];
}

/*!
    \internal
    Find the existing QPointingDevice instance representing a particular tablet or stylus;
    or create and register a new instance if it was not found.

    An instance can be uniquely identified by various properties taken from \a theEvent.
*/
static const QPointingDevice *tabletToolInstance(NSEvent *theEvent)
{
    qint64 uid = theEvent.uniqueID;
    uint bits = theEvent.vendorPointingDeviceType;
    if (bits == 0 && uid != 0) {
        // Fallback. It seems that the driver doesn't always include all the information.
        // High-End Wacom devices store their "type" in the uper bits of the Unique ID.
        // I'm not sure how to handle it for consumer devices, but I'll test that in a bit.
        bits = uid >> 32;
    }

    // Defined in the "EN0056-NxtGenImpGuideX"
    // on Wacom's Developer Website (www.wacomeng.com)
    static const quint32 CursorTypeBitMask = 0x0F06;
    quint32 toolId = bits & CursorTypeBitMask;
    QInputDevice::Capabilities caps = QInputDevice::Capability::Position |
            QInputDevice::Capability::Pressure | QInputDevice::Capability::Hover;
    QInputDevice::DeviceType device;
    int buttonCount = 3; // the tip, plus two barrel buttons
    if (((bits & 0x0006) == 0x0002) && (toolId != 0x0902)) {
        device = QInputDevice::DeviceType::Stylus;
    } else {
        switch (toolId) {
        // TODO same cases as in qxcbconnection_xi2.cpp? then we could share this function
        case 0x0802:
            device = QInputDevice::DeviceType::Stylus;
            break;
        case 0x0902:
            device = QInputDevice::DeviceType::Airbrush;
            caps.setFlag(QInputDevice::Capability::TangentialPressure);
            buttonCount = 2;
            break;
        case 0x0004:
            device = QInputDevice::DeviceType::Mouse;
            caps.setFlag(QInputDevice::Capability::Scroll);
            break;
        case 0x0006:
            device = QInputDevice::DeviceType::Puck;
            break;
        case 0x0804:
            device = QInputDevice::DeviceType::Stylus; // Art Pen
            caps.setFlag(QInputDevice::Capability::Rotation);
            buttonCount = 1;
            break;
        default:
            device = QInputDevice::DeviceType::Unknown;
        }
    }

    uint capabilityMask = theEvent.capabilityMask;
    if (capabilityMask & 0x0200)
        caps.setFlag(QInputDevice::Capability::ZPosition);
    if (capabilityMask & 0x0800)
        Q_ASSERT(caps.testFlag(QInputDevice::Capability::TangentialPressure));

    QPointingDevice::PointerType pointerType = QPointingDevice::PointerType::Unknown;
    switch (theEvent.pointingDeviceType) {
    case NSPointingDeviceTypeUnknown:
    default:
        break;
    case NSPointingDeviceTypePen:
        pointerType = QPointingDevice::PointerType::Pen;
        break;
    case NSPointingDeviceTypeCursor:
        pointerType = QPointingDevice::PointerType::Cursor;
        break;
    case NSPointingDeviceTypeEraser:
        pointerType = QPointingDevice::PointerType::Eraser;
        break;
    }

    const auto uniqueID = QPointingDeviceUniqueId::fromNumericId(uid);
    auto windowSystemId = theEvent.deviceID;
    const QPointingDevice *ret = QPointingDevicePrivate::queryTabletDevice(device, pointerType, uniqueID, caps, windowSystemId);
    if (!ret) {
        // TODO get the device name? (first argument)
        // TODO associate each stylus with a "master" device representing the tablet itself
        ret = new QPointingDevice(QString(), windowSystemId, device, pointerType,
                                  caps, 1, buttonCount, QString(), uniqueID, QCocoaIntegration::instance());
        QWindowSystemInterface::registerInputDevice(ret);
    }
    QPointingDevicePrivate *devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(ret));
    devPriv->toolId = toolId;
    return ret;
}

- (void)tabletProximity:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return [super tabletProximity:theEvent];

    const ulong timestamp = theEvent.timestamp * 1000;
    const qint64 windowSystemId = theEvent.deviceID;
    const QPointingDevice *device = tabletToolInstance(theEvent);
    // TODO which window?
    QWindowSystemInterface::handleTabletEnterLeaveProximityEvent(nullptr, timestamp, device, theEvent.isEnteringProximity);
    // The windowSystemId starts at 0, but is "unique" while in proximity
    if (theEvent.isEnteringProximity)
        devicesInProximity->insert_or_assign(windowSystemId, device);
    else
        devicesInProximity->remove(windowSystemId);
}
@end

#endif
