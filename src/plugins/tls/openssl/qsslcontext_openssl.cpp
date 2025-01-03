// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// Copyright (C) 2014 Governikus GmbH & Co. KG.
// Copyright (C) 2016 Richard J. Moore <rich@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qssldiffiehellmanparameters.h>

#include "qsslsocket_openssl_symbols_p.h"
#include "qsslcontext_openssl_p.h"
#include "qtlsbackend_openssl_p.h"
#include "qtlskey_openssl_p.h"
#include "qopenssl_p.h"

#include <QtNetwork/private/qssl_p.h>
#include <QtNetwork/private/qsslsocket_p.h>
#include <QtNetwork/private/qtlsbackend_p.h>

#include <QtNetwork/private/qssldiffiehellmanparameters_p.h>

#include <vector>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(bool, forceSecurityLevel)

namespace QTlsPrivate
{
// These callback functions are defined in qtls_openssl.cpp.
extern "C" int q_X509Callback(int ok, X509_STORE_CTX *ctx);
extern "C" int q_X509CallbackDirect(int ok, X509_STORE_CTX *ctx);

#if QT_CONFIG(ocsp)
extern "C" int qt_OCSP_status_server_callback(SSL *ssl, void *);
#endif // ocsp

} // namespace QTlsPrivate

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

#ifdef TLS1_3_VERSION
extern "C" int q_ssl_sess_set_new_cb(SSL *context, SSL_SESSION *session);
#endif // TLS1_3_VERSION

static inline QString msgErrorSettingBackendConfig(const QString &why)
{
    return QSslSocket::tr("Error when setting the OpenSSL configuration (%1)").arg(why);
}

static inline QString msgErrorSettingEllipticCurves(const QString &why)
{
    return QSslSocket::tr("Error when setting the elliptic curves (%1)").arg(why);
}

qssloptions QSslContext::setupOpenSslOptions(QSsl::SslProtocol protocol, QSsl::SslOptions sslOptions)
{
    qssloptions options;
    switch (protocol) {
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::TlsV1_0OrLater:
        options = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
        break;
    case QSsl::TlsV1_1OrLater:
        options = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1;
        break;
QT_WARNING_POP
    case QSsl::SecureProtocols:
    case QSsl::TlsV1_2OrLater:
        options = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
        break;
    case QSsl::TlsV1_3OrLater:
        options = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2;
        break;
    default:
        options = SSL_OP_ALL;
    }

    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableEmptyFragments)
        options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
    else
        options &= ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;

#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableLegacyRenegotiation)
        options &= ~SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
    else
        options |= SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
#endif

#ifdef SSL_OP_NO_TICKET
    if (sslOptions & QSsl::SslOptionDisableSessionTickets)
        options |= SSL_OP_NO_TICKET;
#endif
#ifdef SSL_OP_NO_COMPRESSION
    if (sslOptions & QSsl::SslOptionDisableCompression)
        options |= SSL_OP_NO_COMPRESSION;
#endif

    if (!(sslOptions & QSsl::SslOptionDisableServerCipherPreference))
        options |= SSL_OP_CIPHER_SERVER_PREFERENCE;

    return options;
}

QSslContext::QSslContext()
    : ctx(nullptr),
    pkey(nullptr),
    session(nullptr),
    m_sessionTicketLifeTimeHint(-1)
{
}

QSslContext::~QSslContext()
{
    if (ctx)
        // This will decrement the reference count by 1 and free the context eventually when possible
        q_SSL_CTX_free(ctx);

    if (pkey)
        q_EVP_PKEY_free(pkey);

    if (session)
        q_SSL_SESSION_free(session);
}

std::shared_ptr<QSslContext> QSslContext::sharedFromConfiguration(QSslSocket::SslMode mode, const QSslConfiguration &configuration, bool allowRootCertOnDemandLoading)
{
    struct AccessToPrivateCtor : QSslContext {};
    std::shared_ptr<QSslContext> sslContext = std::make_shared<AccessToPrivateCtor>();
    initSslContext(sslContext.get(), mode, configuration, allowRootCertOnDemandLoading);
    return sslContext;
}

std::shared_ptr<QSslContext> QSslContext::sharedFromPrivateConfiguration(QSslSocket::SslMode mode, QSslConfigurationPrivate *privConfiguration,
                                                                        bool allowRootCertOnDemandLoading)
{
    return sharedFromConfiguration(mode, privConfiguration, allowRootCertOnDemandLoading);
}

