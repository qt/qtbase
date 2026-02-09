// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include <qpa/qplatforminputcontext.h>

#include "qwaylandtextinputv1_p.h"

#include "qwaylandinputcontext_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylandinputmethodeventbuilder_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/QGuiApplication>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QTextCharFormat>
#include <QList>
#include <QRectF>
#include <QLocale>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

namespace {

const Qt::InputMethodQueries supportedQueries1 = Qt::ImEnabled |
                                                Qt::ImSurroundingText |
                                                Qt::ImCursorPosition |
                                                Qt::ImAnchorPosition |
                                                Qt::ImHints |
                                                Qt::ImCursorRectangle |
                                                Qt::ImPreferredLanguage;
}

QWaylandTextInputv1::QWaylandTextInputv1(QWaylandDisplay *display, struct ::zwp_text_input_v1 *text_input)
    : QtWayland::zwp_text_input_v1(text_input)
{
    Q_UNUSED(display);
}

QWaylandTextInputv1::~QWaylandTextInputv1()
{
    if (m_resetCallback)
        wl_callback_destroy(m_resetCallback);
    zwp_text_input_v1_destroy(object());
}

void QWaylandTextInputv1::reset()
{
    m_builder.reset();
    m_preeditCommit = QString();
    updateState(Qt::ImQueryAll, QWaylandTextInputInterface::update_state_reset);
}

void QWaylandTextInputv1::commit()
{
    if (QObject *o = QGuiApplication::focusObject()) {
        QInputMethodEvent event;
        event.setCommitString(m_preeditCommit);
        QCoreApplication::sendEvent(o, &event);
    }

    reset();
}

const wl_callback_listener QWaylandTextInputv1::callbackListener = {
    QWaylandTextInputv1::resetCallback
};

void QWaylandTextInputv1::resetCallback(void *data, wl_callback *, uint32_t)
{
    QWaylandTextInputv1 *self = static_cast<QWaylandTextInputv1*>(data);

    if (self->m_resetCallback) {
        wl_callback_destroy(self->m_resetCallback);
        self->m_resetCallback = nullptr;
    }
}

void QWaylandTextInputv1::updateState(Qt::InputMethodQueries queries, uint32_t flags)
{
    if (!QGuiApplication::focusObject())
        return;

    if (!QGuiApplication::focusWindow() || !QGuiApplication::focusWindow()->handle())
        return;

    auto *window = static_cast<QWaylandWindow *>(QGuiApplication::focusWindow()->handle());
    auto *surface = window->wlSurface();
    if (!surface || (surface != m_surface))
        return;

    queries &= supportedQueries1;

    // Surrounding text, cursor and anchor positions are transferred together
    if ((queries & Qt::ImSurroundingText) || (queries & Qt::ImCursorPosition) || (queries & Qt::ImAnchorPosition))
        queries |= Qt::ImSurroundingText | Qt::ImCursorPosition | Qt::ImAnchorPosition;

    QInputMethodQueryEvent event(queries);
    QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);

    if ((queries & Qt::ImSurroundingText) || (queries & Qt::ImCursorPosition) || (queries & Qt::ImAnchorPosition)) {
        QString text = event.value(Qt::ImSurroundingText).toString();
        int cursor = event.value(Qt::ImCursorPosition).toInt();
        int anchor = event.value(Qt::ImAnchorPosition).toInt();

        // Make sure text is not too big
        if (text.toUtf8().size() > 2048) {
            int c = qAbs(cursor - anchor) <= 512 ? qMin(cursor, anchor) + qAbs(cursor - anchor) / 2: cursor;

            const int offset = c - qBound(0, c, 512 - qMin(text.size() - c, 256));
            text = text.mid(offset + c - 256, 512);
            cursor -= offset;
            anchor -= offset;
        }

        set_surrounding_text(text, QWaylandInputMethodEventBuilder::indexToWayland(text, cursor), QWaylandInputMethodEventBuilder::indexToWayland(text, anchor));
    }

    if (queries & Qt::ImHints) {
        QWaylandInputMethodContentType contentType = QWaylandInputMethodContentType::convert(static_cast<Qt::InputMethodHints>(event.value(Qt::ImHints).toInt()));
        set_content_type(contentType.hint, contentType.purpose);
    }

    if (queries & Qt::ImCursorRectangle) {
        const QRect &cRect = event.value(Qt::ImCursorRectangle).toRect();
        const QRect &windowRect = QGuiApplication::inputMethod()->inputItemTransform().mapRect(cRect);
        const QRect &nativeRect = QHighDpi::toNativePixels(windowRect, QGuiApplication::focusWindow());
        const QMargins margins = window->clientSideMargins();
        const QRect &surfaceRect = nativeRect.translated(margins.left(), margins.top());
        set_cursor_rectangle(surfaceRect.x(), surfaceRect.y(), surfaceRect.width(), surfaceRect.height());
    }

    if (queries & Qt::ImPreferredLanguage) {
        const QString &language = event.value(Qt::ImPreferredLanguage).toString();
        set_preferred_language(language);
    }

    if (flags == QWaylandTextInputInterface::update_state_reset)
        QtWayland::zwp_text_input_v1::reset();
    else
        commit_state(m_serial);
}

