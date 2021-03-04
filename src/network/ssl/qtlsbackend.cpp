/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qtlsbackend_p.h"

#if QT_CONFIG(ssl)
#include "qsslpresharedkeyauthenticator_p.h"
#include "qsslpresharedkeyauthenticator.h"
#include "qsslsocket_p.h"
#include "qsslcipher_p.h"
#include "qsslkey_p.h"
#include "qsslkey.h"
#else
#include "qtlsbackend_cert_p.h"
#endif

#include "qssl_p.h"

#include <QtCore/private/qfactoryloader_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qmutex.h>

#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QTlsBackend_iid, QStringLiteral("/tlsbackends")))

namespace {

class BackendCollection
{
public:
    void addBackend(QTlsBackend *backend)
    {
        Q_ASSERT(backend);
        Q_ASSERT(std::find(backends.begin(), backends.end(), backend) == backends.end());
        const QMutexLocker locker(&collectionMutex);
        backends.push_back(backend);
    }

    void removeBackend(QTlsBackend *backend)
    {
        Q_ASSERT(backend);
        const QMutexLocker locker(&collectionMutex);
        const auto it = std::find(backends.begin(), backends.end(), backend);
        Q_ASSERT(it != backends.end());
        backends.erase(it);
    }

    bool tryPopulateCollection()
    {
        if (!loader())
            return false;

        static QBasicMutex mutex;
        const QMutexLocker locker(&mutex);
        if (loaded)
            return true;

#if QT_CONFIG(library)
        loader->update();
#endif
        int index = 0;
        while (loader->instance(index))
            ++index;

        // TLSTODO: obviously, these two below should
        // disappear as soon as plugins are in place.
#if QT_CONFIG(ssl)
        QSslSocketPrivate::registerAdHocFactory();
#else
        static QTlsBackendCertOnly certGenerator;
#endif // QT_CONFIG(ssl)

        return loaded = true;
    }

    QList<QString> backendNames()
    {
        QList<QString> names;
        if (!tryPopulateCollection())
            return names;

        const QMutexLocker locker(&collectionMutex);
        if (!backends.size())
            return names;

        names.reserve(backends.size());
        for (const auto *backend : backends) {
            if (backend->isValid())
                names.append(backend->backendName());
        }

        return names;
    }

    QTlsBackend *backend(const QString &name)
    {
        if (!tryPopulateCollection())
            return nullptr;

        const QMutexLocker locker(&collectionMutex);
        const auto it = std::find_if(backends.begin(), backends.end(),
                                     [&name](const auto *fct) {return fct->backendName() == name;});

        return it == backends.end()  ? nullptr : *it;
    }

private:
    std::vector<QTlsBackend *> backends;
    QMutex collectionMutex;
    bool loaded = false;
};

} // Unnamed namespace

Q_GLOBAL_STATIC(BackendCollection, backends);