#ifndef OPENSSL_NO_NEXTPROTONEG

static int next_proto_cb(SSL *, unsigned char **out, unsigned char *outlen,
                         const unsigned char *in, unsigned int inlen, void *arg)
{
    QSslContext::NPNContext *ctx = reinterpret_cast<QSslContext::NPNContext *>(arg);

    // comment out to debug:
//    QList<QByteArray> supportedVersions;
//    for (unsigned int i = 0; i < inlen; ) {
//        QByteArray version(reinterpret_cast<const char *>(&in[i+1]), in[i]);
//        supportedVersions << version;
//        i += in[i] + 1;
//    }

    int proto = q_SSL_select_next_proto(out, outlen, in, inlen, ctx->data, ctx->len);
    switch (proto) {
    case OPENSSL_NPN_UNSUPPORTED:
        ctx->status = QSslConfiguration::NextProtocolNegotiationNone;
        break;
    case OPENSSL_NPN_NEGOTIATED:
        ctx->status = QSslConfiguration::NextProtocolNegotiationNegotiated;
        break;
    case OPENSSL_NPN_NO_OVERLAP:
        ctx->status = QSslConfiguration::NextProtocolNegotiationUnsupported;
        break;
    default:
        qCWarning(lcTlsBackend, "OpenSSL sent unknown NPN status");
    }

    return SSL_TLSEXT_ERR_OK;
}

QSslContext::NPNContext QSslContext::npnContext() const
{
    return m_npnContext;
}
#endif // !OPENSSL_NO_NEXTPROTONEG



// Needs to be deleted by caller
SSL* QSslContext::createSsl()
{
    SSL* ssl = q_SSL_new(ctx);
    q_SSL_clear(ssl);

    if (!session && !sessionASN1().isEmpty()
            && !sslConfiguration.testSslOption(QSsl::SslOptionDisableSessionPersistence)) {
        const unsigned char *data = reinterpret_cast<const unsigned char *>(m_sessionASN1.constData());
        session = q_d2i_SSL_SESSION(nullptr, &data, m_sessionASN1.size());
        // 'session' has refcount 1 already, set by the function above
    }

    if (session) {
        // Try to resume the last session we cached
        if (!q_SSL_set_session(ssl, session)) {
            qCWarning(lcTlsBackend, "could not set SSL session");
            q_SSL_SESSION_free(session);
            session = nullptr;
        }
    }

#ifndef OPENSSL_NO_NEXTPROTONEG
    QList<QByteArray> protocols = sslConfiguration.d.constData()->nextAllowedProtocols;
    if (!protocols.isEmpty()) {
        m_supportedNPNVersions.clear();
        for (int a = 0; a < protocols.size(); ++a) {
            if (protocols.at(a).size() > 255) {
                qCWarning(lcTlsBackend) << "TLS NPN extension" << protocols.at(a)
                                 << "is too long and will be ignored.";
                continue;
            } else if (protocols.at(a).isEmpty()) {
                continue;
            }
            m_supportedNPNVersions.append(protocols.at(a).size()).append(protocols.at(a));
        }
        if (m_supportedNPNVersions.size()) {
            m_npnContext.data = reinterpret_cast<unsigned char *>(m_supportedNPNVersions.data());
            m_npnContext.len = m_supportedNPNVersions.size();
            m_npnContext.status = QSslConfiguration::NextProtocolNegotiationNone;
            // Callback's type has a parameter 'const unsigned char ** out'
            // since it was introduced in 1.0.2. Internally, OpenSSL's own code
            // (tests/examples) cast it to unsigned char * (since it's 'out').
            // We just re-use our NPN callback and cast here:
            typedef int (*alpn_callback_t) (SSL *, const unsigned char **, unsigned char *,
                                            const unsigned char *, unsigned int, void *);
            // With ALPN callback is for a server side only, for a client m_npnContext.status
            // will stay in NextProtocolNegotiationNone.
            q_SSL_CTX_set_alpn_select_cb(ctx, alpn_callback_t(next_proto_cb), &m_npnContext);
            // Client:
            q_SSL_set_alpn_protos(ssl, m_npnContext.data, m_npnContext.len);
            // And in case our peer does not support ALPN, but supports NPN:
            q_SSL_CTX_set_next_proto_select_cb(ctx, next_proto_cb, &m_npnContext);
        }
    }
#endif // !OPENSSL_NO_NEXTPROTONEG

    return ssl;
}

