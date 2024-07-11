// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasminputcontext.h"

#include <QRectF>
#include <QLoggingCategory>
#include <qguiapplication.h>
#include <qwindow.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qwindowsysteminterface.h>
#include <QClipboard>
#include <QtGui/qtextobject.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcQpaWasmInputContext, "qt.qpa.wasm.inputcontext")

using namespace qstdweb;

static void inputCallback(emscripten::val event)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "isComposing : " << event["isComposing"].as<bool>();

    QString inputStr = (event["data"] != emscripten::val::null()
                        && event["data"] != emscripten::val::undefined()) ?
        QString::fromStdString(event["data"].as<std::string>()) : QString();

    QWasmInputContext *wasmInput =
        reinterpret_cast<QWasmInputContext *>(event["target"]["data-qinputcontext"].as<quintptr>());

    emscripten::val inputType = event["inputType"];
    if (inputType != emscripten::val::null()
            && inputType != emscripten::val::undefined()) {
        const auto inputTypeString = inputType.as<std::string>();
        // There are many inputTypes for InputEvent
        // https://www.w3.org/TR/input-events-1/
        // Some of them should be implemented here later.
        qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "inputType : " << inputTypeString;
        if (!inputTypeString.compare("deleteContentBackward")) {
            QWindowSystemInterface::handleKeyEvent(0,
                                                   QEvent::KeyPress,
                                                   Qt::Key_Backspace,
                                                   Qt::NoModifier);
            QWindowSystemInterface::handleKeyEvent(0,
                                                   QEvent::KeyRelease,
                                                   Qt::Key_Backspace,
                                                   Qt::NoModifier);
            event.call<void>("stopImmediatePropagation");
            return;
        } else if (!inputTypeString.compare("deleteContentForward")) {
            QWindowSystemInterface::handleKeyEvent(0,
                                                   QEvent::KeyPress,
                                                   Qt::Key_Delete,
                                                   Qt::NoModifier);
            QWindowSystemInterface::handleKeyEvent(0,
                                                   QEvent::KeyRelease,
                                                   Qt::Key_Delete,
                                                   Qt::NoModifier);
            event.call<void>("stopImmediatePropagation");
            return;
        } else if (!inputTypeString.compare("insertCompositionText")) {
            qCDebug(qLcQpaWasmInputContext) << "inputString : " << inputStr;
            wasmInput->insertPreedit();
            event.call<void>("stopImmediatePropagation");
            return;
        } else if (!inputTypeString.compare("insertReplacementText")) {
            qCDebug(qLcQpaWasmInputContext) << "inputString : " << inputStr;
            //auto ranges = event.call<emscripten::val>("getTargetRanges");
            //qCDebug(qLcQpaWasmInputContext) << ranges["length"].as<int>();
            // WA For Korean IME
            // insertReplacementText should have targetRanges but
            // Safari cannot have it and just it seems to be supposed
            // to replace previous input.
            wasmInput->insertText(inputStr, true);

            event.call<void>("stopImmediatePropagation");
            return;
        } else if (!inputTypeString.compare("deleteCompositionText")) {
            wasmInput->setPreeditString("", 0);
            wasmInput->insertPreedit();
            event.call<void>("stopImmediatePropagation");
            return;
        } else if (!inputTypeString.compare("insertFromComposition")) {
            wasmInput->setPreeditString(inputStr, 0);
            wasmInput->insertPreedit();
            event.call<void>("stopImmediatePropagation");
            return;
        } else if (!inputTypeString.compare("insertText")) {
            wasmInput->insertText(inputStr);
            event.call<void>("stopImmediatePropagation");
        } else if (!inputTypeString.compare("insertFromPaste")) {
            wasmInput->insertText(QGuiApplication::clipboard()->text());
            event.call<void>("stopImmediatePropagation");
        // These can be supported here,
        // But now, keyCallback in QWasmWindow
        // will take them as exceptions.
        //} else if (!inputTypeString.compare("deleteByCut")) {
        } else {
            qCWarning(qLcQpaWasmInputContext) << Q_FUNC_INFO << "inputType \"" << inputType.as<std::string>() << "\" is not supported in Qt yet";
        }
    }
}

