// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <emscripten/bind.h>

#include "qwasminputcontext.h"
#include "qwasmintegration.h"
#include "qwasmplatform.h"
#include <QRectF>
#include <qpa/qplatforminputcontext.h>
#include "qwasmscreen.h"
#include <qguiapplication.h>
#include <qwindow.h>
#include <QKeySequence>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE
using namespace qstdweb;

static void inputCallback(emscripten::val event)
{
    // android sends all the characters typed since the user started typing in this element
    int length = event["target"]["value"]["length"].as<int>();
    if (length <= 0)
        return;

    emscripten::val _incomingCharVal = event["data"];
    if (_incomingCharVal != emscripten::val::undefined() && _incomingCharVal != emscripten::val::null()) {

        QString str = QString::fromStdString(_incomingCharVal.as<std::string>());
        QWasmInputContext *wasmInput =
                reinterpret_cast<QWasmInputContext*>(event["target"]["data-qinputcontext"].as<quintptr>());
        wasmInput->inputStringChanged(str, EMSCRIPTEN_EVENT_KEYDOWN, wasmInput);
    }
    // this clears the input string, so backspaces do not send a character
    // but stops suggestions
    event["target"].set("value", "");
}

EMSCRIPTEN_BINDINGS(clipboard_module) {
    function("qtInputContextCallback", &inputCallback);
}

QWasmInputContext::QWasmInputContext()
{
    emscripten::val document = emscripten::val::global("document");
    m_inputElement = document.call<emscripten::val>("createElement", std::string("input"));
    m_inputElement.set("type", "text");
    m_inputElement.set("style", "position:absolute;left:-1000px;top:-1000px"); // offscreen
    m_inputElement.set("contenteditable","true");

    if (platform() == Platform::Android || platform() == Platform::Windows) {
        const std::string inputType = platform() == Platform::Windows ? "textInput" : "input";

        document.call<void>("addEventListener", inputType,
                                  emscripten::val::module_property("qtInputContextCallback"),
                                  emscripten::val(false));
        m_inputElement.set("data-qinputcontext",
                           emscripten::val(quintptr(reinterpret_cast<void *>(this))));
        emscripten::val body = document["body"];
        body.call<void>("appendChild", m_inputElement);

        // Enter is sent through target window, let's just handle this here
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, (void *)this, 1,
                                        &inputMethodKeyboardCallback);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, (void *)this, 1,
                                      &inputMethodKeyboardCallback);
    }

    if (platform() == Platform::MacOS || platform() == Platform::iOS) {
        auto callback = [=](emscripten::val) {
            m_inputElement["parentElement"].call<void>("removeChild", m_inputElement);
            inputPanelIsOpen = false;
        };
        m_blurEventHandler.reset(new EventCallback(m_inputElement, "blur", callback));
    }

    QObject::connect(qGuiApp, &QGuiApplication::focusWindowChanged, this,
                     &QWasmInputContext::focusWindowChanged);
}

QWasmInputContext::~QWasmInputContext()
{
    if (platform() == Platform::Android || platform() == Platform::Windows)
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 0, NULL);
}

void QWasmInputContext::focusWindowChanged(QWindow *focusWindow)
{
    m_focusWindow = focusWindow;
}

emscripten::val QWasmInputContext::inputHandlerElementForFocusedWindow()
{
    if (!m_focusWindow)
        return emscripten::val::undefined();
    return static_cast<QWasmWindow *>(m_focusWindow->handle())->inputHandlerElement();
}

void QWasmInputContext::update(Qt::InputMethodQueries queries)
{
    QPlatformInputContext::update(queries);
}

void QWasmInputContext::showInputPanel()
{
    if (platform() == Platform::Windows
        && !inputPanelIsOpen) { // call this only once for win32
            m_inputElement.call<void>("focus");
            return;
        }
    // this is called each time the keyboard is touched

    // Add the input element as a child of the screen for the
    // currently focused window and give it focus. The browser
    // will not display the input element, but mobile browsers
    // should display the virtual keyboard. Key events will be
    // captured by the keyboard event handler installed on the
    // screen element.

    if (platform() == Platform::MacOS // keep for compatibility
     || platform() == Platform::iOS
     || platform() == Platform::Windows) {
        emscripten::val inputWrapper = inputHandlerElementForFocusedWindow();
        if (inputWrapper.isUndefined())
            return;
        inputWrapper.call<void>("appendChild", m_inputElement);
    }

    m_inputElement.call<void>("focus");
    inputPanelIsOpen = true;
}

void QWasmInputContext::hideInputPanel()
{
    if (QWasmIntegration::get()->touchPoints < 1)
        return;
    m_inputElement.call<void>("blur");
    inputPanelIsOpen = false;
}

void QWasmInputContext::inputStringChanged(QString &inputString, int eventType, QWasmInputContext *context)
{
    Q_UNUSED(context)
    QKeySequence keys = QKeySequence::fromString(inputString);
    Qt::Key thisKey = keys[0].key();

    // synthesize this keyevent as android is not normal
    if (inputString.size() > 2 && (thisKey < Qt::Key_F35
                                   || thisKey > Qt::Key_Back)) {
        inputString.clear();
    }
    if (inputString == QStringLiteral("Escape")) {
        thisKey = Qt::Key_Escape;
        inputString.clear();
    } else if (thisKey == Qt::Key(0)) {
        thisKey = Qt::Key_Return;
    }

    QWindowSystemInterface::handleKeyEvent(
        0, eventType == EMSCRIPTEN_EVENT_KEYDOWN ? QEvent::KeyPress : QEvent::KeyRelease,
        thisKey, keys[0].keyboardModifiers(),
        eventType == EMSCRIPTEN_EVENT_KEYDOWN ? inputString : QStringLiteral(""));
}

int QWasmInputContext::inputMethodKeyboardCallback(int eventType,
                                               const EmscriptenKeyboardEvent *keyEvent,
                                               void *userData)
{
    // we get Enter, Backspace and function keys via emscripten on target window
    QString strKey(keyEvent->key);
    if (strKey == "Unidentified" || strKey == "Process")
        return false;

    QWasmInputContext *wasmInput = reinterpret_cast<QWasmInputContext*>(userData);
    wasmInput->inputStringChanged(strKey, eventType, wasmInput);

    return true;
}

QT_END_NAMESPACE
