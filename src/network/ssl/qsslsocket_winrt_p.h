/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSSLSOCKET_WINRT_P_H
#define QSSLSOCKET_WINRT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QtNetwork library.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qsslsocket_p.h"

#include <wrl.h>
#include <windows.networking.sockets.h>

QT_BEGIN_NAMESPACE

class QSslSocketConnectionHelper : public QObject
{
    Q_OBJECT
public:
    QSslSocketConnectionHelper(QSslSocketBackendPrivate *d)
        : d(d) { }

    Q_INVOKABLE void disconnectSocketFromHost();

private:
    QSslSocketBackendPrivate *d;
};

class QSslSocketBackendPrivate : public QSslSocketPrivate
{
    Q_DECLARE_PUBLIC(QSslSocket)
public:
    QSslSocketBackendPrivate();
    ~QSslSocketBackendPrivate();

    // Platform specific functions
    void startClientEncryption() override;
    void startServerEncryption() override;
    void transmit() override;
    void disconnectFromHost() override;
    void disconnected() override;
    QSslCipher sessionCipher() const override;
    QSsl::SslProtocol sessionProtocol() const override;
    void continueHandshake() override;

    static QList<QSslCipher> defaultCiphers();
    static QList<QSslError> verify(const QList<QSslCertificate> &certificateChain, const QString &hostName);
    static bool importPkcs12(QIODevice *device,
                             QSslKey *key, QSslCertificate *cert,
                             QList<QSslCertificate> *caCertificates,
                             const QByteArray &passPhrase);

private:
    HRESULT onSslUpgrade(ABI::Windows::Foundation::IAsyncAction *,
                         ABI::Windows::Foundation::AsyncStatus);

    QScopedPointer<QSslSocketConnectionHelper> connectionHelper;
    ABI::Windows::Networking::Sockets::SocketProtectionLevel protectionLevel;
    QSet<QSslCertificate> previousCaCertificates;
};

QT_END_NAMESPACE

#endif // QSSLSOCKET_WINRT_P_H
