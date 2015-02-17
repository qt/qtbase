/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpeppereventtranslator.h"

#include "qpepperhelpers.h"
#include "qpepperinstance_p.h"
#include "3rdparty/keyboard_codes_posix.h"

#include <qpa/qwindowsysteminterface.h>

#include <ppapi/cpp/point.h>
#include <ppapi/cpp/var.h>

#ifndef QT_NO_PEPPER_INTEGRATION

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_EVENT_KEYBOARD, "qt.platform.pepper.event.keyboard")

QPepperEventTranslator::QPepperEventTranslator()
{
    QWindowSystemInterface::setSynchronousWindowsSystemEvents(true);
}

bool QPepperEventTranslator::processEvent(const pp::InputEvent &event)
{
    switch (event.GetType()) {
    case PP_INPUTEVENT_TYPE_MOUSEDOWN:
    case PP_INPUTEVENT_TYPE_MOUSEUP:
    case PP_INPUTEVENT_TYPE_MOUSEMOVE:
        return processMouseEvent(pp::MouseInputEvent(event), event.GetType());
        break;

    // case PP_INPUTEVENT_TYPE_MOUSEENTER:
    // case PP_INPUTEVENT_TYPE_MOUSELEAVE:

    case PP_INPUTEVENT_TYPE_WHEEL:
        return processWheelEvent(pp::WheelInputEvent(event));
        break;

    // case PP_INPUTEVENT_TYPE_RAWKEYDOWN:

    case PP_INPUTEVENT_TYPE_KEYDOWN:
    case PP_INPUTEVENT_TYPE_KEYUP:
        return processKeyEvent(pp::KeyboardInputEvent(event), event.GetType());
        break;

    case PP_INPUTEVENT_TYPE_CHAR:
        return processCharacterEvent(pp::KeyboardInputEvent(event));
        break;

    default:
        break;
    }

    return false;
}

bool QPepperEventTranslator::processMouseEvent(const pp::MouseInputEvent &event,
                                              PP_InputEvent_Type eventType)
{
    QPoint point = toQPointF(event.GetPosition()) / QPepperInstancePrivate::get()->cssScale();
    currentMouseGlobalPos = point;
    // Qt::MouseButton button = translatePepperMouseButton(event.button);
    Qt::MouseButtons modifiers = translatePepperMouseModifiers(event.GetModifiers());

    QPoint localPoint = point;
    QWindow *window = 0;
    emit getWindowAt(point, &window);

    if (window) {
        localPoint = point - window->position();
        //        qDebug() << window << window->position() << point << localPoint;
    }

    // Qt mouse button state is state *after* the mouse event, send NoButton
    // on mouse up.
    // ### strictly not correct, only the state for the released button should
    // be cleared.
    if (eventType == PP_INPUTEVENT_TYPE_MOUSEUP) {
        QWindowSystemInterface::handleMouseEvent(window, localPoint, point,
                                                 Qt::MouseButtons(Qt::NoButton));
    } else {
        QWindowSystemInterface::handleMouseEvent(window, localPoint, point, modifiers);
    }

    return true;
}

bool QPepperEventTranslator::processWheelEvent(const pp::WheelInputEvent &event)
{
    QWindow *window = 0;
    emit getWindowAt(currentMouseGlobalPos, &window);

    QPoint localPoint = window ? currentMouseGlobalPos - window->position() : currentMouseGlobalPos;

    if (event.GetScrollByPage()) {
        qWarning("QPepperEventTranslator::processWheelEvent: ScrollByPage not implemented.");
    } else {
        QPointF delta = toQPointF(event.GetDelta()); // delta is in (device independent) pixels
        const qreal wheelDegreesPerTick = 1;
        const qreal qtTickUnit = (1.0 / 8.0); // tick unit is "eighths of a degree"
        QPointF ticks = toQPointF(event.GetDelta()) * wheelDegreesPerTick / qtTickUnit;
        QWindowSystemInterface::handleWheelEvent(window, localPoint, currentMouseGlobalPos,
                                                 delta.toPoint(), ticks.toPoint());
    }

    return true;
}

