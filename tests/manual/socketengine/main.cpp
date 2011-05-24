/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include "../../auto/network-settings.h"
#include <QtNetwork>
#include <QDebug>
#include <private/qabstractsocketengine_p.h>
#include <cstdio>
#include <strings.h>
#include <QNetworkConfigurationManager>
#include <QNetworkConfiguration>
#include <QNetworkSession>
#include <QCoreApplication>

const int bufsize = 16*1024;
char buf[bufsize];

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);

#ifdef Q_OS_SYMBIAN
    QNetworkConfigurationManager configurationManager;
    QNetworkConfiguration configuration = configurationManager.defaultConfiguration();
    if (!configuration.isValid()) {
        qDebug() << "Got an invalid session configuration";
        exit(1);
    }

    qDebug() << "Opening session...";
    QNetworkSession *session = new QNetworkSession(configuration);

    // Does not work:
//    session->open();
//    session->waitForOpened();

    // works:
    QEventLoop loop;
    QObject::connect(session, SIGNAL(opened()), &loop, SLOT(quit()), Qt::QueuedConnection);
    QMetaObject::invokeMethod(session, "open", Qt::QueuedConnection);
    loop.exec();


    if (session->isOpen()) {
        qDebug() << "session opened";
    } else {
        qDebug() << "session could not be opened -" << session->errorString();
        exit(1);
    }
#endif

    // create it
    QAbstractSocketEngine *socketEngine =
            QAbstractSocketEngine::createSocketEngine(QAbstractSocket::TcpSocket, QNetworkProxy(QNetworkProxy::NoProxy), 0);
    if (!socketEngine) {
        qDebug() << "could not create engine";
        exit(1);
    }

    // initialize it
    bool initialized = socketEngine->initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol);
    if (!initialized) {
        qDebug() << "not able to initialize engine";
        exit(1);
    }

    // wait for connected
    int r = socketEngine->connectToHost(QHostAddress("74.125.77.99"), 80); // google
    bool readyToRead = false;
    bool readyToWrite = false;
    socketEngine->waitForReadOrWrite(&readyToRead, &readyToWrite, true, true, 10*1000);
    if (r <= 0) //timeout or error
        exit(1);
    if (readyToWrite) {
        // write the request
        QByteArray request("GET /robots.txt HTTP/1.0\r\n\r\n");
        int ret = socketEngine->write(request.constData(), request.length());
        if (ret == request.length()) {
            // read the response in a loop
            do {
                bool waitReadResult = socketEngine->waitForRead(10*1000);
                int available = socketEngine->bytesAvailable();
                if (waitReadResult == true && available == 0) {
                    // disconnected
                    exit(0);
                }
                bzero(buf, bufsize);
                ret = socketEngine->read(buf, available);
                if (ret > 0) {
#ifdef Q_OS_SYMBIAN
                    qDebug() << buf; //printf goes only to screen, this goes to remote debug channel
#else
                    printf("%s", buf);
#endif
                } else {
                    // some failure when reading
                    exit(1);
                }
            } while (1);
        } else {
            qDebug() << "failed writing";
        }
    } else {
        qDebug() << "failed connecting";
    }
    delete socketEngine;
}

