// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>

#include <QtNetwork/qhostinfo.h>
#include <QtNetwork/qnetworkinformation.h>
#include <QtNetwork/private/qnetconmonitor_p.h>

#include <QtTest/qsignalspy.h>

void testDetectRouteDisrupted();

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (!QNetworkConnectionMonitor::isEnabled()) {
        qWarning("QNetworkConnectionMonitor is not enabled for this platform!");
        return 0;
    }

    while (true) {

        QByteArray indent("  ");
        {
            QTextStream writer(stdout);
            writer << "Manual test for QNetworkConnection}Monitor\n"
                   << "The tests are grouped by what they test. Run them in any order\n"
                   << "- QNetworkConnectionMonitor tests:\n"
                   << indent << "r" << indent << "Test detection of disruption of route to target.\n"
                   << "- General\n"
                   << indent << "q" << indent << "Quit the test.\n"
                   << "> ";
        }

        auto getCommand = []() {
            char ch;
            QTextStream reader(stdin);
            reader >> ch;
            return ch;
        };

        switch (getCommand()) {
        case 'r':
            testDetectRouteDisrupted();
            break;
        case 'q':
            return 0;
        }
    }
    Q_UNREACHABLE();
    return 0;
}

bool ensureNetworkAccessible(QTextStream &writer)
{
    auto netInfo = QNetworkInformation::instance();
    if (netInfo->reachability() == QNetworkInformation::Reachability::Disconnected) {
        writer << "Network currently not accessible, please make sure you have an internet "
                  "connection. Will wait for a connection for 20 seconds.\n";
        writer.flush();
        QDeadlineTimer timer{ 20 * 1000 };
        while (!timer.hasExpired()
               && netInfo->reachability() == QNetworkInformation::Reachability::Disconnected) {
            QCoreApplication::processEvents();
        }
        if (netInfo->reachability() == QNetworkInformation::Reachability::Disconnected) {
            writer << "Error: No network in 20 seconds, ending now!\n";
            return false;
        }
        writer << "Network successfully connected, thanks!\n";
    }
    return true;
}

void testDetectRouteDisrupted()
{
    QTextStream writer(stdout);

    {
        if (!QNetworkInformation::loadDefaultBackend()) {
            writer << "Error: Failed to start";
            return;
        }
        if (!ensureNetworkAccessible(writer))
            return;
    }

    QNetworkConnectionMonitor connection;

    auto readLineFromStdin = []() -> QString {
        QTextStream in(stdin);
        return in.readLine();
    };

    writer << "Type your local IP address: ";
    writer.flush();
    const QHostAddress local{ readLineFromStdin() };
    if (local.isNull()) {
        writer << "Error: The address is invalid!\n";
        return;
    }

    const QHostAddress defaultAddress{ QString::fromLatin1("1.1.1.1") };
    QHostAddress remote;
    do {
        writer << "Type a remote IP address [" << defaultAddress.toString() << "]: ";
        writer.flush();
        QString address = readLineFromStdin();
        if (address.isEmpty()) {
            remote = defaultAddress;
        } else {
            QHostAddress remoteTemp{ address };
            if (remoteTemp.isNull()) {
                writer << "Invalid address\n";
            } else {
                remote = remoteTemp;
            }
        }
    } while (remote.isNull());

    if (!connection.setTargets(local, remote)) {
        writer << "Error: Failed to set the targets!\n";
        return;
    }
    if (!connection.isReachable()) {
        writer << "Error: Target is not reachable!\n";
        return;
    }
    if (!connection.startMonitoring()) {
        writer << "Error: Failed to start monitoring!\n";
        return;
    }

    QSignalSpy reachabilitySpy(&connection, &QNetworkConnectionMonitor::reachabilityChanged);

    writer << "QNetworkConnectionMonitor might assume the target is initially reachable.\n"
              "If it is not reachable then this test might not work correctly.\n"
              "Please disrupt the connection between your machine and the target within 20 "
              "seconds\n";
    writer.flush();
    reachabilitySpy.wait(20 * 1000);
    if (reachabilitySpy.count() == 0) {
        writer << "Error: There was a disconnection but there was no signal emitted!\n";
        return;
    }
    // Get the final parameter of the final signal emission and make sure it is false.
    if (reachabilitySpy.last().last().toBool()) {
        writer << "Error: There was a disconnection but the latest signal emitted says the target is "
                  "reachable!\n";
        return;
    }
    writer << "Success, connection disruption was detected!\n";
}