static void compositionEndCallback(emscripten::val event)
{
    const auto inputStr = QString::fromStdString(event["data"].as<std::string>());
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << inputStr;

    QWasmInputContext *wasmInput =
        reinterpret_cast<QWasmInputContext *>(event["target"]["data-qinputcontext"].as<quintptr>());

    if (wasmInput->preeditString().isEmpty())
        return;

    if (inputStr != wasmInput->preeditString()) {
        qCWarning(qLcQpaWasmInputContext) << Q_FUNC_INFO
                    << "Composition string" << inputStr
                    << "is differ from" << wasmInput->preeditString();
    }
    wasmInput->commitPreeditAndClear();
}

static void compositionStartCallback(emscripten::val event)
{
    Q_UNUSED(event);
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;

    // Do nothing when starting composition
}

/*
// Test implementation
static void beforeInputCallback(emscripten::val event)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;

    auto ranges = event.call<emscripten::val>("getTargetRanges");
    auto length = ranges["length"].as<int>();
    for (auto i = 0; i < length; i++) {
        qCDebug(qLcQpaWasmInputContext) << ranges.call<emscripten::val>("get", i)["startOffset"].as<int>();
        qCDebug(qLcQpaWasmInputContext) << ranges.call<emscripten::val>("get", i)["endOffset"].as<int>();
    }
}
*/

static void compositionUpdateCallback(emscripten::val event)
{
    const auto compositionStr = QString::fromStdString(event["data"].as<std::string>());
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << compositionStr;

    QWasmInputContext *wasmInput =
        reinterpret_cast<QWasmInputContext *>(event["target"]["data-qinputcontext"].as<quintptr>());

    // WA for IOS.
    // Not sure now because I cannot test it anymore.
//    int replaceSize = 0;
//    emscripten::val win = emscripten::val::global("window");
//    emscripten::val sel = win.call<emscripten::val>("getSelection");
//    if (sel != emscripten::val::null()
//            && sel != emscripten::val::undefined()
//            && sel["rangeCount"].as<int>() > 0) {
//        QInputMethodQueryEvent queryEvent(Qt::ImQueryAll);
//        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &queryEvent);
//        qCDebug(qLcQpaWasmInputContext) << "Qt surrounding text: " << queryEvent.value(Qt::ImSurroundingText).toString();
//        qCDebug(qLcQpaWasmInputContext) << "Qt current selection: " << queryEvent.value(Qt::ImCurrentSelection).toString();
//        qCDebug(qLcQpaWasmInputContext) << "Qt text before cursor: " << queryEvent.value(Qt::ImTextBeforeCursor).toString();
//        qCDebug(qLcQpaWasmInputContext) << "Qt text after cursor: " << queryEvent.value(Qt::ImTextAfterCursor).toString();
//
//        const QString &selectedStr = QString::fromUtf8(sel.call<emscripten::val>("toString").as<std::string>());
//        const auto &preeditStr = wasmInput->preeditString();
//        qCDebug(qLcQpaWasmInputContext) << "Selection.type : " << sel["type"].as<std::string>();
//        qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "Selected: " << selectedStr;
//        qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "PreeditString: " << preeditStr;
//        if (!sel["type"].as<std::string>().compare("Range")) {
//            QString surroundingTextBeforeCursor = queryEvent.value(Qt::ImTextBeforeCursor).toString();
//            if (surroundingTextBeforeCursor.endsWith(selectedStr)) {
//                replaceSize = selectedStr.size();
//                qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "Current Preedit: " << preeditStr << replaceSize;
//            }
//        }
//        emscripten::val range = sel.call<emscripten::val>("getRangeAt", 0);
//        qCDebug(qLcQpaWasmInputContext) << "Range.startOffset : " << range["startOffset"].as<int>();
//        qCDebug(qLcQpaWasmInputContext) << "Range.endOffset : " << range["endOffset"].as<int>();
//    }
//
//    wasmInput->setPreeditString(compositionStr, replaceSize);
    wasmInput->setPreeditString(compositionStr, 0);
}

static void copyCallback(emscripten::val event)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;

    QClipboard *clipboard = QGuiApplication::clipboard();
    QString inputStr = clipboard->text();
    qCDebug(qLcQpaWasmInputContext) << "QClipboard : " << inputStr;
    event["clipboardData"].call<void>("setData",
                                      emscripten::val("text/plain"),
                                      inputStr.toStdString());
    event.call<void>("preventDefault");
    event.call<void>("stopImmediatePropagation");
}

