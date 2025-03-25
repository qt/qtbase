// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "connection.h"

#include <QTimerEvent>

using namespace std::chrono_literals;

static constexpr auto TransferTimeout = 30s;
static constexpr auto PongTimeout = 60s;
static constexpr auto PingInterval = 5s;

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
 *  greeting    = { 3 => { text, bytes } }
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
    if (isGreetingMessageSent && QAbstractSocket::state() != QAbstractSocket::UnconnectedState) {
        // Indicate clean shutdown.
        writer.endArray();
        waitForBytesWritten(2000);
    }
}

QString Connection::name() const
{
    return username;
}

void Connection::setGreetingMessage(const QString &message, const QByteArray &uniqueId)
{
    greetingMessage = message;
    localUniqueId = uniqueId;
}

QByteArray Connection::uniqueId() const
{
    return peerUniqueId;
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
    if (timerEvent->matches(transferTimer)) {
        abort();
        transferTimer.stop();
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
            if (currentDataType == Greeting) {
                if (state == ReadingGreeting) {
                    if (!reader.isContainer() || !reader.isLengthKnown() || reader.length() != 2)
                        break; // protocol error
                    state = ProcessingGreeting;
                    reader.enterContainer();
                }
                if (state != ProcessingGreeting)
                    break; // protocol error
                if (reader.isString()) {
                    auto r = reader.readString();
                    buffer += r.data;
                } else if (reader.isByteArray()) {
                    auto r = reader.readByteArray();
                    peerUniqueId += r.data;
                    if (r.status == QCborStreamReader::EndOfString) {
                        reader.leaveContainer();
                        processGreeting();
                    }
                }
                if (state == ProcessingGreeting)
                    continue;
            } else if (reader.isString()) {
                auto r = reader.readString();
                buffer += r.data;
                if (r.status != QCborStreamReader::EndOfString)
                    continue;
            } else if (reader.isNull()) {
                reader.next();
            } else {
                break; // protocol error
            }

            // Next state: no command read
            reader.leaveContainer();
            transferTimer.stop();

            processData();
        }
    }

    if (reader.lastError() != QCborError::EndOfFile)
        abort();       // parse error

    if (transferTimer.isActive() && reader.containerDepth() > 1)
        transferTimer.start(TransferTimeout, this);
}

void Connection::sendPing()
{
    if (pongTime.durationElapsed() > PongTimeout) {
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
    writer.startArray(2);
    writer.append(greetingMessage);
    writer.append(localUniqueId);
    writer.endArray();
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
        pongTime.start();
        break;
    default:
        break;
    }

    currentDataType = Undefined;
    buffer.clear();
}
