// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmevent.h"

#include "qwasmkeytranslator.h"

#include <QtCore/private/qmakearray_p.h>
#include <QtCore/private/qstringiterator_p.h>
#include <QtCore/qregularexpression.h>

QT_BEGIN_NAMESPACE

namespace {
constexpr std::string_view WebDeadKeyValue = "Dead";

bool isDeadKeyEvent(const char *key)
{
    return qstrncmp(key, WebDeadKeyValue.data(), WebDeadKeyValue.size()) == 0;
}

Qt::Key getKeyFromCode(const std::string &code)
{
    if (auto mapping = QWasmKeyTranslator::mapWebKeyTextToQtKey(code.c_str()))
        return *mapping;

    static QRegularExpression regex(QString(QStringLiteral(R"re((?:Key|Digit)(\w))re")));
    const auto codeQString = QString::fromStdString(code);
    const auto match = regex.match(codeQString);

    if (!match.hasMatch())
        return Qt::Key_unknown;

    constexpr size_t CharacterIndex = 1;
    return static_cast<Qt::Key>(match.capturedView(CharacterIndex).at(0).toLatin1());
}

Qt::Key webKeyToQtKey(const std::string &code, const std::string &key, bool isDeadKey,
                      QFlags<Qt::KeyboardModifier> modifiers)
{
    if (isDeadKey) {
        auto mapped = getKeyFromCode(code);
        switch (mapped) {
        case Qt::Key_U:
            return Qt::Key_Dead_Diaeresis;
        case Qt::Key_E:
            return Qt::Key_Dead_Acute;
        case Qt::Key_I:
            return Qt::Key_Dead_Circumflex;
        case Qt::Key_N:
            return Qt::Key_Dead_Tilde;
        case Qt::Key_QuoteLeft:
            return modifiers.testFlag(Qt::ShiftModifier) ? Qt::Key_Dead_Tilde : Qt::Key_Dead_Grave;
        case Qt::Key_6:
            return Qt::Key_Dead_Circumflex;
        case Qt::Key_Apostrophe:
            return modifiers.testFlag(Qt::ShiftModifier) ? Qt::Key_Dead_Diaeresis
                                                         : Qt::Key_Dead_Acute;
        case Qt::Key_AsciiTilde:
            return Qt::Key_Dead_Tilde;
        default:
            return Qt::Key_unknown;
        }
    } else if (auto mapping = QWasmKeyTranslator::mapWebKeyTextToQtKey(key.c_str())) {
        if (modifiers.testFlag(Qt::ShiftModifier) && (*mapping == Qt::Key::Key_Tab))
            *mapping = Qt::Key::Key_Backtab;
        return *mapping;
    }

    // cast to unicode key
    QString str = QString::fromUtf8(key.c_str()).toUpper();
    if (str.length() > 1)
        return Qt::Key_unknown;

    QStringIterator i(str);
    return static_cast<Qt::Key>(i.next(0));
}

QFlags<Qt::KeyboardModifier> getKeyboardModifiers(const emscripten::val &event)
{
    QFlags<Qt::KeyboardModifier> keyModifier = Qt::NoModifier;
    if (event["shiftKey"].as<bool>())
        keyModifier |= Qt::ShiftModifier;
    if (event["ctrlKey"].as<bool>())
        keyModifier |= platform() == Platform::MacOS ? Qt::MetaModifier : Qt::ControlModifier;
    if (event["altKey"].as<bool>())
        keyModifier |= Qt::AltModifier;
    if (event["metaKey"].as<bool>())
        keyModifier |= platform() == Platform::MacOS ? Qt::ControlModifier : Qt::MetaModifier;
    if (event["constructor"]["name"].as<std::string>() == "KeyboardEvent" &&
        event["location"].as<unsigned int>() == DOM_KEY_LOCATION_NUMPAD) {
        keyModifier |= Qt::KeypadModifier;
    }
    return keyModifier;
}

} // namespace

Event::Event(EventType type, emscripten::val webEvent)
    : webEvent(webEvent), type(type)
{
}

bool Event::isTargetedForQtElement() const
{
    // Check event target via composedPath, which returns the true path even
    // if the browser retargets the event for Qt's shadow DOM container. This
    // is needed to avoid capturing the pointer in cases where foreign html
    // elements are embedded inside Qt's shadow DOM.
    emscripten::val path = webEvent.call<emscripten::val>("composedPath");
    QString topElementClassName = QString::fromEcmaString(path[0]["className"]);
    return topElementClassName.startsWith("qt-"); // .e.g. qt-window-canvas
}

