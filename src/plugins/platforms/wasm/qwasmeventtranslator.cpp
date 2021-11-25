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
#include "qwasmclipboard.h"
#include "qwasmstring.h"

#include <QtGui/qevent.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>

#include <QtCore/qdeadlinetimer.h>
#include <private/qmakearray_p.h>
#include <QtCore/qnamespace.h>
#include <QCursor>

#include <emscripten/bind.h>

#include <iostream>

using namespace emscripten;

QT_BEGIN_NAMESPACE

typedef struct emkb2qt {
    const char *em;
    unsigned int qt;

    constexpr bool operator <=(const emkb2qt &that) const noexcept
    {
        return !(strcmp(that) > 0);
    }

    bool operator <(const emkb2qt &that) const noexcept
    {
         return ::strcmp(em, that.em) < 0;
    }
    constexpr int strcmp(const emkb2qt &that, const int i = 0) const
     {
         return em[i] == 0 && that.em[i] == 0 ? 0
             : em[i] == 0 ? -1
                 : that.em[i] == 0 ? 1
                     : em[i] < that.em[i] ? -1
                         : em[i] > that.em[i] ? 1
                             : strcmp(that, i + 1);
     }
} emkb2qt_t;

template<unsigned int Qt, char ... EmChar>
struct Emkb2Qt
{
    static constexpr const char storage[sizeof ... (EmChar) + 1] = {EmChar..., '\0'};
    using Type = emkb2qt_t;
    static constexpr Type data() noexcept { return Type{storage, Qt}; }
};

template<unsigned int Qt, char ... EmChar> constexpr char Emkb2Qt<Qt, EmChar...>::storage[];

static constexpr const auto KeyTbl = qMakeArray(
    QSortedData<
        Emkb2Qt< Qt::Key_Escape,        'E','s','c','a','p','e' >,
        Emkb2Qt< Qt::Key_Tab,           'T','a','b' >,
        Emkb2Qt< Qt::Key_Backspace,     'B','a','c','k','s','p','a','c','e' >,
        Emkb2Qt< Qt::Key_Return,        'E','n','t','e','r' >,
        Emkb2Qt< Qt::Key_Insert,        'I','n','s','e','r','t' >,
        Emkb2Qt< Qt::Key_Delete,        'D','e','l','e','t','e' >,
        Emkb2Qt< Qt::Key_Pause,         'P','a','u','s','e' >,
        Emkb2Qt< Qt::Key_Pause,         'C','l','e','a','r' >,
        Emkb2Qt< Qt::Key_Home,          'H','o','m','e' >,
        Emkb2Qt< Qt::Key_End,           'E','n','d' >,
        Emkb2Qt< Qt::Key_Left,          'A','r','r','o','w','L','e','f','t' >,
        Emkb2Qt< Qt::Key_Up,            'A','r','r','o','w','U','p' >,
        Emkb2Qt< Qt::Key_Right,         'A','r','r','o','w','R','i','g','h','t' >,
        Emkb2Qt< Qt::Key_Down,          'A','r','r','o','w','D','o','w','n' >,
        Emkb2Qt< Qt::Key_PageUp,        'P','a','g','e','U','p' >,
        Emkb2Qt< Qt::Key_PageDown,      'P','a','g','e','D','o','w','n' >,
        Emkb2Qt< Qt::Key_Shift,         'S','h','i','f','t' >,
        Emkb2Qt< Qt::Key_Control,       'C','o','n','t','r','o','l' >,
        Emkb2Qt< Qt::Key_Meta,          'M','e','t','a'>,
        Emkb2Qt< Qt::Key_Meta,          'O','S'>,
        Emkb2Qt< Qt::Key_Alt,           'A','l','t','L','e','f','t' >,
        Emkb2Qt< Qt::Key_Alt,           'A','l','t' >,
        Emkb2Qt< Qt::Key_CapsLock,      'C','a','p','s','L','o','c','k' >,
        Emkb2Qt< Qt::Key_NumLock,       'N','u','m','L','o','c','k' >,
        Emkb2Qt< Qt::Key_ScrollLock,    'S','c','r','o','l','l','L','o','c','k' >,
        Emkb2Qt< Qt::Key_F1,            'F','1' >,
        Emkb2Qt< Qt::Key_F2,            'F','2' >,
        Emkb2Qt< Qt::Key_F3,            'F','3' >,
        Emkb2Qt< Qt::Key_F4,            'F','4' >,
        Emkb2Qt< Qt::Key_F5,            'F','5' >,
        Emkb2Qt< Qt::Key_F6,            'F','6' >,
        Emkb2Qt< Qt::Key_F7,            'F','7' >,
        Emkb2Qt< Qt::Key_F8,            'F','8' >,
        Emkb2Qt< Qt::Key_F9,            'F','9' >,
        Emkb2Qt< Qt::Key_F10,           'F','1','0' >,
        Emkb2Qt< Qt::Key_F11,           'F','1','1' >,
        Emkb2Qt< Qt::Key_F12,           'F','1','2' >,
        Emkb2Qt< Qt::Key_F13,           'F','1','3' >,
        Emkb2Qt< Qt::Key_F14,           'F','1','4' >,
        Emkb2Qt< Qt::Key_F15,           'F','1','5' >,
        Emkb2Qt< Qt::Key_F16,           'F','1','6' >,
        Emkb2Qt< Qt::Key_F17,           'F','1','7' >,
        Emkb2Qt< Qt::Key_F18,           'F','1','8' >,
        Emkb2Qt< Qt::Key_F19,           'F','1','9' >,
        Emkb2Qt< Qt::Key_F20,           'F','2','0' >,
        Emkb2Qt< Qt::Key_F21,           'F','2','1' >,
        Emkb2Qt< Qt::Key_F22,           'F','2','2' >,
        Emkb2Qt< Qt::Key_F23,           'F','2','3' >,
        Emkb2Qt< Qt::Key_Paste,         'P','a','s','t','e' >,
        Emkb2Qt< Qt::Key_AltGr,         'A','l','t','R','i','g','h','t' >,
        Emkb2Qt< Qt::Key_Help,          'H','e','l','p' >,
        Emkb2Qt< Qt::Key_yen,           'I','n','t','l','Y','e','n' >,
        Emkb2Qt< Qt::Key_Menu,          'C','o','n','t','e','x','t','M','e','n','u' >
            >::Data{}
        );

