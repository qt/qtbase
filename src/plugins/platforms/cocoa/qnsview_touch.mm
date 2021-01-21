/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

// This file is included from qnsview.mm, and only used to organize the code

Q_LOGGING_CATEGORY(lcQpaTouch, "qt.qpa.input.touch")

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
    QWindowSystemInterface::handleTouchEvent(m_platformWindow->window(), timestamp * 1000, QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), points);
}

- (void)touchesMovedWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesMovedWithEvent" << points << "from device" << Qt::hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent(m_platformWindow->window(), timestamp * 1000, QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), points);
}

- (void)touchesEndedWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesEndedWithEvent" << points << "from device" << Qt::hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent(m_platformWindow->window(), timestamp * 1000, QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), points);
}

- (void)touchesCancelledWithEvent:(NSEvent *)event
{
    if (!m_platformWindow)
        return;

    const NSTimeInterval timestamp = [event timestamp];
    const QList<QWindowSystemInterface::TouchPoint> points = QCocoaTouch::getCurrentTouchPointList(event, [self shouldSendSingleTouch]);
    qCDebug(lcQpaTouch) << "touchesCancelledWithEvent" << points << "from device" << Qt::hex << [event deviceID];
    QWindowSystemInterface::handleTouchEvent(m_platformWindow->window(), timestamp * 1000, QCocoaTouch::getTouchDevice(QTouchDevice::TouchPad, [event deviceID]), points);
}

@end
