// Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowssessionmanager.h"

QT_BEGIN_NAMESPACE

QWindowsSessionManager::QWindowsSessionManager(const QString &id, const QString &key)
    : QPlatformSessionManager(id, key)
{
}

bool QWindowsSessionManager::allowsInteraction()
{
    m_blockUserInput = false;
    return true;
}

bool QWindowsSessionManager::allowsErrorInteraction()
{
    m_blockUserInput = false;
    return true;
}

void QWindowsSessionManager::release()
{
    if (m_isActive)
        m_blockUserInput = true;
}

void QWindowsSessionManager::cancel()
{
    m_canceled = true;
}

QT_END_NAMESPACE
