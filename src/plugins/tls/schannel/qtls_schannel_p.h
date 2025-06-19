// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTLS_SCHANNEL_P_H
#define QTLS_SCHANNEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

QT_REQUIRE_CONFIG(schannel);

#include "../shared/qwincrypt_p.h"

#include "qtlsbackend_schannel_p.h"

#include <QtNetwork/private/qsslsocket_p.h>

#define SECURITY_WIN32
#define SCHANNEL_USE_BLACKLISTS 1
#include <winternl.h> // needed for UNICODE defines
#include <security.h>
#include <schnlsp.h>
#undef SCHANNEL_USE_BLACKLISTS
#undef SECURITY_WIN32

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class TlsCryptographSchannel final : public TlsCryptograph
{
    Q_DISABLE_COPY_MOVE(TlsCryptographSchannel)
public:
    TlsCryptographSchannel();
    ~TlsCryptographSchannel();

    void init(QSslSocket *q, QSslSocketPrivate *d) override;

    void startClientEncryption() override;
    void startServerEncryption() override;
    void transmit() override;
    void disconnectFromHost() override;
    void disconnected() override;
    QSslCipher sessionCipher() const override;
    QSsl::SslProtocol sessionProtocol() const override;
    void continueHandshake() override;
    QList<QSslError> tlsErrors() const override;

private:
    enum class SchannelState {
        InitializeHandshake, // create and transmit context (client)/accept context (server)
        PerformHandshake, // get token back, process it
        VerifyHandshake, // Verify that things are OK
        Done, // Connection encrypted!
        Renegotiate // Renegotiating!
    } schannelState = SchannelState::InitializeHandshake;

    // Close to std::optional<QBArray>, but need to also cover the case where
    // we encrypted some data, but encountered an error later.
    struct MessageBufferResult {
        bool ok = false;
        QByteArray messageBuffer;
    };
    MessageBufferResult getNextEncryptedMessage();

    void reset();
    bool acquireCredentialsHandle();
    ULONG getContextRequirements();
    bool createContext(); // for clients
    bool acceptContext(); // for server
    bool performHandshake();
    bool verifyHandshake();
    bool renegotiate();

    bool sendToken(void *token, unsigned long tokenLength, bool emitError = true);
    QString targetName() const;

    bool checkSslErrors();
    void deallocateContext();
    void freeCredentialsHandle();
    void closeCertificateStores();
    void sendShutdown();

    void initializeCertificateStores();
    bool verifyCertContext(CERT_CONTEXT *certContext);

    bool rootCertOnDemandLoadingAllowed();

    bool hasUndecryptedData() const override { return intermediateBuffer.size() > 0; }

    QSslSocket *q = nullptr;
    QSslSocketPrivate *d = nullptr;

    SecPkgContext_CipherInfo cipherInfo = {};
    SecPkgContext_ConnectionInfo connectionInfo = {};
    SecPkgContext_StreamSizes streamSizes = {};

    CredHandle credentialHandle; // Initialized in ctor
    CtxtHandle contextHandle; // Initialized in ctor

    QByteArray intermediateBuffer; // data which is left-over or incomplete

    QHCertStorePointer localCertificateStore = nullptr;
    QHCertStorePointer peerCertificateStore = nullptr;
    QHCertStorePointer caCertificateStore = nullptr;

    ULONG contextAttributes = 0;
    qint64 missingData = 0;

    bool renegotiating = false;
    bool shutdown = false;
    QList<QSslError> sslErrors;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLS_SCHANNEL_P_H
