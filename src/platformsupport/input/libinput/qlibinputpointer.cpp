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

#include "qlibinputpointer_p.h"
#include <libinput.h>
#include <QtCore/QEvent>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qinputdevicemanager_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

QLibInputPointer::QLibInputPointer()
    : m_buttons(Qt::NoButton)
{
}

void QLibInputPointer::processButton(libinput_event_pointer *e)
{
    const uint32_t b = libinput_event_pointer_get_button(e);
    const bool pressed = libinput_event_pointer_get_button_state(e) == LIBINPUT_BUTTON_STATE_PRESSED;

    Qt::MouseButton button = Qt::NoButton;
    switch (b) {
    case 0x110: button = Qt::LeftButton; break;    // BTN_LEFT
    case 0x111: button = Qt::RightButton; break;
    case 0x112: button = Qt::MiddleButton; break;
    case 0x113: button = Qt::ExtraButton1; break;  // AKA Qt::BackButton
    case 0x114: button = Qt::ExtraButton2; break;  // AKA Qt::ForwardButton
    case 0x115: button = Qt::ExtraButton3; break;  // AKA Qt::TaskButton
    case 0x116: button = Qt::ExtraButton4; break;
    case 0x117: button = Qt::ExtraButton5; break;
    case 0x118: button = Qt::ExtraButton6; break;
    case 0x119: button = Qt::ExtraButton7; break;
    case 0x11a: button = Qt::ExtraButton8; break;
    case 0x11b: button = Qt::ExtraButton9; break;
    case 0x11c: button = Qt::ExtraButton10; break;
    case 0x11d: button = Qt::ExtraButton11; break;
    case 0x11e: button = Qt::ExtraButton12; break;
    case 0x11f: button = Qt::ExtraButton13; break;
    }

    m_buttons.setFlag(button, pressed);

    QEvent::Type type = pressed ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease;
    Qt::KeyboardModifiers mods = QGuiApplicationPrivate::inputDeviceManager()->keyboardModifiers();

    QWindowSystemInterface::handleMouseEvent(nullptr, m_pos, m_pos, m_buttons, button, type, mods);
}

void QLibInputPointer::processMotion(libinput_event_pointer *e)
{
    const double dx = libinput_event_pointer_get_dx(e);
    const double dy = libinput_event_pointer_get_dy(e);
    QScreen * const primaryScreen = QGuiApplication::primaryScreen();
    const QRect g = QHighDpi::toNativePixels(primaryScreen->virtualGeometry(), primaryScreen);

    m_pos.setX(qBound(g.left(), qRound(m_pos.x() + dx), g.right()));
    m_pos.setY(qBound(g.top(), qRound(m_pos.y() + dy), g.bottom()));

    Qt::KeyboardModifiers mods = QGuiApplicationPrivate::inputDeviceManager()->keyboardModifiers();

    QWindowSystemInterface::handleMouseEvent(nullptr, m_pos, m_pos, m_buttons,
                                             Qt::NoButton, QEvent::MouseMove, mods);
}

void QLibInputPointer::processAbsMotion(libinput_event_pointer *e)
{
    QScreen * const primaryScreen = QGuiApplication::primaryScreen();
    const QRect g = QHighDpi::toNativePixels(primaryScreen->virtualGeometry(), primaryScreen);

    const double x = libinput_event_pointer_get_absolute_x_transformed(e, g.width());
    const double y = libinput_event_pointer_get_absolute_y_transformed(e, g.height());

    m_pos.setX(qBound(g.left(), qRound(g.left() + x), g.right()));
    m_pos.setY(qBound(g.top(), qRound(g.top() + y), g.bottom()));

    Qt::KeyboardModifiers mods = QGuiApplicationPrivate::inputDeviceManager()->keyboardModifiers();

    QWindowSystemInterface::handleMouseEvent(nullptr, m_pos, m_pos, m_buttons,
                                             Qt::NoButton, QEvent::MouseMove, mods);

}

void QLibInputPointer::processAxis(libinput_event_pointer *e)
{
    double value; // default axis value is 15 degrees per wheel click
    QPoint angleDelta;
#if !QT_CONFIG(libinput_axis_api)
    value = libinput_event_pointer_get_axis_value(e);
    if (libinput_event_pointer_get_axis(e) == LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL)
        angleDelta.setY(qRound(value));
    else
        angleDelta.setX(qRound(value));
#else
    if (libinput_event_pointer_has_axis(e, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL)) {
        value = libinput_event_pointer_get_axis_value(e, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
        angleDelta.setY(qRound(value));
    }
    if (libinput_event_pointer_has_axis(e, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL)) {
        value = libinput_event_pointer_get_axis_value(e, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
        angleDelta.setX(qRound(value));
    }
#endif
    const int factor = 8;
    angleDelta *= -factor;
    Qt::KeyboardModifiers mods = QGuiApplicationPrivate::inputDeviceManager()->keyboardModifiers();
    QWindowSystemInterface::handleWheelEvent(nullptr, m_pos, m_pos, QPoint(), angleDelta, mods);
}

void QLibInputPointer::setPos(const QPoint &pos)
{
    QScreen * const primaryScreen = QGuiApplication::primaryScreen();
    const QRect g = QHighDpi::toNativePixels(primaryScreen->virtualGeometry(), primaryScreen);

    m_pos.setX(qBound(g.left(), pos.x(), g.right()));
    m_pos.setY(qBound(g.top(), pos.y(), g.bottom()));
}

QT_END_NAMESPACE
