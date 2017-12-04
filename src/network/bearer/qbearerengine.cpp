/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qbearerengine_p.h"
#include <algorithm>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

static void cleanUpConfigurations(QHash<QString, QNetworkConfigurationPrivatePointer> &configurations)
{
    for (const auto &ptr : qAsConst(configurations)) {
        ptr->isValid = false;
        ptr->id.clear();
    }
    configurations.clear();
}

static bool hasUsedConfiguration(const QHash<QString, QNetworkConfigurationPrivatePointer> &configurations)
{
    auto isUsed = [](const QNetworkConfigurationPrivatePointer &ptr) {
        return ptr->ref.load() > 1;
    };
    const auto end = configurations.end();
    return std::find_if(configurations.begin(), end, isUsed) != end;
}

QBearerEngine::QBearerEngine(QObject *parent)
    : QObject(parent), mutex(QMutex::Recursive)
{
}

QBearerEngine::~QBearerEngine()
{
    cleanUpConfigurations(snapConfigurations);
    cleanUpConfigurations(accessPointConfigurations);
    cleanUpConfigurations(userChoiceConfigurations);
}

bool QBearerEngine::requiresPolling() const
{
    return false;
}

/*
    Returns \c true if configurations are in use; otherwise returns \c false.

    If configurations are in use and requiresPolling() returns \c true, polling will be enabled for
    this engine.
*/
bool QBearerEngine::configurationsInUse() const
{
    QMutexLocker locker(&mutex);
    return hasUsedConfiguration(accessPointConfigurations)
        || hasUsedConfiguration(snapConfigurations)
        || hasUsedConfiguration(userChoiceConfigurations);
}

QT_END_NAMESPACE

#include "moc_qbearerengine_p.cpp"

#endif // QT_NO_BEARERMANAGEMENT
