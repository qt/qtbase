// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// This file is included from qnsview.mm, and only used to organize the code

Q_STATIC_LOGGING_CATEGORY(lcQpaTouch, "qt.qpa.input.touch")

@implementation QNSView (Touch)

- (bool)shouldSendSingleTouch
{
    if (!m_platformWindow)
        return true;

    // QtWidgets expects single-point touch events, QtDeclarative does not.
    // Until there is an API we solve this by looking at the window class type.
    return m_platformWindow->window()->inherits("QWidgetWindow");
}

- (void)touchesBeganWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesBeganWithEvent" << points << "from device" << Qt::hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(
        m_platformWindow->window(), timestamp * 1000,
        QCocoaTouch::getTouchDevice(QInputDevice::DeviceType::TouchPad, [event deviceID]),
        points);
}

- (void)touchesMovedWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];

    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    nativeDrag->setLastInputEvent(event, self);

    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesMovedWithEvent" << points << "from device" << Qt::hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(
        m_platformWindow->window(), timestamp * 1000,
        QCocoaTouch::getTouchDevice(QInputDevice::DeviceType::TouchPad, [event deviceID]),
        points);
}

- (void)touchesEndedWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];

    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    nativeDrag->setLastInputEvent(event, self);

    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesEndedWithEvent" << points << "from device" << Qt::hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(
        m_platformWindow->window(), timestamp * 1000,
        QCocoaTouch::getTouchDevice(QInputDevice::DeviceType::TouchPad, [event deviceID]),
        points);
}

- (void)touchesCancelledWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];

    QCocoaDrag* nativeDrag = QCocoaIntegration::instance()->drag();
    nativeDrag->setLastInputEvent(event, self);

    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesCancelledWithEvent" << points << "from device" << Qt::hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(
        m_platformWindow->window(), timestamp * 1000,
        QCocoaTouch::getTouchDevice(QInputDevice::DeviceType::TouchPad, [event deviceID]),
        points);
}

@end
