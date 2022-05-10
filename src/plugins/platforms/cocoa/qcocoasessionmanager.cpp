// Copyright (C) 2019 Samuel Gaist <samuel.gaist@idiap.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
