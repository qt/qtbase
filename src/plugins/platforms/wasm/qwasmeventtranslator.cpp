/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwasmeventtranslator.h"
#include "qwasmeventdispatcher.h"
#include "qwasmcompositor.h"
#include "qwasmintegration.h"

#include <QtGui/qevent.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>

#include <QtCore/qdeadlinetimer.h>
#include <emscripten/bind.h>

#include <iostream>

QT_BEGIN_NAMESPACE
using namespace emscripten;

// macOS CTRL <-> META switching. We most likely want to enable
// the existing switching code in QtGui, but for now do it here.
static bool g_usePlatformMacCtrlMetaSwitching = false;

bool g_useNaturalScrolling = true; // natural scrolling is default on linux/windows

void setNaturalScrolling(bool use) {
    g_useNaturalScrolling = use;
}

EMSCRIPTEN_BINDINGS(mouse_module) {
    function("setNaturalScrolling", &setNaturalScrolling);
}

QWasmEventTranslator::QWasmEventTranslator(QObject *parent)
    : QObject(parent)
    , draggedWindow(nullptr)
    , lastWindow(nullptr)
    , pressedButtons(Qt::NoButton)
    , resizeMode(QWasmWindow::ResizeNone)
{
    emscripten_set_keydown_callback(0, (void *)this, 1, &keyboard_cb);
    emscripten_set_keyup_callback(0, (void *)this, 1, &keyboard_cb);

    emscripten_set_mousedown_callback(0, (void *)this, 1, &mouse_cb);
    emscripten_set_mouseup_callback(0, (void *)this, 1, &mouse_cb);
    emscripten_set_mousemove_callback(0, (void *)this, 1, &mouse_cb);

    emscripten_set_focus_callback(0, (void *)this, 1, &focus_cb);

    emscripten_set_wheel_callback(0, (void *)this, 1, &wheel_cb);

    touchDevice = new QTouchDevice;
    touchDevice->setType(QTouchDevice::TouchScreen);
    touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::NormalizedPosition);
    QWindowSystemInterface::registerTouchDevice(touchDevice);

    emscripten_set_touchstart_callback("#canvas", (void *)this, 1, &touchCallback);
    emscripten_set_touchend_callback("#canvas", (void *)this, 1, &touchCallback);
    emscripten_set_touchmove_callback("#canvas", (void *)this, 1, &touchCallback);
    emscripten_set_touchcancel_callback("#canvas", (void *)this, 1, &touchCallback);

    // The Platform Detect: expand coverage and move as needed
    enum Platform {
        GenericPlatform,
        MacOSPlatform
    };
    Platform platform =
        Platform(EM_ASM_INT("if (navigator.platform.includes(\"Mac\")) return 1; return 0;"));

    g_usePlatformMacCtrlMetaSwitching = (platform == MacOSPlatform);

    if (platform == MacOSPlatform) {
        g_useNaturalScrolling = false; // make this !default on macOS
        EM_ASM(
            if (window.safari !== undefined)  {//this only works on safari
                Module["canvas"].addEventListener('wheel', mouseWheelEvent);
                function mouseWheelEvent(e) {
                    if (event.webkitDirectionInvertedFromDevice) {
                        Module.setNaturalScrolling(event.webkitDirectionInvertedFromDevice);
                    }
                }
            }
        );
    }
}

template <typename Event>
QFlags<Qt::KeyboardModifier> QWasmEventTranslator::translatKeyModifier(const Event *event)
{
    QFlags<Qt::KeyboardModifier> keyModifier = Qt::NoModifier;
    if (event->shiftKey)
        keyModifier |= Qt::ShiftModifier;
    if (event->ctrlKey) {
        if (g_usePlatformMacCtrlMetaSwitching)
            keyModifier |= Qt::MetaModifier;
        else
            keyModifier |= Qt::ControlModifier;
    }
    if (event->altKey)
        keyModifier |= Qt::AltModifier;
    if (event->metaKey) {
        if (g_usePlatformMacCtrlMetaSwitching)
            keyModifier |= Qt::ControlModifier;
        else
            keyModifier |= Qt::MetaModifier;
    }
    return keyModifier;
}

QFlags<Qt::KeyboardModifier> QWasmEventTranslator::translateKeyboardEventModifier(const EmscriptenKeyboardEvent *keyEvent)
{
    QFlags<Qt::KeyboardModifier> keyModifier = translatKeyModifier(keyEvent);
    if (keyEvent->location == DOM_KEY_LOCATION_NUMPAD) {
        keyModifier |= Qt::KeypadModifier;
    }

    return keyModifier;
}

