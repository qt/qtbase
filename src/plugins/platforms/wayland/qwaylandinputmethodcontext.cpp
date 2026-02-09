// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandinputmethodcontext_p.h"
#include "qwaylandinputcontext_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandinputdevice_p.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qtextformat.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

static constexpr int maxStringSize = 1000; // actual max is 4096/3

QWaylandTextInputMethod::QWaylandTextInputMethod(QWaylandDisplay *display, struct ::qt_text_input_method_v1 *textInputMethod)
    : QtWayland::qt_text_input_method_v1(textInputMethod)
{
    Q_UNUSED(display);
}

QWaylandTextInputMethod::~QWaylandTextInputMethod()
{
    qt_text_input_method_v1_destroy(object());
}

void QWaylandTextInputMethod::text_input_method_v1_visible_changed(int32_t visible)
{
    if (m_isVisible != visible) {
        m_isVisible = visible;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitInputPanelVisibleChanged();
    }
}

void QWaylandTextInputMethod::text_input_method_v1_locale_changed(const QString &localeName)
{
    m_locale = QLocale(localeName);
}

void QWaylandTextInputMethod::text_input_method_v1_input_direction_changed(int32_t inputDirection)
{
    m_layoutDirection = Qt::LayoutDirection(inputDirection);
}

void QWaylandTextInputMethod::text_input_method_v1_keyboard_rectangle_changed(wl_fixed_t x, wl_fixed_t y, wl_fixed_t width, wl_fixed_t height)
{
    const QRectF keyboardRectangle(wl_fixed_to_double(x),
                                   wl_fixed_to_double(y),
                                   wl_fixed_to_double(width),
                                   wl_fixed_to_double(height));
    if (m_keyboardRect != keyboardRectangle) {
        m_keyboardRect = keyboardRectangle;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitKeyboardRectChanged();
    }
}

void QWaylandTextInputMethod::text_input_method_v1_start_input_method_event(uint32_t serial, int32_t surrounding_text_offset)
{
    if (m_pendingInputMethodEvents.contains(serial)) {
        qCWarning(qLcQpaInputMethods) << "Input method event with serial" << serial << "already started";
        return;
    }

    m_pendingInputMethodEvents[serial] = QList<QInputMethodEvent::Attribute>{};
    m_offsetFromCompositor[serial] = surrounding_text_offset;
}

// We need to keep surrounding text below maxStringSize characters, with cursorPos centered in that substring

static int calculateOffset(const QString &text, int cursorPos)
{
    int size = text.size();
    int halfSize = maxStringSize/2;
    if (size <= maxStringSize || cursorPos < halfSize)
        return 0;
    if (cursorPos > size - halfSize)
        return size - maxStringSize;
    return  cursorPos - halfSize;
}

static QString mapSurroundingTextToCompositor(const QString &s, int offset)
{
    return s.mid(offset, maxStringSize);
}

static int mapPositionToCompositor(int pos, int offset)
{
    return pos - offset;
}

static int mapPositionFromCompositor(int pos, int offset)
{
    return pos + offset;
}

void QWaylandTextInputMethod::text_input_method_v1_input_method_event_attribute(uint32_t serial, int32_t type, int32_t start, int32_t length, const QString &value)
{
    if (!m_pendingInputMethodEvents.contains(serial)) {
        qCWarning(qLcQpaInputMethods) << "Input method event with serial" << serial << "does not exist";
        return;
    }

    int startMapped = mapPositionFromCompositor(start, m_offsetFromCompositor[serial]);
    QList<QInputMethodEvent::Attribute> &attributes = m_pendingInputMethodEvents[serial];
    switch (type) {
    case QInputMethodEvent::Selection:
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::AttributeType(type), startMapped, length));
        break;
    case QInputMethodEvent::Cursor:
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::AttributeType(type), start, length, QColor::fromString(value)));
        break;
    case QInputMethodEvent::TextFormat:
    {
        QTextCharFormat textFormat;
        textFormat.setProperty(QTextFormat::FontUnderline, true);
        textFormat.setProperty(QTextFormat::TextUnderlineStyle, QTextCharFormat::SingleUnderline);
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::AttributeType(type), start, length, textFormat));
        break;
    }
    case QInputMethodEvent::Language:
    case QInputMethodEvent::Ruby:
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::AttributeType(type), start, length, value));
        break;
    };
}

