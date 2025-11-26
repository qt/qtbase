// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qwaylandinputcontext_p.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QTextCharFormat>
#include <QtGui/QWindow>
#include <QtCore/QVarLengthArray>

#include "qwaylanddisplay_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylandwindow_p.h"

#if QT_CONFIG(xkbcommon)
#include <locale.h>
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcQpaInputMethods, "qt.qpa.input.methods")

namespace QtWaylandClient {

QWaylandInputContext::QWaylandInputContext(QWaylandDisplay *display)
    : mDisplay(display)
{
}

QWaylandInputContext::~QWaylandInputContext()
{
}

bool QWaylandInputContext::isValid() const
{
    return mDisplay->textInputManagerv2() != nullptr || mDisplay->textInputManagerv1() != nullptr || mDisplay->textInputManagerv3() != nullptr;
}

void QWaylandInputContext::reset()
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;
#if QT_CONFIG(xkbcommon)
    if (m_composeState)
        xkb_compose_state_reset(m_composeState);
#endif

    QPlatformInputContext::reset();

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return;

    inputInterface->reset();
}

void QWaylandInputContext::commit()
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return;

    inputInterface->commit();
}

static ::wl_surface *surfaceForWindow(QWindow *window)
{
    if (!window || !window->handle())
        return nullptr;

    auto *waylandWindow = static_cast<QWaylandWindow *>(window->handle());
    return waylandWindow->wlSurface();
}

void QWaylandInputContext::update(Qt::InputMethodQueries queries)
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO << queries;

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!QGuiApplication::focusObject() || !inputInterface)
        return;

    auto *currentSurface = surfaceForWindow(mCurrentWindow);

    if (currentSurface && !inputMethodAccepted()) {
        inputInterface->disableSurface(currentSurface);
        mCurrentWindow.clear();
    } else if (!currentSurface && inputMethodAccepted()) {
        QWindow *window = QGuiApplication::focusWindow();
        if (auto *focusSurface = surfaceForWindow(window)) {
            inputInterface->enableSurface(focusSurface);
            mCurrentWindow = window;
        }
    }

    inputInterface->updateState(queries, QWaylandTextInputInterface::update_state_change);
}

void QWaylandInputContext::invokeAction(QInputMethod::Action action, int cursorPostion)
{
    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return;

    if (action == QInputMethod::Click)
        inputInterface->setCursorInsidePreedit(cursorPostion);
}

void QWaylandInputContext::showInputPanel()
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return;

    inputInterface->showInputPanel();
}

void QWaylandInputContext::hideInputPanel()
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return;

    inputInterface->hideInputPanel();
}

bool QWaylandInputContext::isInputPanelVisible() const
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return QPlatformInputContext::isInputPanelVisible();

    return inputInterface->isInputPanelVisible();
}

QRectF QWaylandInputContext::keyboardRect() const
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return QPlatformInputContext::keyboardRect();

    return inputInterface->keyboardRect();
}

QLocale QWaylandInputContext::locale() const
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return QPlatformInputContext::locale();

    return inputInterface->locale();
}

Qt::LayoutDirection QWaylandInputContext::inputDirection() const
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return QPlatformInputContext::inputDirection();

    return inputInterface->inputDirection();
}

void QWaylandInputContext::setFocusObject(QObject *object)
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;
#if QT_CONFIG(xkbcommon)
    m_focusObject = object;
#else
    Q_UNUSED(object);
#endif

    QWaylandTextInputInterface *inputInterface = textInput();
    if (!inputInterface)
        return;

    QWindow *window = QGuiApplication::focusWindow();

    if (window && window->handle()) {
        if (mCurrentWindow.data() != window) {
            if (!inputMethodAccepted()) {
                auto *surface = static_cast<QWaylandWindow *>(window->handle())->wlSurface();
                if (surface)
                    inputInterface->disableSurface(surface);
                mCurrentWindow.clear();
            } else {
                auto *surface = static_cast<QWaylandWindow *>(window->handle())->wlSurface();
                if (surface) {
                    inputInterface->enableSurface(surface);
                    mCurrentWindow = window;
                } else {
                    mCurrentWindow.clear();
                }
            }
        }
        if (mCurrentWindow)
            inputInterface->updateState(Qt::ImQueryAll, QWaylandTextInputInterface::update_state_enter);
        return;
    }

    if (mCurrentWindow)
        mCurrentWindow.clear();
}

QWaylandTextInputInterface *QWaylandInputContext::textInput() const
{
    return mDisplay->defaultInputDevice() ? mDisplay->defaultInputDevice()->textInput() : nullptr;
}

#if QT_CONFIG(xkbcommon)

void QWaylandInputContext::ensureInitialized()
{
    if (m_initialized)
        return;

    if (!m_XkbContext) {
        qCWarning(qLcQpaInputMethods) << "error: xkb context has not been set on" << metaObject()->className();
        return;
    }

    m_initialized = true;
    const char *const locale = setlocale(LC_CTYPE, nullptr);
    qCDebug(qLcQpaInputMethods) << "detected locale (LC_CTYPE):" << locale;

    m_composeTable = xkb_compose_table_new_from_locale(m_XkbContext, locale, XKB_COMPOSE_COMPILE_NO_FLAGS);
    if (m_composeTable)
        m_composeState = xkb_compose_state_new(m_composeTable, XKB_COMPOSE_STATE_NO_FLAGS);

    if (!m_composeTable) {
        qCWarning(qLcQpaInputMethods, "failed to create compose table");
        return;
    }
    if (!m_composeState) {
        qCWarning(qLcQpaInputMethods, "failed to create compose state");
        return;
    }
}

bool QWaylandInputContext::filterEvent(const QEvent *event)
{
    auto keyEvent = static_cast<const QKeyEvent *>(event);
    if (keyEvent->type() != QEvent::KeyPress)
        return false;

    if (!inputMethodAccepted())
        return false;

    // lazy initialization - we don't want to do this on an app startup
    ensureInitialized();

    if (!m_composeTable || !m_composeState)
        return false;

    xkb_compose_state_feed(m_composeState, keyEvent->nativeVirtualKey());

    switch (xkb_compose_state_get_status(m_composeState)) {
    case XKB_COMPOSE_COMPOSING:
        return true;
    case XKB_COMPOSE_CANCELLED:
        reset();
        return false;
    case XKB_COMPOSE_COMPOSED:
    {
        const int size = xkb_compose_state_get_utf8(m_composeState, nullptr, 0);
        QVarLengthArray<char, 32> buffer(size + 1);
        xkb_compose_state_get_utf8(m_composeState, buffer.data(), buffer.size());
        QString composedText = QString::fromUtf8(buffer.constData());

        QInputMethodEvent event;
        event.setCommitString(composedText);

        if (!m_focusObject && qApp)
            m_focusObject = qApp->focusObject();

        if (m_focusObject)
            QCoreApplication::sendEvent(m_focusObject, &event);
        else
            qCWarning(qLcQpaInputMethods, "no focus object");

        reset();
        return true;
    }
    case XKB_COMPOSE_NOTHING:
        return false;
    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

#endif

}

QT_END_NAMESPACE

#include "moc_qwaylandinputcontext_p.cpp"
