/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#include "association.h"

QT_BEGIN_NAMESPACE

DtlsAssociation::DtlsAssociation(const QHostAddress &address, quint16 port,
                                 const QString &connectionName)
    : name(connectionName),
      crypto(QSslSocket::SslClientMode)
{
    auto configuration = QSslConfiguration::defaultDtlsConfiguration();
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    crypto.setRemote(address, port);
    crypto.setDtlsConfiguration(configuration);

    connect(&crypto, &QDtls::handshakeTimeout, this, &DtlsAssociation::handshakeTimeout);
    connect(&crypto, &QDtls::pskRequired, this, &DtlsAssociation::pskRequired);

    connect(&socket, &QUdpSocket::readyRead, this, &DtlsAssociation::readyRead);

    pingTimer.setInterval(5000);
    connect(&pingTimer, &QTimer::timeout, this, &DtlsAssociation::pingTimeout);
}

DtlsAssociation::~DtlsAssociation()
{
    if (crypto.connectionEncrypted())
        crypto.sendShutdownAlert(&socket);
}

void DtlsAssociation::startHandshake()
{
    if (!crypto.doHandshake(&socket, {}))
        emit errorMessage(name + tr(": failed to start a handshake - ") + crypto.dtlsErrorString());
    else
        emit infoMessage(name + tr(": starting a handshake"));
}

void DtlsAssociation::readyRead()
{
    QByteArray dgram(socket.pendingDatagramSize(), '\0');
    const qint64 bytesRead = socket.readDatagram(dgram.data(), dgram.size());
    if (bytesRead <= 0) {
        emit warningMessage(name + tr(": spurious read notification?"));
        return;
    }

    dgram.resize(bytesRead);
    if (crypto.connectionEncrypted()) {
        const QByteArray plainText = crypto.decryptDatagram(&socket, dgram);
        if (plainText.size()) {
            emit serverResponse(name, dgram, plainText);
            pingTimer.start();
            return;
        }

        if (crypto.dtlsError() == QDtlsError::RemoteClosedConnectionError) {
            emit errorMessage(name + tr(": shutdown alert received"));
            socket.close();
            pingTimer.stop();
            return;
        }

        emit warningMessage(name + tr(": zero-length datagram received?"));
    } else {
        if (!crypto.doHandshake(&socket, dgram)) {
            emit errorMessage(name + tr(": handshake error - ") + crypto.dtlsErrorString());
            return;
        }
        if (crypto.connectionEncrypted()) {
            emit infoMessage(name + tr(": encrypted connection established!"));
            pingTimer.start();
            pingTimeout();
        } else {
            emit infoMessage(name + tr(": continuing with handshake ..."));
        }
    }
}

void DtlsAssociation::handshakeTimeout()
{
    emit warningMessage(name + tr(": handshake timeout, trying to re-transmit"));
    if (!crypto.handleTimeout(&socket))
        emit errorMessage(name + tr(": failed to re-transmit - ") + crypto.dtlsErrorString());
}

void DtlsAssociation::pskRequired(QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);

    emit infoMessage(name + tr(": providing pre-shared key ..."));
    auth->setIdentity(name.toLatin1());
    auth->setPreSharedKey(QByteArrayLiteral("\x1a\x2b\x3c\x4d\x5e\x6f"));
}

void DtlsAssociation::pingTimeout()
{
    static const QString message = QStringLiteral("I am %1, please, accept our ping %2");
    const qint64 written = crypto.writeDatagramEncrypted(&socket, message.arg(name).arg(ping).toLatin1());
    if (written <= 0) {
        emit errorMessage(name + tr(": failed to send a ping - ") + crypto.dtlsErrorString());
        pingTimer.stop();
        return;
    }

    ++ping;
}

QT_END_NAMESPACE
