/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include <QtCore/qt_windows.h>

#include "qtlsbackend_schannel_p.h"
#include "qsslsocket_p.h"

#include "qwincrypt_p.h"

#define SECURITY_WIN32
#define SCHANNEL_USE_BLACKLISTS 1
#include <Winternl.h> // needed for UNICODE defines
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

    SecPkgContext_ConnectionInfo connectionInfo = {};
    SecPkgContext_StreamSizes streamSizes = {};

    CredHandle credentialHandle; // Initialized in ctor
    CtxtHandle contextHandle; // Initialized in ctor

    QByteArray intermediateBuffer; // data which is left-over or incomplete

    QHCertStorePointer localCertificateStore = nullptr;
    QHCertStorePointer peerCertificateStore = nullptr;
    QHCertStorePointer caCertificateStore = nullptr;

    const CERT_CONTEXT *localCertContext = nullptr;

    ULONG contextAttributes = 0;
    qint64 missingData = 0;

    bool renegotiating = false;
    bool shutdown = false;
    QList<QSslError> sslErrors;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLS_SCHANNEL_P_H
