/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qcomposeplatforminputcontext.h"

#include <QtCore/QCoreApplication>
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
    const char *locale = setlocale(LC_CTYPE, "");
    if (!locale)
        locale = setlocale(LC_CTYPE, nullptr);
    qCDebug(lcXkbCompose) << "detected locale (LC_CTYPE):" << locale;

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
        Q_UNREACHABLE();
        return false;
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