KeyEvent::KeyEvent(EventType type, emscripten::val event, QWasmDeadKeySupport *deadKeySupport) : Event(type, event)
{
    const auto code = event["code"].as<std::string>();
    const auto webKey = event["key"].as<std::string>();
    deadKey = isDeadKeyEvent(webKey.c_str());
    autoRepeat = event["repeat"].as<bool>();
    modifiers = getKeyboardModifiers(event);
    key = webKeyToQtKey(code, webKey, deadKey, modifiers);
    isComposing = event["isComposing"].as<bool>();
    keyCode = event["keyCode"].as<int>();

    text = QString::fromUtf8(webKey);

    // Alt + keypad number -> insert utf-8 character
    // The individual numbers shall not be inserted but
    // on some platforms they are if numlock is
    // activated
    if ((modifiers & Qt::AltModifier) && (modifiers & Qt::KeypadModifier))
        text.clear();

    if (text.size() > 1)
        text.clear();

    if (key == Qt::Key_Tab)
        text = "\t";

    deadKeySupport->applyDeadKeyTranslations(this);
}

MouseEvent::MouseEvent(EventType type, emscripten::val event) : Event(type, event)
{
    mouseButton = MouseEvent::buttonFromWeb(event["button"].as<int>());
    mouseButtons = MouseEvent::buttonsFromWeb(event["buttons"].as<unsigned short>());
    // The current button state (event.buttons) may be out of sync for some PointerDown
    // events where the "down" state is very brief, for example taps on Apple trackpads.
    // Qt expects that the current button state is in sync with the event, so we sync
    // it up here.
    if (type == EventType::PointerDown)
        mouseButtons |= mouseButton;
    localPoint = QPointF(event["offsetX"].as<qreal>(), event["offsetY"].as<qreal>());
    pointInPage = QPointF(event["pageX"].as<qreal>(), event["pageY"].as<qreal>());
    pointInViewport = QPointF(event["clientX"].as<qreal>(), event["clientY"].as<qreal>());
    modifiers = getKeyboardModifiers(event);
}

PointerEvent::PointerEvent(EventType type, emscripten::val event) : MouseEvent(type, event)
{
    pointerId = event["pointerId"].as<int>();
    pointerType = ([type = event["pointerType"].as<std::string>()]() {
        if (type == "mouse")
            return PointerType::Mouse;
        if (type == "touch")
            return PointerType::Touch;
        if (type == "pen")
            return PointerType::Pen;
        return PointerType::Other;
    })();
    width = event["width"].as<qreal>();
    height = event["height"].as<qreal>();
    pressure = event["pressure"].as<qreal>();
    tiltX = event["tiltX"].as<qreal>();
    tiltY = event["tiltY"].as<qreal>();
    tangentialPressure = event["tangentialPressure"].as<qreal>();
    twist = event["twist"].as<qreal>();
    isPrimary = event["isPrimary"].as<bool>();
}

DragEvent::DragEvent(EventType type, emscripten::val event, QWindow *window)
    : MouseEvent(type, event), dataTransfer(event["dataTransfer"]), targetWindow(window)
{
    dropAction = ([event]() {
        const std::string effect = event["dataTransfer"]["dropEffect"].as<std::string>();

        if (effect == "copy")
            return Qt::CopyAction;
        else if (effect == "move")
            return Qt::MoveAction;
        else if (effect == "link")
            return Qt::LinkAction;
        return Qt::IgnoreAction;
    })();
}

void DragEvent::cancelDragStart()
{
    Q_ASSERT_X(type == EventType::DragStart, Q_FUNC_INFO, "Only supported for DragStart");
    webEvent.call<void>("preventDefault");
}

void DragEvent::acceptDragOver()
{
    Q_ASSERT_X(type == EventType::DragOver, Q_FUNC_INFO, "Only supported for DragOver");
   webEvent.call<void>("preventDefault");
}

void DragEvent::acceptDrop()
{
    Q_ASSERT_X(type == EventType::Drop, Q_FUNC_INFO, "Only supported for Drop");
    webEvent.call<void>("preventDefault");
}

WheelEvent::WheelEvent(EventType type, emscripten::val event) : MouseEvent(type, event)
{
    deltaMode = ([event]() {
        const int deltaMode = event["deltaMode"].as<int>();
        const auto jsWheelEventType = emscripten::val::global("WheelEvent");
        if (deltaMode == jsWheelEventType["DOM_DELTA_PIXEL"].as<int>())
            return DeltaMode::Pixel;
        else if (deltaMode == jsWheelEventType["DOM_DELTA_LINE"].as<int>())
            return DeltaMode::Line;
        return DeltaMode::Page;
    })();

    delta = QPointF(event["deltaX"].as<qreal>(), event["deltaY"].as<qreal>());

    webkitDirectionInvertedFromDevice = event["webkitDirectionInvertedFromDevice"].as<bool>();
}

QT_END_NAMESPACE
