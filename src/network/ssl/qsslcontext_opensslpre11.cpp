/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Copyright (C) 2014 Governikus GmbH & Co. KG.
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


#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qssldiffiehellmanparameters.h>

#include "private/qssl_p.h"
#include "private/qsslcontext_openssl_p.h"
#include "private/qsslsocket_p.h"
#include "private/qsslsocket_openssl_p.h"
#include "private/qsslsocket_openssl_symbols_p.h"
#include "private/qssldiffiehellmanparameters_p.h"

QT_BEGIN_NAMESPACE

// defined in qsslsocket_openssl.cpp:
extern int q_X509Callback(int ok, X509_STORE_CTX *ctx);
extern QString getErrorsFromOpenSsl();

static inline QString msgErrorSettingEllipticCurves(const QString &why)
{
    return QSslSocket::tr("Error when setting the elliptic curves (%1)").arg(why);
}

// static
void QSslContext::initSslContext(QSslContext *sslContext, QSslSocket::SslMode mode, const QSslConfiguration &configuration, bool allowRootCertOnDemandLoading)
{
    sslContext->sslConfiguration = configuration;
    sslContext->errorCode = QSslError::NoError;

    bool client = (mode == QSslSocket::SslClientMode);

    bool reinitialized = false;
    bool unsupportedProtocol = false;
init_context:
    switch (sslContext->sslConfiguration.protocol()) {
    case QSsl::SslV2:
#ifndef OPENSSL_NO_SSL2
        sslContext->ctx = q_SSL_CTX_new(client ? q_SSLv2_client_method() : q_SSLv2_server_method());
#else
        // SSL 2 not supported by the system, but chosen deliberately -> error
        sslContext->ctx = 0;
        unsupportedProtocol = true;
#endif
        break;
    case QSsl::SslV3:
#ifndef OPENSSL_NO_SSL3_METHOD
        sslContext->ctx = q_SSL_CTX_new(client ? q_SSLv3_client_method() : q_SSLv3_server_method());
#else
        // SSL 3 not supported by the system, but chosen deliberately -> error
        sslContext->ctx = 0;
        unsupportedProtocol = true;
#endif
        break;
    case QSsl::SecureProtocols:
        // SSLv2 and SSLv3 will be disabled by SSL options
        // But we need q_SSLv23_server_method() otherwise AnyProtocol will be unable to connect on Win32.
    case QSsl::TlsV1SslV3:
        // SSLv2 will will be disabled by SSL options
    case QSsl::AnyProtocol:
    default:
        sslContext->ctx = q_SSL_CTX_new(client ? q_SSLv23_client_method() : q_SSLv23_server_method());
        break;
    case QSsl::TlsV1_0:
        sslContext->ctx = q_SSL_CTX_new(client ? q_TLSv1_client_method() : q_TLSv1_server_method());
        break;
    case QSsl::TlsV1_1:
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
        sslContext->ctx = q_SSL_CTX_new(client ? q_TLSv1_1_client_method() : q_TLSv1_1_server_method());
#else
        // TLS 1.1 not supported by the system, but chosen deliberately -> error
        sslContext->ctx = 0;
        unsupportedProtocol = true;
#endif
        break;
    case QSsl::TlsV1_2:
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
        sslContext->ctx = q_SSL_CTX_new(client ? q_TLSv1_2_client_method() : q_TLSv1_2_server_method());
#else
        // TLS 1.2 not supported by the system, but chosen deliberately -> error
        sslContext->ctx = 0;
        unsupportedProtocol = true;
#endif
        break;
    case QSsl::TlsV1_0OrLater:
        // Specific protocols will be specified via SSL options.
        sslContext->ctx = q_SSL_CTX_new(client ? q_SSLv23_client_method() : q_SSLv23_server_method());
        break;
    case QSsl::TlsV1_1OrLater:
    case QSsl::TlsV1_2OrLater:
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
        // Specific protocols will be specified via SSL options.
        sslContext->ctx = q_SSL_CTX_new(client ? q_SSLv23_client_method() : q_SSLv23_server_method());
#else
        // TLS 1.1/1.2 not supported by the system, but chosen deliberately -> error
        sslContext->ctx = 0;
        unsupportedProtocol = true;
#endif
        break;
    }

    if (!sslContext->ctx) {
        // After stopping Flash 10 the SSL library loses its ciphers. Try re-adding them
        // by re-initializing the library.
        if (!reinitialized) {
            reinitialized = true;
            if (q_SSL_library_init() == 1)
                goto init_context;
        }

        sslContext->errorStr = QSslSocket::tr("Error creating SSL context (%1)").arg(
            unsupportedProtocol ? QSslSocket::tr("unsupported protocol") : QSslSocketBackendPrivate::getErrorsFromOpenSsl()
        );
        sslContext->errorCode = QSslError::UnspecifiedError;
        return;
    }

    // Enable bug workarounds.
    long options = QSslSocketBackendPrivate::setupOpenSslOptions(configuration.protocol(), configuration.d->sslOptions);
    q_SSL_CTX_set_options(sslContext->ctx, options);

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
    // Tell OpenSSL to release memory early
    // http://www.openssl.org/docs/ssl/SSL_CTX_set_mode.html
    if (q_SSLeay() >= 0x10000000L)
        q_SSL_CTX_set_mode(sslContext->ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

    // Initialize ciphers
    QByteArray cipherString;
    bool first = true;
    QList<QSslCipher> ciphers = sslContext->sslConfiguration.ciphers();
    if (ciphers.isEmpty())
        ciphers = QSslSocketPrivate::defaultCiphers();
    for (const QSslCipher &cipher : qAsConst(ciphers)) {
        if (first)
            first = false;
        else
            cipherString.append(':');
        cipherString.append(cipher.name().toLatin1());
    }

    if (!q_SSL_CTX_set_cipher_list(sslContext->ctx, cipherString.data())) {
        sslContext->errorStr = QSslSocket::tr("Invalid or empty cipher list (%1)").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
        sslContext->errorCode = QSslError::UnspecifiedError;
        return;
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();

    // Add all our CAs to this store.
    const auto caCertificates = sslContext->sslConfiguration.caCertificates();
    for (const QSslCertificate &caCertificate : caCertificates) {
        // From https://www.openssl.org/docs/ssl/SSL_CTX_load_verify_locations.html:
        //
        // If several CA certificates matching the name, key identifier, and
        // serial number condition are available, only the first one will be
        // examined. This may lead to unexpected results if the same CA
        // certificate is available with different expiration dates. If a
        // ``certificate expired'' verification error occurs, no other
        // certificate will be searched. Make sure to not have expired
        // certificates mixed with valid ones.
        //
        // See also: QSslSocketBackendPrivate::verify()
        if (caCertificate.expiryDate() >= now) {
            q_X509_STORE_add_cert(q_SSL_CTX_get_cert_store(sslContext->ctx), (X509 *)caCertificate.handle());
        }
    }

    if (QSslSocketPrivate::s_loadRootCertsOnDemand && allowRootCertOnDemandLoading) {
        // tell OpenSSL the directories where to look up the root certs on demand
        const QList<QByteArray> unixDirs = QSslSocketPrivate::unixRootCertDirectories();
        for (const QByteArray &unixDir : unixDirs)
            q_SSL_CTX_load_verify_locations(sslContext->ctx, 0, unixDir.constData());
    }

    if (!sslContext->sslConfiguration.localCertificate().isNull()) {
        // Require a private key as well.
        if (sslContext->sslConfiguration.privateKey().isNull()) {
            sslContext->errorStr = QSslSocket::tr("Cannot provide a certificate with no key, %1").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }

        // Load certificate
        if (!q_SSL_CTX_use_certificate(sslContext->ctx, (X509 *)sslContext->sslConfiguration.localCertificate().handle())) {
            sslContext->errorStr = QSslSocket::tr("Error loading local certificate, %1").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }

        if (configuration.d->privateKey.algorithm() == QSsl::Opaque) {
            sslContext->pkey = reinterpret_cast<EVP_PKEY *>(configuration.d->privateKey.handle());
        } else {
            // Load private key
            sslContext->pkey = q_EVP_PKEY_new();
            // before we were using EVP_PKEY_assign_R* functions and did not use EVP_PKEY_free.
            // this lead to a memory leak. Now we use the *_set1_* functions which do not
            // take ownership of the RSA/DSA key instance because the QSslKey already has ownership.
            if (configuration.d->privateKey.algorithm() == QSsl::Rsa)
                q_EVP_PKEY_set1_RSA(sslContext->pkey, reinterpret_cast<RSA *>(configuration.d->privateKey.handle()));
            else if (configuration.d->privateKey.algorithm() == QSsl::Dsa)
                q_EVP_PKEY_set1_DSA(sslContext->pkey, reinterpret_cast<DSA *>(configuration.d->privateKey.handle()));
#ifndef OPENSSL_NO_EC
            else if (configuration.d->privateKey.algorithm() == QSsl::Ec)
                q_EVP_PKEY_set1_EC_KEY(sslContext->pkey, reinterpret_cast<EC_KEY *>(configuration.d->privateKey.handle()));
#endif
        }

        if (!q_SSL_CTX_use_PrivateKey(sslContext->ctx, sslContext->pkey)) {
            sslContext->errorStr = QSslSocket::tr("Error loading private key, %1").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }
        if (configuration.d->privateKey.algorithm() == QSsl::Opaque)
            sslContext->pkey = 0; // Don't free the private key, it belongs to QSslKey

        // Check if the certificate matches the private key.
        if (!q_SSL_CTX_check_private_key(sslContext->ctx)) {
            sslContext->errorStr = QSslSocket::tr("Private key does not certify public key, %1").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }

        // If we have any intermediate certificates then we need to add them to our chain
        bool first = true;
        for (const QSslCertificate &cert : qAsConst(configuration.d->localCertificateChain)) {
            if (first) {
                first = false;
                continue;
            }
            q_SSL_CTX_ctrl(sslContext->ctx, SSL_CTRL_EXTRA_CHAIN_CERT, 0,
                           q_X509_dup(reinterpret_cast<X509 *>(cert.handle())));
        }
    }

    // Initialize peer verification.
    if (sslContext->sslConfiguration.peerVerifyMode() == QSslSocket::VerifyNone) {
        q_SSL_CTX_set_verify(sslContext->ctx, SSL_VERIFY_NONE, 0);
    } else {
        q_SSL_CTX_set_verify(sslContext->ctx, SSL_VERIFY_PEER, q_X509Callback);
    }

    // Set verification depth.
    if (sslContext->sslConfiguration.peerVerifyDepth() != 0)
        q_SSL_CTX_set_verify_depth(sslContext->ctx, sslContext->sslConfiguration.peerVerifyDepth());

    // set persisted session if the user set it
    if (!configuration.sessionTicket().isEmpty())
        sslContext->setSessionASN1(configuration.sessionTicket());

    // Set temp DH params
    QSslDiffieHellmanParameters dhparams = configuration.diffieHellmanParameters();

    if (!dhparams.isValid()) {
        sslContext->errorStr = QSslSocket::tr("Diffie-Hellman parameters are not valid");
        sslContext->errorCode = QSslError::UnspecifiedError;
        return;
    }

    if (!dhparams.isEmpty()) {
        const QByteArray &params = dhparams.d->derData;
        const char *ptr = params.constData();
        DH *dh = q_d2i_DHparams(NULL, reinterpret_cast<const unsigned char **>(&ptr), params.length());
        if (dh == NULL)
            qFatal("q_d2i_DHparams failed to convert QSslDiffieHellmanParameters to DER form");
        q_SSL_CTX_set_tmp_dh(sslContext->ctx, dh);
        q_DH_free(dh);
    }

#ifndef OPENSSL_NO_EC
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
    if (q_SSLeay() >= 0x10002000L) {
        q_SSL_CTX_ctrl(sslContext->ctx, SSL_CTRL_SET_ECDH_AUTO, 1, NULL);
    } else
#endif
    {
        // Set temp ECDH params
        EC_KEY *ecdh = 0;
        ecdh = q_EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        q_SSL_CTX_set_tmp_ecdh(sslContext->ctx, ecdh);
        q_EC_KEY_free(ecdh);
    }
#endif // OPENSSL_NO_EC

#if OPENSSL_VERSION_NUMBER >= 0x10001000L && !defined(OPENSSL_NO_PSK)
    if (!client)
        q_SSL_CTX_use_psk_identity_hint(sslContext->ctx, sslContext->sslConfiguration.preSharedKeyIdentityHint().constData());
#endif // OPENSSL_VERSION_NUMBER >= 0x10001000L && !defined(OPENSSL_NO_PSK)

    const QVector<QSslEllipticCurve> qcurves = sslContext->sslConfiguration.ellipticCurves();
    if (!qcurves.isEmpty()) {
#if OPENSSL_VERSION_NUMBER >= 0x10002000L && !defined(OPENSSL_NO_EC)
        // Set the curves to be used
        if (q_SSLeay() >= 0x10002000L) {
            // SSL_CTX_ctrl wants a non-const pointer as last argument,
            // but let's avoid a copy into a temporary array
            if (!q_SSL_CTX_ctrl(sslContext->ctx,
                                SSL_CTRL_SET_CURVES,
                                qcurves.size(),
                                const_cast<int *>(reinterpret_cast<const int *>(qcurves.data())))) {
                sslContext->errorStr = msgErrorSettingEllipticCurves(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
                sslContext->errorCode = QSslError::UnspecifiedError;
                return;
            }
        } else
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L && !defined(OPENSSL_NO_EC)
        {
            // specific curves requested, but not possible to set -> error
            sslContext->errorStr = msgErrorSettingEllipticCurves(QSslSocket::tr("OpenSSL version too old, need at least v1.0.2"));
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }
    }

    applyBackendConfig(sslContext);
}

QT_END_NAMESPACE
