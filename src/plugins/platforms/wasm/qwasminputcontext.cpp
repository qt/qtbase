// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwasminputcontext.h"
#include "qwasmwindow.h"
#include "qwasmaccessibility.h"

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
    emscripten::val inputType = event["inputType"];
    if (inputType.isNull() || inputType.isUndefined())
        return;
    const auto inputTypeString = inputType.as<std::string>();

    emscripten::val inputData = event["data"];
    QString inputStr = (!inputData.isNull() && !inputData.isUndefined())
        ? QString::fromEcmaString(inputData) : QString();


    // also do dataTransfer
    // containing rich text
    emscripten::val dataTr = event["dataTransfer"];
    if (!dataTr.isUndefined() && !dataTr.isNull()) {
        qCDebug(qLcQpaWasmInputContext) << "inputEvent dataTransfer" << inputStr;
        inputStr = QString::fromStdString(dataTr.
                                          call<emscripten::val>("getData",
                                                                std::string("text")).as<std::string>());
    }

    if (m_ignoreNextInput) {
        m_ignoreNextInput = false;
        return;
    }
    // There are many inputTypes for InputEvent
    // https://www.w3.org/TR/input-events-1/
    // Some of them should be implemented here later.

    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << "inputType : " << inputTypeString;

    if (!inputTypeString.compare("deleteContentBackward")) {
        const ResolvedRange resolved = resolveRange(getFirstRange());

        const int deleteFrom = resolved.replaceFrom <= 0 ? resolved.replaceFrom : -1;
        QInputMethodEvent e;
        e.setCommitString(QString(), deleteFrom, resolved.replaceLength);
        QCoreApplication::sendEvent(m_focusObject, &e);
    } else if (!inputTypeString.compare("deleteContentForward")) {
        QWindowSystemInterface::handleKeyEvent(0, QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
        QWindowSystemInterface::handleKeyEvent(0, QEvent::KeyRelease, Qt::Key_Delete, Qt::NoModifier);
    } else if (!inputTypeString.compare("insertCompositionText")) {

        qCDebug(qLcQpaWasmInputContext) << "insertCompositionText : " << inputStr;
        if (event["isComposing"].as<bool>()) {
            const CompositionRange range = getFirstRange();
            setPreeditString(inputStr);
            insertPreedit(range.end - range.start);
        }
    } else if (!inputTypeString.compare("insertReplacementText")) {

        qCDebug(qLcQpaWasmInputContext) << "insertReplacementText >>>>" << "inputString : " << inputStr;
        const CompositionRange range = getFirstRange();

        if (range.isValid) {
            const ResolvedRange resolved = resolveRange(range);
            replaceText(inputStr, resolved.replaceFrom, resolved.replaceLength);
        } else {
            QInputMethodQueryEvent queryEvent(Qt::ImQueryAll);
            QCoreApplication::sendEvent(m_focusObject, &queryEvent);
            int qCursorPosition = queryEvent.value(Qt::ImCursorPosition).toInt();
            QString textFieldString = queryEvent.value(Qt::ImTextBeforeCursor).toString();

            int spaceIndex = textFieldString.lastIndexOf(' ') + 1;
            int replaceIndex = (qCursorPosition - spaceIndex);
            replaceText(inputStr, -replaceIndex, replaceIndex);
        }
    } else if (!inputTypeString.compare("deleteCompositionText")) {
        setPreeditString("");
        insertPreedit();
    } else if (!inputTypeString.compare("insertFromComposition")) {
        setPreeditString(inputStr);
        insertPreedit();
    } else if (!inputTypeString.compare("insertText")) {
        const CompositionRange range = getFirstRange();

        if (range.isValid && range.start != range.end) {
            const ResolvedRange resolved = resolveRange(range);
            replaceText(inputStr, resolved.replaceFrom, resolved.replaceLength);
        } else {
            insertText(inputStr);
        }
    } else if (!inputTypeString.compare("insertFromPaste")) {
        insertText(inputStr);
    // These can be supported here,
    // But now, keyCallback in QWasmWindow
    // will take them as exceptions.
    //} else if (!inputTypeString.compare("deleteByCut")) {
    } else {
        qCWarning(qLcQpaWasmInputContext) << Q_FUNC_INFO << "inputType \"" <<
            inputType.as<std::string>() << "\" is not supported in Qt yet";
    }

    // Prevent the event from propagting to any listeners outside of the Qt window
    event.call<void>("stopPropagation");

    // The composition target range set by the preceding "beforeinput" event has
    // been consumed by this input event; clear it so a later input event without
    // a fresh beforeinput cannot reuse a stale range.
    m_compositionRange = emscripten::val::null();
}

void QWasmInputContext::compositionEndCallback(emscripten::val event)
{
    const auto inputStr = QString::fromEcmaString(event["data"]);

    if (preeditString().isEmpty()) // we get final results from inputCallback
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

    // Do nothing when starting composition
}

void QWasmInputContext::compositionUpdateCallback(emscripten::val event)
{
    const auto compositionStr = QString::fromEcmaString(event["data"]);
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << compositionStr;

    setPreeditString(compositionStr);
}

