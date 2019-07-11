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
#ifndef SERVER_H
#define SERVER_H

#include <QtCore>
#include <QtNetwork>

#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE

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

QT_END_NAMESPACE

#endif // SERVER_H
