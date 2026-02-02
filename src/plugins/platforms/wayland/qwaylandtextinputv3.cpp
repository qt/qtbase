// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandtextinputv3_p.h"

#include "qwaylandwindow_p.h"
#include "qwaylandinputmethodeventbuilder_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qpa/qplatforminputcontext.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QtGui/qpalette.h>

#include <QTextCharFormat>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcQpaWaylandTextInput, "qt.qpa.wayland.textinput")

namespace QtWaylandClient {

QWaylandTextInputv3::QWaylandTextInputv3(QWaylandDisplay *display,
                                         struct ::zwp_text_input_v3 *text_input)
    : QtWayland::zwp_text_input_v3(text_input)
{
    Q_UNUSED(display)

    if (version() >= ZWP_TEXT_INPUT_V3_SET_AVAILABLE_ACTIONS_SINCE_VERSION) {
        uint32_t availableActions[1] = {action_submit};
        const QByteArray availableActionsData = QByteArray::fromRawData(reinterpret_cast<char *>(availableActions), sizeof(availableActions));
        set_available_actions(availableActionsData);
    }
}

QWaylandTextInputv3::~QWaylandTextInputv3()
{
    destroy();
}

namespace {
const Qt::InputMethodQueries supportedQueries3 = Qt::ImEnabled |
                                                Qt::ImSurroundingText |
                                                Qt::ImCursorPosition |
                                                Qt::ImAnchorPosition |
                                                Qt::ImHints |
                                                Qt::ImCursorRectangle;
}

void QWaylandTextInputv3::zwp_text_input_v3_enter(struct ::wl_surface *surface)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "Trying to enable surface" << surface << "with focusing surface" << m_surface;

    if (m_surface == surface)
        return; // already enabled

    m_surface = surface;
    m_pendingPreeditString.clear();
    m_pendingCommitString.clear();
    m_pendingDeleteBeforeText = 0;
    m_pendingDeleteAfterText = 0;
    m_surroundingText.clear();
    m_cursor = 0;
    m_cursorPos = 0;
    m_anchorPos = 0;
    m_contentHint = 0;
    m_contentPurpose = 0;
    m_cursorRect = QRect();

    enable();
    updateState(supportedQueries3, update_state_enter);
}

void QWaylandTextInputv3::zwp_text_input_v3_leave(struct ::wl_surface *surface)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;

    if (!m_surface)
        return; // Nothing to leave

    // surface == nullptr means the wl_surface proxy was already freed (surface destroyed
    // before the leave event was dispatched); treat it as normal teardown, not a mismatch.
    if (surface && m_surface != surface)
        qCWarning(qLcQpaWaylandTextInput()) << Q_FUNC_INFO << "Got leave event for surface" << surface << "with focusing surface" << m_surface;

    m_currentPreeditString.clear();
    m_surface = nullptr;
    disable();
    commit();
}

void QWaylandTextInputv3::zwp_text_input_v3_preedit_string(const QString &text, int32_t cursorBegin, int32_t cursorEnd)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << text << cursorBegin << cursorEnd;
    if (!m_surface) {
        qCWarning(qLcQpaWaylandTextInput) << "Got preedit_string event without entering a surface";
        return;
    }

    if (!QGuiApplication::focusObject())
        return;

    m_pendingPreeditString.text = text;
    m_pendingPreeditString.cursorBegin = cursorBegin;
    m_pendingPreeditString.cursorEnd = cursorEnd;
}

void QWaylandTextInputv3::zwp_text_input_v3_commit_string(const QString &text)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << text;
    if (!m_surface) {
        qCWarning(qLcQpaWaylandTextInput) << "Got commit_string event without entering a surface";
        return;
    }

    if (!QGuiApplication::focusObject())
        return;

    m_pendingCommitString = text;
}

