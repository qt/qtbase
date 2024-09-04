// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CONNECTION_H
#define CONNECTION_H

#include <QBasicTimer>
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

class Connection : public QTcpSocket
{
    Q_OBJECT

public:
    enum ConnectionState {
        WaitingForGreeting,
        ReadingGreeting,
        ProcessingGreeting,
        ReadyForUse
    };
    enum DataType {
        PlainText,
        Ping,
        Pong,
        Greeting,
        Undefined
    };

    explicit Connection(QObject *parent = nullptr);
    explicit Connection(qintptr socketDescriptor, QObject *parent = nullptr);
    ~Connection();

    QString name() const;
    void setGreetingMessage(const QString &message, const QByteArray &uniqueId);
    bool sendMessage(const QString &message);

    QByteArray uniqueId() const;

signals:
    void readyForUse();
    void newMessage(const QString &from, const QString &message);

protected:
    void timerEvent(QTimerEvent *timerEvent) override;

private slots:
    void processReadyRead();
    void sendPing();
    void sendGreetingMessage();

private:
    bool hasEnoughData();
    void processGreeting();
    void processData();

    QCborStreamReader reader;
    QCborStreamWriter writer;
    QString greetingMessage = tr("undefined");
    QString username = tr("unknown");
    QTimer pingTimer;
    QElapsedTimer pongTime;
    QString buffer;
    QByteArray localUniqueId;
    QByteArray peerUniqueId;
    ConnectionState state = WaitingForGreeting;
    DataType currentDataType = Undefined;
    QBasicTimer transferTimer;
    bool isGreetingMessageSent = false;
};

#endif