// We cache exactly one session here
bool QSslContext::cacheSession(SSL* ssl)
{
    // don't cache the same session again
    if (session && session == q_SSL_get_session(ssl))
        return true;

    // decrease refcount of currently stored session
    // (this might happen if there are several concurrent handshakes in flight)
    if (session)
        q_SSL_SESSION_free(session);

    // cache the session the caller gave us and increase reference count
    session = q_SSL_get1_session(ssl);

    if (session && !sslConfiguration.testSslOption(QSsl::SslOptionDisableSessionPersistence)) {
        int sessionSize = q_i2d_SSL_SESSION(session, nullptr);
        if (sessionSize > 0) {
            m_sessionASN1.resize(sessionSize);
            unsigned char *data = reinterpret_cast<unsigned char *>(m_sessionASN1.data());
            if (!q_i2d_SSL_SESSION(session, &data))
                qCWarning(lcTlsBackend, "could not store persistent version of SSL session");
            m_sessionTicketLifeTimeHint = q_SSL_SESSION_get_ticket_lifetime_hint(session);
        }
    }

    return (session != nullptr);
}

QByteArray QSslContext::sessionASN1() const
{
    return m_sessionASN1;
}

void QSslContext::setSessionASN1(const QByteArray &session)
{
    m_sessionASN1 = session;
}

int QSslContext::sessionTicketLifeTimeHint() const
{
    return m_sessionTicketLifeTimeHint;
}

void QSslContext::forceAutoTestSecurityLevel()
{
    *forceSecurityLevel() = true;
}

QSslError::SslError QSslContext::error() const
{
    return errorCode;
}

QString QSslContext::errorString() const
{
    return errorStr;
}

void QSslContext::initSslContext(QSslContext *sslContext, QSslSocket::SslMode mode,
                                 const QSslConfiguration &configuration,
                                 bool allowRootCertOnDemandLoading)
{
    sslContext->sslConfiguration = configuration;
    sslContext->errorCode = QSslError::NoError;

    bool client = (mode == QSslSocket::SslClientMode);

    bool reinitialized = false;
    bool unsupportedProtocol = false;
    bool isDtls = false;
init_context:
    switch (sslContext->sslConfiguration.protocol()) {
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::DtlsV1_0:
    case QSsl::DtlsV1_0OrLater:
QT_WARNING_POP
    case QSsl::DtlsV1_2:
    case QSsl::DtlsV1_2OrLater:
#if QT_CONFIG(dtls)
        isDtls = true;
        sslContext->ctx = q_SSL_CTX_new(client ? q_DTLS_client_method() : q_DTLS_server_method());
#else // dtls
        sslContext->ctx = nullptr;
        unsupportedProtocol = true;
        qCWarning(lcTlsBackend, "DTLS protocol requested, but feature 'dtls' is disabled");
#endif // dtls
        break;
    case QSsl::TlsV1_3:
    case QSsl::TlsV1_3OrLater:
#if !defined(TLS1_3_VERSION)
        qCWarning(lcTlsBackend, "TLS 1.3 is not supported");
        sslContext->ctx = nullptr;
        unsupportedProtocol = true;
        break;
#endif // TLS1_3_VERSION
    default:
        // The ssl options will actually control the supported methods
        sslContext->ctx = q_SSL_CTX_new(client ? q_TLS_client_method() : q_TLS_server_method());
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
            unsupportedProtocol ? QSslSocket::tr("unsupported protocol") : QTlsBackendOpenSSL::getErrorsFromOpenSsl()
        );
        sslContext->errorCode = QSslError::UnspecifiedError;
        return;
    }

    // A nasty hacked OpenSSL using a level that will make our auto-tests fail:
    if (q_SSL_CTX_get_security_level(sslContext->ctx) > 1 && *forceSecurityLevel())
        q_SSL_CTX_set_security_level(sslContext->ctx, 1);

    const long anyVersion =
#if QT_CONFIG(dtls)
                            isDtls ? DTLS_ANY_VERSION : TLS_ANY_VERSION;
#else
                            TLS_ANY_VERSION;