void QWaylandTextInputMethod::sendInputState(QInputMethodQueryEvent *event, Qt::InputMethodQueries queries)
{
    int cursorPosition = event->value(Qt::ImCursorPosition).toInt();
    int anchorPosition = event->value(Qt::ImAnchorPosition).toInt();
    QString surroundingText = event->value(Qt::ImSurroundingText).toString();
    int offset = calculateOffset(surroundingText, cursorPosition);

    if (queries & Qt::ImCursorPosition)
        update_cursor_position(mapPositionToCompositor(cursorPosition, offset));
    if (queries & Qt::ImSurroundingText)
        update_surrounding_text(mapSurroundingTextToCompositor(surroundingText, offset), offset);
    if (queries & Qt::ImAnchorPosition)
        update_anchor_position(mapPositionToCompositor(anchorPosition, offset));
    if (queries & Qt::ImAbsolutePosition)
        update_absolute_position(event->value(Qt::ImAbsolutePosition).toInt()); // do not map: this is the position in the whole document
}


void QWaylandTextInputMethod::text_input_method_v1_end_input_method_event(uint32_t serial, const QString &commitString, const QString &preeditString, int32_t replacementStart, int32_t replacementLength)
{
    if (!m_pendingInputMethodEvents.contains(serial)) {
        qCWarning(qLcQpaInputMethods) << "Input method event with serial" << serial << "does not exist";
        return;
    }

    QList<QInputMethodEvent::Attribute> attributes = m_pendingInputMethodEvents.take(serial);
    m_offsetFromCompositor.remove(serial);
    if (QGuiApplication::focusObject() != nullptr) {
        QInputMethodEvent event(preeditString, attributes);
        event.setCommitString(commitString, replacementStart, replacementLength);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);
    }

    // Send current state to make sure it matches
    if (QGuiApplication::focusObject() != nullptr) {
        QInputMethodQueryEvent event(Qt::ImCursorPosition | Qt::ImSurroundingText | Qt::ImAnchorPosition | Qt::ImAbsolutePosition);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);
        sendInputState(&event);
    }

    acknowledge_input_method();
}

void QWaylandTextInputMethod::text_input_method_v1_key(int32_t type,
                                                    int32_t key,
                                                    int32_t modifiers,
                                                    int32_t autoRepeat,
                                                    int32_t count,
                                                    int32_t nativeScanCode,
                                                    int32_t nativeVirtualKey,
                                                    int32_t nativeModifiers,
                                                    const QString &text)
{
    if (QGuiApplication::focusObject() != nullptr) {
        QKeyEvent event(QKeyEvent::Type(type),
                        key,
                        Qt::KeyboardModifiers(modifiers),
                        nativeScanCode,
                        nativeVirtualKey,
                        nativeModifiers,
                        text,
                        autoRepeat,
                        count);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);
    }
}

void QWaylandTextInputMethod::text_input_method_v1_enter(struct ::wl_surface *surface)
{
    m_surface = surface;
}

void QWaylandTextInputMethod::text_input_method_v1_leave(struct ::wl_surface *surface)
{
    if (surface != m_surface) {
        qCWarning(qLcQpaInputMethods) << "Got leave event for surface without corresponding enter";
    } else {
        m_surface = nullptr;
    }
}

QWaylandInputMethodContext::QWaylandInputMethodContext(QWaylandDisplay *display)
    : m_display(display)
{
}

QWaylandInputMethodContext::~QWaylandInputMethodContext()
{
}

bool QWaylandInputMethodContext::isValid() const
{
    return m_display->textInputMethodManager() != nullptr;
}

void QWaylandInputMethodContext::reset()
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr)
        inputMethod->reset();
}

void QWaylandInputMethodContext::commit()
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr)
        inputMethod->commit();

    m_display->forceRoundTrip();
}

