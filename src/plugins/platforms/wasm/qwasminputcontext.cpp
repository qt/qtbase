/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <emscripten/bind.h>

#include "qwasminputcontext.h"
#include "qwasmintegration.h"
#include <QRectF>
#include <qpa/qplatforminputcontext.h>
#include "qwasmeventtranslator.h"
#include "qwasmscreen.h"
#include <qguiapplication.h>
#include <qwindow.h>
#include <QKeySequence>
#include <qpa/qwindowsysteminterface.h>
using namespace qstdweb;

static void inputCallback(emscripten::val event)
{
    QString str = QString::fromStdString(event["target"]["value"].as<std::string>());
    QWasmInputContext *wasmInput =
            reinterpret_cast<QWasmInputContext*>(event["target"]["data-context"].as<quintptr>());
    wasmInput->inputStringChanged(str, wasmInput);

    // this stops suggestions
    // but allows us to send only one character like a normal keyboard
    event["target"].set("value", "");
}

EMSCRIPTEN_BINDINGS(clipboard_module) {
    function("qt_InputContextCallback", &inputCallback);
}

QWasmInputContext::QWasmInputContext()
{
    emscripten::val document = emscripten::val::global("document");
    m_inputElement = document.call<emscripten::val>("createElement", std::string("input"));
    m_inputElement.set("type", "text");
    m_inputElement.set("style", "position:absolute;left:-1000px;top:-1000px"); // offscreen
    m_inputElement.set("contentaediable","true");

    if (QWasmIntegration::get()->platform == QWasmIntegration::AndroidPlatform) {
        emscripten::val body = document["body"];
        body.call<void>("appendChild", m_inputElement);

        m_inputElement.call<void>("addEventListener", std::string("input"),
                                  emscripten::val::module_property("qt_InputContextCallback"),
                                  emscripten::val(false));
        m_inputElement.set("data-context",
                           emscripten::val(quintptr(reinterpret_cast<void *>(this))));

        // android sends Enter through target window, let's just handle this here
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, (void *)this, 1,
                                        &androidKeyboardCallback);

    }
    if (QWasmIntegration::get()->platform == QWasmIntegration::MacOSPlatform)
     {
        auto callback = [=](emscripten::val) {
            m_inputElement["parentElement"].call<void>("removeChild", m_inputElement); };
        m_blurEventHandler.reset(new EventCallback(m_inputElement, "blur", callback));
        inputPanelIsOpen = false;
    }

    QObject::connect(qGuiApp, &QGuiApplication::focusWindowChanged, this,
                     &QWasmInputContext::focusWindowChanged);
}

QWasmInputContext::~QWasmInputContext()
{
    if (QWasmIntegration::get()->platform == QWasmIntegration::AndroidPlatform)
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 0, NULL);
}

void QWasmInputContext::focusWindowChanged(QWindow *focusWindow)
{
    m_focusWindow = focusWindow;
}

emscripten::val QWasmInputContext::focusCanvas()
{
    if (!m_focusWindow)
        return emscripten::val::undefined();
    QScreen *screen = m_focusWindow->screen();
    if (!screen)
        return emscripten::val::undefined();
    return QWasmScreen::get(screen)->canvas();
}

void QWasmInputContext::update(Qt::InputMethodQueries queries)
{
    QPlatformInputContext::update(queries);
}

void QWasmInputContext::showInputPanel()
{
    if (QWasmIntegration::get()->platform == QWasmIntegration::WindowsPlatform
        && inputPanelIsOpen) // call this only once for win32
        return;
    // this is called each time the keyboard is touched

    // Add the input element as a child of the canvas for the
    // currently focused window and give it focus. The browser
    // will not display the input element, but mobile browsers
    // should display the virtual keyboard. Key events will be
    // captured by the keyboard event handler installed on the
    // canvas.

    if (QWasmIntegration::get()->platform == QWasmIntegration::MacOSPlatform
     || QWasmIntegration::get()->platform == QWasmIntegration::WindowsPlatform) {
        emscripten::val canvas = focusCanvas();
        if (canvas == emscripten::val::undefined())
            return;
        canvas.call<void>("appendChild", m_inputElement);
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

void QWasmInputContext::inputStringChanged(QString &inputString, QWasmInputContext *context)
{
    Q_UNUSED(context)
    QKeySequence keys = QKeySequence::fromString(inputString);
    // synthesize this keyevent as android is not normal
    QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
                0, QEvent::KeyPress,keys[0].key(), keys[0].keyboardModifiers(), inputString);
}

int QWasmInputContext::androidKeyboardCallback(int eventType,
                                               const EmscriptenKeyboardEvent *keyEvent,
                                               void *userData)
{
    Q_UNUSED(eventType)
    QString strKey(keyEvent->key);
    if (strKey == "Unidentified")
        return false;
    QWasmInputContext *wasmInput = reinterpret_cast<QWasmInputContext*>(userData);
    wasmInput->inputStringChanged(strKey, wasmInput);

    return true;
}
