/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QCoreApplication>
#include <QStringList>
#include <QLocalSocket>

#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/qnetworkconfiguration.h>
#include <QtNetwork/qnetworkconfigmanager.h>
#include <QtNetwork/qnetworksession.h>
#endif

#include <QEventLoop>
#include <QTimer>
#include <QDebug>

QT_USE_NAMESPACE


#define NO_DISCOVERED_CONFIGURATIONS_ERROR 1
#define SESSION_OPEN_ERROR 2


int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

#ifndef QT_NO_BEARERMANAGEMENT
    // Update configurations so that everything is up to date for this process too.
    // Event loop is used to wait for awhile.
    QNetworkConfigurationManager manager;
    manager.updateConfigurations();
    QEventLoop iIgnoreEventLoop;
    QTimer::singleShot(3000, &iIgnoreEventLoop, SLOT(quit()));
    iIgnoreEventLoop.exec();

    QList<QNetworkConfiguration> discovered =
        manager.allConfigurations(QNetworkConfiguration::Discovered);

        foreach(QNetworkConfiguration config, discovered) {
            qDebug() << "Lackey: Name of the config enumerated: " << config.name();
            qDebug() << "Lackey: State of the config enumerated: " << config.state();
        }

    if (discovered.isEmpty()) {
        qDebug("Lackey: no discovered configurations, returning empty error.");
        return NO_DISCOVERED_CONFIGURATIONS_ERROR;
    }

    // Cannot read/write to processes on WinCE.
    // Easiest alternative is to use sockets for IPC.
    QLocalSocket oopSocket;

    oopSocket.connectToServer("tst_qnetworksession");
    oopSocket.waitForConnected(-1);

    qDebug() << "Lackey started";

    QNetworkSession *session = 0;
    do {
        if (session) {
            delete session;
            session = 0;
        }

        qDebug() << "Discovered configurations:" << discovered.count();

        if (discovered.isEmpty()) {
            qDebug() << "No more discovered configurations";
            break;
        }

        qDebug() << "Taking first configuration";

        QNetworkConfiguration config = discovered.takeFirst();

        if ((config.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
            qDebug() << config.name() << "is active, therefore skipping it (looking for configs in 'discovered' state).";
            continue;
        }

        qDebug() << "Creating session for" << config.name() << config.identifier();

        session = new QNetworkSession(config);

        QString output = QString("Starting session for %1\n").arg(config.identifier());
        oopSocket.write(output.toLatin1());
        oopSocket.waitForBytesWritten();
        session->open();
        session->waitForOpened();
    } while (!(session && session->isOpen()));

    qDebug() << "lackey: loop done";

    if (!session) {
        qDebug() << "Could not start session";

        oopSocket.disconnectFromServer();
        oopSocket.waitForDisconnected(-1);

        return SESSION_OPEN_ERROR;
    }

    QByteArray output = "Started session for "
        + session->configuration().identifier().toLatin1() + '\n';
    oopSocket.write(output);
    oopSocket.waitForBytesWritten();

    oopSocket.waitForReadyRead();
    oopSocket.readLine();

    session->stop();

    delete session;

    oopSocket.disconnectFromServer();
    oopSocket.waitForDisconnected(-1);
#endif

    return 0;
}
