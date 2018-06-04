/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