static void cutCallback(emscripten::val event)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;

    QClipboard *clipboard = QGuiApplication::clipboard();
    QString inputStr = clipboard->text();
    qCDebug(qLcQpaWasmInputContext) << "QClipboard : " << inputStr;
    event["clipboardData"].call<void>("setData",
                                      emscripten::val("text/plain"),
                                      inputStr.toStdString());
    event.call<void>("preventDefault");
    event.call<void>("stopImmediatePropagation");
}

static void pasteCallback(emscripten::val event)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;

    emscripten::val clipboardData = event["clipboardData"].call<emscripten::val>("getData", emscripten::val("text/plain"));
    QString clipboardStr = QString::fromStdString(clipboardData.as<std::string>());
    qCDebug(qLcQpaWasmInputContext) << "wasm clipboard : " << clipboardStr;
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard->text() != clipboardStr)
        clipboard->setText(clipboardStr);

    // propagate to input event (insertFromPaste)
}

EMSCRIPTEN_BINDINGS(wasminputcontext_module) {
    function("qtCompositionEndCallback", &compositionEndCallback);
    function("qtCompositionStartCallback", &compositionStartCallback);
    function("qtCompositionUpdateCallback", &compositionUpdateCallback);
    function("qtInputCallback", &inputCallback);
    //function("qtBeforeInputCallback", &beforeInputCallback);

    function("qtCopyCallback", &copyCallback);
    function("qtCutCallback", &cutCallback);
    function("qtPasteCallback", &pasteCallback);
}

QWasmInputContext::QWasmInputContext()
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;
    emscripten::val document = emscripten::val::global("document");
    // This 'input' can be an issue to handle multiple lines,
    // 'textarea' can be used instead.
    m_inputElement = document.call<emscripten::val>("createElement", std::string("input"));
    m_inputElement.set("type", "text");
    m_inputElement.set("contenteditable","true");

    m_inputElement["style"].set("position", "absolute");
    m_inputElement["style"].set("left", 0);
    m_inputElement["style"].set("top", 0);
    m_inputElement["style"].set("opacity", 0);
    m_inputElement["style"].set("display", "");
    m_inputElement["style"].set("z-index", -2);

    m_inputElement.set("data-qinputcontext",
                       emscripten::val(quintptr(reinterpret_cast<void *>(this))));
    emscripten::val body = document["body"];
    body.call<void>("appendChild", m_inputElement);

    m_inputElement.call<void>("addEventListener", std::string("compositionstart"),
                              emscripten::val::module_property("qtCompositionStartCallback"),
                              emscripten::val(false));
    m_inputElement.call<void>("addEventListener", std::string("compositionupdate"),
                              emscripten::val::module_property("qtCompositionUpdateCallback"),
                              emscripten::val(false));
    m_inputElement.call<void>("addEventListener", std::string("compositionend"),
                              emscripten::val::module_property("qtCompositionEndCallback"),
                              emscripten::val(false));
    m_inputElement.call<void>("addEventListener", std::string("input"),
                              emscripten::val::module_property("qtInputCallback"),
                              emscripten::val(false));
    //m_inputElement.call<void>("addEventListener", std::string("beforeinput"),
    //                          emscripten::val::module_property("qtBeforeInputCallback"),
    //                          emscripten::val(false));

    // Clipboard for InputContext
    m_inputElement.call<void>("addEventListener", std::string("cut"),
                              emscripten::val::module_property("qtCutCallback"),
                              emscripten::val(false));
    m_inputElement.call<void>("addEventListener", std::string("copy"),
                              emscripten::val::module_property("qtCopyCallback"),
                              emscripten::val(false));
    m_inputElement.call<void>("addEventListener", std::string("paste"),
                              emscripten::val::module_property("qtPasteCallback"),
                              emscripten::val(false));
}

QWasmInputContext::~QWasmInputContext()
{
}

void QWasmInputContext::update(Qt::InputMethodQueries queries)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << queries;

    QPlatformInputContext::update(queries);
}

