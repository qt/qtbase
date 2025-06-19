// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qtlsbackend_st_p.h"
#include "qtlskey_st_p.h"
#include "qx509_st_p.h"
#include "qtls_st_p.h"

#include <QtCore/qsysinfo.h>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC(QRecursiveMutex, qt_securetransport_mutex)

Q_LOGGING_CATEGORY(lcSecureTransport, "qt.tlsbackend.securetransport");

namespace QTlsPrivate {

QList<QSslCertificate> systemCaCertificates(); // defined in qsslsocket_mac_shared.cpp

SSLContextRef qt_createSecureTransportContext(QSslSocket::SslMode mode);

QSslCipher QSslCipher_from_SSLCipherSuite(SSLCipherSuite cipher)
{
    QString name;
    switch (cipher) {
    // Sorted as in CipherSuite.h (and groupped by their RFC)
    // TLS addenda using AES, per RFC 3268
    case TLS_RSA_WITH_AES_128_CBC_SHA:
        name = "AES128-SHA"_L1;
        break;
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA:
        name = "DHE-RSA-AES128-SHA"_L1;
        break;
    case TLS_RSA_WITH_AES_256_CBC_SHA:
        name = "AES256-SHA"_L1;
        break;
    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA:
        name = "DHE-RSA-AES256-SHA"_L1;
        break;

    // ECDSA addenda, RFC 4492
    case TLS_ECDH_ECDSA_WITH_NULL_SHA:
        name = "ECDH-ECDSA-NULL-SHA"_L1;
        break;
    case TLS_ECDH_ECDSA_WITH_RC4_128_SHA:
        name = "ECDH-ECDSA-RC4-SHA"_L1;
        break;
    case TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA:
        name = "ECDH-ECDSA-DES-CBC3-SHA"_L1;
        break;
    case TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA:
        name = "ECDH-ECDSA-AES128-SHA"_L1;
        break;
    case TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA:
        name = "ECDH-ECDSA-AES256-SHA"_L1;
        break;
    case TLS_ECDHE_ECDSA_WITH_NULL_SHA:
        name = "ECDHE-ECDSA-NULL-SHA"_L1;
        break;
    case TLS_ECDHE_ECDSA_WITH_RC4_128_SHA:
        name = "ECDHE-ECDSA-RC4-SHA"_L1;
        break;
    case TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA:
        name = "ECDHE-ECDSA-DES-CBC3-SHA"_L1;
        break;
    case TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA:
        name = "ECDHE-ECDSA-AES128-SHA"_L1;
        break;
    case TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA:
        name = "ECDHE-ECDSA-AES256-SHA"_L1;
        break;
    case TLS_ECDH_RSA_WITH_NULL_SHA:
        name = "ECDH-RSA-NULL-SHA"_L1;
        break;
    case TLS_ECDH_RSA_WITH_RC4_128_SHA:
        name = "ECDH-RSA-RC4-SHA"_L1;
        break;
    case TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA:
        name = "ECDH-RSA-DES-CBC3-SHA"_L1;
        break;
    case TLS_ECDH_RSA_WITH_AES_128_CBC_SHA:
        name = "ECDH-RSA-AES128-SHA"_L1;
        break;
    case TLS_ECDH_RSA_WITH_AES_256_CBC_SHA:
        name = "ECDH-RSA-AES256-SHA"_L1;
        break;
    case TLS_ECDHE_RSA_WITH_NULL_SHA:
        name = "ECDHE-RSA-NULL-SHA"_L1;
        break;
    case TLS_ECDHE_RSA_WITH_RC4_128_SHA:
        name = "ECDHE-RSA-RC4-SHA"_L1;
        break;
    case TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA:
        name = "ECDHE-RSA-DES-CBC3-SHA"_L1;
        break;
    case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA:
        name = "ECDHE-RSA-AES128-SHA"_L1;
        break;
    case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA:
        name = "ECDHE-RSA-AES256-SHA"_L1;
        break;

    // TLS 1.2 addenda, RFC 5246
    case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
        name = "DES-CBC3-SHA"_L1;
        break;
    case TLS_RSA_WITH_AES_128_CBC_SHA256:
        name = "AES128-SHA256"_L1;
        break;
    case TLS_RSA_WITH_AES_256_CBC_SHA256:
        name = "AES256-SHA256"_L1;
        break;
    case TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA:
        name = "DHE-RSA-DES-CBC3-SHA"_L1;
        break;
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA256:
        name = "DHE-RSA-AES128-SHA256"_L1;
        break;
    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA256:
        name = "DHE-RSA-AES256-SHA256"_L1;
        break;

    // Addendum from RFC 4279, TLS PSK
    // all missing atm.

    // RFC 4785 - Pre-Shared Key (PSK) Ciphersuites with NULL Encryption
    // all missing atm.

    // Addenda from rfc 5288 AES Galois Counter Mode (CGM) Cipher Suites for TLS
    case TLS_RSA_WITH_AES_256_GCM_SHA384:
        name = "AES256-GCM-SHA384"_L1;
        break;

    // RFC 5487 - PSK with SHA-256/384 and AES GCM
    // all missing atm.

    // Addenda from rfc 5289 Elliptic Curve Cipher Suites with HMAC SHA-256/384
    case TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256:
        name = "ECDHE-ECDSA-AES128-SHA256"_L1;
        break;
    case TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384:
        name = "ECDHE-ECDSA-AES256-SHA384"_L1;
        break;
    case TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256:
        name = "ECDH-ECDSA-AES128-SHA256"_L1;
        break;
    case TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384:
        name = "ECDH-ECDSA-AES256-SHA384"_L1;
        break;
    case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256:
        name = "ECDHE-RSA-AES128-SHA256"_L1;
        break;
    case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384:
        name = "ECDHE-RSA-AES256-SHA384"_L1;
        break;
    case TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256:
        name = "ECDH-RSA-AES128-SHA256"_L1;
        break;
    case TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384:
        name = "ECDH-RSA-AES256-SHA384"_L1;
        break;

    // Addenda from rfc 5289 Elliptic Curve Cipher Suites
    // with SHA-256/384 and AES Galois Counter Mode (GCM)
    case TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384:
        name = "ECDHE-RSA-AES256-GCM-SHA384"_L1;
        break;

    // TLS 1.3 standard cipher suites for ChaCha20+Poly1305.
    // Note: TLS 1.3 ciphersuites do not specify the key exchange
    // algorithm -- they only specify the symmetric ciphers.
    case TLS_AES_128_GCM_SHA256:
        name = "AES128-GCM-SHA256"_L1;
        break;
    case TLS_AES_256_GCM_SHA384:
        name = "AES256-GCM-SHA384"_L1;
        break;
    case TLS_CHACHA20_POLY1305_SHA256:
        name = "CHACHA20-POLY1305-SHA256"_L1;
        break;
    case TLS_AES_128_CCM_SHA256:
        name = "AES128-CCM-SHA256"_L1;
        break;
    case TLS_AES_128_CCM_8_SHA256:
        name = "AES128-CCM8-SHA256"_L1;
        break;
    // Addenda from rfc 5289  Elliptic Curve Cipher Suites with
    // SHA-256/384 and AES Galois Counter Mode (GCM).
    case TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256:
        name = "ECDHE-ECDSA-AES128-GCM-SHA256"_L1;
        break;
    case TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384:
        name = "ECDHE-ECDSA-AES256-GCM-SHA384"_L1;
        break;
    case TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256:
        name = "ECDH-ECDSA-AES128-GCM-SHA256"_L1;
        break;
    case TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384:
        name = "ECDH-ECDSA-AES256-GCM-SHA384"_L1;
        break;
    case TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256:
        name = "ECDHE-RSA-AES128-GCM-SHA256"_L1;
        break;
    case TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256:
        name = "ECDH-RSA-AES128-GCM-SHA256"_L1;
        break;
    case TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384:
        name = "ECDH-RSA-AES256-GCM-SHA384"_L1;
        break;
    // Addenda from rfc 7905  ChaCha20-Poly1305 Cipher Suites for
    // Transport Layer Security (TLS).
    case TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256:
        name = "ECDHE-RSA-CHACHA20-POLY1305-SHA256"_L1;
        break;
    case TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256:
        name = "ECDHE-ECDSA-CHACHA20-POLY1305-SHA256"_L1;
        break;
    default:
        return {};
    }

    return QTlsBackend::createCiphersuite(name, QSsl::TlsV1_2, "TLSv1.2"_L1);
}

} // namespace QTlsPrivate

