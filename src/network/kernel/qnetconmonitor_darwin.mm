// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "private/qnetconmonitor_p.h"

#include "private/qobject_p.h"

#include <Network/Network.h>

#include <QtCore/qreadwritelock.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcNetMon, "qt.network.monitor");

namespace {

class ReachabilityDispatchQueue
{
public:
    ReachabilityDispatchQueue()
    {
        queue = dispatch_queue_create("qt-network-reachability-queue", nullptr);
        if (!queue)
            qCWarning(lcNetMon, "Failed to create a dispatch queue for reachability probes");
    }

    ~ReachabilityDispatchQueue()
    {
        if (queue)
            dispatch_release(queue);
    }

    dispatch_queue_t data() const
    {
        return queue;
    }

private:
    dispatch_queue_t queue = nullptr;

    Q_DISABLE_COPY_MOVE(ReachabilityDispatchQueue)
};

dispatch_queue_t qt_reachability_queue()
{
    static const ReachabilityDispatchQueue reachabilityQueue;
    return reachabilityQueue.data();
}

} // unnamed namespace

class QNetworkConnectionMonitorPrivate : public QObjectPrivate
{
public:
    nw_path_status_t status = nw_path_status_invalid;
    mutable QReadWriteLock monitorLock;
    nw_path_monitor_t monitor = nullptr;
    using InterfaceType = QNetworkConnectionMonitor::InterfaceType;
    InterfaceType interface = InterfaceType::Unknown;

    void updateState(nw_path_t newState);
    void reset();
    bool isReachable() const;
    QNetworkConnectionMonitor::InterfaceType getInterfaceType() const;

    bool startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

    Q_DECLARE_PUBLIC(QNetworkConnectionMonitor)
};

void QNetworkConnectionMonitorPrivate::updateState(nw_path_t state)
{
    QReadLocker lock(&monitorLock);
    if (monitor == nullptr)
        return;

    // To be executed only on the reachability queue.
    Q_Q(QNetworkConnectionMonitor);

    // NETMONTODO: for now, 'online' for us means nw_path_status_satisfied
    // is set. There are more possible flags that require more tests/some special
    // setup. So in future this part and related can change/be extended.
    const bool wasReachable = isReachable();
    const QNetworkConnectionMonitor::InterfaceType hadInterfaceType = interface;
    const nw_path_status_t previousStatus = status;

    status = nw_path_get_status(state);
    if (wasReachable != isReachable() || previousStatus == nw_path_status_invalid)
        emit q->reachabilityChanged(isReachable());

    nw_path_enumerate_interfaces(state, ^(nw_interface_t nwInterface) {
        if (nw_path_uses_interface_type(state, nw_interface_get_type(nwInterface))) {
            const nw_interface_type_t type = nw_interface_get_type(nwInterface);

            switch (type) {
                case nw_interface_type_wifi:
                    interface = QNetworkConnectionMonitor::InterfaceType::WiFi;
                    break;
                case nw_interface_type_cellular:
                    interface = QNetworkConnectionMonitor::InterfaceType::Cellular;
                    break;
                case nw_interface_type_wired:
                    interface = QNetworkConnectionMonitor::InterfaceType::Ethernet;
                    break;
                default:
                    interface = QNetworkConnectionMonitor::InterfaceType::Unknown;
                    break;
            }

            return false;
        }

        return true;
    });

    if (hadInterfaceType != interface)
        emit q->interfaceTypeChanged(interface);
}

void QNetworkConnectionMonitorPrivate::reset()
{
    stopMonitoring();
    status = nw_path_status_invalid;
}

bool QNetworkConnectionMonitorPrivate::isReachable() const
{
    return status == nw_path_status_satisfied;
}

QNetworkConnectionMonitor::InterfaceType QNetworkConnectionMonitorPrivate::getInterfaceType() const
{
    return interface;
}

bool QNetworkConnectionMonitorPrivate::startMonitoring()
{
    QWriteLocker lock(&monitorLock);
    monitor = nw_path_monitor_create();
    if (monitor == nullptr) {
        qCWarning(lcNetMon, "Failed to create a path monitor, cannot determine current reachability.");
        return false;
    }

    nw_path_monitor_set_update_handler(monitor, [this](nw_path_t path){
        updateState(path);
    });

    auto queue = qt_reachability_queue();
    if (!queue) {
        qCWarning(lcNetMon, "Failed to create a dispatch queue to schedule a probe on");
        nw_release(monitor);
        monitor = nullptr;
        return false;
    }

    nw_path_monitor_set_queue(monitor, queue);
    nw_path_monitor_start(monitor);
    return true;
}

void QNetworkConnectionMonitorPrivate::stopMonitoring()
{
    QWriteLocker lock(&monitorLock);
    if (monitor != nullptr) {
        nw_path_monitor_cancel(monitor);
        nw_release(monitor);
        monitor = nullptr;
    }
}

void QNetworkConnectionMonitor::stopMonitoring()
{
    Q_D(QNetworkConnectionMonitor);
    d->stopMonitoring();
}

bool QNetworkConnectionMonitorPrivate::isMonitoring() const
{
    QReadLocker lock(&monitorLock);
    return monitor != nullptr;
}

QNetworkConnectionMonitor::QNetworkConnectionMonitor()
    : QObject(*new QNetworkConnectionMonitorPrivate)
{
}

QNetworkConnectionMonitor::QNetworkConnectionMonitor(const QHostAddress &/*local*/, const QHostAddress &/*remote*/)
    : QNetworkConnectionMonitor()
{
}

QNetworkConnectionMonitor::~QNetworkConnectionMonitor()
{
    Q_D(QNetworkConnectionMonitor);
    d->reset();
}

bool QNetworkConnectionMonitor::setTargets(const QHostAddress &/*local*/, const QHostAddress &/*remote*/)
{
    return false;
}

bool QNetworkConnectionMonitor::startMonitoring()
{
    Q_D(QNetworkConnectionMonitor);

    if (d->isMonitoring()) {
        qCWarning(lcNetMon, "Monitor is already active, call stopMonitoring() first");
        return false;
    }

    return d->startMonitoring();
}

bool QNetworkConnectionMonitor::isReachable()
{
    Q_D(QNetworkConnectionMonitor);

    if (isMonitoring()) {
        qCWarning(lcNetMon, "Calling isReachable() is unsafe after the monitoring started");
        return false;
    }

    return d->isReachable();
}

QNetworkConnectionMonitor::InterfaceType QNetworkConnectionMonitor::getInterfaceType() const
{
    Q_D(const QNetworkConnectionMonitor);

    if (d->isMonitoring()) {
        qCWarning(lcNetMon, "Calling getInterfaceType() is unsafe after the monitoring started");
        return QNetworkConnectionMonitor::InterfaceType::Unknown;
    }

    return d->getInterfaceType();
}

bool QNetworkConnectionMonitor::isEnabled()
{
    return true;
}

bool QNetworkConnectionMonitor::isMonitoring() const
{
    Q_D(const QNetworkConnectionMonitor);
    return d->isMonitoring();
}

QT_END_NAMESPACE
