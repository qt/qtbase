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