namespace QTlsPrivate {

TlsKey::~TlsKey() = default;

QByteArray TlsKey::pemHeader() const
{
    if (type() == QSsl::PublicKey)
        return QByteArrayLiteral("-----BEGIN PUBLIC KEY-----");
    else if (algorithm() == QSsl::Rsa)
        return QByteArrayLiteral("-----BEGIN RSA PRIVATE KEY-----");
    else if (algorithm() == QSsl::Dsa)
        return QByteArrayLiteral("-----BEGIN DSA PRIVATE KEY-----");
    else if (algorithm() == QSsl::Ec)
        return QByteArrayLiteral("-----BEGIN EC PRIVATE KEY-----");
    else if (algorithm() == QSsl::Dh)
        return QByteArrayLiteral("-----BEGIN PRIVATE KEY-----");

    Q_UNREACHABLE();
    return {};
}

QByteArray TlsKey::pemFooter() const
{
    if (type() == QSsl::PublicKey)
        return QByteArrayLiteral("-----END PUBLIC KEY-----");
    else if (algorithm() == QSsl::Rsa)
        return QByteArrayLiteral("-----END RSA PRIVATE KEY-----");
    else if (algorithm() == QSsl::Dsa)
        return QByteArrayLiteral("-----END DSA PRIVATE KEY-----");
    else if (algorithm() == QSsl::Ec)
        return QByteArrayLiteral("-----END EC PRIVATE KEY-----");
    else if (algorithm() == QSsl::Dh)
        return QByteArrayLiteral("-----END PRIVATE KEY-----");

    Q_UNREACHABLE();
    return {};
}

X509Certificate::~X509Certificate() = default;

TlsKey *X509Certificate::publicKey() const
{
    // 'no-ssl' build has no key support either.
    return nullptr;
}

#if QT_CONFIG(ssl)

TlsCryptograph::~TlsCryptograph() = default;

void TlsCryptograph::checkSettingSslContext(QSharedPointer<QSslContext> tlsContext)
{
    Q_UNUSED(tlsContext);
}

QSharedPointer<QSslContext> TlsCryptograph::sslContext() const
{
    return {};
}

void TlsCryptograph::enableHandshakeContinuation()
{
}

void TlsCryptograph::cancelCAFetch()
{
}

bool TlsCryptograph::hasUndecryptedData() const
{
    return false;
}

QList<QOcspResponse> TlsCryptograph::ocsps() const
{
    return {};
}

bool TlsCryptograph::isMatchingHostname(const QSslCertificate &cert, const QString &peerName)
{
    return QSslSocketPrivate::isMatchingHostname(cert, peerName);
}

bool TlsCryptograph::isMatchingHostname(const QString &cn, const QString &hostname)
{
    return QSslSocketPrivate::isMatchingHostname(cn, hostname);
}

#endif // QT_CONFIG(ssl)

#if QT_CONFIG(dtls)
DtlsBase::~DtlsBase() = default;
#endif // QT_CONFIG(dtls)

} // namespace QTlsPrivate

const QString QTlsBackend::builtinBackendNames[] = {
    QStringLiteral("schannel"),
    QStringLiteral("securetransport"),
    QStringLiteral("openssl")
};

QTlsBackend::QTlsBackend()
{
    if (backends())
        backends->addBackend(this);
}

QTlsBackend::~QTlsBackend()
{
    if (backends())
        backends->removeBackend(this);
}

bool QTlsBackend::isValid() const
{
    return true;
}

long QTlsBackend::tlsLibraryVersionNumber() const
{
    return 0;
}

QString QTlsBackend::tlsLibraryVersionString() const
{
    return {};
}

long QTlsBackend::tlsLibraryBuildVersionNumber() const
{
    return 0;
}

QString QTlsBackend::tlsLibraryBuildVersionString() const
{
    return {};
}

void QTlsBackend::ensureInitialized() const
{
}

QString QTlsBackend::backendName() const
{
    return QStringLiteral("dummyTLS");
}

#define REPORT_MISSING_SUPPORT(message) \
    qCWarning(lcSsl) << "The backend" << backendName() << message

QTlsPrivate::TlsKey *QTlsBackend::createKey() const
{
    REPORT_MISSING_SUPPORT("does not support QSslKey");
    return nullptr;
}

QTlsPrivate::X509Certificate *QTlsBackend::createCertificate() const
{
    REPORT_MISSING_SUPPORT("does not support QSslCertificate");
    return nullptr;
}

QList<QSslCertificate> QTlsBackend::systemCaCertificates() const
{
    REPORT_MISSING_SUPPORT("does not provide system CA certificates");
    return {};
}

QTlsPrivate::TlsCryptograph *QTlsBackend::createTlsCryptograph() const
{
    REPORT_MISSING_SUPPORT("does not support QSslSocket");
    return nullptr;
}

QTlsPrivate::DtlsCryptograph *QTlsBackend::createDtlsCryptograph(QDtls *qObject, int mode) const
{
    Q_UNUSED(qObject);
    Q_UNUSED(mode);
    REPORT_MISSING_SUPPORT("does not support QDtls");
    return nullptr;
}

QTlsPrivate::DtlsCookieVerifier *QTlsBackend::createDtlsCookieVerifier() const
{
    REPORT_MISSING_SUPPORT("does not support DTLS cookies");
    return nullptr;
}