QFlags<Qt::KeyboardModifier> QWasmEventTranslator::translateMouseEventModifier(const EmscriptenMouseEvent *mouseEvent)
{
    return translatKeyModifier(mouseEvent);
}

int QWasmEventTranslator::keyboard_cb(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData)
{
    Q_UNUSED(userData)

    bool alphanumeric;
    Qt::Key qtKey = translateEmscriptKey(keyEvent, &alphanumeric);

    QEvent::Type keyType = QEvent::None;
    switch (eventType) {
    case EMSCRIPTEN_EVENT_KEYPRESS:
    case EMSCRIPTEN_EVENT_KEYDOWN: //down
        keyType = QEvent::KeyPress;
        break;
    case EMSCRIPTEN_EVENT_KEYUP: //up
        keyType = QEvent::KeyRelease;
        break;
    default:
        break;
    };

    if (keyType == QEvent::None)
        return 0;

    QString keyText = alphanumeric ? QString(keyEvent->key) : QString();
    bool accepted = QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
        0, keyType, qtKey, translateKeyboardEventModifier(keyEvent), keyText);
    QWasmEventDispatcher::maintainTimers();
    return accepted ? 1 : 0;
}

Qt::Key QWasmEventTranslator::translateEmscriptKey(const EmscriptenKeyboardEvent *emscriptKey, bool *outAlphanumeric)
{
    Qt::Key qtKey;
    if (outAlphanumeric)
        *outAlphanumeric = false;

    switch (emscriptKey->keyCode) {
    case KeyMultiply: qtKey = Qt::Key_Asterisk; *outAlphanumeric = true; break;
    case KeyAdd: qtKey = Qt::Key_Plus; *outAlphanumeric = true; break;
    case KeyMinus: qtKey = Qt::Key_Minus; *outAlphanumeric = true; break;
    case KeySubtract: qtKey = Qt::Key_Minus; *outAlphanumeric = true; break;
    case KeyDecimal: qtKey = Qt::Key_Period; *outAlphanumeric = true; break;
    case KeyDivide: qtKey = Qt::Key_Slash; *outAlphanumeric = true; break;
    case KeyNumPad0: qtKey = Qt::Key_0; *outAlphanumeric = true; break;
    case KeyNumPad1: qtKey = Qt::Key_1; *outAlphanumeric = true; break;
    case KeyNumPad2: qtKey = Qt::Key_2; *outAlphanumeric = true; break;
    case KeyNumPad3: qtKey = Qt::Key_3; *outAlphanumeric = true; break;
    case KeyNumPad4: qtKey = Qt::Key_4; *outAlphanumeric = true; break;
    case KeyNumPad5: qtKey = Qt::Key_5; *outAlphanumeric = true; break;
    case KeyNumPad6: qtKey = Qt::Key_6; *outAlphanumeric = true; break;
    case KeyNumPad7: qtKey = Qt::Key_7; *outAlphanumeric = true; break;
    case KeyNumPad8: qtKey = Qt::Key_8; *outAlphanumeric = true; break;
    case KeyNumPad9: qtKey = Qt::Key_9; *outAlphanumeric = true; break;
    case KeyComma: qtKey = Qt::Key_Comma; *outAlphanumeric = true; break;
    case KeyPeriod: qtKey = Qt::Key_Period; *outAlphanumeric = true; break;
    case KeySlash: qtKey = Qt::Key_Slash; *outAlphanumeric = true; break;
    case KeySemiColon: qtKey = Qt::Key_Semicolon; *outAlphanumeric = true; break;
    case KeyEquals: qtKey = Qt::Key_Equal; *outAlphanumeric = true; break;
    case KeyOpenBracket: qtKey = Qt::Key_BracketLeft; *outAlphanumeric = true; break;
    case KeyCloseBracket: qtKey = Qt::Key_BracketRight; *outAlphanumeric = true; break;
    case KeyBackSlash: qtKey = Qt::Key_Backslash; *outAlphanumeric = true; break;
    case KeyMeta:
        Q_FALLTHROUGH();
    case KeyMetaRight:
        qtKey = Qt::Key_Meta;
        break;
    case KeyTab: qtKey = Qt::Key_Tab; break;
    case KeyClear: qtKey = Qt::Key_Clear; break;
    case KeyBackSpace: qtKey = Qt::Key_Backspace; break;
    case KeyEnter: qtKey = Qt::Key_Return; break;
    case KeyShift: qtKey = Qt::Key_Shift; break;
    case KeyControl: qtKey = Qt::Key_Control; break;
    case KeyAlt: qtKey = Qt::Key_Alt; break;
    case KeyCapsLock: qtKey = Qt::Key_CapsLock; break;
    case KeyEscape: qtKey = Qt::Key_Escape; break;
    case KeyPageUp: qtKey = Qt::Key_PageUp; break;
    case KeyPageDown: qtKey = Qt::Key_PageDown; break;
    case KeyEnd: qtKey = Qt::Key_End; break;
    case KeyHome: qtKey = Qt::Key_Home; break;
    case KeyLeft: qtKey = Qt::Key_Left; break;
    case KeyUp: qtKey = Qt::Key_Up; break;
    case KeyRight: qtKey = Qt::Key_Right; break;
    case KeyDown: qtKey = Qt::Key_Down; break;
    case KeyBrightnessDown: qtKey = Qt::Key_MonBrightnessDown; break;
    case KeyBrightnessUp: qtKey = Qt::Key_MonBrightnessUp; break;
    case KeyMediaTrackPrevious: qtKey = Qt::Key_MediaPrevious; break;
    case KeyMediaPlayPause: qtKey = Qt::Key_MediaTogglePlayPause; break;
    case KeyMediaTrackNext: qtKey = Qt::Key_MediaNext; break;
    case KeyAudioVolumeMute: qtKey = Qt::Key_VolumeMute; break;
    case KeyAudioVolumeDown: qtKey = Qt::Key_VolumeDown; break;
    case KeyAudioVolumeUp: qtKey = Qt::Key_VolumeUp; break;
    case KeyDelete: qtKey = Qt::Key_Delete; break;

    case KeyF1: qtKey = Qt::Key_F1; break;
    case KeyF2: qtKey = Qt::Key_F2; break;
    case KeyF3: qtKey = Qt::Key_F3; break;
    case KeyF4: qtKey = Qt::Key_F4; break;
    case KeyF5: qtKey = Qt::Key_F5; break;
    case KeyF6: qtKey = Qt::Key_F6; break;
    case KeyF7: qtKey = Qt::Key_F7; break;
    case KeyF8: qtKey = Qt::Key_F8; break;
    case KeyF9: qtKey = Qt::Key_F9; break;
    case KeyF10: qtKey = Qt::Key_F10; break;
    case KeyF11: qtKey = Qt::Key_F11; break;
    case KeyF12: qtKey = Qt::Key_F12; break;
    case 124: qtKey = Qt::Key_F13; break;
    case 125: qtKey = Qt::Key_F14; break;

    case KeySpace:
    default:
        if (outAlphanumeric)
            *outAlphanumeric = true;
        qtKey = static_cast<Qt::Key>(emscriptKey->keyCode);
        break;
    }

    // Handle Mac command key. Using event->keyCode as above is
    // no reliable since the codes differ between browsers.
    if (qstrncmp(emscriptKey->key, "Meta", 4) == 0) {
        qtKey = Qt::Key_Meta;
        *outAlphanumeric = false;
    }

    if (g_usePlatformMacCtrlMetaSwitching) {
        if (qtKey == Qt::Key_Meta)
            qtKey = Qt::Key_Control;
        else if (qtKey == Qt::Key_Control)
            qtKey = Qt::Key_Meta;
    }

    return qtKey;
}

