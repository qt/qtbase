/****************************************************************************
**
** Copyright (C) 2019 Samuel Gaist <samuel.gaist@idiap.ch>
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

#ifndef QT_NO_SESSIONMANAGER
#include <private/qsessionmanager_p.h>
#include <private/qguiapplication_p.h>

#include "qcocoasessionmanager.h"
#include <qstring.h>

QT_BEGIN_NAMESPACE

QCocoaSessionManager::QCocoaSessionManager(const QString &id, const QString &key)
    : QPlatformSessionManager(id, key),
      m_canceled(false)
{
}

QCocoaSessionManager::~QCocoaSessionManager()
{
}

bool QCocoaSessionManager::allowsInteraction()
{
    return false;
}

void QCocoaSessionManager::resetCancellation()
{
    m_canceled = false;
}

void QCocoaSessionManager::cancel()
{
    m_canceled = true;
}

bool QCocoaSessionManager::wasCanceled() const
{
    return m_canceled;
}

QCocoaSessionManager *QCocoaSessionManager::instance()
{
    auto *qGuiAppPriv = QGuiApplicationPrivate::instance();
    auto *managerPrivate = static_cast<QSessionManagerPrivate*>(QObjectPrivate::get(qGuiAppPriv->session_manager));
    return static_cast<QCocoaSessionManager *>(managerPrivate->platformSessionManager);
}

QT_END_NAMESPACE

#endif // QT_NO_SESSIONMANAGER
