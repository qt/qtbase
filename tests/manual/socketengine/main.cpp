// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDebug>
#include <qtest.h>
#include <QTest>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork>
#include <QDebug>
#include <private/qabstractsocketengine_p.h>
#include <cstdio>
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
                std::fill(buf, buf + bufsize, 0);
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

