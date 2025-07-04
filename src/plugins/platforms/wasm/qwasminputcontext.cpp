// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasminputcontext.h"
#include "qwasmwindow.h"

#include <QRectF>
#include <QLoggingCategory>
#include <qguiapplication.h>
#include <qwindow.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qwindowsysteminterface.h>
#if QT_CONFIG(clipboard)
#include <QClipboard>
#endif
#include <QtGui/qtextobject.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcQpaWasmInputContext, "qt.qpa.wasm.inputcontext")

using namespace qstdweb;

void QWasmInputContext::inputCallback(emscripten::val event)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "isComposing : " << event["isComposing"].as<bool>();

    emscripten::val inputType = event["inputType"];
    if (inputType.isNull() || inputType.isUndefined())
        return;
    const auto inputTypeString = inputType.as<std::string>();

    emscripten::val inputData = event["data"];
    QString inputStr = (!inputData.isNull() && !inputData.isUndefined())
        ? QString::fromEcmaString(inputData) : QString();

    // There are many inputTypes for InputEvent
    // https://www.w3.org/TR/input-events-1/
    // Some of them should be implemented here later.
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "inputType : " << inputTypeString;
    if (!inputTypeString.compare("deleteContentBackward")) {
        QWindowSystemInterface::handleKeyEvent(0, QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
        QWindowSystemInterface::handleKeyEvent(0, QEvent::KeyRelease, Qt::Key_Backspace, Qt::NoModifier);
        event.call<void>("stopImmediatePropagation");
        return;
    } else if (!inputTypeString.compare("deleteContentForward")) {
        QWindowSystemInterface::handleKeyEvent(0, QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
        QWindowSystemInterface::handleKeyEvent(0, QEvent::KeyRelease, Qt::Key_Delete, Qt::NoModifier);
        event.call<void>("stopImmediatePropagation");
        return;
    } else if (!inputTypeString.compare("insertCompositionText")) {
        qCDebug(qLcQpaWasmInputContext) << "inputString : " << inputStr;
        insertPreedit();
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
        insertText(inputStr, true);

        event.call<void>("stopImmediatePropagation");
        return;
    } else if (!inputTypeString.compare("deleteCompositionText")) {
        setPreeditString("", 0);
        insertPreedit();
        event.call<void>("stopImmediatePropagation");
        return;
    } else if (!inputTypeString.compare("insertFromComposition")) {
        setPreeditString(inputStr, 0);
        insertPreedit();
        event.call<void>("stopImmediatePropagation");
        return;
    } else if (!inputTypeString.compare("insertText")) {
        insertText(inputStr);
        event.call<void>("stopImmediatePropagation");
#if QT_CONFIG(clipboard)
    } else if (!inputTypeString.compare("insertFromPaste")) {
        insertText(QGuiApplication::clipboard()->text());
        event.call<void>("stopImmediatePropagation");
    // These can be supported here,
    // But now, keyCallback in QWasmWindow
    // will take them as exceptions.
    //} else if (!inputTypeString.compare("deleteByCut")) {
#endif
    } else {
        qCWarning(qLcQpaWasmInputContext) << Q_FUNC_INFO << "inputType \"" <<
            inputType.as<std::string>() << "\" is not supported in Qt yet";
    }
}

void QWasmInputContext::compositionEndCallback(emscripten::val event)
{
    const auto inputStr = QString::fromEcmaString(event["data"]);
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << inputStr;

    if (preeditString().isEmpty())
        return;

    if (inputStr != preeditString()) {
        qCWarning(qLcQpaWasmInputContext) << Q_FUNC_INFO
                    << "Composition string" << inputStr
                    << "is differ from" << preeditString();
    }
    commitPreeditAndClear();
}

void QWasmInputContext::compositionStartCallback(emscripten::val event)
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

void QWasmInputContext::compositionUpdateCallback(emscripten::val event)
{
    const auto compositionStr = QString::fromEcmaString(event["data"]);
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << compositionStr;

    // WA for IOS.
    // Not sure now because I cannot test it anymore.
//    int replaceSize = 0;
//    emscripten::val win = emscripten::val::global("window");
//    emscripten::val sel = win.call<emscripten::val>("getSelection");
//    if (!sel.isNull() && !sel.isUndefined()
//            && sel["rangeCount"].as<int>() > 0) {
//        QInputMethodQueryEvent queryEvent(Qt::ImQueryAll);
//        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &queryEvent);
//        qCDebug(qLcQpaWasmInputContext) << "Qt surrounding text: " << queryEvent.value(Qt::ImSurroundingText).toString();
//        qCDebug(qLcQpaWasmInputContext) << "Qt current selection: " << queryEvent.value(Qt::ImCurrentSelection).toString();
//        qCDebug(qLcQpaWasmInputContext) << "Qt text before cursor: " << queryEvent.value(Qt::ImTextBeforeCursor).toString();
//        qCDebug(qLcQpaWasmInputContext) << "Qt text after cursor: " << queryEvent.value(Qt::ImTextAfterCursor).toString();
//
//        const QString &selectedStr = QString::fromEcmaString(sel.call<emscripten::val>("toString"));
//        const auto &preeditStr = preeditString();
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
//    setPreeditString(compositionStr, replaceSize);
    setPreeditString(compositionStr, 0);
}

QWasmInputContext::QWasmInputContext()
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;
}