static constexpr const auto DeadKeyShiftTbl = qMakeArray(
    QSortedData<
       // shifted
        Emkb2Qt< Qt::Key_Agrave,        '\xc3','\x80' >,
        Emkb2Qt< Qt::Key_Aacute,        '\xc3','\x81' >,
        Emkb2Qt< Qt::Key_Acircumflex,   '\xc3','\x82' >,
        Emkb2Qt< Qt::Key_Adiaeresis,    '\xc3','\x84' >,
        Emkb2Qt< Qt::Key_AE,            '\xc3','\x86' >,
        Emkb2Qt< Qt::Key_Atilde,        '\xc3','\x83' >,
        Emkb2Qt< Qt::Key_Aring,         '\xc3','\x85' >,
        Emkb2Qt< Qt::Key_Egrave,        '\xc3','\x88' >,
        Emkb2Qt< Qt::Key_Eacute,        '\xc3','\x89' >,
        Emkb2Qt< Qt::Key_Ecircumflex,   '\xc3','\x8a' >,
        Emkb2Qt< Qt::Key_Ediaeresis,    '\xc3','\x8b' >,
        Emkb2Qt< Qt::Key_Icircumflex,   '\xc3','\x8e' >,
        Emkb2Qt< Qt::Key_Idiaeresis,    '\xc3','\x8f' >,
        Emkb2Qt< Qt::Key_Ocircumflex,   '\xc3','\x94' >,
        Emkb2Qt< Qt::Key_Odiaeresis,    '\xc3','\x96' >,
        Emkb2Qt< Qt::Key_Ograve,        '\xc3','\x92' >,
        Emkb2Qt< Qt::Key_Oacute,        '\xc3','\x93' >,
        Emkb2Qt< Qt::Key_Ooblique,      '\xc3','\x98' >,
        Emkb2Qt< Qt::Key_Otilde,        '\xc3','\x95' >,
        Emkb2Qt< Qt::Key_Ucircumflex,   '\xc3','\x9b' >,
        Emkb2Qt< Qt::Key_Udiaeresis,    '\xc3','\x9c' >,
        Emkb2Qt< Qt::Key_Ugrave,        '\xc3','\x99' >,
        Emkb2Qt< Qt::Key_Uacute,        '\xc3','\x9a' >,
        Emkb2Qt< Qt::Key_Ntilde,        '\xc3','\x91' >,
        Emkb2Qt< Qt::Key_Ccedilla,      '\xc3','\x87' >,
        Emkb2Qt< Qt::Key_ydiaeresis,    '\xc3','\x8f' >
    >::Data{}
);

