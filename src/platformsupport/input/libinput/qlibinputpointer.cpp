// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#if QT_CONFIG(libinput_hires_wheel_support)
        value = libinput_event_pointer_get_scroll_value_v120(e, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
#else
        value = libinput_event_pointer_get_axis_value(e, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
#endif
        angleDelta.setY(qRound(value));
    }
    if (libinput_event_pointer_has_axis(e, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL)) {
#if QT_CONFIG(libinput_hires_wheel_support)
        value = libinput_event_pointer_get_scroll_value_v120(e, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
#else
        value = libinput_event_pointer_get_axis_value(e, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
#endif
        angleDelta.setX(qRound(value));
    }
#endif
#if QT_CONFIG(libinput_hires_wheel_support)
    const int factor = -1;
#else
    const int factor = -8;
#endif
    angleDelta *= factor;
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
