// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetconmonitor_p.h"

#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcNetMon, "qt.network.monitor");

// Note: this 'stub' version is never enabled (see QNetworkConnectionMonitor::isEnabled below)
// and thus should never affect QNAM in any unusuall way. Having this 'stub' version is similar
// to building Qt with bearer management configured out.

class QNetworkConnectionMonitorPrivate : public QObjectPrivate
{
};

QNetworkConnectionMonitor::QNetworkConnectionMonitor()
    : QObject(*new QNetworkConnectionMonitorPrivate)
{
}

QNetworkConnectionMonitor::QNetworkConnectionMonitor(const QHostAddress &local, const QHostAddress &remote)
    : QObject(*new QNetworkConnectionMonitorPrivate)
{
    Q_UNUSED(local);
    Q_UNUSED(remote);
}

QNetworkConnectionMonitor::~QNetworkConnectionMonitor()
{
}

bool QNetworkConnectionMonitor::setTargets(const QHostAddress &local, const QHostAddress &remote)
{
    Q_UNUSED(local);
    Q_UNUSED(remote);

    return false;
}

bool QNetworkConnectionMonitor::startMonitoring()
{
    return false;
}

bool QNetworkConnectionMonitor::isMonitoring() const
{
    return false;
}

void QNetworkConnectionMonitor::stopMonitoring()
{
}

bool QNetworkConnectionMonitor::isReachable()
{
    return false;
}

bool QNetworkConnectionMonitor::isEnabled()
{
    return false;
}

QT_END_NAMESPACE
