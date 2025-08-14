// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![file]
#include <QCoreApplication>
#include <QNetworkInformation>
#include <QHostAddress>
#include <QDebug>

//! [0]
// Simple helper to decide whether an IP address is "local"
bool isLocalAddress(const QHostAddress &address)
{
    return address.isInSubnet(QHostAddress("192.168.0.0"), 16) ||
           address.isInSubnet(QHostAddress("10.0.0.0"), 8)     ||
           address.isInSubnet(QHostAddress("172.16.0.0"), 12)  ||
           address.isLoopback();
}


int main(int argc, char *argv[])
{
//! [0]
    QCoreApplication app(argc, argv);

    // Load the default backend for QNetworkInformation
    if (!QNetworkInformation::loadDefaultBackend()) {
        qWarning() << "Failed to load QNetworkInformation backend. Exiting.";
        return 1;
    }

    QNetworkInformation *networkInfo = QNetworkInformation::instance();
//! [1]
    // Target IP address (default: Google DNS)
    QString targetIpStr = argc > 1 ? argv[1] : "8.8.8.8";
    QHostAddress targetIp(targetIpStr);

    if (targetIp.isNull()) {
        qWarning() << "Invalid IP address:" << targetIpStr;
        return 1;
    }

    // Decide what level of reachability is needed for the target
    QNetworkInformation::Reachability requiredReachability =
        isLocalAddress(targetIp)
            ? QNetworkInformation::Reachability::Local
            : QNetworkInformation::Reachability::Online;

    // Fetch the current system-reported reachability
    QNetworkInformation::Reachability currentReachability = networkInfo->reachability();

    qDebug() << "Target IP:" << targetIp.toString();
    qDebug() << "Target is considered"
             << (isLocalAddress(targetIp) ? "local/site." : "external/online.");
    qDebug() << "Required reachability level:" << requiredReachability;
    qDebug() << "Current reachability:" << currentReachability;

    if (currentReachability < requiredReachability) {
        qWarning() << "Current network state may not allow reaching the target address.";
    } else {
        qDebug() << "Target may be reachable based on current network state.";
    }
//! [1]
    return 0;
}
//![file]
