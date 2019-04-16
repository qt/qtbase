/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Copyright (C) 2014 Governikus GmbH & Co. KG.
** Copyright (C) 2016 Richard J. Moore <rich@kde.org>
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

#include <vector>

QT_BEGIN_NAMESPACE

// defined in qsslsocket_openssl.cpp:
extern int q_X509Callback(int ok, X509_STORE_CTX *ctx);
extern QString getErrorsFromOpenSsl();

#if QT_CONFIG(dtls)
// defined in qdtls_openssl.cpp:
namespace dtlscallbacks
{
extern "C" int q_X509DtlsCallback(int ok, X509_STORE_CTX *ctx);
extern "C" int q_generate_cookie_callback(SSL *ssl, unsigned char *dst,
                                          unsigned *cookieLength);
extern "C" int q_verify_cookie_callback(SSL *ssl, const unsigned char *cookie,
                                        unsigned cookieLength);
}
#endif // dtls

static inline QString msgErrorSettingEllipticCurves(const QString &why)
{
    return QSslSocket::tr("Error when setting the elliptic curves (%1)").arg(why);
}

// Defined in qsslsocket.cpp
QList<QSslCipher> q_getDefaultDtlsCiphers();

// static
void QSslContext::initSslContext(QSslContext *sslContext, QSslSocket::SslMode mode, const QSslConfiguration &configuration, bool allowRootCertOnDemandLoading)
{
    sslContext->sslConfiguration = configuration;
    sslContext->errorCode = QSslError::NoError;

    bool client = (mode == QSslSocket::SslClientMode);

    bool reinitialized = false;
    bool unsupportedProtocol = false;
    bool isDtls = false;
init_context:
    if (sslContext->sslConfiguration.protocol() == QSsl::SslV2) {
        // SSL 2 is no longer supported, but chosen deliberately -> error
        sslContext->ctx = nullptr;
        unsupportedProtocol = true;
    } else if (sslContext->sslConfiguration.protocol() == QSsl::SslV3) {
        // SSL 3 is no longer supported, but chosen deliberately -> error
        sslContext->ctx = nullptr;
        unsupportedProtocol = true;
    } else {
        switch (sslContext->sslConfiguration.protocol()) {
        case QSsl::DtlsV1_0:
        case QSsl::DtlsV1_0OrLater:
        case QSsl::DtlsV1_2:
        case QSsl::DtlsV1_2OrLater:
#if QT_CONFIG(dtls)
            isDtls = true;
            sslContext->ctx = q_SSL_CTX_new(client ? q_DTLS_client_method() : q_DTLS_server_method());
#else // dtls
            sslContext->ctx = nullptr;
            unsupportedProtocol = true;
            qCWarning(lcSsl, "DTLS protocol requested, but feature 'dtls' is disabled");

#endif // dtls
            break;
        case QSsl::TlsV1_3:
        case QSsl::TlsV1_3OrLater:
#if !defined(TLS1_3_VERSION)
            qCWarning(lcSsl, "TLS 1.3 is not supported");
            sslContext->ctx = nullptr;
            unsupportedProtocol = true;
            break;
#endif // TLS1_3_VERSION
        default:
            // The ssl options will actually control the supported methods
            sslContext->ctx = q_SSL_CTX_new(client ? q_TLS_client_method() : q_TLS_server_method());
        }
    }

    if (!sslContext->ctx) {
        // After stopping Flash 10 the SSL library loses its ciphers. Try re-adding them
        // by re-initializing the library.
        if (!reinitialized) {
            reinitialized = true;
            if (q_OPENSSL_init_ssl(0, nullptr) == 1)
                goto init_context;
        }

        sslContext->errorStr = QSslSocket::tr("Error creating SSL context (%1)").arg(
            unsupportedProtocol ? QSslSocket::tr("unsupported protocol") : QSslSocketBackendPrivate::getErrorsFromOpenSsl()
        );
        sslContext->errorCode = QSslError::UnspecifiedError;
        return;
    }

    const long anyVersion =
#if QT_CONFIG(dtls)
                            isDtls ? DTLS_ANY_VERSION : TLS_ANY_VERSION;
#else
                            TLS_ANY_VERSION;
#endif // dtls
    long minVersion = anyVersion;
    long maxVersion = anyVersion;

    switch (sslContext->sslConfiguration.protocol()) {
    case QSsl::TlsV1_0:
        minVersion = TLS1_VERSION;
        maxVersion = TLS1_VERSION;
        break;
    case QSsl::TlsV1_1:
        minVersion = TLS1_1_VERSION;
        maxVersion = TLS1_1_VERSION;
        break;
    case QSsl::TlsV1_2:
        minVersion = TLS1_2_VERSION;
        maxVersion = TLS1_2_VERSION;
        break;
    case QSsl::TlsV1_3:
#ifdef TLS1_3_VERSION
        minVersion = TLS1_3_VERSION;
        maxVersion = TLS1_3_VERSION;
#else
        // This protocol is not supported by OpenSSL 1.1 and we handle
        // it as an error (see the code above).
        Q_UNREACHABLE();
#endif // TLS1_3_VERSION
        break;
    // Ranges:
    case QSsl::TlsV1SslV3:
    case QSsl::AnyProtocol:
    case QSsl::SecureProtocols:
    case QSsl::TlsV1_0OrLater:
        minVersion = TLS1_VERSION;
        maxVersion = 0;
        break;
    case QSsl::TlsV1_1OrLater:
        minVersion = TLS1_1_VERSION;
        maxVersion = 0;
        break;
    case QSsl::TlsV1_2OrLater:
        minVersion = TLS1_2_VERSION;
        maxVersion = 0;
        break;
    case QSsl::DtlsV1_0:
        minVersion = DTLS1_VERSION;
        maxVersion = DTLS1_VERSION;
        break;
    case QSsl::DtlsV1_0OrLater:
        minVersion = DTLS1_VERSION;
        maxVersion = DTLS_MAX_VERSION;
        break;
    case QSsl::DtlsV1_2:
        minVersion = DTLS1_2_VERSION;
        maxVersion = DTLS1_2_VERSION;
        break;
    case QSsl::DtlsV1_2OrLater:
        minVersion = DTLS1_2_VERSION;
        maxVersion = DTLS_MAX_VERSION;
        break;
    case QSsl::TlsV1_3OrLater:
#ifdef TLS1_3_VERSION
        minVersion = TLS1_3_VERSION;
        maxVersion = 0;
        break;
#else
        // This protocol is not supported by OpenSSL 1.1 and we handle
        // it as an error (see the code above).
        Q_UNREACHABLE();
        break;
#endif // TLS1_3_VERSION
    case QSsl::SslV2:
    case QSsl::SslV3:
        // These protocols are not supported, and we handle
        // them as an error (see the code above).
        Q_UNREACHABLE();
        break;
    case QSsl::UnknownProtocol:
        break;
    }

    if (minVersion != anyVersion
        && !q_SSL_CTX_set_min_proto_version(sslContext->ctx, minVersion)) {
        sslContext->errorStr = QSslSocket::tr("Error while setting the minimal protocol version");
        sslContext->errorCode = QSslError::UnspecifiedError;
        return;
    }

    if (maxVersion != anyVersion
        && !q_SSL_CTX_set_max_proto_version(sslContext->ctx, maxVersion)) {
        sslContext->errorStr = QSslSocket::tr("Error while setting the maximum protocol version");
        sslContext->errorCode = QSslError::UnspecifiedError;
        return;
    }

    // Enable bug workarounds.
    long options = QSslSocketBackendPrivate::setupOpenSslOptions(configuration.protocol(), configuration.d->sslOptions);
    q_SSL_CTX_set_options(sslContext->ctx, options);

    // Tell OpenSSL to release memory early
    // http://www.openssl.org/docs/ssl/SSL_CTX_set_mode.html
    q_SSL_CTX_set_mode(sslContext->ctx, SSL_MODE_RELEASE_BUFFERS);

    auto filterCiphers = [](const QList<QSslCipher> &ciphers, bool selectTls13)
    {
        QByteArray cipherString;
        bool first = true;

        for (const QSslCipher &cipher : qAsConst(ciphers)) {
            const bool isTls13Cipher = cipher.protocol() == QSsl::TlsV1_3 || cipher.protocol() == QSsl::TlsV1_3OrLater;
            if (selectTls13 != isTls13Cipher)
                continue;

            if (first)
                first = false;
            else
                cipherString.append(':');
            cipherString.append(cipher.name().toLatin1());
        }
        return cipherString;
    };

    // Initialize ciphers
    QList<QSslCipher> ciphers = sslContext->sslConfiguration.ciphers();
    if (ciphers.isEmpty())
        ciphers = isDtls ? q_getDefaultDtlsCiphers() : QSslSocketPrivate::defaultCiphers();

    const QByteArray preTls13Ciphers = filterCiphers(ciphers, false);

    if (preTls13Ciphers.size()) {
        if (!q_SSL_CTX_set_cipher_list(sslContext->ctx, preTls13Ciphers.data())) {
            sslContext->errorStr = QSslSocket::tr("Invalid or empty cipher list (%1)").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }
    }

    const QByteArray tls13Ciphers = filterCiphers(ciphers, true);
#ifdef TLS1_3_VERSION
    if (tls13Ciphers.size()) {
        if (!q_SSL_CTX_set_ciphersuites(sslContext->ctx, tls13Ciphers.data())) {
            sslContext->errorStr = QSslSocket::tr("Invalid or empty cipher list (%1)").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }
    }
#endif // TLS1_3_VERSION
    if (!preTls13Ciphers.size() && !tls13Ciphers.size()) {
        sslContext->errorStr = QSslSocket::tr("Invalid or empty cipher list (%1)").arg(QStringLiteral(""));
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
            q_SSL_CTX_load_verify_locations(sslContext->ctx, nullptr, unixDir.constData());
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
            sslContext->pkey = nullptr; // Don't free the private key, it belongs to QSslKey

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
        q_SSL_CTX_set_verify(sslContext->ctx, SSL_VERIFY_NONE, nullptr);
    } else {
        q_SSL_CTX_set_verify(sslContext->ctx, SSL_VERIFY_PEER,
#if QT_CONFIG(dtls)
                             isDtls ? dtlscallbacks::q_X509DtlsCallback :
#endif // dtls
                             q_X509Callback);
    }

#if QT_CONFIG(dtls)
    if (mode == QSslSocket::SslServerMode && isDtls && configuration.dtlsCookieVerificationEnabled()) {
        q_SSL_CTX_set_cookie_generate_cb(sslContext->ctx, dtlscallbacks::q_generate_cookie_callback);
        q_SSL_CTX_set_cookie_verify_cb(sslContext->ctx, dtlscallbacks::q_verify_cookie_callback);
    }
#endif // dtls

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
        DH *dh = q_d2i_DHparams(nullptr, reinterpret_cast<const unsigned char **>(&ptr),
                                params.length());
        if (dh == nullptr)
            qFatal("q_d2i_DHparams failed to convert QSslDiffieHellmanParameters to DER form");
        q_SSL_CTX_set_tmp_dh(sslContext->ctx, dh);
        q_DH_free(dh);
    }

#ifndef OPENSSL_NO_PSK
    if (!client)
        q_SSL_CTX_use_psk_identity_hint(sslContext->ctx, sslContext->sslConfiguration.preSharedKeyIdentityHint().constData());
#endif // !OPENSSL_NO_PSK

    const QVector<QSslEllipticCurve> qcurves = sslContext->sslConfiguration.ellipticCurves();
    if (!qcurves.isEmpty()) {
#ifdef OPENSSL_NO_EC
        sslContext->errorStr = msgErrorSettingEllipticCurves(QSslSocket::tr("OpenSSL version with disabled elliptic curves"));
        sslContext->errorCode = QSslError::UnspecifiedError;
        return;
#else
        // Set the curves to be used.
        std::vector<int> curves;
        curves.reserve(qcurves.size());
        for (const auto &sslCurve : qcurves)
            curves.push_back(sslCurve.id);
        if (!q_SSL_CTX_ctrl(sslContext->ctx, SSL_CTRL_SET_CURVES, long(curves.size()), &curves[0])) {
            sslContext->errorStr = msgErrorSettingEllipticCurves(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }
#endif
    }

    applyBackendConfig(sslContext);
}

QT_END_NAMESPACE