void QWaylandTextInputv1::setCursorInsidePreedit(int)
{
    // Not supported yet
}

bool QWaylandTextInputv1::isInputPanelVisible() const
{
    return m_inputPanelVisible;
}

QRectF QWaylandTextInputv1::keyboardRect() const
{
    return m_keyboardRectangle;
}

QLocale QWaylandTextInputv1::locale() const
{
    return m_locale;
}

Qt::LayoutDirection QWaylandTextInputv1::inputDirection() const
{
    return m_inputDirection;
}

void QWaylandTextInputv1::zwp_text_input_v1_enter(::wl_surface *surface)
{
    m_surface = surface;

    updateState(Qt::ImQueryAll, QWaylandTextInputInterface::update_state_reset);
}

void QWaylandTextInputv1::zwp_text_input_v1_leave()
{
    m_surface = nullptr;
}

void QWaylandTextInputv1::zwp_text_input_v1_modifiers_map(wl_array *map)
{
    const QList<QByteArray> modifiersMap = QByteArray::fromRawData(static_cast<const char*>(map->data), map->size).split('\0');

    m_modifiersMap.clear();

    for (const QByteArray &modifier : modifiersMap) {
        if (modifier == "Shift")
            m_modifiersMap.append(Qt::ShiftModifier);
        else if (modifier == "Control")
            m_modifiersMap.append(Qt::ControlModifier);
        else if (modifier == "Alt")
            m_modifiersMap.append(Qt::AltModifier);
        else if (modifier == "Mod1")
            m_modifiersMap.append(Qt::AltModifier);
        else if (modifier == "Mod4")
            m_modifiersMap.append(Qt::MetaModifier);
        else
            m_modifiersMap.append(Qt::NoModifier);
    }
}

void QWaylandTextInputv1::zwp_text_input_v1_input_panel_state(uint32_t visible)
{
    const bool inputPanelVisible = (visible == 1);
    if (m_inputPanelVisible != inputPanelVisible) {
        m_inputPanelVisible = inputPanelVisible;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitInputPanelVisibleChanged();
    }
}

void QWaylandTextInputv1::zwp_text_input_v1_preedit_string(uint32_t serial, const QString &text, const QString &commit)
{
    m_serial = serial;

    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard preedit_string: reset not confirmed";
        m_builder.reset();
        return;
    }

    if (!QGuiApplication::focusObject())
        return;

    QInputMethodEvent *event = m_builder.buildPreedit(text);

    m_builder.reset();
    m_preeditCommit = commit;

    QCoreApplication::sendEvent(QGuiApplication::focusObject(), event);
    delete event;
}