QTlsPrivate::X509ChainVerifyPtr QTlsBackend::X509Verifier() const
{
    REPORT_MISSING_SUPPORT("does not support (manual) certificate verification");
    return nullptr;
}

QTlsPrivate::X509PemReaderPtr QTlsBackend::X509PemReader() const
{
    REPORT_MISSING_SUPPORT("cannot read PEM format");
    return nullptr;
}

QTlsPrivate::X509DerReaderPtr QTlsBackend::X509DerReader() const
{
    REPORT_MISSING_SUPPORT("cannot read DER format");
    return nullptr;
}

QTlsPrivate::X509Pkcs12ReaderPtr QTlsBackend::X509Pkcs12Reader() const
{
    REPORT_MISSING_SUPPORT("cannot read PKCS12 format");
    return nullptr;
}

QList<int> QTlsBackend::ellipticCurvesIds() const
{
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return {};
}

int QTlsBackend::curveIdFromShortName(const QString &name) const
{
    Q_UNUSED(name);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return 0;
}

int QTlsBackend::curveIdFromLongName(const QString &name) const
{
    Q_UNUSED(name);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return 0;
}

QString QTlsBackend::shortNameForId(int cid) const
{
    Q_UNUSED(cid);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return {};
}

QString QTlsBackend::longNameForId(int cid) const
{
    Q_UNUSED(cid);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return {};
}

bool QTlsBackend::isTlsNamedCurve(int cid) const
{
    Q_UNUSED(cid);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return false;
}

int QTlsBackend::dhParametersFromDer(const QByteArray &derData, QByteArray *data) const
{
    Q_UNUSED(derData);
    Q_UNUSED(data);
    REPORT_MISSING_SUPPORT("does not support QSslDiffieHellmanParameters in DER format");
    return {};
}

int QTlsBackend::dhParametersFromPem(const QByteArray &pemData, QByteArray *data) const
{
    Q_UNUSED(pemData);
    Q_UNUSED(data);
    REPORT_MISSING_SUPPORT("does not support QSslDiffieHellmanParameters in PEM format");
    return {};
}

QList<QString> QTlsBackend::availableBackendNames()
{
    if (!backends())
        return {};

    return backends->backendNames();
}

QString QTlsBackend::defaultBackendName()
{
    // We prefer native as default:
    const auto names = availableBackendNames();
    auto name = builtinBackendNames[nameIndexSchannel];
    if (names.contains(name))
        return name;
    name = builtinBackendNames[nameIndexSecureTransport];
    if (names.contains(name))
        return name;
    name = builtinBackendNames[nameIndexOpenSSL];
    if (names.contains(name))
        return name;

    if (names.size())
        return names[0];

    return {};
}

QTlsBackend *QTlsBackend::findBackend(const QString &backendName)
{
    if (!backends())
        return {};

    if (auto *fct = backends->backend(backendName))
        return fct;

    qCWarning(lcSsl) << "Cannot create unknown backend named" << backendName;
    return nullptr;
}

QTlsBackend *QTlsBackend::activeOrAnyBackend()
{
#if QT_CONFIG(ssl)
    return QSslSocketPrivate::tlsBackendInUse();
#else
    return findBackend(defaultBackendName());
#endif // QT_CONFIG(ssl)
}

QList<QSsl::SslProtocol> QTlsBackend::supportedProtocols(const QString &backendName)
{
    if (!backends())
        return {};

    if (const auto *fct = backends->backend(backendName))
        return fct->supportedProtocols();

    return {};
}

QList<QSsl::SupportedFeature> QTlsBackend::supportedFeatures(const QString &backendName)
{
    if (!backends())
        return {};

    if (const auto *fct = backends->backend(backendName))
        return fct->supportedFeatures();

    return {};
}

QList<QSsl::ImplementedClass> QTlsBackend::implementedClasses(const QString &backendName)
{
    if (!backends())
        return {};

    if (const auto *fct = backends->backend(backendName))
        return fct->implementedClasses();

    return {};
}