//    Key translation: Pepper sends three types of events. Pressing and
//    holding a key gives the following sequence:
//
//        NPEventType_KeyDown
//        [NPEventType_Char]
//        NPEventType_KeyDown
//        [NPEventType_Char]
//        ...
//        NPEventType_KeyUp
//
//    The "NPEventType_Char" is optional and is sent only when the key stroke should
//    generate text input. Qt needs the text from the NPEventType_Char event; delay
//    sending the keypress until NPEventType_Char in cases where the KeyDown will be
//    followed by a NPEventType_Char.
//
bool QPepperEventTranslator::processKeyEvent(const pp::KeyboardInputEvent &event,
                                            PP_InputEvent_Type eventType)
{
    Qt::KeyboardModifiers modifiers = translatePepperKeyModifiers(event.GetModifiers());
    bool alphanumeric;
    Qt::Key key = translatePepperKey(event.GetKeyCode(), &alphanumeric);

    QWindow *window = 0;
    emit getKeyWindow(&window);

    if (eventType == PP_INPUTEVENT_TYPE_KEYDOWN) {
        currentPepperKey = event.GetKeyCode();
        qCDebug(QT_PLATFORM_PEPPER_EVENT_KEYBOARD) << "Key Down" << currentPepperKey << "alphanum"
                                                   << alphanumeric;
        if (!alphanumeric || modifiers != Qt::NoModifier) {
            return QWindowSystemInterface::handleKeyEvent(window, QEvent::KeyPress, key, modifiers);
        }
    }

    if (eventType == PP_INPUTEVENT_TYPE_KEYUP) {
        qCDebug(QT_PLATFORM_PEPPER_EVENT_KEYBOARD) << "Key Up" << currentPepperKey;
        return QWindowSystemInterface::handleKeyEvent(window, QEvent::KeyRelease, key, modifiers);
    }
    return false;
}

bool QPepperEventTranslator::processCharacterEvent(const pp::KeyboardInputEvent &event)
{
    QString text
        = QString::fromUtf8(event.GetCharacterText().AsString().c_str()); // ### wide characters?
    Qt::KeyboardModifiers modifiers = translatePepperKeyModifiers(event.GetModifiers());
    Qt::Key key = translatePepperKey(currentPepperKey, 0);

    qCDebug(QT_PLATFORM_PEPPER_EVENT_KEYBOARD) << "Key Character" << key << text;

    // Discard newline/spaces character events, these are handled by processKeyEvent()
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Tab
        || key == Qt::Key_Backtab)
        return false;

    QWindow *window = 0;
    emit getKeyWindow(&window);

    return QWindowSystemInterface::handleKeyEvent(window, QEvent::KeyPress, key, modifiers, text);
}

Qt::MouseButton
QPepperEventTranslator::translatePepperMouseButton(PP_InputEvent_MouseButton pepperButton)
{
    Qt::MouseButton button;
    if (pepperButton == PP_INPUTEVENT_MOUSEBUTTON_LEFT)
        button = Qt::LeftButton;
    if (pepperButton == PP_INPUTEVENT_MOUSEBUTTON_MIDDLE)
        button = Qt::MidButton;
    if (pepperButton == PP_INPUTEVENT_MOUSEBUTTON_RIGHT)
        button = Qt::RightButton;
    return button;
}

Qt::MouseButtons QPepperEventTranslator::translatePepperMouseModifiers(uint32_t modifier)
{
    Qt::MouseButtons buttons;
    if (modifier & PP_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN)
        buttons |= Qt::LeftButton;
    if (modifier & PP_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN)
        buttons |= Qt::RightButton;
    if (modifier & PP_INPUTEVENT_MODIFIER_MIDDLEBUTTONDOWN)
        buttons |= Qt::MidButton;

    return buttons;
}

/*
    Translate key codes. Some of the codes match (0..1, A..Z),
    so copy the value over by default and then handle the rest
    as special cases.

    This implementation is incomplete - most used keys are
    added first.
*/