void QWaylandTextInputv3::zwp_text_input_v3_delete_surrounding_text(uint32_t beforeText, uint32_t afterText)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << beforeText << afterText;
    if (!m_surface) {
        qCWarning(qLcQpaWaylandTextInput) << "Got delete_surrounding_text event without entering a surface";
        return;
    }

    if (!QGuiApplication::focusObject())
        return;

    m_pendingDeleteBeforeText = beforeText;
    m_pendingDeleteAfterText = afterText;
}

void QWaylandTextInputv3::zwp_text_input_v3_done(uint32_t serial)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "with serial" << serial << m_currentSerial;

    if (!m_surface)
        return;

    // This is a case of double click.
    // text_input_v3 will ignore this done signal and just keep the selection of the clicked word.
    if (m_cursorPos != m_anchorPos && (m_pendingDeleteBeforeText != 0 || m_pendingDeleteAfterText != 0)) {
        qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "Ignore done";
        m_pendingDeleteBeforeText = 0;
        m_pendingDeleteAfterText = 0;
        m_pendingPreeditString.clear();
        m_pendingCommitString.clear();
        return;
    }

    QObject *focusObject = QGuiApplication::focusObject();
    if (!focusObject)
        return;

    if (!m_surface) {
        qCWarning(qLcQpaWaylandTextInput) << Q_FUNC_INFO << serial << "Surface is not enabled yet";
        return;
    }

    if ((m_pendingPreeditString == m_currentPreeditString)
           && (m_pendingCommitString.isEmpty() && m_pendingDeleteBeforeText == 0
                                               && m_pendingDeleteAfterText == 0)) {
        // Current done doesn't need additional updates
        m_pendingPreeditString.clear();
        return;
    }

    const int newCursorIndex = QWaylandInputMethodEventBuilder::indexFromWayland(m_pendingPreeditString.text, m_pendingPreeditString.cursorBegin);
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "PREEDIT" << m_pendingPreeditString.text << newCursorIndex;

    QList<QInputMethodEvent::Attribute> attributes;
    {
        if (m_pendingPreeditString.cursorBegin == -1 &&
            m_pendingPreeditString.cursorEnd == -1) {
            QInputMethodEvent::Attribute attribute(QInputMethodEvent::Cursor,
                                                    0,
                                                    0); // hide cursor
            attributes.append(attribute);
        } else if (m_pendingPreeditString.cursorBegin != 0 ||
                m_pendingPreeditString.cursorEnd != 0) {
            // Current supported cursor shape is just line.
            // It means, cursorEnd and cursorBegin are the same.
            QInputMethodEvent::Attribute attribute1(QInputMethodEvent::Cursor,
                                                    newCursorIndex,
                                                    1); // keep visible
            attributes.append(attribute1);
        }

        if (version() == 1) {
            // only use single underline style for now
            QTextCharFormat format;
            format.setFontUnderline(true);
            format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
            QInputMethodEvent::Attribute attribute2(QInputMethodEvent::TextFormat,
                                                    0,
                                                    m_pendingPreeditString.text.length(), format);
            attributes.append(attribute2);
        } else {
            for (const StyleHint &prededitStyle : std::as_const(m_pendingPreeditString.styleHints)) {
                int begin = QWaylandInputMethodEventBuilder::indexFromWayland(m_pendingPreeditString.text, prededitStyle.begin);
                int end = QWaylandInputMethodEventBuilder::indexFromWayland(m_pendingPreeditString.text, prededitStyle.end);
                QTextCharFormat format;

                // styles taken from https://github.com/ibus/ibus/wiki/Wayland-Colors
                switch (prededitStyle.hint) {
                case preedit_hint_whole:
                    format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                    break;
                case preedit_hint_selection:
                    format.setForeground(QPalette().highlightedText());
                    format.setBackground(QPalette().highlight());
                    break;
                case preedit_hint_prediction:
                    // this is meant to be normal text on a light grey
                    format.setBackground(QPalette().placeholderText());
                    break;
                case preedit_hint_prefix:
                    format.setForeground(QColor("#F90F0F"));
                    break;
                case preedit_hint_suffix:
                    format.setForeground(QColor("#1EDC1A"));
                    break;
                case preedit_hint_spelling_error:
                    format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
                    format.setUnderlineColor(QColor("#A40000"));
                    break;
                case preedit_hint_compose_error:
                    format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
                    format.setUnderlineColor(QColor("#FF00FF"));
                    break;
                }

                QInputMethodEvent::Attribute attribute2(QInputMethodEvent::TextFormat,
                                                        begin,
                                                        end - begin,
                                                        format);
                attributes.append(attribute2);
            }
        }
    }
    QInputMethodEvent event(m_pendingPreeditString.text, attributes);

    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "DELETE" << m_pendingDeleteBeforeText << m_pendingDeleteAfterText;
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "COMMIT" << m_pendingCommitString;

    int replaceFrom = 0;
    int replaceLength = 0;
    if (m_pendingDeleteBeforeText != 0 || m_pendingDeleteAfterText != 0) {
        // A workaround for reselection
        // It will disable redundant commit after reselection
        m_condReselection = true;
        const QByteArray &utf8 = QStringView{m_surroundingText}.toUtf8();
        if (m_cursorPos < int(m_pendingDeleteBeforeText)) {
            replaceFrom = -QString::fromUtf8(QByteArrayView{utf8}.first(m_pendingDeleteBeforeText)).size();
            replaceLength = QString::fromUtf8(QByteArrayView{utf8}.first(m_pendingDeleteBeforeText + m_pendingDeleteAfterText)).size();
        } else {
            replaceFrom = -QString::fromUtf8(QByteArrayView{utf8}.sliced(m_cursorPos - m_pendingDeleteBeforeText, m_pendingDeleteBeforeText)).size();
            replaceLength = QString::fromUtf8(QByteArrayView{utf8}.sliced(m_cursorPos - m_pendingDeleteBeforeText, m_pendingDeleteBeforeText + m_pendingDeleteAfterText)).size();
        }
    }

    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "DELETE from " << replaceFrom << " length " << replaceLength;
    event.setCommitString(m_pendingCommitString,
                          replaceFrom,
                          replaceLength);
    m_currentPreeditString = m_pendingPreeditString;
    m_pendingPreeditString.clear();
    m_pendingCommitString.clear();
    m_pendingDeleteBeforeText = 0;
    m_pendingDeleteAfterText = 0;
    QCoreApplication::sendEvent(focusObject, &event);

    if (serial == m_currentSerial)
        updateState(supportedQueries3, update_state_full);
}