void QTlsBackend::resetBackend(QSslKey &key, QTlsPrivate::TlsKey *keyBackend)
{
#if QT_CONFIG(ssl)
    key.d->backend.reset(keyBackend);
#else
    Q_UNUSED(key);
    Q_UNUSED(keyBackend);
#endif // QT_CONFIG(ssl)
}

void QTlsBackend::setupClientPskAuth(QSslPreSharedKeyAuthenticator *auth, const char *hint,
                                     int hintLength, unsigned maxIdentityLen, unsigned maxPskLen)
{
    Q_ASSERT(auth);
#if QT_CONFIG(ssl)
    if (hint)
        auth->d->identityHint = QByteArray::fromRawData(hint, hintLength); // it's NUL terminated, but do not include the NUL

    auth->d->maximumIdentityLength = int(maxIdentityLen) - 1; // needs to be NUL terminated
    auth->d->maximumPreSharedKeyLength = int(maxPskLen);
#else
    Q_UNUSED(auth);
    Q_UNUSED(hint);
    Q_UNUSED(hintLength);
    Q_UNUSED(maxIdentityLen);
    Q_UNUSED(maxPskLen);
#endif
}

void QTlsBackend::setupServerPskAuth(QSslPreSharedKeyAuthenticator *auth, const char *identity,
                                     const QByteArray &identityHint, unsigned int maxPskLen)
{
#if QT_CONFIG(ssl)
    Q_ASSERT(auth);
    auth->d->identityHint = identityHint;
    auth->d->identity = identity;
    auth->d->maximumIdentityLength = 0; // user cannot set an identity
    auth->d->maximumPreSharedKeyLength = int(maxPskLen);
#else
    Q_UNUSED(auth);
    Q_UNUSED(identity);
    Q_UNUSED(identityHint);
    Q_UNUSED(maxPskLen);
#endif
}

#if QT_CONFIG(ssl)
QSslCipher QTlsBackend::createCiphersuite(const QString &descriptionOneLine, int bits, int supportedBits)
{
    QSslCipher ciph;

    const auto descriptionList = QStringView{descriptionOneLine}.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (descriptionList.size() > 5) {
        ciph.d->isNull = false;
        ciph.d->name = descriptionList.at(0).toString();

        QString protoString = descriptionList.at(1).toString();
        ciph.d->protocolString = protoString;
        ciph.d->protocol = QSsl::UnknownProtocol;
        if (protoString == QLatin1String("TLSv1"))
            ciph.d->protocol = QSsl::TlsV1_0;
        else if (protoString == QLatin1String("TLSv1.1"))
            ciph.d->protocol = QSsl::TlsV1_1;
        else if (protoString == QLatin1String("TLSv1.2"))
            ciph.d->protocol = QSsl::TlsV1_2;
        else if (protoString == QLatin1String("TLSv1.3"))
            ciph.d->protocol = QSsl::TlsV1_3;

        if (descriptionList.at(2).startsWith(QLatin1String("Kx=")))
            ciph.d->keyExchangeMethod = descriptionList.at(2).mid(3).toString();
        if (descriptionList.at(3).startsWith(QLatin1String("Au=")))
            ciph.d->authenticationMethod = descriptionList.at(3).mid(3).toString();
        if (descriptionList.at(4).startsWith(QLatin1String("Enc=")))
            ciph.d->encryptionMethod = descriptionList.at(4).mid(4).toString();
        ciph.d->exportable = (descriptionList.size() > 6 && descriptionList.at(6) == QLatin1String("export"));

        ciph.d->bits = bits;
        ciph.d->supportedBits = supportedBits;
    }

    return ciph;
}