Qt::MouseButton QWasmEventTranslator::translateMouseButton(unsigned short button)
{
    if (button == 0)
        return Qt::LeftButton;
    else if (button == 1)
        return Qt::MiddleButton;
    else if (button == 2)
        return Qt::RightButton;

    return Qt::NoButton;
}

int QWasmEventTranslator::mouse_cb(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
{
    QWasmEventTranslator *translator = (QWasmEventTranslator*)userData;
    translator->processMouse(eventType,mouseEvent);
    QWasmEventDispatcher::maintainTimers();
    return 0;
}

void resizeWindow(QWindow *window, QWasmWindow::ResizeMode mode,
                  QRect startRect, QPoint amount)
{
    if (mode == QWasmWindow::ResizeNone)
        return;

    bool top = mode == QWasmWindow::ResizeTopLeft ||
               mode == QWasmWindow::ResizeTop ||
               mode == QWasmWindow::ResizeTopRight;

    bool bottom = mode == QWasmWindow::ResizeBottomLeft ||
                  mode == QWasmWindow::ResizeBottom ||
                  mode == QWasmWindow::ResizeBottomRight;

    bool left = mode == QWasmWindow::ResizeLeft ||
                mode == QWasmWindow::ResizeTopLeft ||
                mode == QWasmWindow::ResizeBottomLeft;

    bool right = mode == QWasmWindow::ResizeRight ||
                 mode == QWasmWindow::ResizeTopRight ||
                 mode == QWasmWindow::ResizeBottomRight;

    int x1 = startRect.left();
    int y1 = startRect.top();
    int x2 = startRect.right();
    int y2 = startRect.bottom();

    if (left)
        x1 += amount.x();
    if (top)
        y1 += amount.y();
    if (right)
        x2 += amount.x();
    if (bottom)
        y2 += amount.y();

    int w = x2-x1;
    int h = y2-y1;

    if (w < window->minimumWidth()) {
        if (left)
            x1 -= window->minimumWidth() - w;

        w = window->minimumWidth();
    }

    if (h < window->minimumHeight()) {
        if (top)
            y1 -= window->minimumHeight() - h;

        h = window->minimumHeight();
    }

    window->setGeometry(x1, y1, w, h);
}

void QWasmEventTranslator::processMouse(int eventType, const EmscriptenMouseEvent *mouseEvent)
{
    auto timestamp = mouseEvent->timestamp;
    QPoint point(mouseEvent->canvasX, mouseEvent->canvasY);

    QEvent::Type buttonEventType = QEvent::None;

    Qt::MouseButton button = translateMouseButton(mouseEvent->button);
    Qt::KeyboardModifiers modifiers = translateMouseEventModifier(mouseEvent);

    QWindow *window2 = QWasmIntegration::get()->compositor()->windowAt(point, 5);
    if (window2 != nullptr)
        lastWindow = window2;

    QWasmWindow *htmlWindow = static_cast<QWasmWindow*>(window2->handle());

    bool interior = window2 && window2->geometry().contains(point);

    QPoint localPoint(point.x() - window2->geometry().x(), point.y() - window2->geometry().y());
    switch (eventType) {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
    {
        if (window2)
            window2->raise();

        pressedButtons.setFlag(button);

        if (mouseEvent->button == 0) {
            pressedWindow = window2;
            buttonEventType = QEvent::MouseButtonPress;
            if (!(htmlWindow->m_windowState & Qt::WindowFullScreen) && !(htmlWindow->m_windowState & Qt::WindowMaximized)) {
                if (htmlWindow && window2->flags().testFlag(Qt::WindowTitleHint) && htmlWindow->isPointOnTitle(point))
                    draggedWindow = window2;
                else if (htmlWindow && htmlWindow->isPointOnResizeRegion(point)) {
                    draggedWindow = window2;
                    resizeMode = htmlWindow->resizeModeAtPoint(point);
                    resizePoint = point;
                    resizeStartRect = window2->geometry();
                }
            }
        }

        htmlWindow->injectMousePressed(localPoint, point, button, modifiers);
        break;
    }
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
        pressedButtons.setFlag(translateMouseButton(mouseEvent->button), false);
        buttonEventType = QEvent::MouseButtonRelease;
        QWasmWindow *oldWindow = nullptr;

        if (mouseEvent->button == 0 && pressedWindow) {
            oldWindow = static_cast<QWasmWindow*>(pressedWindow->handle());
            pressedWindow = nullptr;
        }

        if (mouseEvent->button == 0) {
            draggedWindow = nullptr;
            resizeMode = QWasmWindow::ResizeNone;
        }

        if (oldWindow)
            oldWindow->injectMouseReleased(localPoint, point, button, modifiers);
        break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE: // drag event
    {
        buttonEventType = QEvent::MouseMove;
        if (!(htmlWindow->m_windowState & Qt::WindowFullScreen) && !(htmlWindow->m_windowState & Qt::WindowMaximized)) {
            if (resizeMode == QWasmWindow::ResizeNone && draggedWindow) {
                draggedWindow->setX(draggedWindow->x() + mouseEvent->movementX);
                draggedWindow->setY(draggedWindow->y() + mouseEvent->movementY);
            }

            if (resizeMode != QWasmWindow::ResizeNone && !(htmlWindow->m_windowState & Qt::WindowFullScreen)) {
                QPoint delta = QPoint(mouseEvent->canvasX, mouseEvent->canvasY) - resizePoint;
                resizeWindow(draggedWindow, resizeMode, resizeStartRect, delta);
            }
        }
        break;
    }
    default: // MOUSELEAVE MOUSEENTER
        break;
    };
    if (!window2 && buttonEventType == QEvent::MouseButtonRelease) {
        window2 = lastWindow;
        lastWindow = nullptr;
        interior = true;
    }
    if (window2 && interior) {
        QWindowSystemInterface::handleMouseEvent<QWindowSystemInterface::SynchronousDelivery>(
            window2, timestamp, localPoint, point, pressedButtons, button, buttonEventType, modifiers);
    }
}

int QWasmEventTranslator::focus_cb(int /*eventType*/, const EmscriptenFocusEvent */*focusEvent*/, void */*userData*/)
{
    return 0;
}

int QWasmEventTranslator::wheel_cb(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData)
{
    Q_UNUSED(eventType)
    Q_UNUSED(userData)

    EmscriptenMouseEvent mouseEvent = wheelEvent->mouse;

    int scrollFactor = 0;
    switch (wheelEvent->deltaMode) {
    case DOM_DELTA_PIXEL://chrome safari
        scrollFactor = 1;
        break;
    case DOM_DELTA_LINE: //firefox
        scrollFactor = 12;
        break;
    case DOM_DELTA_PAGE:
        scrollFactor = 20;
        break;
    };

    if (g_useNaturalScrolling) //macOS platform has document oriented scrolling
        scrollFactor = -scrollFactor;

    Qt::KeyboardModifiers modifiers = translateMouseEventModifier(&mouseEvent);
    auto timestamp = mouseEvent.timestamp;
    QPoint globalPoint(mouseEvent.canvasX, mouseEvent.canvasY);

    QWindow *window2 = QWasmIntegration::get()->compositor()->windowAt(globalPoint, 5);

    QPoint localPoint(globalPoint.x() - window2->geometry().x(), globalPoint.y() - window2->geometry().y());

    QPoint pixelDelta;

    if (wheelEvent->deltaY != 0) pixelDelta.setY(wheelEvent->deltaY * scrollFactor);
    if (wheelEvent->deltaX != 0) pixelDelta.setX(wheelEvent->deltaX * scrollFactor);

    QWindowSystemInterface::handleWheelEvent(window2, timestamp, localPoint, globalPoint, QPoint(), pixelDelta, modifiers);

    QWasmEventDispatcher::maintainTimers();

    return 1;
}

int QWasmEventTranslator::touchCallback(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData)
{
    QList<QWindowSystemInterface::TouchPoint> touchPointList;
    touchPointList.reserve(touchEvent->numTouches);
    QWindow *window2;

    for (int i = 0; i < touchEvent->numTouches; i++) {

        const EmscriptenTouchPoint *touches = &touchEvent->touches[i];

        QPoint point(touches->canvasX, touches->canvasY);
        window2 = QWasmIntegration::get()->compositor()->windowAt(point, 5);

        QWindowSystemInterface::TouchPoint touchPoint;

        auto cX = point.x();
        auto cY = point.y();
        touchPoint.area = QRect(0, 0, 8, 8);
        touchPoint.area.moveCenter(QPointF(cX,cY)); // simulate area

        touchPoint.id = touches->identifier;
        touchPoint.normalPosition = QPointF(cX / window2->width(), cY / window2->height());

        switch (eventType) {
        case EMSCRIPTEN_EVENT_TOUCHSTART:
            touchPoint.state = Qt::TouchPointPressed;
            break;
        case EMSCRIPTEN_EVENT_TOUCHEND:
            touchPoint.state = Qt::TouchPointReleased;
            break;
        case EMSCRIPTEN_EVENT_TOUCHMOVE:
            touchPoint.state = Qt::TouchPointMoved;
            break;
        default:
            Q_UNREACHABLE();
        }

        touchPointList.append(touchPoint);
    }

    QWasmEventTranslator *wasmEventTranslator = (QWasmEventTranslator*)userData;
    QFlags<Qt::KeyboardModifier> keyModifier = translatKeyModifier(touchEvent);

    if (eventType != EMSCRIPTEN_EVENT_TOUCHCANCEL)
        QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(window2, wasmEventTranslator->getTimestamp(), wasmEventTranslator->touchDevice, touchPointList, keyModifier);
    else
        QWindowSystemInterface::handleTouchCancelEvent(window2, wasmEventTranslator->getTimestamp(), wasmEventTranslator->touchDevice, keyModifier);

    QWasmEventDispatcher::maintainTimers();
    return 1;
}

quint64 QWasmEventTranslator::getTimestamp()
{
    return QDeadlineTimer::current().deadlineNSecs() / 1000;
}

QT_END_NAMESPACE