// macOS CTRL <-> META switching. We most likely want to enable
// the existing switching code in QtGui, but for now do it here.
static bool g_usePlatformMacSpecifics = false;

bool g_useNaturalScrolling = true; // natural scrolling is default on linux/windows

static void mouseWheelEvent(emscripten::val event) {

    emscripten::val wheelInterted = event["webkitDirectionInvertedFromDevice"];

    if (wheelInterted.as<bool>()) {
        g_useNaturalScrolling = true;
    }
}

EMSCRIPTEN_BINDINGS(qtMouseModule) {
    function("qtMouseWheelEvent", &mouseWheelEvent);
}

QWasmEventTranslator::QWasmEventTranslator(QWasmScreen *screen)
    : QObject(screen)
    , draggedWindow(nullptr)
    , lastWindow(nullptr)
    , pressedButtons(Qt::NoButton)
    , resizeMode(QWasmWindow::ResizeNone)
{
    touchDevice = new QPointingDevice("touchscreen", 1, QInputDevice::DeviceType::TouchScreen,
    QPointingDevice::PointerType::Finger,
    QPointingDevice::Capability::Position | QPointingDevice::Capability::Area | QPointingDevice::Capability::NormalizedPosition,
    10, 0);
    QWindowSystemInterface::registerInputDevice(touchDevice);

    initEventHandlers();
}

QWasmEventTranslator::~QWasmEventTranslator()
{
    // deregister event handlers
    QByteArray canvasSelector = "#" + screen()->canvasId().toUtf8();
    emscripten_set_keydown_callback(canvasSelector.constData(), 0, 0, NULL);
    emscripten_set_keyup_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_mousedown_callback(canvasSelector.constData(), 0, 0, NULL);
    emscripten_set_mouseup_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_mousemove_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_focus_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_wheel_callback(canvasSelector.constData(),  0, 0, NULL);

    emscripten_set_touchstart_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_touchend_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_touchmove_callback(canvasSelector.constData(),  0, 0, NULL);
    emscripten_set_touchcancel_callback(canvasSelector.constData(),  0, 0, NULL);
}

void QWasmEventTranslator::initEventHandlers()
{
    QByteArray canvasSelector = "#" + screen()->canvasId().toUtf8();

    // The Platform Detect: expand coverage and move as needed
    enum Platform {
        GenericPlatform,
        MacOSPlatform
    };
    Platform platform = Platform(emscripten::val::global("navigator")["platform"]
            .call<bool>("includes", emscripten::val("Mac")));
    g_usePlatformMacSpecifics = (platform == MacOSPlatform);

    if (platform == MacOSPlatform) {
        g_useNaturalScrolling = false; // make this !default on macOS

        if (!emscripten::val::global("window")["safari"].isUndefined()) {
            val canvas = screen()->canvas();
            canvas.call<void>("addEventListener",
                              val("wheel"),
                              val::module_property("qtMouseWheelEvent"));
        }
    }

    emscripten_set_keydown_callback(canvasSelector.constData(), (void *)this, 1, &keyboard_cb);
    emscripten_set_keyup_callback(canvasSelector.constData(), (void *)this, 1, &keyboard_cb);

    emscripten_set_mousedown_callback(canvasSelector.constData(), (void *)this, 1, &mouse_cb);
    emscripten_set_mouseup_callback(canvasSelector.constData(), (void *)this, 1, &mouse_cb);
    emscripten_set_mousemove_callback(canvasSelector.constData(), (void *)this, 1, &mouse_cb);

    emscripten_set_focus_callback(canvasSelector.constData(), (void *)this, 1, &focus_cb);

    emscripten_set_wheel_callback(canvasSelector.constData(), (void *)this, 1, &wheel_cb);

    emscripten_set_touchstart_callback(canvasSelector.constData(), (void *)this, 1, &touchCallback);
    emscripten_set_touchend_callback(canvasSelector.constData(), (void *)this, 1, &touchCallback);
    emscripten_set_touchmove_callback(canvasSelector.constData(), (void *)this, 1, &touchCallback);
    emscripten_set_touchcancel_callback(canvasSelector.constData(), (void *)this, 1, &touchCallback);
}

