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

#include <QDebug>
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork>
#include <QDebug>
#include <private/qabstractsocketengine_p.h>
#include <cstdio>
#include <QNetworkConfigurationManager>
#include <QNetworkConfiguration>
#include <QNetworkSession>
#include <QCoreApplication>

const int bufsize = 16*1024;
char buf[bufsize];

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);

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
                qFill(buf, buf + bufsize, 0);
                ret = socketEngine->read(buf, available);
                if (ret > 0) {
                    printf("%s", buf);
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