#endif // dtls
    long minVersion = anyVersion;
    long maxVersion = anyVersion;

    switch (sslContext->sslConfiguration.protocol()) {
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::TlsV1_0:
        minVersion = TLS1_VERSION;
        maxVersion = TLS1_VERSION;
        break;
    case QSsl::TlsV1_1:
        minVersion = TLS1_1_VERSION;
        maxVersion = TLS1_1_VERSION;
        break;
QT_WARNING_POP
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
    case QSsl::AnyProtocol:
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::TlsV1_0OrLater:
        minVersion = TLS1_VERSION;
        maxVersion = 0;
        break;
    case QSsl::TlsV1_1OrLater:
        minVersion = TLS1_1_VERSION;
        maxVersion = 0;
        break;
QT_WARNING_POP
    case QSsl::SecureProtocols:
    case QSsl::TlsV1_2OrLater:
        minVersion = TLS1_2_VERSION;
        maxVersion = 0;
        break;
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::DtlsV1_0:
        minVersion = DTLS1_VERSION;
        maxVersion = DTLS1_VERSION;
        break;
    case QSsl::DtlsV1_0OrLater:
        minVersion = DTLS1_VERSION;
        maxVersion = 0;
        break;
QT_WARNING_POP
    case QSsl::DtlsV1_2:
        minVersion = DTLS1_2_VERSION;
        maxVersion = DTLS1_2_VERSION;
        break;
    case QSsl::DtlsV1_2OrLater:
        minVersion = DTLS1_2_VERSION;
        maxVersion = 0;
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
    const qssloptions options = setupOpenSslOptions(configuration.protocol(), configuration.d->sslOptions);
    q_SSL_CTX_set_options(sslContext->ctx, options);

    // Tell OpenSSL to release memory early
    // http://www.openssl.org/docs/ssl/SSL_CTX_set_mode.html
    q_SSL_CTX_set_mode(sslContext->ctx, SSL_MODE_RELEASE_BUFFERS);

    auto filterCiphers = [](const QList<QSslCipher> &ciphers, bool selectTls13)
    {
        QByteArray cipherString;

        for (const QSslCipher &cipher : ciphers) {
            const bool isTls13Cipher = cipher.protocol() == QSsl::TlsV1_3 || cipher.protocol() == QSsl::TlsV1_3OrLater;
            if (selectTls13 != isTls13Cipher)
                continue;

            if (cipherString.size())
                cipherString.append(':');
            cipherString.append(cipher.name().toLatin1());
        }
        return cipherString;
    };

    // Initialize ciphers
    QList<QSslCipher> ciphers = sslContext->sslConfiguration.ciphers();
    if (ciphers.isEmpty())
        ciphers = isDtls ? QTlsBackend::defaultDtlsCiphers() : QTlsBackend::defaultCiphers();

    const QByteArray preTls13Ciphers = filterCiphers(ciphers, false);

    if (preTls13Ciphers.size()) {
        if (!q_SSL_CTX_set_cipher_list(sslContext->ctx, preTls13Ciphers.data())) {
            sslContext->errorStr = QSslSocket::tr("Invalid or empty cipher list (%1)").arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }
    }

    const QByteArray tls13Ciphers = filterCiphers(ciphers, true);
#ifdef TLS1_3_VERSION
    if (tls13Ciphers.size()) {
        if (!q_SSL_CTX_set_ciphersuites(sslContext->ctx, tls13Ciphers.data())) {
            sslContext->errorStr = QSslSocket::tr("Invalid or empty cipher list (%1)").arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl());
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

    if (QSslSocketPrivate::rootCertOnDemandLoadingSupported() && allowRootCertOnDemandLoading) {
        // tell OpenSSL the directories where to look up the root certs on demand
        const QList<QByteArray> unixDirs = QSslSocketPrivate::unixRootCertDirectories();
        int success = 1;
#if OPENSSL_VERSION_MAJOR < 3
        for (const QByteArray &unixDir : unixDirs) {
            if ((success = q_SSL_CTX_load_verify_locations(sslContext->ctx, nullptr, unixDir.constData())) != 1)
                break;
        }
#else
        for (const QByteArray &unixDir : unixDirs) {
            if ((success = q_SSL_CTX_load_verify_dir(sslContext->ctx, unixDir.constData())) != 1)
                break;
        }
#endif // OPENSSL_VERSION_MAJOR
        if (success != 1) {
            const auto qtErrors = QTlsBackendOpenSSL::getErrorsFromOpenSsl();
            qCWarning(lcTlsBackend) << "An error encountered while to set root certificates location:"
                              << qtErrors;
        }
    }

    if (!sslContext->sslConfiguration.localCertificate().isNull()) {
        // Require a private key as well.
        if (sslContext->sslConfiguration.privateKey().isNull()) {
            sslContext->errorStr = QSslSocket::tr("Cannot provide a certificate with no key");
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }

        // Load certificate
        if (!q_SSL_CTX_use_certificate(sslContext->ctx, (X509 *)sslContext->sslConfiguration.localCertificate().handle())) {
            sslContext->errorStr = QSslSocket::tr("Error loading local certificate, %1").arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }

        if (configuration.d->privateKey.algorithm() == QSsl::Opaque) {
            sslContext->pkey = reinterpret_cast<EVP_PKEY *>(configuration.d->privateKey.handle());
        } else {
#ifdef OPENSSL_NO_DEPRECATED_3_0
            auto qtKey = QTlsBackend::backend<QTlsPrivate::TlsKeyOpenSSL>(configuration.d->privateKey);
            Q_ASSERT(qtKey);
            sslContext->pkey = qtKey->genericKey;
            Q_ASSERT(sslContext->pkey);
            q_EVP_PKEY_up_ref(sslContext->pkey);
#else
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
#endif // OPENSSL_NO_EC
#endif // OPENSSL_NO_DEPRECATED_3_0
        }
        auto pkey = sslContext->pkey;
        if (configuration.d->privateKey.algorithm() == QSsl::Opaque)
            sslContext->pkey = nullptr; // Don't free the private key, it belongs to QSslKey

        if (!q_SSL_CTX_use_PrivateKey(sslContext->ctx, pkey)) {
            sslContext->errorStr = QSslSocket::tr("Error loading private key, %1").arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }

        // Check if the certificate matches the private key.
        if (!q_SSL_CTX_check_private_key(sslContext->ctx)) {
            sslContext->errorStr = QSslSocket::tr("Private key does not certify public key, %1").arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }

        // If we have any intermediate certificates then we need to add them to our chain
        bool first = true;
        for (const QSslCertificate &cert : std::as_const(configuration.d->localCertificateChain)) {
            if (first) {
                first = false;
                continue;
            }
            q_SSL_CTX_ctrl(sslContext->ctx, SSL_CTRL_EXTRA_CHAIN_CERT, 0,
                           q_X509_dup(reinterpret_cast<X509 *>(cert.handle())));
        }
    }

    // Initialize peer verification, different callbacks, TLS/DTLS verification first
    // (note, all these set_some_callback do not have return value):
    if (sslContext->sslConfiguration.peerVerifyMode() == QSslSocket::VerifyNone) {
        q_SSL_CTX_set_verify(sslContext->ctx, SSL_VERIFY_NONE, nullptr);
    } else {
        auto verificationCallback =
        #if QT_CONFIG(dtls)
                                            isDtls ? dtlscallbacks::q_X509DtlsCallback :
        #endif // dtls
                                            QTlsPrivate::q_X509Callback;

        if (!isDtls && configuration.handshakeMustInterruptOnError())
            verificationCallback = QTlsPrivate::q_X509CallbackDirect;

        auto verificationMode = SSL_VERIFY_PEER;
        if (!isDtls && sslContext->sslConfiguration.missingCertificateIsFatal())
            verificationMode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;

        q_SSL_CTX_set_verify(sslContext->ctx, verificationMode, verificationCallback);
    }

#ifdef TLS1_3_VERSION
    // NewSessionTicket callback:
    if (mode == QSslSocket::SslClientMode && !isDtls) {
        q_SSL_CTX_sess_set_new_cb(sslContext->ctx, q_ssl_sess_set_new_cb);
        q_SSL_CTX_set_session_cache_mode(sslContext->ctx, SSL_SESS_CACHE_CLIENT);
    }

#endif // TLS1_3_VERSION

#if QT_CONFIG(dtls)
    // DTLS cookies:
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
#ifndef OPENSSL_NO_DEPRECATED_3_0
        const QByteArray &params = dhparams.d->derData;
        const char *ptr = params.constData();
        DH *dh = q_d2i_DHparams(nullptr, reinterpret_cast<const unsigned char **>(&ptr),
                                params.size());
        if (dh == nullptr)
            qFatal("q_d2i_DHparams failed to convert QSslDiffieHellmanParameters to DER form");
        q_SSL_CTX_set_tmp_dh(sslContext->ctx, dh);
        q_DH_free(dh);
#else
        qCWarning(lcTlsBackend, "Diffie-Hellman parameters are not supported, because OpenSSL v3 was built with deprecated API removed");
#endif
    }

#ifndef OPENSSL_NO_PSK
    if (!client)
        q_SSL_CTX_use_psk_identity_hint(sslContext->ctx, sslContext->sslConfiguration.preSharedKeyIdentityHint().constData());
#endif // !OPENSSL_NO_PSK

    const auto qcurves = sslContext->sslConfiguration.ellipticCurves();
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
            sslContext->errorStr = msgErrorSettingEllipticCurves(QTlsBackendOpenSSL::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return;
        }
#endif
    }

    applyBackendConfig(sslContext);
}