void QWasmInputContext::beforeInputCallback(emscripten::val event)
{
    m_compositionRange = event.call<emscripten::val>("getTargetRanges");

    if (const CompositionRange range = getFirstRange(); range.isValid) {
        qCDebug(qLcQpaWasmInputContext) << "startOffset" << range.start;
        qCDebug(qLcQpaWasmInputContext) << "endOffset" << range.end;
    }
}

// Returns the first of the target ranges from the most recent "beforeinput"
// event (InputEvent.getTargetRanges(), a possibly empty list of StaticRanges).
QWasmInputContext::CompositionRange QWasmInputContext::getFirstRange() const
{
    CompositionRange range;
    if (!m_compositionRange.isNull() && !m_compositionRange.isUndefined()
        && m_compositionRange["length"].as<int>() > 0) {
        emscripten::val first = m_compositionRange[0];
        range.start = first["startOffset"].as<int>();
        range.end = first["endOffset"].as<int>();
        range.isValid = true;
    }
    return range;
}

// Resolves a composition range against the current input cursor position for 
// the focus object. Returns (replaceFrom, replaceLength) which can be passed
// to QInputMethodEvent, where replaceFrom is an offset from the cursor position.
QWasmInputContext::ResolvedRange QWasmInputContext::resolveRange(const CompositionRange &range) const
{
    QInputMethodQueryEvent queryEvent(Qt::ImQueryAll);
    QCoreApplication::sendEvent(m_focusObject, &queryEvent);
    const int cursorPosition = queryEvent.value(Qt::ImCursorPosition).toInt();
    return { range.start - cursorPosition, range.end - range.start };
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
    if (QWasmAccessibility::isEnabled())
        return;

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
        m_inputElement["style"].set("width", "1px");
        m_inputElement["style"].set("height", "1px");
    }
}

void QWasmInputContext::updateInputElement()
{
    m_inputMethodAccepted = inputMethodAccepted();

    if (QWasmAccessibility::isEnabled())
        return;

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

// change inputmode to suit Qt imhints

    Qt::InputMethodHints imHints = static_cast<Qt::InputMethodHints>(query.value(Qt::ImHints).toInt());
    std::string inMode = "text";

    if (imHints & Qt::ImhDigitsOnly)
        inMode = "numeric";
    if (imHints & Qt::ImhFormattedNumbersOnly)
        inMode = "decimal";
    if (imHints & Qt::ImhDialableCharactersOnly)
        inMode = "tel";
    if (imHints & Qt::ImhEmailCharactersOnly)
        inMode = "email";
    if (imHints & Qt::ImhUrlCharactersOnly)
        inMode = "url";
    // search ??
    m_inputElement.set("inputMode",inMode);

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

void QWasmInputContext::setPreeditString(QString preeditStr)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << preeditStr;
    m_preeditString = preeditStr;
}

void QWasmInputContext::insertPreedit(int replaceLength)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << m_preeditString;
    if (replaceLength == 0)
        replaceLength = m_preeditString.length();

    QList<QInputMethodEvent::Attribute> attributes;
    {
        QInputMethodEvent::Attribute attr_cursor(QInputMethodEvent::Cursor,
                                                 0,
                                                 1);
        attributes.append(attr_cursor);

        QTextCharFormat format;
        format.setFontUnderline(true);
        format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        QInputMethodEvent::Attribute attr_format(QInputMethodEvent::TextFormat,
                                                0,
                                                replaceLength, format);
        attributes.append(attr_format);
    }

    QInputMethodEvent e(m_preeditString, attributes);
    QCoreApplication::sendEvent(m_focusObject, &e);
}

void QWasmInputContext::commitPreeditAndClear()
{
    if (m_preeditString.isEmpty())
        return;
    QInputMethodEvent e;
    e.setCommitString(m_preeditString);
    m_preeditString.clear();
    QCoreApplication::sendEvent(m_focusObject, &e);
}

void QWasmInputContext::insertText(QString inputStr, bool replace)
{ // commitString
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << inputStr << replace;
    Q_UNUSED(replace);
    if (!inputStr.isEmpty()) {
        const int replaceLen = 0;
        QInputMethodEvent e;
        e.setCommitString(inputStr, -replaceLen, replaceLen);
        QCoreApplication::sendEvent(m_focusObject, &e);
    }
}

/* This will replace the text in the focusobject at replaceFrom position, and replaceSize length
 with the text in inputStr. */

 void QWasmInputContext::replaceText(QString inputStr, int replaceFrom, int replaceSize)
 {
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO << inputStr << replaceFrom << replaceSize;

    QList<QInputMethodEvent::Attribute> attributes;
    {
        QInputMethodEvent::Attribute attr_cursor(QInputMethodEvent::Cursor,
                                                0, // start
                                                1); // length
        attributes.append(attr_cursor);

        QTextCharFormat format;
        format.setFontUnderline(true);
        format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        QInputMethodEvent::Attribute attr_format(QInputMethodEvent::TextFormat,
                                                0,
                                                replaceSize,
                                                format);
        attributes.append(attr_format);
    }

    QInputMethodEvent e1(QString(), attributes);
    e1.setCommitString(inputStr, replaceFrom, replaceSize);
    QCoreApplication::sendEvent(m_focusObject, &e1);

    m_preeditString.clear();
 }

QT_END_NAMESPACE
