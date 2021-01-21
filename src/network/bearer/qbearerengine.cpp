/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qbearerengine_p.h"
#include <QtCore/private/qlocking_p.h>

#include <algorithm>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

static void cleanUpConfigurations(QHash<QString, QNetworkConfigurationPrivatePointer> &configurations)
{
    for (auto &ptr : qExchange(configurations, {})) {
        ptr->isValid = false;
        ptr->id.clear();
    }
}

static bool hasUsedConfiguration(const QHash<QString, QNetworkConfigurationPrivatePointer> &configurations)
{
    auto isUsed = [](const QNetworkConfigurationPrivatePointer &ptr) {
        return ptr->ref.loadRelaxed() > 1;
    };
    const auto end = configurations.end();
    return std::find_if(configurations.begin(), end, isUsed) != end;
}

QBearerEngine::QBearerEngine(QObject *parent)
    : QObject(parent)
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
    const auto locker = qt_scoped_lock(mutex);
    return hasUsedConfiguration(accessPointConfigurations)
        || hasUsedConfiguration(snapConfigurations)
        || hasUsedConfiguration(userChoiceConfigurations);
}

QT_END_NAMESPACE

#include "moc_qbearerengine_p.cpp"

#endif // QT_NO_BEARERMANAGEMENT
