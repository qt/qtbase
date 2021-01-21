/****************************************************************************
**
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
