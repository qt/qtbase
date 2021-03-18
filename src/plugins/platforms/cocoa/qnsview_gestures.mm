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

#ifndef QT_NO_GESTURES

Q_LOGGING_CATEGORY(lcQpaGestures, "qt.qpa.input.gestures")

@implementation QNSView (Gestures)

- (bool)handleGestureAsBeginEnd:(NSEvent *)event
{
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::OSXElCapitan)
        return false;

    if ([event phase] == NSEventPhaseBegan) {
        [self beginGestureWithEvent:event];
        return true;
    }

    if ([event phase] == NSEventPhaseEnded) {
        [self endGestureWithEvent:event];
        return true;
    }

    return false;
}
- (void)magnifyWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    if ([self handleGestureAsBeginEnd:event])
        return;

    qCDebug(lcQpaGestures) << "magnifyWithEvent" << [event magnification] << "from device" << Qt::hex << [event deviceID];
    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindowSystemInterface::handleGestureEventWithRealValue(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), timestamp, Qt::ZoomNativeGesture,
                                                            [event magnification], windowPoint, screenPoint);
}

- (void)smartMagnifyWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    static bool zoomIn = true;
    qCDebug(lcQpaGestures) << "smartMagnifyWithEvent" << zoomIn << "from device" << Qt::hex << [event deviceID];
    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindowSystemInterface::handleGestureEventWithRealValue(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), timestamp, Qt::SmartZoomNativeGesture,
                                                            zoomIn ? 1.0f : 0.0f, windowPoint, screenPoint);
    zoomIn = !zoomIn;
}

- (void)rotateWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    if ([self handleGestureAsBeginEnd:event])
        return;

    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindowSystemInterface::handleGestureEventWithRealValue(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), timestamp, Qt::RotateNativeGesture,
                                                            -[event rotation], windowPoint, screenPoint);
}

- (void)swipeWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    qCDebug(lcQpaGestures) << "swipeWithEvent" << [event deltaX] << [event deltaY] << "from device" << Qt::hex << [event deviceID];
    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];

    qreal angle = 0.0f;
    if ([event deltaX] == 1)
        angle = 180.0f;
    else if ([event deltaX] == -1)
        angle = 0.0f;
    else if ([event deltaY] == 1)
        angle = 90.0f;
    else if ([event deltaY] == -1)
        angle = 270.0f;

    QWindowSystemInterface::handleGestureEventWithRealValue(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]),  timestamp, Qt::SwipeNativeGesture,
                                                            angle, windowPoint, screenPoint);
}

- (void)beginGestureWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    qCDebug(lcQpaGestures) << "beginGestureWithEvent @" << windowPoint << "from device" << Qt::hex << [event deviceID];
    QWindowSystemInterface::handleGestureEvent(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]),  timestamp, Qt::BeginNativeGesture,
                                               windowPoint, screenPoint);
}

- (void)endGestureWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    qCDebug(lcQpaGestures) << "endGestureWithEvent" << "from device" << Qt::hex << [event deviceID];
    const NSTimeInterval timestamp = [event timestamp];
    QPointF windowPoint;
    QPointF screenPoint;
    [self convertFromScreen:[self screenMousePoint:event] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
    QWindowSystemInterface::handleGestureEvent(m_platformWindow->window(), QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]),  timestamp, Qt::EndNativeGesture,
                                               windowPoint, screenPoint);
}

@end

#endif // QT_NO_GESTURES
