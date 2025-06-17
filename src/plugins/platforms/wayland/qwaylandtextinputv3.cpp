// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandtextinputv3_p.h"

#include "qwaylandwindow_p.h"
#include "qwaylandinputmethodeventbuilder_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcQpaWaylandTextInput, "qt.qpa.wayland.textinput")

namespace QtWaylandClient {

QWaylandTextInputv3::QWaylandTextInputv3(QWaylandDisplay *display,
                                         struct ::zwp_text_input_v3 *text_input)
    : QtWayland::zwp_text_input_v3(text_input)
{
    Q_UNUSED(display)
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

void QWaylandTextInputv3::enableSurface(::wl_surface *surface)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << surface;

    if (m_surface == surface)
        return; // already enabled
    if (m_surface)
        qCWarning(qLcQpaWaylandTextInput()) << Q_FUNC_INFO << "Try to enable surface" << surface << "with focusing surface" << m_surface;

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

void QWaylandTextInputv3::disableSurface(::wl_surface *surface)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << surface;

    if (!m_surface)
        return; // already disabled
    if (m_surface != surface)
        qCWarning(qLcQpaWaylandTextInput()) << Q_FUNC_INFO << "Try to disable surface" << surface << "with focusing surface" << m_surface;

    m_currentPreeditString.clear();
    m_surface = nullptr;
    disable();
    commit();
}

void QWaylandTextInputv3::zwp_text_input_v3_enter(struct ::wl_surface *surface)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << m_surface << surface;

    if (m_surface)
        qCWarning(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "Got enter event without leaving a surface " << m_surface;

    enableSurface(surface);
}

void QWaylandTextInputv3::zwp_text_input_v3_leave(struct ::wl_surface *surface)
{
    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO;

    if (!m_surface)
        return; // Nothing to leave

    if (m_surface != surface)
        qCWarning(qLcQpaWaylandTextInput()) << Q_FUNC_INFO << "Got leave event for surface" << surface << "with focusing surface" << m_surface;

    disableSurface(surface);
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
    m_pendingPreeditString.cursorBegin = QWaylandInputMethodEventBuilder::indexFromWayland(text, cursorBegin);
    m_pendingPreeditString.cursorEnd = QWaylandInputMethodEventBuilder::indexFromWayland(text, cursorEnd);
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

    qCDebug(qLcQpaWaylandTextInput) << Q_FUNC_INFO << "PREEDIT" << m_pendingPreeditString.text << m_pendingPreeditString.cursorBegin;

    QList<QInputMethodEvent::Attribute> attributes;
    {
        if (m_pendingPreeditString.cursorBegin != -1 ||
                m_pendingPreeditString.cursorEnd != -1) {
            // Current supported cursor shape is just line.
            // It means, cursorEnd and cursorBegin are the same.
            QInputMethodEvent::Attribute attribute1(QInputMethodEvent::Cursor,
                                                    m_pendingPreeditString.cursorBegin,
                                                    1);
            attributes.append(attribute1);
        }

        // only use single underline style for now
        QTextCharFormat format;
        format.setFontUnderline(true);
        format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        QInputMethodEvent::Attribute attribute2(QInputMethodEvent::TextFormat,
                                                0,
                                                m_pendingPreeditString.text.length(), format);
        attributes.append(attribute2);
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
    return QLocale();
}

Qt::LayoutDirection QWaylandTextInputv3::inputDirection() const
{
    return Qt::LeftToRight;
}

}

QT_END_NAMESPACE