QSslCipher QTlsBackend::createCiphersuite(const QString &suiteName, QSsl::SslProtocol protocol,
                                          const QString &protocolString)
{
    QSslCipher ciph;

    if (!suiteName.size())
        return ciph;

    ciph.d->isNull = false;
    ciph.d->name = suiteName;
    ciph.d->protocol = protocol;
    ciph.d->protocolString = protocolString;

    const auto bits = QStringView{ciph.d->name}.split(QLatin1Char('-'));
    if (bits.size() >= 2) {
        if (bits.size() == 2 || bits.size() == 3)
            ciph.d->keyExchangeMethod = QLatin1String("RSA");
        else if (bits.front() == QLatin1String("DH") || bits.front() == QLatin1String("DHE"))
            ciph.d->keyExchangeMethod = QLatin1String("DH");
        else if (bits.front() == QLatin1String("ECDH") || bits.front() == QLatin1String("ECDHE"))
            ciph.d->keyExchangeMethod = QLatin1String("ECDH");
        else
            qCWarning(lcSsl) << "Unknown Kx" << ciph.d->name;

        if (bits.size() == 2 || bits.size() == 3)
            ciph.d->authenticationMethod = QLatin1String("RSA");
        else if (ciph.d->name.contains(QLatin1String("-ECDSA-")))
            ciph.d->authenticationMethod = QLatin1String("ECDSA");
        else if (ciph.d->name.contains(QLatin1String("-RSA-")))
            ciph.d->authenticationMethod = QLatin1String("RSA");
        else
            qCWarning(lcSsl) << "Unknown Au" << ciph.d->name;

        if (ciph.d->name.contains(QLatin1String("RC4-"))) {
            ciph.d->encryptionMethod = QLatin1String("RC4(128)");
            ciph.d->bits = 128;
            ciph.d->supportedBits = 128;
        } else if (ciph.d->name.contains(QLatin1String("DES-CBC3-"))) {
            ciph.d->encryptionMethod = QLatin1String("3DES(168)");
            ciph.d->bits = 168;
            ciph.d->supportedBits = 168;
        } else if (ciph.d->name.contains(QLatin1String("AES128-"))) {
            ciph.d->encryptionMethod = QLatin1String("AES(128)");
            ciph.d->bits = 128;
            ciph.d->supportedBits = 128;
        } else if (ciph.d->name.contains(QLatin1String("AES256-GCM"))) {
            ciph.d->encryptionMethod = QLatin1String("AESGCM(256)");
            ciph.d->bits = 256;
            ciph.d->supportedBits = 256;
        } else if (ciph.d->name.contains(QLatin1String("AES256-"))) {
            ciph.d->encryptionMethod = QLatin1String("AES(256)");
            ciph.d->bits = 256;
            ciph.d->supportedBits = 256;
        } else if (ciph.d->name.contains(QLatin1String("NULL-"))) {
            ciph.d->encryptionMethod = QLatin1String("NULL");
        } else {
            qCWarning(lcSsl) << "Unknown Enc" << ciph.d->name;
        }
    }
    return ciph;
}

QSslCipher QTlsBackend::createCipher(const QString &name, QSsl::SslProtocol protocol,
                                     const QString &protocolString)
{
    // Note the name 'createCipher' (not 'ciphersuite'): we don't provide
    // information about Kx, Au, bits/supported etc.
    QSslCipher cipher;
    cipher.d->isNull = false;
    cipher.d->name = name;
    cipher.d->protocol = protocol;
    cipher.d->protocolString = protocolString;
    return cipher;
}

QList<QSslCipher> QTlsBackend::defaultCiphers()
{
    return QSslSocketPrivate::defaultCiphers();
}

QList<QSslCipher> QTlsBackend::defaultDtlsCiphers()
{
    return QSslSocketPrivate::defaultDtlsCiphers();
}

void QTlsBackend::setDefaultCiphers(const QList<QSslCipher> &ciphers)
{
    QSslSocketPrivate::setDefaultCiphers(ciphers);
}

void QTlsBackend::setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers)
{
    QSslSocketPrivate::setDefaultDtlsCiphers(ciphers);
}

void QTlsBackend::setDefaultSupportedCiphers(const QList<QSslCipher> &ciphers)
{
    QSslSocketPrivate::setDefaultSupportedCiphers(ciphers);
}

void QTlsBackend::resetDefaultEllipticCurves()
{
    QSslSocketPrivate::resetDefaultEllipticCurves();
}

void QTlsBackend::setDefaultCaCertificates(const QList<QSslCertificate> &certs)
{
    QSslSocketPrivate::setDefaultCaCertificates(certs);
}

#endif // QT_CONFIG(ssl)

QT_END_NAMESPACE
