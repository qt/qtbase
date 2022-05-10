// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class DtlsServer : public QObject
{
public:
    bool listen(const QHostAddress &address, quint16 port);
    // ...

private:
    void readyRead();
    // ...

    QUdpSocket serverSocket;
    QDtlsClientVerifier verifier;
    // ...
};

bool DtlsServer::listen(const QHostAddress &serverAddress, quint16 serverPort)
{
    if (serverSocket.bind(serverAddress, serverPort))
        connect(&serverSocket, &QUdpSocket::readyRead, this, &DtlsServer::readyRead);
    return serverSocket.state() == QAbstractSocket::BoundState;
}

void DtlsServer::readyRead()
{
    QByteArray dgram(serverSocket.pendingDatagramSize(), Qt::Uninitialized);
    QHostAddress address;
    quint16 port = {};
    serverSocket.readDatagram(dgram.data(), dgram.size(), &address, &port);
    if (verifiedClients.contains({address, port}) {
        // This client was verified previously, we either continue the
        // handshake or decrypt the incoming message.
    } else if (verifier.verifyClient(&serverSocket, dgram, address, port)) {
        // Apparently we have a real DTLS client who wants to send us
        // encrypted datagrams. Remember this client as verified
        // and proceed with a handshake.
    } else {
        // No matching cookie was found in the incoming datagram,
        // verifyClient() has sent a ClientVerify message.
        // We'll hear from the client again soon, if they're real.
    }
}
//! [0]

//! [1]
void DtlsServer::updateServerSecret()
{
    const QByteArray newSecret(generateCryptoStrongSecret());
    if (newSecret.size()) {
        usedCookies.append(newSecret);
        verifier.setCookieGeneratorParameters({QCryptographicHash::Sha1, newSecret});
    }
}
//! [1]

//! [2]
if (!verifier.verifyClient(&socket, message, address, port)) {
    switch (verifyClient.dtlsError()) {
    case QDtlsError::NoError:
        // Not verified yet, but no errors found and we have to wait for the next
        // message from this client.
        return;
    case QDtlsError::TlsInitializationError:
        // This error is fatal, nothing we can do about it.
        // Probably, quit the server after reporting the error.
        return;
    case QDtlsError::UnderlyingSocketError:
        // There is some problem in QUdpSocket, handle it (see QUdpSocket::error())
        return;
    case QDtlsError::InvalidInputParameters:
    default:
        Q_UNREACHABLE();
    }
}
//! [2]