template <typename Event>
QFlags<Qt::KeyboardModifier> QWasmEventTranslator::translatKeyModifier(const Event *event)
{
    QFlags<Qt::KeyboardModifier> keyModifier = Qt::NoModifier;
    if (event->shiftKey)
        keyModifier |= Qt::ShiftModifier;
    if (event->ctrlKey) {
        if (g_usePlatformMacSpecifics)
            keyModifier |= Qt::MetaModifier;
        else
            keyModifier |= Qt::ControlModifier;
    }
    if (event->altKey)
        keyModifier |= Qt::AltModifier;
    if (event->metaKey) {
        if (g_usePlatformMacSpecifics)
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
    QWasmEventTranslator *wasmTranslator = reinterpret_cast<QWasmEventTranslator *>(userData);
    bool accepted = wasmTranslator->processKeyboard(eventType, keyEvent);

    return accepted ? 1 : 0;
}

QWasmScreen *QWasmEventTranslator::screen()
{
    return static_cast<QWasmScreen *>(parent());
}

Qt::Key QWasmEventTranslator::translateEmscriptKey(const EmscriptenKeyboardEvent *emscriptKey)
{
    Qt::Key qtKey = Qt::Key_unknown;

    if (qstrncmp(emscriptKey->key, "Dead", 4) == 0 ) {
        emkb2qt_t searchKey1{emscriptKey->code, 0};
        for (auto it1 = KeyTbl.cbegin(); it1 != KeyTbl.end(); ++it1)
            if (it1 != KeyTbl.end() && (qstrcmp(searchKey1.em, it1->em) == 0)) {
                qtKey = static_cast<Qt::Key>(it1->qt);
            }
    }
    if (qtKey == Qt::Key_unknown) {
        emkb2qt_t searchKey{emscriptKey->key, 0};
        // search key
        auto it1 = std::lower_bound(KeyTbl.cbegin(), KeyTbl.cend(), searchKey);
        if (it1 != KeyTbl.end() && !(searchKey < *it1)) {
            qtKey = static_cast<Qt::Key>(it1->qt);
        }
    }

    if (qtKey == Qt::Key_unknown) {
        // cast to unicode key
        QString str = QString::fromUtf8(emscriptKey->key);
        ushort c = str.unicode()->toUpper().unicode(); // uppercase
        qtKey = static_cast<Qt::Key>(c);
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
    bool accepted = translator->processMouse(eventType,mouseEvent);
    QWasmEventDispatcher::maintainTimers();
    return accepted;
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

bool QWasmEventTranslator::processMouse(int eventType, const EmscriptenMouseEvent *mouseEvent)
{
    QPoint targetPoint(mouseEvent->targetX, mouseEvent->targetY);
    QPoint globalPoint = screen()->geometry().topLeft() + targetPoint;

    QEvent::Type buttonEventType = QEvent::None;
    Qt::MouseButton button = Qt::NoButton;
    Qt::KeyboardModifiers modifiers = translateMouseEventModifier(mouseEvent);

    QWindow *window2 = nullptr;
    if (resizeMode == QWasmWindow::ResizeNone)
        window2 = screen()->compositor()->windowAt(globalPoint, 5);

    if (window2 == nullptr) {
        window2 = lastWindow;
    } else {
        lastWindow = window2;
    }

    QPoint localPoint = window2->mapFromGlobal(globalPoint);
    bool interior = window2->geometry().contains(globalPoint);

    QWasmWindow *htmlWindow = static_cast<QWasmWindow*>(window2->handle());
    switch (eventType) {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
    {
        button = translateMouseButton(mouseEvent->button);

        if (window2)
            window2->requestActivate();

        pressedButtons.setFlag(button);

        pressedWindow = window2;
        buttonEventType = QEvent::MouseButtonPress;

        // button overview:
        // 0 = primary mouse button, usually left click
        // 1 = middle mouse button, usually mouse wheel
        // 2 = right mouse button, usually right click
        // from: https://w3c.github.io/uievents/#dom-mouseevent-button
        if (mouseEvent->button == 0) {
            if (!(htmlWindow->m_windowState & Qt::WindowFullScreen) && !(htmlWindow->m_windowState & Qt::WindowMaximized)) {
                if (htmlWindow && window2->flags().testFlag(Qt::WindowTitleHint) && htmlWindow->isPointOnTitle(globalPoint))
                    draggedWindow = window2;
                else if (htmlWindow && htmlWindow->isPointOnResizeRegion(globalPoint)) {
                    draggedWindow = window2;
                    resizeMode = htmlWindow->resizeModeAtPoint(globalPoint);
                    resizePoint = globalPoint;
                    resizeStartRect = window2->geometry();
                }
            }
        }

        htmlWindow->injectMousePressed(localPoint, globalPoint, button, modifiers);
        break;
    }
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
        button = translateMouseButton(mouseEvent->button);
        pressedButtons.setFlag(button, false);
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
            oldWindow->injectMouseReleased(localPoint, globalPoint, button, modifiers);
        else
            htmlWindow->injectMouseReleased(localPoint, globalPoint, button, modifiers);
        break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE: // drag event
    {
        buttonEventType = QEvent::MouseMove;

        if (htmlWindow && htmlWindow->isPointOnResizeRegion(globalPoint))
            window2->setCursor(cursorForMode(htmlWindow->resizeModeAtPoint(globalPoint)));

        if (!(htmlWindow->m_windowState & Qt::WindowFullScreen) && !(htmlWindow->m_windowState & Qt::WindowMaximized)) {
            if (resizeMode == QWasmWindow::ResizeNone && draggedWindow) {
                draggedWindow->setX(draggedWindow->x() + mouseEvent->movementX);
                draggedWindow->setY(draggedWindow->y() + mouseEvent->movementY);
            }

            if (resizeMode != QWasmWindow::ResizeNone && !(htmlWindow->m_windowState & Qt::WindowFullScreen)) {
                QPoint delta = QPoint(mouseEvent->targetX, mouseEvent->targetY) - resizePoint;
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
    bool accepted = true;
    if (window2 && interior) {
        accepted = QWindowSystemInterface::handleMouseEvent<QWindowSystemInterface::SynchronousDelivery>(
            window2, getTimestamp(), localPoint, globalPoint, pressedButtons, button, buttonEventType, modifiers);
    }
    return accepted;
}

int QWasmEventTranslator::focus_cb(int /*eventType*/, const EmscriptenFocusEvent */*focusEvent*/, void */*userData*/)
{
    return 0;
}

int QWasmEventTranslator::wheel_cb(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData)
{
    Q_UNUSED(eventType);

    QWasmEventTranslator *eventTranslator = static_cast<QWasmEventTranslator *>(userData);
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

    QWasmEventTranslator *translator = (QWasmEventTranslator*)userData;
    Qt::KeyboardModifiers modifiers = translator->translateMouseEventModifier(&mouseEvent);
    QPoint targetPoint(mouseEvent.targetX, mouseEvent.targetY);
    QPoint globalPoint = eventTranslator->screen()->geometry().topLeft() + targetPoint;

    QWindow *window2 = eventTranslator->screen()->compositor()->windowAt(globalPoint, 5);
    if (!window2)
        return 0;
    QPoint localPoint = window2->mapFromGlobal(globalPoint);

    QPoint pixelDelta;

    if (wheelEvent->deltaY != 0) pixelDelta.setY(wheelEvent->deltaY * scrollFactor);
    if (wheelEvent->deltaX != 0) pixelDelta.setX(wheelEvent->deltaX * scrollFactor);

    bool accepted = QWindowSystemInterface::handleWheelEvent(window2, getTimestamp(), localPoint,
                                             globalPoint, QPoint(), pixelDelta, modifiers);
    QWasmEventDispatcher::maintainTimers();
    return static_cast<int>(accepted);
}

int QWasmEventTranslator::touchCallback(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData)
{
    auto translator = reinterpret_cast<QWasmEventTranslator*>(userData);
    return translator->handleTouch(eventType, touchEvent);
}

int QWasmEventTranslator::handleTouch(int eventType, const EmscriptenTouchEvent *touchEvent)
{
    QList<QWindowSystemInterface::TouchPoint> touchPointList;
    touchPointList.reserve(touchEvent->numTouches);
    QWindow *window2;

    for (int i = 0; i < touchEvent->numTouches; i++) {

        const EmscriptenTouchPoint *touches = &touchEvent->touches[i];

        QPoint targetPoint(touches->targetX, touches->targetY);
        QPoint globalPoint = screen()->geometry().topLeft() + targetPoint;

        window2 = this->screen()->compositor()->windowAt(globalPoint, 5);
        if (window2 == nullptr)
            continue;

        QWindowSystemInterface::TouchPoint touchPoint;

        touchPoint.area = QRect(0, 0, 8, 8);
        touchPoint.id = touches->identifier;
        touchPoint.pressure = 1.0;

        touchPoint.area.moveCenter(globalPoint);

        const auto tp = pressedTouchIds.constFind(touchPoint.id);
        if (tp != pressedTouchIds.constEnd())
            touchPoint.normalPosition = tp.value();

        QPointF localPoint = QPointF(window2->mapFromGlobal(globalPoint));
        QPointF normalPosition(localPoint.x() / window2->width(),
                               localPoint.y() / window2->height());

        const bool stationaryTouchPoint = (normalPosition == touchPoint.normalPosition);
        touchPoint.normalPosition = normalPosition;

        switch (eventType) {
        case EMSCRIPTEN_EVENT_TOUCHSTART:
            if (tp != pressedTouchIds.constEnd()) {
                touchPoint.state = (stationaryTouchPoint
                                    ? QEventPoint::State::Stationary
                                    : QEventPoint::State::Updated);
            } else {
                touchPoint.state = QEventPoint::State::Pressed;
            }
            pressedTouchIds.insert(touchPoint.id, touchPoint.normalPosition);

            break;
        case EMSCRIPTEN_EVENT_TOUCHEND:
            touchPoint.state = QEventPoint::State::Released;
            pressedTouchIds.remove(touchPoint.id);
            break;
        case EMSCRIPTEN_EVENT_TOUCHMOVE:
            touchPoint.state = (stationaryTouchPoint
                                ? QEventPoint::State::Stationary
                                : QEventPoint::State::Updated);

            pressedTouchIds.insert(touchPoint.id, touchPoint.normalPosition);
            break;
        default:
            break;
        }

        touchPointList.append(touchPoint);
    }

    QFlags<Qt::KeyboardModifier> keyModifier = translatKeyModifier(touchEvent);

    bool accepted = QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(
                window2, getTimestamp(), touchDevice, touchPointList, keyModifier);

    if (eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL)
        accepted = QWindowSystemInterface::handleTouchCancelEvent(window2, getTimestamp(), touchDevice, keyModifier);

    QWasmEventDispatcher::maintainTimers();

    return static_cast<int>(accepted);
}

quint64 QWasmEventTranslator::getTimestamp()
{
    return emscripten_performance_now();
}

struct KeyMapping { Qt::Key from, to; };

constexpr KeyMapping tildeKeyTable[] = { // ~
    { Qt::Key_A, Qt::Key_Atilde },
    { Qt::Key_N, Qt::Key_Ntilde },
    { Qt::Key_O, Qt::Key_Otilde },
};
constexpr KeyMapping graveKeyTable[] = { // `
    { Qt::Key_A, Qt::Key_Agrave },
    { Qt::Key_E, Qt::Key_Egrave },
    { Qt::Key_I, Qt::Key_Igrave },
    { Qt::Key_O, Qt::Key_Ograve },
    { Qt::Key_U, Qt::Key_Ugrave },
};
constexpr KeyMapping acuteKeyTable[] = { // '
    { Qt::Key_A, Qt::Key_Aacute },
    { Qt::Key_E, Qt::Key_Eacute },
    { Qt::Key_I, Qt::Key_Iacute },
    { Qt::Key_O, Qt::Key_Oacute },
    { Qt::Key_U, Qt::Key_Uacute },
    { Qt::Key_Y, Qt::Key_Yacute },
};
constexpr KeyMapping diaeresisKeyTable[] = { // umlaut ¨
    { Qt::Key_A, Qt::Key_Adiaeresis },
    { Qt::Key_E, Qt::Key_Ediaeresis },
    { Qt::Key_I, Qt::Key_Idiaeresis },
    { Qt::Key_O, Qt::Key_Odiaeresis },
    { Qt::Key_U, Qt::Key_Udiaeresis },
    { Qt::Key_Y, Qt::Key_ydiaeresis },
};
constexpr KeyMapping circumflexKeyTable[] = { // ^
    { Qt::Key_A, Qt::Key_Acircumflex },
    { Qt::Key_E, Qt::Key_Ecircumflex },
    { Qt::Key_I, Qt::Key_Icircumflex },
    { Qt::Key_O, Qt::Key_Ocircumflex },
    { Qt::Key_U, Qt::Key_Ucircumflex },
};

static Qt::Key find_impl(const KeyMapping *first, const KeyMapping *last, Qt::Key key) noexcept
{
    while (first != last) {
        if (first->from == key)
            return first->to;
        ++first;
    }
    return Qt::Key_unknown;
}

template <size_t N>
static Qt::Key find(const KeyMapping (&map)[N], Qt::Key key) noexcept
{
    return find_impl(map, map + N, key);
}

Qt::Key QWasmEventTranslator::translateDeadKey(Qt::Key deadKey, Qt::Key accentBaseKey)
{
    Qt::Key wasmKey = Qt::Key_unknown;

    if (deadKey == Qt::Key_QuoteLeft ) {
        if (g_usePlatformMacSpecifics) { // ` macOS: Key_Dead_Grave
            wasmKey = find(graveKeyTable, accentBaseKey);
        } else {
            wasmKey = find(diaeresisKeyTable, accentBaseKey);
        }
        return wasmKey;
    }

    switch (deadKey) {
    //    case Qt::Key_QuoteLeft:
    case Qt::Key_O: // ´ Key_Dead_Grave
        wasmKey = find(graveKeyTable, accentBaseKey);
        break;
    case Qt::Key_E: // ´ Key_Dead_Acute
        wasmKey = find(acuteKeyTable, accentBaseKey);
        break;
    case Qt::Key_AsciiTilde:
    case Qt::Key_N:// Key_Dead_Tilde
        wasmKey = find(tildeKeyTable, accentBaseKey);
        break;
    case Qt::Key_U:// ¨ Key_Dead_Diaeresis
        wasmKey = find(diaeresisKeyTable, accentBaseKey);
        break;
    case Qt::Key_I:// macOS Key_Dead_Circumflex
    case Qt::Key_6:// linux
    case Qt::Key_Apostrophe:// linux
        wasmKey = find(circumflexKeyTable, accentBaseKey);
        break;
    default:
        break;

    };
    return wasmKey;
}

bool QWasmEventTranslator::processKeyboard(int eventType, const EmscriptenKeyboardEvent *keyEvent)
{
    Qt::Key qtKey = translateEmscriptKey(keyEvent);

    Qt::KeyboardModifiers modifiers = translateKeyboardEventModifier(keyEvent);

    QString keyText;
    QEvent::Type keyType = QEvent::None;
    switch (eventType) {
    case EMSCRIPTEN_EVENT_KEYPRESS:
    case EMSCRIPTEN_EVENT_KEYDOWN: // down
        keyType = QEvent::KeyPress;

        if (m_emDeadKey != Qt::Key_unknown) {

            Qt::Key transformedKey = translateDeadKey(m_emDeadKey, qtKey);

            if (transformedKey != Qt::Key_unknown)
                qtKey = transformedKey;

            if (keyEvent->shiftKey == 0) {
                for (auto it = KeyTbl.cbegin(); it != KeyTbl.end(); ++it) {
                    if (it != KeyTbl.end() && (qtKey == static_cast<Qt::Key>(it->qt))) {
                        keyText = it->em;
                        m_emDeadKey = Qt::Key_unknown;
                        break;
                    }
                }
            } else {
                for (auto it = DeadKeyShiftTbl.cbegin(); it != DeadKeyShiftTbl.end(); ++it) {
                    if (it != DeadKeyShiftTbl.end() && (qtKey == static_cast<Qt::Key>(it->qt))) {
                        keyText = it->em;
                        m_emDeadKey = Qt::Key_unknown;
                        break;
                    }
                }
            }
        }
        if (qstrncmp(keyEvent->key, "Dead", 4) == 0 || qtKey == Qt::Key_AltGr) {
            qtKey = translateEmscriptKey(keyEvent);
            m_emStickyDeadKey = true;
            if (keyEvent->shiftKey == 1 && qtKey == Qt::Key_QuoteLeft)
                qtKey = Qt::Key_AsciiTilde;
            m_emDeadKey = qtKey;
        }
        break;
    case EMSCRIPTEN_EVENT_KEYUP: // up
        keyType = QEvent::KeyRelease;
        if (m_emStickyDeadKey && qtKey != Qt::Key_Alt) {
            m_emStickyDeadKey = false;
        }
        break;
    default:
        break;
    };

    if (keyType == QEvent::None)
        return 0;

    QFlags<Qt::KeyboardModifier> mods = translateKeyboardEventModifier(keyEvent);

    // Clipboard fallback path: cut/copy/paste are handled by clipboard event
    // handlers if direct clipboard access is not available.
    if (!QWasmIntegration::get()->getWasmClipboard()->hasClipboardApi && modifiers & Qt::ControlModifier &&
        (qtKey == Qt::Key_X || qtKey == Qt::Key_C || qtKey == Qt::Key_V)) {
            return 0;
    }

    bool accepted = false;

    if (keyType == QEvent::KeyPress &&
            mods.testFlag(Qt::ControlModifier)
            && qtKey == Qt::Key_V) {
        QWasmIntegration::get()->getWasmClipboard()->readTextFromClipboard();
    } else {
        if (keyText.isEmpty())
            keyText = QString(keyEvent->key);
        if (keyText.size() > 1)
            keyText.clear();
        accepted = QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
                    0, keyType, qtKey, modifiers, keyText);
    }
    if (keyType == QEvent::KeyPress &&
            mods.testFlag(Qt::ControlModifier)
            && qtKey == Qt::Key_C) {
        QWasmIntegration::get()->getWasmClipboard()->writeTextToClipboard();
    }

    QWasmEventDispatcher::maintainTimers();

    return accepted;
}

QCursor QWasmEventTranslator::cursorForMode(QWasmWindow::ResizeMode m)
{
    switch (m) {
    case QWasmWindow::ResizeTopLeft:
    case QWasmWindow::ResizeBottomRight:
        return Qt::SizeFDiagCursor;
        break;
    case QWasmWindow::ResizeBottomLeft:
    case QWasmWindow::ResizeTopRight:
        return Qt::SizeBDiagCursor;
        break;
    case QWasmWindow::ResizeTop:
    case QWasmWindow::ResizeBottom:
        return Qt::SizeVerCursor;
        break;
    case QWasmWindow::ResizeLeft:
    case QWasmWindow::ResizeRight:
        return Qt::SizeHorCursor;
        break;
    case QWasmWindow::ResizeNone:
        return Qt::ArrowCursor;
        break;
    }
    return Qt::ArrowCursor;
}

QT_END_NAMESPACE
