// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "connection.h"

#include <QTimerEvent>

static const int TransferTimeout = 30 * 1000;
static const int PongTimeout = 60 * 1000;
static const int PingInterval = 5 * 1000;

/*
 * Protocol is defined as follows, using the CBOR Data Definition Language:
 *
 *  protocol    = [
 *     greeting,        ; must start with a greeting command
 *     * command        ; zero or more regular commands after
 *  ]
 *  command     = plaintext / ping / pong / greeting
 *  plaintext   = { 0 => text }
 *  ping        = { 1 => null }
 *  pong        = { 2 => null }
 *  greeting    = { 3 => text }
 */

Connection::Connection(QObject *parent)
    : QTcpSocket(parent), writer(this)
{
    pingTimer.setInterval(PingInterval);

    connect(this, &QTcpSocket::readyRead, this,
            &Connection::processReadyRead);
    connect(this, &QTcpSocket::disconnected,
            &pingTimer, &QTimer::stop);
    connect(&pingTimer, &QTimer::timeout,
            this, &Connection::sendPing);
    connect(this, &QTcpSocket::connected,
            this, &Connection::sendGreetingMessage);
}

Connection::Connection(qintptr socketDescriptor, QObject *parent)
    : Connection(parent)
{
    setSocketDescriptor(socketDescriptor);
    reader.setDevice(this);
}

Connection::~Connection()
{
    if (isGreetingMessageSent) {
        // Indicate clean shutdown.
        writer.endArray();
        waitForBytesWritten(2000);
    }
}

QString Connection::name() const
{
    return username;
}

void Connection::setGreetingMessage(const QString &message)
{
    greetingMessage = message;
}

bool Connection::sendMessage(const QString &message)
{
    if (message.isEmpty())
        return false;

    writer.startMap(1);
    writer.append(PlainText);
    writer.append(message);
    writer.endMap();
    return true;
}

void Connection::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() == transferTimerId) {
        abort();
        killTimer(transferTimerId);
        transferTimerId = -1;
    }
}

void Connection::processReadyRead()
{
    // we've got more data, let's parse
    reader.reparse();
    while (reader.lastError() == QCborError::NoError) {
        if (state == WaitingForGreeting) {
            if (!reader.isArray())
                break;                  // protocol error

            reader.enterContainer();    // we'll be in this array forever
            state = ReadingGreeting;
        } else if (reader.containerDepth() == 1) {
            // Current state: no command read
            // Next state: read command ID
            if (!reader.hasNext()) {
                reader.leaveContainer();
                disconnectFromHost();
                return;
            }

            if (!reader.isMap() || !reader.isLengthKnown() || reader.length() != 1)
                break;                  // protocol error
            reader.enterContainer();
        } else if (currentDataType == Undefined) {
            // Current state: read command ID
            // Next state: read command payload
            if (!reader.isInteger())
                break;                  // protocol error
            currentDataType = DataType(reader.toInteger());
            reader.next();
        } else {
            // Current state: read command payload
            if (reader.isString()) {
                auto r = reader.readString();
                buffer += r.data;
                if (r.status != QCborStreamReader::EndOfString)
                    continue;
            } else if (reader.isNull()) {
                reader.next();
            } else {
                break;                   // protocol error
            }

            // Next state: no command read
            reader.leaveContainer();
            if (transferTimerId != -1) {
                killTimer(transferTimerId);
                transferTimerId = -1;
            }

            if (state == ReadingGreeting) {
                if (currentDataType != Greeting)
                    break;              // protocol error
                processGreeting();
            } else {
                processData();
            }
        }
    }

    if (reader.lastError() != QCborError::EndOfFile)
        abort();       // parse error

    if (transferTimerId != -1 && reader.containerDepth() > 1)
        transferTimerId = startTimer(TransferTimeout);
}

void Connection::sendPing()
{
    if (pongTime.elapsed() > PongTimeout) {
        abort();
        return;
    }

    writer.startMap(1);
    writer.append(Ping);
    writer.append(nullptr);     // no payload
    writer.endMap();
}

void Connection::sendGreetingMessage()
{
    writer.startArray();        // this array never ends

    writer.startMap(1);
    writer.append(Greeting);
    writer.append(greetingMessage);
    writer.endMap();
    isGreetingMessageSent = true;

    if (!reader.device())
        reader.setDevice(this);
}

void Connection::processGreeting()
{
    username = buffer + '@' + peerAddress().toString() + ':'
            + QString::number(peerPort());
    currentDataType = Undefined;
    buffer.clear();

    if (!isValid()) {
        abort();
        return;
    }

    if (!isGreetingMessageSent)
        sendGreetingMessage();

    pingTimer.start();
    pongTime.start();
    state = ReadyForUse;
    emit readyForUse();
}

void Connection::processData()
{
    switch (currentDataType) {
    case PlainText:
        emit newMessage(username, buffer);
        break;
    case Ping:
        writer.startMap(1);
        writer.append(Pong);
        writer.append(nullptr);     // no payload
        writer.endMap();
        break;
    case Pong:
        pongTime.restart();
        break;
    default:
        break;
    }

    currentDataType = Undefined;
    buffer.clear();
}
