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

#ifndef QNETCONMONITOR_P_H
#define QNETCONMONITOR_P_H

#include <private/qtnetworkglobal_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qhostaddress.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QNetworkConnectionMonitorPrivate;
class Q_AUTOTEST_EXPORT QNetworkConnectionMonitor : public QObject
{
    Q_OBJECT

public:
    QNetworkConnectionMonitor();
    QNetworkConnectionMonitor(const QHostAddress &local, const QHostAddress &remote = {});
    ~QNetworkConnectionMonitor();

    bool setTargets(const QHostAddress &local, const QHostAddress &remote);
    bool isReachable();

    // Important: on Darwin you should not call isReachable() after
    // startMonitoring(), you have to listen to reachabilityChanged()
    // signal instead.
    bool startMonitoring();
    bool isMonitoring() const;
    void stopMonitoring();

Q_SIGNALS:
    // Important: connect to this using QueuedConnection. On Darwin
    // callback is coming on a special dispatch queue.
    void reachabilityChanged(bool isOnline);

private:
    Q_DECLARE_PRIVATE(QNetworkConnectionMonitor)
    Q_DISABLE_COPY_MOVE(QNetworkConnectionMonitor)
};

class QNetworkStatusMonitorPrivate;
class Q_AUTOTEST_EXPORT QNetworkStatusMonitor : public QObject
{
    Q_OBJECT

public:
    QNetworkStatusMonitor(QObject *parent);
    ~QNetworkStatusMonitor();

    bool isNetworkAccessible();

    bool start();
    void stop();
    bool isMonitoring() const;

    bool event(QEvent *event) override;

    static bool isEnabled();

Q_SIGNALS:
    // Unlike QNetworkConnectionMonitor, this can be connected to directly.
    void onlineStateChanged(bool isOnline);

private slots:
    void reachabilityChanged(bool isOnline);

private:
    Q_DECLARE_PRIVATE(QNetworkStatusMonitor)
    Q_DISABLE_COPY_MOVE(QNetworkStatusMonitor)
};

Q_DECLARE_LOGGING_CATEGORY(lcNetMon)

QT_END_NAMESPACE

#endif // QNETCONMONITOR_P_H