void QSslContext::applyBackendConfig(QSslContext *sslContext)
{
    const QMap<QByteArray, QVariant> &conf = sslContext->sslConfiguration.backendConfiguration();
    if (conf.isEmpty())
        return;

#if QT_CONFIG(ocsp)
    auto ocspResponsePos = conf.find("Qt-OCSP-response");
    if (ocspResponsePos != conf.end()) {
        // This is our private, undocumented configuration option, existing only for
        // the purpose of testing OCSP status responses. We don't even check this
        // callback was set. If no - the test must fail.
        q_SSL_CTX_set_tlsext_status_cb(sslContext->ctx, QTlsPrivate::qt_OCSP_status_server_callback);
        if (conf.size() == 1)
            return;
    }
#endif // ocsp

    QSharedPointer<SSL_CONF_CTX> cctx(q_SSL_CONF_CTX_new(), &q_SSL_CONF_CTX_free);
    if (cctx) {
        q_SSL_CONF_CTX_set_ssl_ctx(cctx.data(), sslContext->ctx);
        q_SSL_CONF_CTX_set_flags(cctx.data(), SSL_CONF_FLAG_FILE);

        for (auto i = conf.constBegin(); i != conf.constEnd(); ++i) {
            if (i.key() == "Qt-OCSP-response") // This never goes to SSL_CONF_cmd().
                continue;

            if (!i.value().canConvert(QMetaType(QMetaType::QByteArray))) {
                sslContext->errorCode = QSslError::UnspecifiedError;
                sslContext->errorStr = msgErrorSettingBackendConfig(
                QSslSocket::tr("Expecting QByteArray for %1").arg(
                               QString::fromUtf8(i.key())));
                return;
            }

            const QByteArray &value = i.value().toByteArray();
            const int result = q_SSL_CONF_cmd(cctx.data(), i.key().constData(), value.constData());
            if (result == 2)
                continue;

            sslContext->errorCode = QSslError::UnspecifiedError;
            switch (result) {
            case 0:
                sslContext->errorStr = msgErrorSettingBackendConfig(
                    QSslSocket::tr("An error occurred attempting to set %1 to %2").arg(
                        QString::fromUtf8(i.key()), QString::fromUtf8(value)));
                return;
            case 1:
                sslContext->errorStr = msgErrorSettingBackendConfig(
                    QSslSocket::tr("Wrong value for %1 (%2)").arg(
                        QString::fromUtf8(i.key()), QString::fromUtf8(value)));
                return;
            default:
                sslContext->errorStr = msgErrorSettingBackendConfig(
                    QSslSocket::tr("Unrecognized command %1 = %2").arg(
                        QString::fromUtf8(i.key()), QString::fromUtf8(value)));
                return;
            }
        }

        if (q_SSL_CONF_CTX_finish(cctx.data()) == 0) {
            sslContext->errorStr = msgErrorSettingBackendConfig(QSslSocket::tr("SSL_CONF_finish() failed"));
            sslContext->errorCode = QSslError::UnspecifiedError;
        }
    } else {
        sslContext->errorStr = msgErrorSettingBackendConfig(QSslSocket::tr("SSL_CONF_CTX_new() failed"));
        sslContext->errorCode = QSslError::UnspecifiedError;
    }
}

QT_END_NAMESPACE
