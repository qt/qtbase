/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

// This file is included from qnsview.mm, and only used to organize the code

#ifndef QT_NO_TABLETEVENT

Q_LOGGING_CATEGORY(lcQpaTablet, "qt.qpa.input.tablet")

struct QCocoaTabletDeviceData
{
    QTabletEvent::TabletDevice device;
    QTabletEvent::PointerType pointerType;
    uint capabilityMask;
    qint64 uid;
};

typedef QHash<uint, QCocoaTabletDeviceData> QCocoaTabletDeviceDataHash;
Q_GLOBAL_STATIC(QCocoaTabletDeviceDataHash, tabletDeviceDataHash)

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

    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:theEvent] toWindowPoint: &windowPoint andScreenPoint: &screenPoint];

    uint deviceId = [theEvent deviceID];
    if (!tabletDeviceDataHash->contains(deviceId)) {
        // Error: Unknown tablet device. Qt also gets into this state
        // when running on a VM. This appears to be harmless; don't
        // print a warning.
        return false;
    }
    const QCocoaTabletDeviceData &deviceData = tabletDeviceDataHash->value(deviceId);

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
    if (deviceData.capabilityMask & 0x0200)
        z = [theEvent absoluteZ];

    if (deviceData.capabilityMask & 0x0800)
        tangentialPressure = ([theEvent tangentialPressure] * 2.0) - 1.0;

    rotation = 360.0 - [theEvent rotation];
    if (rotation > 180.0)
        rotation -= 360.0;

    Qt::KeyboardModifiers keyboardModifiers = [QNSView convertKeyModifiers:[theEvent modifierFlags]];
    Qt::MouseButtons buttons = ignoreButtonMapping ? static_cast<Qt::MouseButtons>(static_cast<uint>([theEvent buttonMask])) : m_buttons;

    qCDebug(lcQpaTablet, "event on tablet %d with tool %d type %d unique ID %lld pos %6.1f, %6.1f root pos %6.1f, %6.1f buttons 0x%x pressure %4.2lf tilt %d, %d rotation %6.2lf",
        deviceId, deviceData.device, deviceData.pointerType, deviceData.uid,
        windowPoint.x(), windowPoint.y(), screenPoint.x(), screenPoint.y(),
        static_cast<uint>(buttons), pressure, xTilt, yTilt, rotation);

    QWindowSystemInterface::handleTabletEvent(m_platformWindow->window(), timestamp, windowPoint, screenPoint,
                                              deviceData.device, deviceData.pointerType, buttons, pressure, xTilt, yTilt,
                                              tangentialPressure, rotation, z, deviceData.uid,
                                              keyboardModifiers);
    return true;
}

- (void)tabletPoint:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return [super tabletPoint:theEvent];

    [self handleTabletEvent: theEvent];
}

static QTabletEvent::TabletDevice wacomTabletDevice(NSEvent *theEvent)
{
    qint64 uid = [theEvent uniqueID];
    uint bits = [theEvent vendorPointingDeviceType];
    if (bits == 0 && uid != 0) {
        // Fallback. It seems that the driver doesn't always include all the information.
        // High-End Wacom devices store their "type" in the uper bits of the Unique ID.
        // I'm not sure how to handle it for consumer devices, but I'll test that in a bit.
        bits = uid >> 32;
    }

    QTabletEvent::TabletDevice device;
    // Defined in the "EN0056-NxtGenImpGuideX"
    // on Wacom's Developer Website (www.wacomeng.com)
    if (((bits & 0x0006) == 0x0002) && ((bits & 0x0F06) != 0x0902)) {
        device = QTabletEvent::Stylus;
    } else {
        switch (bits & 0x0F06) {
            case 0x0802:
                device = QTabletEvent::Stylus;
                break;
            case 0x0902:
                device = QTabletEvent::Airbrush;
                break;
            case 0x0004:
                device = QTabletEvent::FourDMouse;
                break;
            case 0x0006:
                device = QTabletEvent::Puck;
                break;
            case 0x0804:
                device = QTabletEvent::RotationStylus;
                break;
            default:
                device = QTabletEvent::NoDevice;
        }
    }
    return device;
}

- (void)tabletProximity:(NSEvent *)theEvent
{
    if ([self isTransparentForUserInput])
        return [super tabletProximity:theEvent];

    ulong timestamp = [theEvent timestamp] * 1000;

    QCocoaTabletDeviceData deviceData;
    deviceData.uid = [theEvent uniqueID];
    deviceData.capabilityMask = [theEvent capabilityMask];

    switch ([theEvent pointingDeviceType]) {
        case NSPointingDeviceTypeUnknown:
        default:
            deviceData.pointerType = QTabletEvent::UnknownPointer;
            break;
        case NSPointingDeviceTypePen:
            deviceData.pointerType = QTabletEvent::Pen;
            break;
        case NSPointingDeviceTypeCursor:
            deviceData.pointerType = QTabletEvent::Cursor;
            break;
        case NSPointingDeviceTypeEraser:
            deviceData.pointerType = QTabletEvent::Eraser;
            break;
    }

    deviceData.device = wacomTabletDevice(theEvent);

    // The deviceID is "unique" while in the proximity, it's a key that we can use for
    // linking up QCocoaTabletDeviceData to an event (especially if there are two devices in action).
    bool entering = [theEvent isEnteringProximity];
    uint deviceId = [theEvent deviceID];
    if (entering) {
        tabletDeviceDataHash->insert(deviceId, deviceData);
    } else {
        tabletDeviceDataHash->remove(deviceId);
    }

    qCDebug(lcQpaTablet, "proximity change on tablet %d: current tool %d type %d unique ID %lld",
        deviceId, deviceData.device, deviceData.pointerType, deviceData.uid);

    if (entering) {
        QWindowSystemInterface::handleTabletEnterProximityEvent(timestamp, deviceData.device, deviceData.pointerType, deviceData.uid);
    } else {
        QWindowSystemInterface::handleTabletLeaveProximityEvent(timestamp, deviceData.device, deviceData.pointerType, deviceData.uid);
    }
}
@end

#endif