Qt::Key QPepperEventTranslator::translatePepperKey(uint32_t pepperKey, bool *outAlphanumeric)
{
    Qt::Key qtKey;
    if (outAlphanumeric)
        *outAlphanumeric = false;

    using namespace base; // keybaord codes from keyboard_codes_posix.h

    switch (pepperKey) {
        // (in VKEY order)
        case VKEY_BACK: qtKey = Qt::Key_Backspace; break;
        case VKEY_TAB: qtKey = Qt::Key_Tab; break;
        case VKEY_CLEAR : qtKey = Qt::Key_Clear; break;
        case VKEY_RETURN: qtKey = Qt::Key_Return; break;
        case VKEY_SHIFT: qtKey = Qt::Key_Shift; break;
        case VKEY_CONTROL: qtKey = Qt::Key_Control; break;
        case VKEY_MENU: qtKey = Qt::Key_Menu; break;
        case VKEY_PAUSE: qtKey = Qt::Key_Pause; break;
        case VKEY_SPACE: qtKey = Qt::Key_Space; break;
        case VKEY_END: qtKey = Qt::Key_End; break;
        case VKEY_HOME: qtKey = Qt::Key_Home; break;
        case VKEY_LEFT: qtKey = Qt::Key_Left; break;
        case VKEY_UP: qtKey = Qt::Key_Up; break;
        case VKEY_RIGHT: qtKey = Qt::Key_Right; break;
        case VKEY_DOWN: qtKey = Qt::Key_Down; break;
        case VKEY_SELECT: qtKey = Qt::Key_Select; break;
        case VKEY_PRINT: qtKey = Qt::Key_Print; break;
        case VKEY_EXECUTE: qtKey = Qt::Key_Execute; break;
        //case VKEY_SNAPSHOT: qtKey = Qt::Key_; break;
        case VKEY_INSERT: qtKey = Qt::Key_Insert; break;
        case VKEY_DELETE: qtKey = Qt::Key_Delete; break;
        case VKEY_HELP: qtKey = Qt::Key_Help; break;
        case VKEY_NUMPAD0: qtKey = Qt::Key_0; break;
        case VKEY_NUMPAD1: qtKey = Qt::Key_1; break;
        case VKEY_NUMPAD2: qtKey = Qt::Key_2; break;
        case VKEY_NUMPAD3: qtKey = Qt::Key_3; break;
        case VKEY_NUMPAD4: qtKey = Qt::Key_4; break;
        case VKEY_NUMPAD5: qtKey = Qt::Key_5; break;
        case VKEY_NUMPAD6: qtKey = Qt::Key_6; break;
        case VKEY_NUMPAD7: qtKey = Qt::Key_7; break;
        case VKEY_NUMPAD8: qtKey = Qt::Key_8; break;
        case VKEY_NUMPAD9: qtKey = Qt::Key_9; break;
        case VKEY_F1: qtKey = Qt::Key_F1; break;
        case VKEY_F2: qtKey = Qt::Key_F2; break;
        case VKEY_F3: qtKey = Qt::Key_F3; break;
        case VKEY_F4: qtKey = Qt::Key_F4; break;
        case VKEY_F5: qtKey = Qt::Key_F5; break;
        case VKEY_F6: qtKey = Qt::Key_F6; break;
        case VKEY_F7: qtKey = Qt::Key_F7; break;
        case VKEY_F8: qtKey = Qt::Key_F8; break;
        case VKEY_F9: qtKey = Qt::Key_F9; break;
        case VKEY_F10: qtKey = Qt::Key_F10; break;
        case VKEY_F11: qtKey = Qt::Key_F11; break;
        case VKEY_F12: qtKey = Qt::Key_F12; break;
        case VKEY_F13: qtKey = Qt::Key_F13; break;
        case VKEY_F14: qtKey = Qt::Key_F14; break;
        case VKEY_F15: qtKey = Qt::Key_F15; break;
        case VKEY_F16: qtKey = Qt::Key_F16; break;
        case VKEY_F17: qtKey = Qt::Key_F17; break;
        case VKEY_F18: qtKey = Qt::Key_F18; break;
        case VKEY_F19: qtKey = Qt::Key_F19; break;
        case VKEY_F20: qtKey = Qt::Key_F20; break;
        case VKEY_F21: qtKey = Qt::Key_F21; break;
        case VKEY_F22: qtKey = Qt::Key_F22; break;
        case VKEY_F23: qtKey = Qt::Key_F23; break;
        case VKEY_F24: qtKey = Qt::Key_F24; break;

      default:
            if (outAlphanumeric)
                *outAlphanumeric = true;
            qtKey = static_cast<Qt::Key>(pepperKey);
        break;
    }

    return qtKey;
}

/*
    Translate supported modifiers (first five pepper modifiers).
    pepper starts at 0x1, Qt starts at 0x02000000. (same order)
*/
Qt::KeyboardModifiers QPepperEventTranslator::translatePepperKeyModifiers(uint32_t modifier)
{
    return Qt::KeyboardModifiers((modifier & 0x1F) << 25);
}

#endif
