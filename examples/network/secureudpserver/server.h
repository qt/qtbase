// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef SERVER_H
#define SERVER_H

#include <QtCore>
#include <QtNetwork>

#include <vector>
#include <memory>

//! [0]
class DtlsServer : public QObject
{
    Q_OBJECT

public:
    DtlsServer();
    ~DtlsServer();

    bool listen(const QHostAddress &address, quint16 port);
    bool isListening() const;
    void close();

signals:
    void errorMessage(const QString &message);
    void warningMessage(const QString &message);
    void infoMessage(const QString &message);

    void datagramReceived(const QString &peerInfo, const QByteArray &cipherText,
                          const QByteArray &plainText);

private slots:
    void readyRead();
    void pskRequired(QSslPreSharedKeyAuthenticator *auth);

private:
    void handleNewConnection(const QHostAddress &peerAddress, quint16 peerPort,
                             const QByteArray &clientHello);

    void doHandshake(QDtls *newConnection, const QByteArray &clientHello);
    void decryptDatagram(QDtls *connection, const QByteArray &clientMessage);
    void shutdown();

    bool listening = false;
    QUdpSocket serverSocket;

    QSslConfiguration serverConfiguration;
    QDtlsClientVerifier cookieSender;
    std::vector<std::unique_ptr<QDtls>> knownClients;

    Q_DISABLE_COPY(DtlsServer)
};
//! [0]

#endif // SERVER_H