void QWaylandInputMethodContext::update(Qt::InputMethodQueries queries)
{
    wl_surface *currentSurface = m_currentWindow != nullptr && m_currentWindow->handle() != nullptr
                ? static_cast<QWaylandWindow *>(m_currentWindow->handle())->wlSurface()
                : nullptr;
    if (currentSurface != nullptr && !inputMethodAccepted()) {
        textInputMethod()->disable(currentSurface);
        m_currentWindow.clear();
    } else if (currentSurface == nullptr && inputMethodAccepted()) {
        QWindow *window = QGuiApplication::focusWindow();
        currentSurface = window != nullptr && window->handle() != nullptr
                ? static_cast<QWaylandWindow *>(window->handle())->wlSurface()
                : nullptr;
        if (currentSurface != nullptr) {
            textInputMethod()->disable(currentSurface);
            m_currentWindow = window;
        }
    }

    queries &= (Qt::ImEnabled
                | Qt::ImHints
                | Qt::ImCursorRectangle
                | Qt::ImCursorPosition
                | Qt::ImSurroundingText
                | Qt::ImCurrentSelection
                | Qt::ImAnchorPosition
                | Qt::ImTextAfterCursor
                | Qt::ImTextBeforeCursor
                | Qt::ImPreferredLanguage);

    const Qt::InputMethodQueries queriesNeedingOffset = Qt::ImCursorPosition | Qt::ImSurroundingText | Qt::ImAnchorPosition;
    if (queries & queriesNeedingOffset)
        queries |= queriesNeedingOffset;

    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr && QGuiApplication::focusObject() != nullptr) {
        QInputMethodQueryEvent event(queries);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);

        inputMethod->start_update(int(queries));

        if (queries & Qt::ImHints)
            inputMethod->update_hints(event.value(Qt::ImHints).toInt());

        if (queries & Qt::ImCursorRectangle) {
            QRect rect = event.value(Qt::ImCursorRectangle).toRect();
            inputMethod->update_cursor_rectangle(rect.x(), rect.y(), rect.width(), rect.height());
        }

        inputMethod->sendInputState(&event, queries);

        if (queries & Qt::ImPreferredLanguage)
            inputMethod->update_preferred_language(event.value(Qt::ImPreferredLanguage).toString());

        inputMethod->end_update();

        // ### Should we do a display sync here and ignore all events until it is received?
    }
}

void QWaylandInputMethodContext::invokeAction(QInputMethod::Action action, int cursorPosition)
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr)
        inputMethod->invoke_action(int(action), cursorPosition);
}

void QWaylandInputMethodContext::showInputPanel()
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr)
        inputMethod->show_input_panel();
}

void QWaylandInputMethodContext::hideInputPanel()
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr)
        inputMethod->hide_input_panel();
}

bool QWaylandInputMethodContext::isInputPanelVisible() const
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr)
        return inputMethod->isVisible();
    else
        return false;
}

QRectF QWaylandInputMethodContext::keyboardRect() const
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr)
        return inputMethod->keyboardRect();
    else
        return QRectF();
}

QLocale QWaylandInputMethodContext::locale() const
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr)
        return inputMethod->locale();
    else
        return QLocale();
}

Qt::LayoutDirection QWaylandInputMethodContext::inputDirection() const
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod != nullptr)
        return inputMethod->inputDirection();
    else
        return Qt::LeftToRight;
}

void QWaylandInputMethodContext::setFocusObject(QObject *)
{
    QWaylandTextInputMethod *inputMethod = textInputMethod();
    if (inputMethod == nullptr)
        return;

    if (inputMethod->isVisible() && !inputMethodAccepted())
        inputMethod->hide_input_panel();

    QWindow *window = QGuiApplication::focusWindow();

    if (m_currentWindow != nullptr && m_currentWindow->handle() != nullptr) {
        if (m_currentWindow.data() != window || !inputMethodAccepted()) {
            auto *surface = static_cast<QWaylandWindow *>(m_currentWindow->handle())->wlSurface();
            if (surface)
                inputMethod->disable(surface);
            m_currentWindow.clear();
        }
    }

    if (window != nullptr && window->handle() != nullptr && inputMethodAccepted()) {
        if (m_currentWindow.data() != window) {
            auto *surface = static_cast<QWaylandWindow *>(window->handle())->wlSurface();
            if (surface != nullptr) {
                inputMethod->enable(surface);
                m_currentWindow = window;
            }
        }

        update(Qt::ImQueryAll);
    }
}

QWaylandTextInputMethod *QWaylandInputMethodContext::textInputMethod() const
{
    return m_display->defaultInputDevice() ? m_display->defaultInputDevice()->textInputMethod() : nullptr;
}

} // QtWaylandClient

QT_END_NAMESPACE

#include "moc_qwaylandinputmethodcontext_p.cpp"
