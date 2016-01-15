/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#include "qlibinputtouch_p.h"
#include <libinput.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

QWindowSystemInterface::TouchPoint *QLibInputTouch::DeviceState::point(int32_t slot)
{
    const int id = qMax(0, slot);

    for (int i = 0; i < m_points.count(); ++i)
        if (m_points.at(i).id == id)
            return &m_points[i];

    return Q_NULLPTR;
}

QLibInputTouch::DeviceState *QLibInputTouch::deviceState(libinput_event_touch *e)
{
    libinput_device *dev = libinput_event_get_device(libinput_event_touch_get_base_event(e));
    return &m_devState[dev];
}

static inline QPointF getPos(libinput_event_touch *e)
{
    const QSize screenSize = QGuiApplication::primaryScreen()->geometry().size();
    const double x = libinput_event_touch_get_x_transformed(e, screenSize.width());
    const double y = libinput_event_touch_get_y_transformed(e, screenSize.height());
    return QPointF(x, y);
}

void QLibInputTouch::registerDevice(libinput_device *dev)
{
    QTouchDevice *&td = m_devState[dev].m_touchDevice;
    td = new QTouchDevice;
    td->setName(QString::fromUtf8(libinput_device_get_name(dev)));
    td->setType(QTouchDevice::TouchScreen);
    td->setCapabilities(QTouchDevice::Position | QTouchDevice::Area);
    QWindowSystemInterface::registerTouchDevice(td);
}

void QLibInputTouch::unregisterDevice(libinput_device *dev)
{
    Q_UNUSED(dev);
    // There is no way to remove a QTouchDevice.
}

void QLibInputTouch::processTouchDown(libinput_event_touch *e)
{
    int slot = libinput_event_touch_get_slot(e);
    DeviceState *state = deviceState(e);
    QWindowSystemInterface::TouchPoint *tp = state->point(slot);
    if (tp) {
        qWarning("Incorrect touch state");
    } else {
        QWindowSystemInterface::TouchPoint newTp;
        newTp.id = qMax(0, slot);
        newTp.state = Qt::TouchPointPressed;
        newTp.area = QRect(0, 0, 8, 8);
        newTp.area.moveCenter(getPos(e));
        state->m_points.append(newTp);
    }
}

void QLibInputTouch::processTouchMotion(libinput_event_touch *e)
{
    int slot = libinput_event_touch_get_slot(e);
    DeviceState *state = deviceState(e);
    QWindowSystemInterface::TouchPoint *tp = state->point(slot);
    if (tp) {
        const QPointF p = getPos(e);
        if (tp->area.center() != p) {
            tp->area.moveCenter(p);
            // 'down' may be followed by 'motion' within the same "frame".
            // Handle this by compressing and keeping the Pressed state until the 'frame'.
            if (tp->state != Qt::TouchPointPressed)
                tp->state = Qt::TouchPointMoved;
        } else {
            tp->state = Qt::TouchPointStationary;
        }
    } else {
        qWarning("Inconsistent touch state (got 'motion' without 'down')");
    }
}

void QLibInputTouch::processTouchUp(libinput_event_touch *e)
{
    int slot = libinput_event_touch_get_slot(e);
    DeviceState *state = deviceState(e);
    QWindowSystemInterface::TouchPoint *tp = state->point(slot);
    if (tp) {
        tp->state = Qt::TouchPointReleased;
        // There may not be a Frame event after the last Up. Work this around.
        Qt::TouchPointStates s = 0;
        for (int i = 0; i < state->m_points.count(); ++i)
            s |= state->m_points.at(i).state;
        if (s == Qt::TouchPointReleased)
            processTouchFrame(e);
    } else {
        qWarning("Inconsistent touch state (got 'up' without 'down')");
    }
}

void QLibInputTouch::processTouchCancel(libinput_event_touch *e)
{
    DeviceState *state = deviceState(e);
    if (state->m_touchDevice)
        QWindowSystemInterface::handleTouchCancelEvent(Q_NULLPTR, state->m_touchDevice, QGuiApplication::keyboardModifiers());
    else
        qWarning("TouchCancel without registered device");
}

void QLibInputTouch::processTouchFrame(libinput_event_touch *e)
{
    DeviceState *state = deviceState(e);
    if (!state->m_touchDevice) {
        qWarning("TouchFrame without registered device");
        return;
    }
    if (state->m_points.isEmpty())
        return;

    QWindowSystemInterface::handleTouchEvent(Q_NULLPTR, state->m_touchDevice, state->m_points,
                                             QGuiApplication::keyboardModifiers());

    for (int i = 0; i < state->m_points.count(); ++i) {
        QWindowSystemInterface::TouchPoint &tp(state->m_points[i]);
        if (tp.state == Qt::TouchPointReleased)
            state->m_points.removeAt(i--);
        else if (tp.state == Qt::TouchPointPressed)
            tp.state = Qt::TouchPointStationary;
    }
}

QT_END_NAMESPACE