void QWaylandTextInputv3::zwp_text_input_v3_language(const QString &language)
{
    const QLocale locale(language);
    if (m_locale != locale) {
        m_locale = locale;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitLocaleChanged();
    }
}

void QWaylandTextInputv3::zwp_text_input_v3_action(uint32_t action, uint32_t serial)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << action << serial;
    switch (action) {
    case action_none:
        break;
    case action_submit: {
        if (!QGuiApplication::focusObject())
            break;
        QKeyEvent keyPressEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &keyPressEvent);
        QKeyEvent keyReleaseEvent(QEvent::KeyRelease, Qt::Key_Enter, Qt::NoModifier);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &keyReleaseEvent);
        break;
    }
    default:
        // it's a bug as we declare our supported actions on startup
        qCWarning(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "Unexpected text input action received. This is a compositor bug";
        break;
    }
}

void QWaylandTextInputv3::zwp_text_input_v3_preedit_hint(uint32_t begin, uint32_t end, uint32_t hint)
{
    Q_ASSERT(hint <= preedit_hint_compose_error);
    // they have to be cached as raw values, as we can't work out cursor indexes without the text
    m_pendingPreeditString.styleHints.append({begin, end, static_cast<preedit_hint>(hint)});
}


void QWaylandTextInputv3::reset()
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;

    m_pendingPreeditString.clear();
}

void QWaylandTextInputv3::commit()
{
    m_currentSerial = (m_currentSerial < UINT_MAX) ? m_currentSerial + 1U: 0U;

    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "with serial" << m_currentSerial;
    QtWayland::zwp_text_input_v3::commit();
}

