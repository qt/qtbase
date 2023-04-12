// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

using namespace Qt::StringLiterals;

//! [0]
QSslSocket *socket = new QSslSocket(this);
connect(socket, &QSslSocket::encrypted, this, &Receiver::ready);

socket->connectToHostEncrypted("imap.example.com", 993);
//! [0]


//! [1]
void SslServer::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket *serverSocket = new QSslSocket;
    if (serverSocket->setSocketDescriptor(socketDescriptor)) {
        addPendingConnection(serverSocket);
        connect(serverSocket, &QSslSocket::encrypted, this, &SslServer::ready);
        serverSocket->startServerEncryption();
    } else {
        delete serverSocket;
    }
}
//! [1]


//! [2]
QSslSocket socket;
socket.connectToHostEncrypted("http.example.com", 443);
if (!socket.waitForEncrypted()) {
    qDebug() << socket.errorString();
    return false;
}

socket.write("GET / HTTP/1.0\r\n\r\n");
while (socket.waitForReadyRead())
    qDebug() << socket.readAll().data();
//! [2]


//! [3]
QSslSocket socket;
connect(&socket, &QSslSocket::encrypted, receiver, &Receiver::socketEncrypted);

socket.connectToHostEncrypted("imap", 993);
socket->write("1 CAPABILITY\r\n");
//! [3]


//! [5]
socket->connectToHostEncrypted("imap", 993);
if (socket->waitForEncrypted(1000))
    qDebug("Encrypted!");
//! [5]

//! [6]
QList<QSslCertificate> cert = QSslCertificate::fromPath("server-certificate.pem"_L1);
QSslError error(QSslError::SelfSignedCertificate, cert.at(0));
QList<QSslError> expectedSslErrors;
expectedSslErrors.append(error);

QSslSocket socket;
socket.ignoreSslErrors(expectedSslErrors);
socket.connectToHostEncrypted("server.tld", 443);
//! [6]
