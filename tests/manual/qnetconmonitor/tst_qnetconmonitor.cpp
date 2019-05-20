/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>

#include <QtNetwork/qhostinfo.h>
#include <QtNetwork/private/qnetconmonitor_p.h>

#include <QtTest/qsignalspy.h>

void testDetectDisconnection();
void testDetectRouteDisrupted();

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (!QNetworkStatusMonitor::isEnabled()) {
        qWarning("QNetworkStatusMonitor is not enabled for this platform!");
        return 0;
    }

    while (true) {

        QByteArray indent("  ");
        {
            QTextStream writer(stdout);
            writer << "Manual test for QNetwork{Status|Connection}Monitor\n"
                   << "The tests are grouped by what they test. Run them in any order\n"
                   << "- QNetworkStatusMonitor tests:\n"
                   << indent << "c" << indent << "Test connection and disconnection detection.\n"
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
        case 'c':
            testDetectDisconnection();
            break;
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

bool ensureNetworkAccessible(QNetworkStatusMonitor &status, QTextStream &writer)
{
    if (!status.isNetworkAccessible()) {
        writer << "Network currently not accessible, please make sure you have an internet "
                  "connection. Will wait for a connection for 20 seconds.\n";
        writer.flush();
        QDeadlineTimer timer{ 20 * 1000 };
        while (!timer.hasExpired() && !status.isNetworkAccessible())
            QCoreApplication::processEvents();
        if (!status.isNetworkAccessible()) {
            writer << "Error: No network in 20 seconds, ending now!\n";
            return false;
        }
        writer << "Network successfully connected, thanks!\n";
    }
    return true;
}

void testDetectDisconnection()
{
    QTextStream writer(stdout);
    QNetworkStatusMonitor status;

    if (!status.start()) {
        writer << "Error: Failed to start";
        return;
    }

    if (!ensureNetworkAccessible(status, writer))
        return;

    QSignalSpy onlineStateSpy(&status, &QNetworkStatusMonitor::onlineStateChanged);

    writer << "Please disconnect from the internet within 20 seconds\n";
    writer.flush();
    QDeadlineTimer timer{ 20 * 1000 };
    while (!timer.hasExpired() && status.isNetworkAccessible())
        QCoreApplication::processEvents();
    if (status.isNetworkAccessible()) {
        writer << "Error: Still connected after 20 seconds, ending now!\n";
        return;
    }
    if (onlineStateSpy.count() == 0) {
        writer << "Error: There was a disconnection but there was no signal emitted!\n";
        return;
    }
    // Get the final parameter of the final signal emission and make sure it is false.
    if (onlineStateSpy.last().last().toBool()) {
        writer << "Error: There was a disconnection but the latest signal emitted says we are online!\n";
        return;
    }
    writer << "Success, connection loss was detected!\n";
}

void testDetectRouteDisrupted()
{
    QTextStream writer(stdout);

    {
        QNetworkStatusMonitor status;
        if (!status.start()) {
            writer << "Error: Failed to start";
            return;
        }
        if (!ensureNetworkAccessible(status, writer))
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
