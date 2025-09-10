// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qcomposeplatforminputcontext.h"

#include <QtCore/QCoreApplication>
#include <QtCore/qvarlengtharray.h>
#include <QtGui/QKeyEvent>
#include <QtGui/QGuiApplication>

#include <locale.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcXkbCompose, "qt.xkb.compose")

QComposeInputContext::QComposeInputContext()
{
    setObjectName(QStringLiteral("QComposeInputContext"));
    qCDebug(lcXkbCompose, "using xkb compose input context");
}

QComposeInputContext::~QComposeInputContext()
{
    xkb_compose_state_unref(m_composeState);
    xkb_compose_table_unref(m_composeTable);
}

void QComposeInputContext::ensureInitialized()
{
    if (m_initialized)
        return;

    if (!m_XkbContext) {
        qCWarning(lcXkbCompose) << "error: xkb context has not been set on" << metaObject()->className();
        return;
    }

    m_initialized = true;
    // Get locale from user env settings, see also
    // https://xkbcommon.org/doc/current/group__compose.html#compose-locale
    const char *locale = getenv("LC_ALL");
    if (!locale || !*locale)
        locale = getenv("LC_CTYPE");
    if (!locale || !*locale)
        locale = getenv("LANG");
    if (!locale || !*locale)
         locale = "C";
    qCDebug(lcXkbCompose) << "detected locale:" << locale;

    m_composeTable = xkb_compose_table_new_from_locale(m_XkbContext, locale, XKB_COMPOSE_COMPILE_NO_FLAGS);
    if (m_composeTable)
        m_composeState = xkb_compose_state_new(m_composeTable, XKB_COMPOSE_STATE_NO_FLAGS);

    if (!m_composeTable) {
        qCWarning(lcXkbCompose, "failed to create compose table");
        return;
    }
    if (!m_composeState) {
        qCWarning(lcXkbCompose, "failed to create compose state");
        return;
    }
}

bool QComposeInputContext::filterEvent(const QEvent *event)
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
            qCWarning(lcXkbCompose, "no focus object");

        reset();
        return true;
    }
    case XKB_COMPOSE_NOTHING:
        return false;
    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

bool QComposeInputContext::isValid() const
{
    return true;
}

void QComposeInputContext::setFocusObject(QObject *object)
{
    m_focusObject = object;
}

void QComposeInputContext::reset()
{
    if (m_composeState)
        xkb_compose_state_reset(m_composeState);
}

void QComposeInputContext::update(Qt::InputMethodQueries q)
{
    QPlatformInputContext::update(q);
}

QT_END_NAMESPACE

#include "moc_qcomposeplatforminputcontext.cpp"
