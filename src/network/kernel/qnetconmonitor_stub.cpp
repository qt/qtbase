/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qnetconmonitor_p.h"

#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcNetMon, "qt.network.monitor");

// Note: this 'stub' version is never enabled (see QNetworkStatusMonitor::isEnabled below)
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
    Q_UNUSED(local)
    Q_UNUSED(remote)
}

QNetworkConnectionMonitor::~QNetworkConnectionMonitor()
{
}

bool QNetworkConnectionMonitor::setTargets(const QHostAddress &local, const QHostAddress &remote)
{
    Q_UNUSED(local)
    Q_UNUSED(remote)

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

class QNetworkStatusMonitorPrivate : public QObjectPrivate
{
};

QNetworkStatusMonitor::QNetworkStatusMonitor(QObject *parent)
    : QObject(*new QNetworkStatusMonitorPrivate, parent)
{
}

QNetworkStatusMonitor::~QNetworkStatusMonitor()
{
}

bool QNetworkStatusMonitor::start()
{
    return false;
}

void QNetworkStatusMonitor::stop()
{
}

bool QNetworkStatusMonitor::isMonitoring() const
{
    return false;
}

bool QNetworkStatusMonitor::isNetworkAccessible()
{
    return false;
}

bool QNetworkStatusMonitor::event(QEvent *event)
{
    return QObject::event(event);
}

bool QNetworkStatusMonitor::isEnabled()
{
    return false;
}

void QNetworkStatusMonitor::reachabilityChanged(bool online)
{
    Q_UNUSED(online)
}

QT_END_NAMESPACE