bool QSecureTransportBackend::s_loadedCiphersAndCerts = false;

QString QSecureTransportBackend::tlsLibraryVersionString() const
{
    return "Secure Transport, "_L1 + QSysInfo::prettyProductName();
}

QString QSecureTransportBackend::tlsLibraryBuildVersionString() const
{
    return tlsLibraryVersionString();
}

void QSecureTransportBackend::ensureInitialized() const
{
    const QMutexLocker locker(qt_securetransport_mutex());
    if (s_loadedCiphersAndCerts)
        return;

    // We have to set it before setDefaultSupportedCiphers,
    // since this function can trigger static (global)'s initialization
    // and as a result - recursive ensureInitialized call
    // from QSslCertificatePrivate's ctor.
    s_loadedCiphersAndCerts = true;

    const QTlsPrivate::QSecureTransportContext context(QTlsPrivate::qt_createSecureTransportContext(QSslSocket::SslClientMode));
    if (context) {
        QList<QSslCipher> ciphers;
        QList<QSslCipher> defaultCiphers;

        size_t numCiphers = 0;
        // Fails only if any of parameters is null.
        SSLGetNumberSupportedCiphers(context, &numCiphers);
        QList<SSLCipherSuite> cfCiphers(numCiphers);
        // Fails only if any of parameter is null or number of ciphers is wrong.
        SSLGetSupportedCiphers(context, cfCiphers.data(), &numCiphers);

        for (size_t i = 0; i < size_t(cfCiphers.size()); ++i) {
            const QSslCipher ciph(QTlsPrivate::QSslCipher_from_SSLCipherSuite(cfCiphers.at(i)));
            if (!ciph.isNull()) {
                ciphers << ciph;
                if (ciph.usedBits() >= 128)
                    defaultCiphers << ciph;
            }
        }

        setDefaultSupportedCiphers(ciphers);
        setDefaultCiphers(defaultCiphers);

        if (!QSslSocketPrivate::rootCertOnDemandLoadingSupported())
            setDefaultCaCertificates(systemCaCertificates());
    } else {
        s_loadedCiphersAndCerts = false;
    }
}

