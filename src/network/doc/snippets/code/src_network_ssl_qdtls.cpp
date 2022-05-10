// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

using namespace Qt::StringLiterals;
//! [0]
// A client initiates a handshake:
QUdpSocket clientSocket;
QDtls clientDtls;
clientDtls.setPeer(address, port, peerName);
clientDtls.doHandshake(&clientSocket);

// A server accepting an incoming connection; address, port, clientHello are
// read by QUdpSocket::readDatagram():
QByteArray clientHello(serverSocket.pendingDatagramSize(), Qt::Uninitialized);
QHostAddress address;
quin16 port = {};
serverSocket.readDatagram(clientHello.data(), clientHello.size(), &address, &port);

QDtls serverDtls;
serverDtls.setPeer(address, port);
serverDtls.doHandshake(&serverSocket, clientHello);

// Handshake completion, both for server and client:
void DtlsConnection::continueHandshake(const QByteArray &datagram)
{
    if (dtls.doHandshake(&udpSocket, datagram)) {
        // Check handshake status:
        if (dtls.handshakeStatus() == QDlts::HandshakeComplete) {
            // Secure DTLS connection is now established.
        }
    } else {
        // Error handling.
    }
}

//! [0]

//! [1]
DtlsClient::DtlsClient()
{
    // Some initialization code here ...
    connect(&clientDtls, &QDtls::handshakeTimeout, this, &DtlsClient::handleTimeout);
}

void DtlsClient::handleTimeout()
{
    clientDtls.handleTimeout(&clientSocket);
}
//! [1]

//! [2]
// Sending an encrypted datagram:
dtlsConnection.writeDatagramEncrypted(&clientSocket, "Hello DTLS server!");

// Decryption:
QByteArray encryptedMessage(dgramSize);
socket.readDatagram(encryptedMessage.data(), dgramSize);
const QByteArray plainText = dtlsConnection.decryptDatagram(&socket, encryptedMessage);
//! [2]

//! [3]
DtlsClient::~DtlsClient()
{
    clientDtls.shutdown(&clientSocket);
}
//! [3]

//! [4]
auto config = QSslConfiguration::defaultDtlsConfiguration();
config.setDtlsCookieVerificationEnabled(false);
// Some other customization ...
dtlsConnection.setDtlsConfiguration(config);
//! [4]

//! [5]
if (!dtls.doHandshake(&socket, dgram)) {
    if (dtls.dtlsError() == QDtlsError::PeerVerificationError)
        dtls.abortAfterError(&socket);
}
//! [5]

//! [6]
QList<QSslCertificate> cert = QSslCertificate::fromPath("server-certificate.pem"_L1);
QSslError error(QSslError::SelfSignedCertificate, cert.at(0));
QList<QSslError> expectedSslErrors;
expectedSslErrors.append(error);

QDtls dtls;
dtls.ignoreVerificationErrors(expectedSslErrors);
dtls.doHandshake(udpSocket);
//! [6]