QWasmInputContext::~QWasmInputContext()
{
}

void QWasmInputContext::update(Qt::InputMethodQueries queries)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << queries;

    if ((queries & Qt::ImEnabled) && (inputMethodAccepted() != m_inputMethodAccepted)) {
        if (m_focusObject && !m_preeditString.isEmpty())
            commitPreeditAndClear();
        updateInputElement();
    }
    QPlatformInputContext::update(queries);
}

void QWasmInputContext::showInputPanel()
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;

    // Note: showInputPanel not necessarily called, we shall
    // still accept input if we have a focus object and
    // inputMethodAccepted().
    updateInputElement();
}

void QWasmInputContext::updateGeometry()
{
    if (m_inputElement.isNull())
        return;

    const QWindow *focusWindow = QGuiApplication::focusWindow();
    if (!m_focusObject || !focusWindow || !m_inputMethodAccepted) {
        m_inputElement["style"].set("left", "0px");
        m_inputElement["style"].set("top", "0px");
    } else {
        Q_ASSERT(focusWindow);
        Q_ASSERT(m_focusObject);
        Q_ASSERT(m_inputMethodAccepted);

        const QRect inputItemRectangle = QPlatformInputContext::inputItemRectangle().toRect();
        qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "propagating inputItemRectangle:" << inputItemRectangle;
        m_inputElement["style"].set("left", std::to_string(inputItemRectangle.x()) + "px");
        m_inputElement["style"].set("top", std::to_string(inputItemRectangle.y()) + "px");
        m_inputElement["style"].set("width", std::to_string(inputItemRectangle.width()) + "px");
        m_inputElement["style"].set("height", std::to_string(inputItemRectangle.height()) + "px");
    }
}

void QWasmInputContext::updateInputElement()
{
    m_inputMethodAccepted = inputMethodAccepted();

    // Mobile devices can dismiss keyboard/IME and focus is still on input.
    // Successive clicks on the same input should open the keyboard/IME.
    updateGeometry();

    // If there is no focus object, or no visible input panel, remove focus
    QWasmWindow *focusWindow = QWasmWindow::fromWindow(QGuiApplication::focusWindow());
    if (!m_focusObject || !focusWindow || !m_inputMethodAccepted) {
        if (!m_inputElement.isNull()) {
            m_inputElement.set("value", "");
            m_inputElement.set("inputMode", std::string("none"));
        }

        if (focusWindow) {
            focusWindow->focus();
        } else {
            if (!m_inputElement.isNull())
                m_inputElement.call<void>("blur");
        }

        m_inputElement = emscripten::val::null();
        return;
    }

    Q_ASSERT(focusWindow);
    Q_ASSERT(m_focusObject);
    Q_ASSERT(m_inputMethodAccepted);

    m_inputElement = focusWindow->inputElement();

    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << QRectF::fromDOMRect(m_inputElement.call<emscripten::val>("getBoundingClientRect"));

    // Set the text input
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

    QInputMethodQueryEvent query((Qt::InputMethodQueries(Qt::ImHints)));
    QCoreApplication::sendEvent(m_focusObject, &query);
    if (Qt::InputMethodHints(query.value(Qt::ImHints).toInt()).testFlag(Qt::ImhHiddenText))
        m_inputElement.set("type", "password");
    else
        m_inputElement.set("type", "text");

    m_inputElement.set("inputMode", std::string("text"));

    m_inputElement.call<void>("focus");
}

void QWasmInputContext::setFocusObject(QObject *object)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << object << inputMethodAccepted();

    // Commit the previous composition before change m_focusObject
    if (m_focusObject && !m_preeditString.isEmpty())
        commitPreeditAndClear();

    m_focusObject = object;

    updateInputElement();
    QPlatformInputContext::setFocusObject(object);
}

void QWasmInputContext::hideInputPanel()
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;

    // hide only if m_focusObject does not exist
    if (!m_focusObject)
        updateInputElement();
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