QString QSecureTransportBackend::backendName() const
{
    return builtinBackendNames[nameIndexSecureTransport];
}

QTlsPrivate::TlsKey *QSecureTransportBackend::createKey() const
{
    return new QTlsPrivate::TlsKeySecureTransport;
}

QTlsPrivate::X509Certificate *QSecureTransportBackend::createCertificate() const
{
    return new QTlsPrivate::X509CertificateSecureTransport;
}

QList<QSslCertificate> QSecureTransportBackend::systemCaCertificates() const
{
    return QTlsPrivate::systemCaCertificates();
}

QList<QSsl::SslProtocol> QSecureTransportBackend::supportedProtocols() const
{
    QList<QSsl::SslProtocol> protocols;

    protocols << QSsl::AnyProtocol;
    protocols << QSsl::SecureProtocols;
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    protocols << QSsl::TlsV1_0;
    protocols << QSsl::TlsV1_0OrLater;
    protocols << QSsl::TlsV1_1;
    protocols << QSsl::TlsV1_1OrLater;
QT_WARNING_POP
    protocols << QSsl::TlsV1_2;
    protocols << QSsl::TlsV1_2OrLater;

    return protocols;
}

QList<QSsl::SupportedFeature> QSecureTransportBackend::supportedFeatures() const
{
    QList<QSsl::SupportedFeature> features;
    features << QSsl::SupportedFeature::ClientSideAlpn;

    return features;
}

QList<QSsl::ImplementedClass> QSecureTransportBackend::implementedClasses() const
{
    QList<QSsl::ImplementedClass> classes;
    classes << QSsl::ImplementedClass::Socket;
    classes << QSsl::ImplementedClass::Certificate;
    classes << QSsl::ImplementedClass::Key;

    return classes;
}

QTlsPrivate::X509PemReaderPtr QSecureTransportBackend::X509PemReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromPem;
}

QTlsPrivate::X509DerReaderPtr QSecureTransportBackend::X509DerReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromDer;
}

QTlsPrivate::TlsCryptograph *QSecureTransportBackend::createTlsCryptograph() const
{
    return new QTlsPrivate::TlsCryptographSecureTransport;
}

QT_END_NAMESPACE


#include "moc_qtlsbackend_st_p.cpp"
