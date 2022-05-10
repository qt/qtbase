// Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
// Copyright (C) 2013 Teo Mrnjavac <teo@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformsessionmanager.h"

#include "qguiapplication_p.h"

#ifndef QT_NO_SESSIONMANAGER

QT_BEGIN_NAMESPACE

QPlatformSessionManager::QPlatformSessionManager(const QString &id, const QString &key)
    : m_sessionId(id),
      m_sessionKey(key),
      m_restartHint(QSessionManager::RestartIfRunning)
{
}

QPlatformSessionManager::~QPlatformSessionManager()
{
}

QString QPlatformSessionManager::sessionId() const
{
    return m_sessionId;
}

QString QPlatformSessionManager::sessionKey() const
{
    return m_sessionKey;
}

bool QPlatformSessionManager::allowsInteraction()
{
    return false;
}

bool QPlatformSessionManager::allowsErrorInteraction()
{
    return false;
}

void QPlatformSessionManager::release()
{
}

void QPlatformSessionManager::cancel()
{
}

void QPlatformSessionManager::setRestartHint(QSessionManager::RestartHint restartHint)
{
    m_restartHint = restartHint;
}

QSessionManager::RestartHint QPlatformSessionManager::restartHint() const
{
    return m_restartHint;
}

void QPlatformSessionManager::setRestartCommand(const QStringList &command)
{
    m_restartCommand = command;
}

QStringList QPlatformSessionManager::restartCommand() const
{
    return m_restartCommand;
}

void QPlatformSessionManager::setDiscardCommand(const QStringList &command)
{
    m_discardCommand = command;
}

QStringList QPlatformSessionManager::discardCommand() const
{
    return m_discardCommand;
}

void QPlatformSessionManager::setManagerProperty(const QString &name, const QString &value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
}

void QPlatformSessionManager::setManagerProperty(const QString &name, const QStringList &value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
}

bool QPlatformSessionManager::isPhase2() const
{
    return false;
}

void QPlatformSessionManager::requestPhase2()
{
}

void QPlatformSessionManager::appCommitData()
{
    qGuiApp->d_func()->commitData();
}

void QPlatformSessionManager::appSaveState()
{
    qGuiApp->d_func()->saveState();
}

QT_END_NAMESPACE

#endif // QT_NO_SESSIONMANAGER