void QWasmInputContext::showInputPanel()
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;

    if (!inputMethodAccepted())
        return;

    m_inputElement.call<void>("focus");
    m_usingTextInput = true;

    QWindow *window = QGuiApplication::focusWindow();
    if (!window || !m_focusObject)
        return;

    const QRect cursorRectangle = QPlatformInputContext::cursorRectangle().toRect();
    if (!cursorRectangle.isValid())
        return;
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "cursorRectangle: " << cursorRectangle;
    const QPoint &globalPos = window->mapToGlobal(cursorRectangle.topLeft());
    const auto styleLeft = std::to_string(globalPos.x()) + "px";
    const auto styleTop = std::to_string(globalPos.y()) + "px";
    m_inputElement["style"].set("left", styleLeft);
    m_inputElement["style"].set("top", styleTop);

    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << QRectF::fromDOMRect(m_inputElement.call<emscripten::val>("getBoundingClientRect"));

    QInputMethodQueryEvent queryEvent(Qt::ImQueryAll);
    QCoreApplication::sendEvent(m_focusObject, &queryEvent);
    qCDebug(qLcQpaWasmInputContext) << "Qt surrounding text: " << queryEvent.value(Qt::ImSurroundingText).toString();
    qCDebug(qLcQpaWasmInputContext) << "Qt current selection: " << queryEvent.value(Qt::ImCurrentSelection).toString();
    qCDebug(qLcQpaWasmInputContext) << "Qt text before cursor: " << queryEvent.value(Qt::ImTextBeforeCursor).toString();
    qCDebug(qLcQpaWasmInputContext) << "Qt text after cursor: " << queryEvent.value(Qt::ImTextAfterCursor).toString();
    qCDebug(qLcQpaWasmInputContext) << "Qt cursor position: " << queryEvent.value(Qt::ImCursorPosition).toInt();
    qCDebug(qLcQpaWasmInputContext) << "Qt anchor position: " << queryEvent.value(Qt::ImAnchorPosition).toInt();

    m_inputElement.set("value", queryEvent.value(Qt::ImSurroundingText).toString().toStdString());

    m_inputElement.set("selectionStart", queryEvent.value(Qt::ImAnchorPosition).toUInt());
    m_inputElement.set("selectionEnd", queryEvent.value(Qt::ImCursorPosition).toUInt());
}

void QWasmInputContext::setFocusObject(QObject *object)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << object << inputMethodAccepted();

    // Commit the previous composition before change m_focusObject
    if (m_focusObject && !m_preeditString.isEmpty())
        commitPreeditAndClear();

    if (inputMethodAccepted()) {
        m_inputElement.call<void>("focus");
        m_usingTextInput = true;

        m_focusObject = object;
    } else {
        m_inputElement.call<void>("blur");
        m_usingTextInput = false;

        m_focusObject = nullptr;
    }
    QPlatformInputContext::setFocusObject(object);
}

void QWasmInputContext::hideInputPanel()
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;

    // hide only if m_focusObject does not exist
    if (!m_focusObject) {
        m_inputElement.call<void>("blur");
        m_usingTextInput = false;
    }
}

void QWasmInputContext::setPreeditString(QString preeditStr, int replaceSize)
{
    m_preeditString = preeditStr;
    m_replaceSize = replaceSize;
}

void QWasmInputContext::insertPreedit()
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << m_preeditString;

    QList<QInputMethodEvent::Attribute> attributes;
    {
        QInputMethodEvent::Attribute attr_cursor(QInputMethodEvent::Cursor,
                                                 m_preeditString.length(),
                                                 1);
        attributes.append(attr_cursor);

        QTextCharFormat format;
        format.setFontUnderline(true);
        format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        QInputMethodEvent::Attribute attr_format(QInputMethodEvent::TextFormat,
                                                 0,
                                                 m_preeditString.length(), format);
        attributes.append(attr_format);
    }

    QInputMethodEvent e(m_preeditString, attributes);
    if (m_replaceSize > 0)
        e.setCommitString("", -m_replaceSize, m_replaceSize);
    QCoreApplication::sendEvent(m_focusObject, &e);
}

void QWasmInputContext::commitPreeditAndClear()
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << m_preeditString;

    if (m_preeditString.isEmpty())
        return;
    QInputMethodEvent e;
    e.setCommitString(m_preeditString);
    m_preeditString.clear();
    QCoreApplication::sendEvent(m_focusObject, &e);
}

void QWasmInputContext::insertText(QString inputStr, bool replace)
{
    Q_UNUSED(replace);
    if (!inputStr.isEmpty()) {
        const int replaceLen = 0;
        QInputMethodEvent e;
        e.setCommitString(inputStr, -replaceLen, replaceLen);
        QCoreApplication::sendEvent(m_focusObject, &e);
    }
}

QT_END_NAMESPACE
