// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![file]
#include <QCoreApplication>
#include <QNetworkInformation>
#include <QDebug>

void onReachabilityChanged(QNetworkInformation::Reachability reachability) {
    switch (reachability) {
    case QNetworkInformation::Reachability::Unknown:
        qDebug() << "Network reachability is unknown.";
        break;
    case QNetworkInformation::Reachability::Disconnected:
        qDebug() << "Network is disconnected.";
        break;
    case QNetworkInformation::Reachability::Local:
        qDebug() << "Network is locally reachable.";
        break;
    case QNetworkInformation::Reachability::Site:
        qDebug() << "Network can reach the site.";
        break;
    case QNetworkInformation::Reachability::Online:
        qDebug() << "Network is online.";
        break;
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // Check if QNetworkInformation is supported
    if (!QNetworkInformation::loadDefaultBackend()) {
        qWarning() << "QNetworkInformation is not supported on this platform or backend.";
        return 1;
    }

    QNetworkInformation* netInfo = QNetworkInformation::instance();

    // Connect to the reachabilityChanged signal
    QObject::connect(netInfo, &QNetworkInformation::reachabilityChanged,
                     &onReachabilityChanged);

    // Print initial status
    onReachabilityChanged(netInfo->reachability());

    return a.exec();
}
//![file]