void QWaylandTextInputv3::updateState(Qt::InputMethodQueries queries, uint32_t flags)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << queries << flags;

    if (!QGuiApplication::focusObject())
        return;

    if (!QGuiApplication::focusWindow() || !QGuiApplication::focusWindow()->handle())
        return;

    auto *window = static_cast<QWaylandWindow *>(QGuiApplication::focusWindow()->handle());
    auto *surface = window->wlSurface();
    if (!surface || (surface != m_surface))
        return;

    queries &= supportedQueries3;
    bool needsCommit = false;

    QInputMethodQueryEvent event(queries);
    QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);

    // For some reason, a query for Qt::ImSurroundingText gives an empty string even though it is not.
    if (!(queries & Qt::ImSurroundingText) && event.value(Qt::ImSurroundingText).toString().isEmpty()) {
        return;
    }

    if (queries & Qt::ImCursorRectangle) {
        const QRect &cRect = event.value(Qt::ImCursorRectangle).toRect();
        const QRect &windowRect = QGuiApplication::inputMethod()->inputItemTransform().mapRect(cRect);
        const QRect &nativeRect = QHighDpi::toNativePixels(windowRect, QGuiApplication::focusWindow());
        const QMargins margins = window->clientSideMargins();
        const QRect &surfaceRect = nativeRect.translated(margins.left(), margins.top());
        if (surfaceRect != m_cursorRect) {
            set_cursor_rectangle(surfaceRect.x(), surfaceRect.y(), surfaceRect.width(), surfaceRect.height());
            m_cursorRect = surfaceRect;
            needsCommit = true;
        }
    }

    if ((queries & Qt::ImSurroundingText) || (queries & Qt::ImCursorPosition) || (queries & Qt::ImAnchorPosition)) {
        QString text = event.value(Qt::ImSurroundingText).toString();
        int cursor = event.value(Qt::ImCursorPosition).toInt();
        int anchor = event.value(Qt::ImAnchorPosition).toInt();

        qCDebug(qLcQpaWaylandTextInput) << "Original surrounding_text from InputMethodQuery: " << text << cursor << anchor;

        // Make sure text is not too big
        // surround_text cannot exceed 4000byte in wayland protocol
        // The worst case will be supposed here.
        const int MAX_MESSAGE_SIZE = 4000;

        const QByteArray utf8 = text.toUtf8();
        const int textSize = utf8.size();
        if (textSize > MAX_MESSAGE_SIZE) {
            qCDebug(qLcQpaWaylandTextInput) << "SurroundText size is over "
                                            << MAX_MESSAGE_SIZE
                                            << " byte, some text will be clipped.";
            const int selectionStart = qMin(cursor, anchor);
            const int selectionEnd = qMax(cursor, anchor);
            const int selectionLength = selectionEnd - selectionStart;
            QByteArray selection = QStringView{text}.sliced(selectionStart, selectionLength).toUtf8();
            const int selectionSize = selection.size();
            // If selection is bigger than 4000 byte, it is fixed to 4000 byte.
            // anchor will be moved in the 4000 byte boundary.
            if (selectionSize > MAX_MESSAGE_SIZE) {
                if (anchor > cursor) {
                    cursor = 0;
                    anchor = MAX_MESSAGE_SIZE;
                    text = QString::fromUtf8(QByteArrayView{selection}.sliced(0, MAX_MESSAGE_SIZE));
                } else {
                    anchor = 0;
                    cursor = MAX_MESSAGE_SIZE;
                    text = QString::fromUtf8(QByteArrayView{selection}.sliced(selectionSize - MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE));
                }
            } else {
                // This is not optimal in some cases.
                // For examples, if the cursor position and
                // the selectionEnd are close to the end of the surround text,
                // the tail of the text might always be clipped.
                // However all the cases of over 4000 byte are just exceptions.
                int selEndSize = QStringView{text}.first(selectionEnd).toUtf8().size();
                cursor = QWaylandInputMethodEventBuilder::indexToWayland(text, cursor);
                anchor = QWaylandInputMethodEventBuilder::indexToWayland(text, anchor);
                if (selEndSize < MAX_MESSAGE_SIZE) {
                    text = QString::fromUtf8(QByteArrayView{utf8}.first(MAX_MESSAGE_SIZE));
                } else {
                    const int startOffset = selEndSize - MAX_MESSAGE_SIZE;
                    text = QString::fromUtf8(QByteArrayView{utf8}.sliced(startOffset, MAX_MESSAGE_SIZE));
                    cursor -= startOffset;
                    anchor -= startOffset;
                }
            }
        } else {
            cursor = QWaylandInputMethodEventBuilder::indexToWayland(text, cursor);
            anchor = QWaylandInputMethodEventBuilder::indexToWayland(text, anchor);
        }
        qCDebug(qLcQpaWaylandTextInput) << "Modified surrounding_text: " << text << cursor << anchor;

        if (m_surroundingText != text || m_cursorPos != cursor || m_anchorPos != anchor) {
            qCDebug(qLcQpaWaylandTextInput) << "Current surrounding_text: " << m_surroundingText << m_cursorPos << m_anchorPos;
            qCDebug(qLcQpaWaylandTextInput) << "New surrounding_text: " << text << cursor << anchor;

            set_surrounding_text(text, cursor, anchor);

            // A workaround in the case of reselection
            // It will work when re-clicking a preedit text
            if (m_condReselection) {
                qCDebug(qLcQpaWaylandTextInput) << "\"commit\" is disabled when Reselection by changing focus";
                m_condReselection = false;
                needsCommit = false;

            }

            m_surroundingText = text;
            m_cursorPos = cursor;
            m_anchorPos = anchor;
            m_cursor = cursor;
        }
    }

    if (queries & Qt::ImHints) {
        QWaylandInputMethodContentType contentType = QWaylandInputMethodContentType::convertV3(static_cast<Qt::InputMethodHints>(event.value(Qt::ImHints).toInt()));

        if (version() >= ZWP_TEXT_INPUT_V3_CONTENT_HINT_PREEDIT_SHOWN_SINCE_VERSION)
            contentType.hint |= ZWP_TEXT_INPUT_V3_CONTENT_HINT_PREEDIT_SHOWN;

        qCDebug(qLcQpaWaylandTextInput) << m_contentHint << contentType.hint;
        qCDebug(qLcQpaWaylandTextInput) << m_contentPurpose << contentType.purpose;

        if (m_contentHint != contentType.hint || m_contentPurpose != contentType.purpose) {
            qCDebug(qLcQpaWaylandTextInput) << "set_content_type: " << contentType.hint << contentType.purpose;
            set_content_type(contentType.hint, contentType.purpose);

            m_contentHint = contentType.hint;
            m_contentPurpose = contentType.purpose;
            needsCommit = true;
        }
    }

    if (needsCommit)
        commit();
}

void QWaylandTextInputv3::setCursorInsidePreedit(int cursor)
{
    Q_UNUSED(cursor);
}

bool QWaylandTextInputv3::isInputPanelVisible() const
{
    return false;
}

QRectF QWaylandTextInputv3::keyboardRect() const
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;
    return m_cursorRect;
}

QLocale QWaylandTextInputv3::locale() const
{
    return m_locale;
}

Qt::LayoutDirection QWaylandTextInputv3::inputDirection() const
{
    return Qt::LeftToRight;
}

void QWaylandTextInputv3::showInputPanel()
{
    if (version() >= ZWP_TEXT_INPUT_V3_SHOW_INPUT_PANEL_SINCE_VERSION)
        show_input_panel();
}

void QWaylandTextInputv3::hideInputPanel()
{
    if (version() >= ZWP_TEXT_INPUT_V3_HIDE_INPUT_PANEL_SINCE_VERSION)
        hide_input_panel();
}



}

QT_END_NAMESPACE
