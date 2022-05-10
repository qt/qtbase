// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include "minihttpserver.h"
#include <QtCore/QFile>
#include <QtCore/QSemaphore>
#include <QtCore/QUrl>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

MiniHttpServer::MiniHttpServer(QObject *parent) :
    QThread(parent)
{
    readyToGo = new QSemaphore;
    start();
    readyToGo->acquire();
    delete readyToGo;
}

MiniHttpServer::~MiniHttpServer()
{
    quitObject->deleteLater();
    wait();
}

void MiniHttpServer::run()
{
    server = new QTcpServer;
    server->listen(QHostAddress::LocalHost);
    portnum = server->serverPort();
    connect(server, SIGNAL(newConnection()), this, SLOT(handleConnection()), Qt::DirectConnection);

    quitObject = new QObject;
    connect(quitObject, SIGNAL(destroyed()), this, SLOT(quit()), Qt::DirectConnection);

    readyToGo->release();
    exec();

    // cleanup
    delete server;
}

void MiniHttpServer::handleConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket *socket = server->nextPendingConnection();
        new MiniHttpServerConnection(socket); // handles its own lifetime
    }
}

MiniHttpServerConnection::MiniHttpServerConnection(QTcpSocket *socket)
    : QObject(socket), socket(socket), source(0)
{
    connect(socket, SIGNAL(readyRead()), SLOT(handleReadyRead()));
    connect(socket, SIGNAL(bytesWritten(qint64)), SLOT(handleBytesWritten()));
    connect(socket, SIGNAL(disconnected()), SLOT(handleDisconnected()));

    timeout.setInterval(30000);
    timeout.setSingleShot(true);
    connect(&timeout, SIGNAL(timeout()), SLOT(handleTimeout()));
    timeout.start();
}

void MiniHttpServerConnection::sendError500()
{
    static const char body[] =
            "HTTP/1.1 500 Server Error\r\n"
            "Connection: close\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
    socket->write(body, strlen(body));
    socket->disconnectFromHost();
}

void MiniHttpServerConnection::sendError404()
{
    static const char body1[] =
            "HTTP/1.1 404 File not found\r\n"
            "Content-Length: 0\r\n";
    socket->write(body1, strlen(body1));
    if (connectionClose) {
        socket->write("Connection: close\r\n\r\n");
        socket->disconnectFromHost();
    } else {
        socket->write("\r\n");
        handlePendingRequest();
    }
}

void MiniHttpServerConnection::handlePendingRequest()
{
    int endOfRequest = buffer.indexOf("\r\n\r\n");
    if (endOfRequest == -1)
        return; // nothing to do

    QByteArray request = buffer.left(endOfRequest);
    buffer = buffer.mid(endOfRequest + 4);
    //qDebug("request: %s", request.constData());

    if (!request.startsWith("GET ")) {
        sendError500();
        return;
    }

    int eol = request.indexOf("\r\n");
    static const char http11[] = " HTTP/1.1";
    if (memcmp(request.data() + eol - strlen(http11), http11, strlen(http11)) != 0) {
        sendError500();
        return;
    }

    QUrl uri = QUrl::fromEncoded(request.mid(4, eol - int(strlen(http11)) - 4));
    source.setFileName(QLatin1Char(':') + uri.path());

    // connection-close?
    request = request.toLower();
    connectionClose = request.contains("\r\nconnection: close\r\n");

    if (!source.open(QIODevice::ReadOnly)) {
        sendError404();
        return;
    }

    // success
    timeout.stop();
    static const char body[] =
            "HTTP/1.1 200 Ok\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: ";
    socket->write(body, strlen(body));
    socket->write(QByteArray::number(source.size()));
    if (connectionClose)
        socket->write("\r\nConnection: close");
    socket->write("\r\n\r\n");

    handleBytesWritten();
}

void MiniHttpServerConnection::handleReadyRead()
{
    buffer += socket->readAll();
    if (!source.isOpen())
        handlePendingRequest();
}

void MiniHttpServerConnection::handleDisconnected()
{
    socket->deleteLater(); // will delete us too
}

void MiniHttpServerConnection::handleBytesWritten()
{
    qint64 maxBytes = qMin<qint64>(128*1024, source.bytesAvailable());
    maxBytes = qMin(maxBytes, 128*1024 - socket->bytesToWrite());
    if (maxBytes < 0)
        return;

    socket->write(source.read(maxBytes));

    if (source.atEnd()) {
        // file ended
        source.close();
        if (connectionClose) {
            socket->disconnectFromHost();
        } else {
            timeout.start();
            handlePendingRequest();
        }
    }
}

void MiniHttpServerConnection::handleTimeout()
{
    socket->disconnectFromHost();
}