void QWaylandTextInputv1::zwp_text_input_v1_preedit_styling(uint32_t index, uint32_t length, uint32_t style)
{
    m_builder.addPreeditStyling(index, length, style);
}

void QWaylandTextInputv1::zwp_text_input_v1_preedit_cursor(int32_t index)
{
    m_builder.setPreeditCursor(index);
}

void QWaylandTextInputv1::zwp_text_input_v1_commit_string(uint32_t serial, const QString &text)
{
    m_serial = serial;

    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard commit_string: reset not confirmed";
        m_builder.reset();
        return;
    }

    if (!QGuiApplication::focusObject())
        return;

    // When committing the text, the preeditString needs to be reset, to prevent it to be
    // send again in the commit() function
    m_preeditCommit.clear();

    QInputMethodEvent *event = m_builder.buildCommit(text);

    m_builder.reset();

    QCoreApplication::sendEvent(QGuiApplication::focusObject(), event);
    delete event;
}

void QWaylandTextInputv1::zwp_text_input_v1_cursor_position(int32_t index, int32_t anchor)
{
    m_builder.setCursorPosition(index, anchor);
}

void QWaylandTextInputv1::zwp_text_input_v1_delete_surrounding_text(int32_t before_length, uint32_t after_length)
{
    //before_length is negative, but the builder expects it to be positive
    m_builder.setDeleteSurroundingText(-before_length, after_length);
}

void QWaylandTextInputv1::zwp_text_input_v1_keysym(uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers)
{
    m_serial = serial;

#if QT_CONFIG(xkbcommon)
    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard keysym: reset not confirmed";
        return;
    }

    if (!QGuiApplication::focusWindow())
        return;

    Qt::KeyboardModifiers qtModifiers = modifiersToQtModifiers(modifiers);

    QEvent::Type type = state == WL_KEYBOARD_KEY_STATE_PRESSED ? QEvent::KeyPress : QEvent::KeyRelease;
    QString text = QXkbCommon::lookupStringNoKeysymTransformations(sym);
    int qtkey = QXkbCommon::keysymToQtKey(sym, qtModifiers);

    QWindowSystemInterface::handleKeyEvent(QGuiApplication::focusWindow(),
                                           time, type, qtkey, qtModifiers, text);
#else
    Q_UNUSED(time);
    Q_UNUSED(sym);
    Q_UNUSED(state);
    Q_UNUSED(modifiers);
#endif
}

void QWaylandTextInputv1::zwp_text_input_v1_language(uint32_t serial, const QString &language)
{
    m_serial = serial;

    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard language: reset not confirmed";
        return;
    }

    const QLocale locale(language);
    if (m_locale != locale) {
        m_locale = locale;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitLocaleChanged();
    }
}

void QWaylandTextInputv1::zwp_text_input_v1_text_direction(uint32_t serial, uint32_t direction)
{
    m_serial = serial;

    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard text_direction: reset not confirmed";
        return;
    }

    const Qt::LayoutDirection inputDirection = (direction == text_direction_auto) ? Qt::LayoutDirectionAuto :
                                               (direction == text_direction_ltr) ? Qt::LeftToRight :
                                               (direction == text_direction_rtl) ? Qt::RightToLeft : Qt::LayoutDirectionAuto;
    if (m_inputDirection != inputDirection) {
        m_inputDirection = inputDirection;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitInputDirectionChanged(m_inputDirection);
    }
}

Qt::KeyboardModifiers QWaylandTextInputv1::modifiersToQtModifiers(uint32_t modifiers)
{
    Qt::KeyboardModifiers ret = Qt::NoModifier;
    for (int i = 0; i < m_modifiersMap.size(); ++i) {
        if (modifiers & (1 << i)) {
            ret |= m_modifiersMap[i];
        }
    }
    return ret;
}

}

QT_END_NAMESPACE

