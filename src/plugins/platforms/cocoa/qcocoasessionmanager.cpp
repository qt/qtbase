/****************************************************************************
**
** Copyright (C) 2019 Samuel Gaist <samuel.gaist@idiap.ch>
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
