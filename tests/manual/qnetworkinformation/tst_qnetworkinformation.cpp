/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifdef MOBILE
#    include "mainwindow.h"
#    include <QtWidgets/qapplication.h>
#else
#    include <QtCore/qcoreapplication.h>
#endif

#include <QtCore/qdebug.h>
#include <QtNetwork/qnetworkinformation.h>

int main(int argc, char **argv)
{
#ifdef MOBILE
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
#else
    QCoreApplication app(argc, argv);
#endif

    // Use the platform-default:
    if (!QNetworkInformation::loadDefaultBackend()) {
        qWarning("Failed to load any backend");
        qDebug() << "Backends available:" << QNetworkInformation::availableBackends().join(", ");
        return -1;
    }
    QNetworkInformation *info = QNetworkInformation::instance();
    qDebug() << "Backend loaded:" << info->backendName();
    qDebug() << "Supports:" << info->supportedFeatures();
    qDebug() << "Now you can make changes to the current network connection. Qt should see the "
                "changes and notify about it.";
    QObject::connect(info, &QNetworkInformation::reachabilityChanged,
                     [](QNetworkInformation::Reachability newStatus) {
                         qDebug() << "Updated:" << newStatus;
                     });

    QObject::connect(info, &QNetworkInformation::isBehindCaptivePortalChanged,
                     [](bool status) { qDebug() << "Updated, behind captive portal:" << status; });

    QObject::connect(info, &QNetworkInformation::transportMediumChanged,
                     [](QNetworkInformation::TransportMedium newMedium) {
                         qDebug() << "Updated, current transport medium:" << newMedium;
                     });

    QObject::connect(info, &QNetworkInformation::isMeteredChanged,
                     [](bool metered) {
                         qDebug() << "Updated, metered:" << metered;
                     });

#ifdef MOBILE
    // Some extra connections to update the window if we're on mobile
    QObject::connect(info, &QNetworkInformation::reachabilityChanged, &window,
                     &MainWindow::updateReachability);
    QObject::connect(info, &QNetworkInformation::isBehindCaptivePortalChanged, &window,
                     &MainWindow::updateCaptiveState);
    QObject::connect(info, &QNetworkInformation::transportMediumChanged, &window,
                     &MainWindow::updateTransportMedium);
    QObject::connect(info, &QNetworkInformation::isMeteredChanged, &window,
                     &MainWindow::updateMetered);
#endif

    qDebug() << "Initial reachability:" << info->reachability();
    qDebug() << "Behind captive portal:" << info->isBehindCaptivePortal();
    qDebug() << "Transport medium:" << info->transportMedium();
    qDebug() << "Is metered:" << info->isMetered();

    return app.exec();
}

// Include the moc output of the MainWindow from here
#ifdef MOBILE
#    include "moc_mainwindow.cpp"
#endif
