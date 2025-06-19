// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qtlsbackend_p.h"

#if QT_CONFIG(ssl)
#include "qsslpresharedkeyauthenticator_p.h"
#include "qsslpresharedkeyauthenticator.h"
#include "qsslsocket_p.h"
#include "qsslcipher_p.h"
#include "qsslkey_p.h"
#include "qsslkey.h"
#endif

#include "qssl_p.h"

#include <QtCore/private/qfactoryloader_p.h>

#include "QtCore/qapplicationstatic.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmutex.h>

#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_APPLICATION_STATIC(QFactoryLoader, qtlsbLoader, QTlsBackend_iid,
                     QStringLiteral("/tls"))

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
        if (!qtlsbLoader())
            return false;

        Q_CONSTINIT static QBasicMutex mutex;
        const QMutexLocker locker(&mutex);
        if (backends.size())
            return true;

#if QT_CONFIG(library)
        qtlsbLoader->update();
#endif
        int index = 0;
        while (qtlsbLoader->instance(index))
            ++index;

        return true;
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
};

} // Unnamed namespace

Q_GLOBAL_STATIC(BackendCollection, backends);

/*!
    \class QTlsBackend
    \internal (Network-private)
    \brief QTlsBackend is a factory class, providing implementations
    for the QSsl classes.

    The purpose of QTlsBackend is to enable and simplify the addition
    of new TLS backends to be used by QSslSocket and related classes.
    Starting from Qt 6.1, these backends have plugin-based design (and
    thus can co-exist simultaneously, unlike pre 6.1 times), although
    any given run of a program can only use one of them.

    Inheriting from QTlsBackend and creating an object of such
    a class adds a new backend into the list of available TLS backends.

    A new backend must provide a list of classes, features and protocols
    it supports, and override the corresponding virtual functions that
    create backend-specific implementations for these QSsl-classes.

    The base abstract class - QTlsBackend - provides, where possible,
    default implementations of its virtual member functions. These
    default implementations can be overridden by a derived backend
    class, if needed.

    QTlsBackend also provides some auxiliary functions that a derived
    backend class can use to interact with the internals of network-private classes.

    \sa QSslSocket::availableBackends(), supportedFeatures(), supportedProtocols(), implementedClasses()
*/

/*!
    \fn QString QTlsBackend::backendName() const
    \internal
    Returns the name of this backend. The name will be reported by QSslSocket::availableBackends().
    Example of backend names: "openssl", "schannel", "securetransport".

    \sa QSslSocket::availableBackends(), isValid()
*/

const QString QTlsBackend::builtinBackendNames[] = {
    QStringLiteral("schannel"),
    QStringLiteral("securetransport"),
    QStringLiteral("openssl"),
    QStringLiteral("cert-only")
};

/*!
    \internal
    The default constructor, adds a new backend to the list of available backends.

    \sa ~QTlsBackend(), availableBackendNames(), QSslSocket::availableBackends()
*/
QTlsBackend::QTlsBackend()
{
    if (backends())
        backends->addBackend(this);

    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), &QCoreApplication::destroyed, this, [this] {
            delete this;
        });
    }
}

/*!
    \internal
    Removes this backend from the list of available backends.

    \sa QTlsBackend(), availableBackendNames(), QSslSocket::availableBackends()
*/
QTlsBackend::~QTlsBackend()
{
    if (backends())
        backends->removeBackend(this);
}

/*!
    \internal
    Returns \c true if this backend was initialised successfully. The default implementation
    always returns \c true.

    \note This function must be overridden if a particular backend has a non-trivial initialization
    that can fail. If reimplemented, returning \c false will exclude this backend from the list of
    backends, reported as available by QSslSocket.

    \sa QSslSocket::availableBackends()
*/

bool QTlsBackend::isValid() const
{
    return true;
}

/*!
    \internal
    Returns an implementations-specific integer value, representing the TLS library's
    version, that is currently used by this backend (i.e. runtime library version).
    The default implementation returns 0.

    \sa tlsLibraryBuildVersionNumber()
*/
long QTlsBackend::tlsLibraryVersionNumber() const
{
    return 0;
}

/*!
    \internal
    Returns an implementation-specific string, representing the TLS library's version,
    that is currently used by this backend (i.e. runtime library version). The default
    implementation returns an empty string.

    \sa tlsLibraryBuildVersionString()
*/

QString QTlsBackend::tlsLibraryVersionString() const
{
    return {};
}

/*!
    \internal
    Returns an implementation-specific integer value, representing the TLS library's
    version that this backend was built against (i.e. compile-time library version).
    The default implementation returns 0.

    \sa tlsLibraryVersionNumber()
*/

long QTlsBackend::tlsLibraryBuildVersionNumber() const
{
    return 0;
}

/*!
    \internal
    Returns an implementation-specific string, representing the TLS library's version
    that this backend was built against (i.e. compile-time version). The default
    implementation returns an empty string.

    \sa tlsLibraryVersionString()
*/
QString QTlsBackend::tlsLibraryBuildVersionString() const
{
    return {};
}

/*!
    \internal
    QSslSocket and related classes call this function to ensure that backend's internal
    resources - e.g. CA certificates, or ciphersuites - were properly initialized.
*/
void QTlsBackend::ensureInitialized() const
{
}

#define REPORT_MISSING_SUPPORT(message) \
    qCWarning(lcSsl) << "The backend" << backendName() << message

/*!
    \internal
    If QSsl::ImplementedClass::Key is present in this backend's implementedClasses(),
    the backend must reimplement this method to return a dynamically-allocated instance
    of an implementation-specific type, inheriting from the class QTlsPrivate::TlsKey.
    The default implementation of this function returns \nullptr.

    \sa QSslKey, implementedClasses(), QTlsPrivate::TlsKey
*/
QTlsPrivate::TlsKey *QTlsBackend::createKey() const
{
    REPORT_MISSING_SUPPORT("does not support QSslKey");
    return nullptr;
}

/*!
    \internal
    If QSsl::ImplementedClass::Certificate is present in this backend's implementedClasses(),
    the backend must reimplement this method to return a dynamically-allocated instance of an
    implementation-specific type, inheriting from the class QTlsPrivate::X509Certificate.
    The default implementation of this function returns \nullptr.

    \sa QSslCertificate, QTlsPrivate::X509Certificate, implementedClasses()
*/
QTlsPrivate::X509Certificate *QTlsBackend::createCertificate() const
{
    REPORT_MISSING_SUPPORT("does not support QSslCertificate");
    return nullptr;
}

/*!
    \internal
    This function returns a list of system CA certificates - e.g. certificates, loaded
    from a system store, if available. This function allows implementation of the class
    QSslConfiguration. The default implementation of this function returns an empty list.

    \sa QSslCertificate, QSslConfiguration
*/
QList<QSslCertificate> QTlsBackend::systemCaCertificates() const
{
    REPORT_MISSING_SUPPORT("does not provide system CA certificates");
    return {};
}

/*!
    \internal
    If QSsl::ImplementedClass::Socket is present in this backend's implementedClasses(),
    the backend must reimplement this method to return a dynamically-allocated instance of an
    implementation-specific type, inheriting from the class QTlsPrivate::TlsCryptograph.
    The default implementation of this function returns \nullptr.

    \sa QSslSocket, QTlsPrivate::TlsCryptograph, implementedClasses()
*/
QTlsPrivate::TlsCryptograph *QTlsBackend::createTlsCryptograph() const
{
    REPORT_MISSING_SUPPORT("does not support QSslSocket");
    return nullptr;
}

/*!
    \internal
    If QSsl::ImplementedClass::Dtls is present in this backend's implementedClasses(),
    the backend must reimplement this method to return a dynamically-allocated instance of an
    implementation-specific type, inheriting from the class QTlsPrivate::DtlsCryptograph.
    The default implementation of this function returns \nullptr.

    \sa QDtls, QTlsPrivate::DtlsCryptograph, implementedClasses()
*/
QTlsPrivate::DtlsCryptograph *QTlsBackend::createDtlsCryptograph(QDtls *qObject, int mode) const
{
    Q_UNUSED(qObject);
    Q_UNUSED(mode);
    REPORT_MISSING_SUPPORT("does not support QDtls");
    return nullptr;
}

/*!
    \internal
    If QSsl::ImplementedClass::DtlsCookie is present in this backend's implementedClasses(),
    the backend must reimplement this method to return a dynamically-allocated instance of an
    implementation-specific type, inheriting from the class QTlsPrivate::DtlsCookieVerifier. The
    default implementation returns \nullptr.

    \sa QDtlsClientVerifier, QTlsPrivate::DtlsCookieVerifier, implementedClasses()
*/
QTlsPrivate::DtlsCookieVerifier *QTlsBackend::createDtlsCookieVerifier() const
{
    REPORT_MISSING_SUPPORT("does not support DTLS cookies");
    return nullptr;
}

/*!
    \internal
    If QSsl::SupportedFeature::CertificateVerification is present in this backend's
    supportedFeatures(), the backend must reimplement this method to return a pointer
    to a function, that checks a certificate (or a chain of certificates) against available
    CA certificates. The default implementation returns \nullptr.

    \sa supportedFeatures(), QSslCertificate
*/

QTlsPrivate::X509ChainVerifyPtr QTlsBackend::X509Verifier() const
{
    REPORT_MISSING_SUPPORT("does not support (manual) certificate verification");
    return nullptr;
}

/*!
    \internal
    Returns a pointer to function, that reads certificates in PEM format. The
    default implementation returns \nullptr.

    \sa QSslCertificate
*/
QTlsPrivate::X509PemReaderPtr QTlsBackend::X509PemReader() const
{
    REPORT_MISSING_SUPPORT("cannot read PEM format");
    return nullptr;
}

/*!
    \internal
    Returns a pointer to function, that can read certificates in DER format.
    The default implementation returns \nullptr.

    \sa QSslCertificate
*/
QTlsPrivate::X509DerReaderPtr QTlsBackend::X509DerReader() const
{
    REPORT_MISSING_SUPPORT("cannot read DER format");
    return nullptr;
}

/*!
    \internal
    Returns a pointer to function, that can read certificates in PKCS 12 format.
    The default implementation returns \nullptr.

    \sa QSslCertificate
*/
QTlsPrivate::X509Pkcs12ReaderPtr QTlsBackend::X509Pkcs12Reader() const
{
    REPORT_MISSING_SUPPORT("cannot read PKCS12 format");
    return nullptr;
}

/*!
    \internal
    If QSsl::ImplementedClass::EllipticCurve is present in this backend's implementedClasses(),
    and the backend provides information about supported curves, it must reimplement this
    method to return a list of unique identifiers of the supported elliptic curves. The default
    implementation returns an empty list.

    \note The meaning of a curve identifier is implementation-specific.

    \sa implemenedClasses(), QSslEllipticCurve
*/
QList<int> QTlsBackend::ellipticCurvesIds() const
{
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return {};
}

/*!
    \internal
    If this backend provides information about available elliptic curves, this
    function should return a unique integer identifier for a curve named \a name,
    which is a conventional short name for the curve. The default implementation
    returns 0.

    \note The meaning of a curve identifier is implementation-specific.

    \sa QSslEllipticCurve::shortName()
*/
int QTlsBackend::curveIdFromShortName(const QString &name) const
{
    Q_UNUSED(name);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return 0;
}

/*!
    \internal
    If this backend provides information about available elliptic curves, this
    function should return a unique integer identifier for a curve named \a name,
    which is a conventional long name for the curve. The default implementation
    returns 0.

    \note The meaning of a curve identifier is implementation-specific.

    \sa QSslElliptiCurve::longName()
*/
int QTlsBackend::curveIdFromLongName(const QString &name) const
{
    Q_UNUSED(name);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return 0;
}

/*!
    \internal
    If this backend provides information about available elliptic curves,
    this function should return a conventional short name for a curve identified
    by \a cid. The default implementation returns an empty string.

    \note The meaning of a curve identifier is implementation-specific.

    \sa ellipticCurvesIds(), QSslEllipticCurve::shortName()
*/
QString QTlsBackend::shortNameForId(int cid) const
{
    Q_UNUSED(cid);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return {};
}

/*!
    \internal
    If this backend provides information about available elliptic curves,
    this function should return a conventional long name for a curve identified
    by \a cid. The default implementation returns an empty string.

    \note The meaning of a curve identifier is implementation-specific.

    \sa ellipticCurvesIds(), QSslEllipticCurve::shortName()
*/
QString QTlsBackend::longNameForId(int cid) const
{
    Q_UNUSED(cid);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return {};
}

/*!
    \internal
    Returns true if the elliptic curve identified by \a cid is one of the named
    curves, that can be used in the key exchange when using an elliptic curve
    cipher with TLS; false otherwise. The default implementation returns false.

    \note The meaning of curve identifier is implementation-specific.
*/
bool QTlsBackend::isTlsNamedCurve(int cid) const
{
    Q_UNUSED(cid);
    REPORT_MISSING_SUPPORT("does not support QSslEllipticCurve");
    return false;
}

/*!
    \internal
    If this backend supports the class QSslDiffieHellmanParameters, this function is
    needed for construction of a QSslDiffieHellmanParameters from DER encoded data.
    This function is expected to return a value that matches an enumerator in
    QSslDiffieHellmanParameters::Error enumeration. The default implementation of this
    function returns 0 (equals to QSslDiffieHellmanParameters::NoError).

    \sa QSslDiffieHellmanParameters, implementedClasses()
*/
int QTlsBackend::dhParametersFromDer(const QByteArray &derData, QByteArray *data) const
{
    Q_UNUSED(derData);
    Q_UNUSED(data);
    REPORT_MISSING_SUPPORT("does not support QSslDiffieHellmanParameters in DER format");
    return {};
}

/*!
    \internal
    If this backend supports the class QSslDiffieHellmanParameters, this function is
    is needed for construction of a QSslDiffieHellmanParameters from PEM encoded data.
    This function is expected to return a value that matches an enumerator in
    QSslDiffieHellmanParameters::Error enumeration. The default implementation of this
    function returns 0 (equals to QSslDiffieHellmanParameters::NoError).

    \sa QSslDiffieHellmanParameters, implementedClasses()
*/
int QTlsBackend::dhParametersFromPem(const QByteArray &pemData, QByteArray *data) const
{
    Q_UNUSED(pemData);
    Q_UNUSED(data);
    REPORT_MISSING_SUPPORT("does not support QSslDiffieHellmanParameters in PEM format");
    return {};
}

/*!
    \internal
    Returns a list of names of available backends.

    \note This list contains only properly initialized backends.

    \sa QTlsBackend(), isValid()
*/
QList<QString> QTlsBackend::availableBackendNames()
{
    if (!backends())
        return {};

    return backends->backendNames();
}

/*!
    \internal
    Returns the name of the backend that QSslSocket() would use by default. If no
    backend was found, the function returns an empty string.
*/
QString QTlsBackend::defaultBackendName()
{
    // We prefer OpenSSL as default:
    const auto names = availableBackendNames();
    auto name = builtinBackendNames[nameIndexOpenSSL];
    if (names.contains(name))
        return name;
    name = builtinBackendNames[nameIndexSchannel];
    if (names.contains(name))
        return name;
    name = builtinBackendNames[nameIndexSecureTransport];
    if (names.contains(name))
        return name;

    const auto pos = std::find_if(names.begin(), names.end(), [](const auto &name) {
        return name != builtinBackendNames[nameIndexCertOnly];
    });

    if (pos != names.end())
        return *pos;

    if (names.size())
        return names[0];

    return {};
}

/*!
    \internal
    Returns a backend named \a backendName, if it exists.
    Otherwise, it returns \nullptr.

    \sa backendName(), QSslSocket::availableBackends()
*/
QTlsBackend *QTlsBackend::findBackend(const QString &backendName)
{
    if (!backends())
        return {};

    if (auto *fct = backends->backend(backendName))
        return fct;

    qCWarning(lcSsl) << "Cannot create unknown backend named" << backendName;
    return nullptr;
}

/*!
    \internal
    Returns the backend that QSslSocket is using. If Qt was built without TLS support,
    this function returns a minimal backend that only supports QSslCertificate.

    \sa defaultBackend()
*/
QTlsBackend *QTlsBackend::activeOrAnyBackend()
{
#if QT_CONFIG(ssl)
    return QSslSocketPrivate::tlsBackendInUse();
#else
    return findBackend(defaultBackendName());
#endif // QT_CONFIG(ssl)
}

/*!
    \internal
    Returns a list of TLS and DTLS protocol versions, that a backend named
    \a backendName supports.

    \note This list is supposed to also include range-based versions, which
    allows negotiation of protocols during the handshake, so that these versions
    can be used when configuring QSslSocket (e.g. QSsl::TlsV1_2OrLater).

    \sa QSsl::SslProtocol
*/
QList<QSsl::SslProtocol> QTlsBackend::supportedProtocols(const QString &backendName)
{
    if (!backends())
        return {};

    if (const auto *fct = backends->backend(backendName))
        return fct->supportedProtocols();

    return {};
}

/*!
    \internal
    Returns a list of features that a backend named \a backendName supports. E.g.
    a backend may support PSK (pre-shared keys, defined as QSsl::SupportedFeature::Psk)
    or ALPN (application layer protocol negotiation, identified by
    QSsl::SupportedFeature::ClientSideAlpn or QSsl::SupportedFeature::ServerSideAlpn).

    \sa QSsl::SupportedFeature
*/
QList<QSsl::SupportedFeature> QTlsBackend::supportedFeatures(const QString &backendName)
{
    if (!backends())
        return {};

    if (const auto *fct = backends->backend(backendName))
        return fct->supportedFeatures();

    return {};
}

/*!
    \internal
    Returns a list of classes that a backend named \a backendName supports. E.g. a backend
    may implement QSslSocket (QSsl::ImplementedClass::Socket), and QDtls
    (QSsl::ImplementedClass::Dtls).

    \sa QSsl::ImplementedClass
*/
QList<QSsl::ImplementedClass> QTlsBackend::implementedClasses(const QString &backendName)
{
    if (!backends())
        return {};

    if (const auto *fct = backends->backend(backendName))
        return fct->implementedClasses();

    return {};
}

/*!
    \internal
    Auxiliary function. Initializes \a key to use \a keyBackend.
*/
void QTlsBackend::resetBackend(QSslKey &key, QTlsPrivate::TlsKey *keyBackend)
{
#if QT_CONFIG(ssl)
    key.d->backend.reset(keyBackend);
#else
    Q_UNUSED(key);
    Q_UNUSED(keyBackend);
#endif // QT_CONFIG(ssl)
}

/*!
    \internal
    Auxiliary function. Initializes client-side \a auth using the \a hint, \a hintLength,
    \a maxIdentityLength and \a maxPskLen.
*/
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

/*!
    \internal
    Auxiliary function. Initializes server-side \a auth using the \a identity, \a identityHint and
    \a maxPskLen.
*/
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
/*!
    \internal
    Auxiliary function. Creates a new QSslCipher from \a descriptionOneLine, \a bits
    and \a supportedBits. \a descriptionOneLine consists of several fields, separated by
    whitespace. These include: cipher name, protocol version, key exchange method,
    authentication method, encryption method, message digest (Mac). Example:
    "ECDHE-RSA-AES256-GCM-SHA256 TLSv1.2 Kx=ECDH Au=RSA Enc=AESGCM(256) Mac=AEAD"
*/
QSslCipher QTlsBackend::createCiphersuite(const QString &descriptionOneLine, int bits, int supportedBits)
{
    QSslCipher ciph;

    const auto descriptionList = QStringView{descriptionOneLine}.split(u' ', Qt::SkipEmptyParts);
    if (descriptionList.size() > 5) {
        ciph.d->isNull = false;
        ciph.d->name = descriptionList.at(0).toString();

        QStringView protoString = descriptionList.at(1);
        ciph.d->protocolString = protoString.toString();
        ciph.d->protocol = QSsl::UnknownProtocol;
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        if (protoString.startsWith(u"TLSv1")) {
            QStringView tail = protoString.sliced(5);
            if (tail.startsWith(u'.')) {
                tail = tail.sliced(1);
                if (tail == u"3")
                    ciph.d->protocol = QSsl::TlsV1_3;
                else if (tail == u"2")
                    ciph.d->protocol = QSsl::TlsV1_2;
                else if (tail == u"1")
                    ciph.d->protocol = QSsl::TlsV1_1;
            } else if (tail.isEmpty()) {
                ciph.d->protocol = QSsl::TlsV1_0;
            }
        }
QT_WARNING_POP

        if (descriptionList.at(2).startsWith("Kx="_L1))
            ciph.d->keyExchangeMethod = descriptionList.at(2).mid(3).toString();
        if (descriptionList.at(3).startsWith("Au="_L1))
            ciph.d->authenticationMethod = descriptionList.at(3).mid(3).toString();
        if (descriptionList.at(4).startsWith("Enc="_L1))
            ciph.d->encryptionMethod = descriptionList.at(4).mid(4).toString();
        ciph.d->exportable = (descriptionList.size() > 6 && descriptionList.at(6) == "export"_L1);

        ciph.d->bits = bits;
        ciph.d->supportedBits = supportedBits;
    }

    return ciph;
}

/*!
    \internal
    Auxiliary function. Creates a new QSslCipher from \a suiteName, \a protocol version and
    \a protocolString. For example:
    \code
    createCiphersuite("ECDHE-RSA-AES256-GCM-SHA256"_L1, QSsl::TlsV1_2, "TLSv1.2"_L1);
    \endcode
*/
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

    const auto bits = QStringView{ciph.d->name}.split(u'-');
    if (bits.size() >= 2) {
        if (bits.size() == 2 || bits.size() == 3)
            ciph.d->keyExchangeMethod = "RSA"_L1;
        else if (bits.front() == "DH"_L1 || bits.front() == "DHE"_L1)
            ciph.d->keyExchangeMethod = "DH"_L1;
        else if (bits.front() == "ECDH"_L1 || bits.front() == "ECDHE"_L1)
            ciph.d->keyExchangeMethod = "ECDH"_L1;
        else
            qCWarning(lcSsl) << "Unknown Kx" << ciph.d->name;

        if (bits.size() == 2 || bits.size() == 3)
            ciph.d->authenticationMethod = "RSA"_L1;
        else if (ciph.d->name.contains("-ECDSA-"_L1))
            ciph.d->authenticationMethod = "ECDSA"_L1;
        else if (ciph.d->name.contains("-RSA-"_L1))
            ciph.d->authenticationMethod = "RSA"_L1;
        else
            qCWarning(lcSsl) << "Unknown Au" << ciph.d->name;

        if (ciph.d->name.contains("RC4-"_L1)) {
            ciph.d->encryptionMethod = "RC4(128)"_L1;
            ciph.d->bits = 128;
            ciph.d->supportedBits = 128;
        } else if (ciph.d->name.contains("DES-CBC3-"_L1)) {
            ciph.d->encryptionMethod = "3DES(168)"_L1;
            ciph.d->bits = 168;
            ciph.d->supportedBits = 168;
        } else if (ciph.d->name.contains("AES128-"_L1)) {
            ciph.d->encryptionMethod = "AES(128)"_L1;
            ciph.d->bits = 128;
            ciph.d->supportedBits = 128;
        } else if (ciph.d->name.contains("AES256-GCM"_L1)) {
            ciph.d->encryptionMethod = "AESGCM(256)"_L1;
            ciph.d->bits = 256;
            ciph.d->supportedBits = 256;
        } else if (ciph.d->name.contains("AES256-"_L1)) {
            ciph.d->encryptionMethod = "AES(256)"_L1;
            ciph.d->bits = 256;
            ciph.d->supportedBits = 256;
        } else if (ciph.d->name.contains("CHACHA20-"_L1)) {
            ciph.d->encryptionMethod = "CHACHA20"_L1;
            ciph.d->bits = 256;
            ciph.d->supportedBits = 256;
        } else if (ciph.d->name.contains("NULL-"_L1)) {
            ciph.d->encryptionMethod = "NULL"_L1;
        } else {
            qCWarning(lcSsl) << "Unknown Enc" << ciph.d->name;
        }
    }
    return ciph;
}

/*!
    \internal
    Auxiliary function. Creates a new QSslCipher from \a name, \a keyExchangeMethod, \a encryptionMethod,
    \a authenticationMethod, \a bits, \a protocol version and \a protocolString.
    For example:
    \code
    createCiphersuite("ECDHE-RSA-AES256-GCM-SHA256"_L1, "ECDH"_L1, "AES"_L1, "RSA"_L1, 256,
                      QSsl::TlsV1_2, "TLSv1.2"_L1);
    \endcode
*/
QSslCipher QTlsBackend::createCiphersuite(const QString &name, const QString &keyExchangeMethod,
                                          const QString &encryptionMethod,
                                          const QString &authenticationMethod,
                                          int bits, QSsl::SslProtocol protocol,
                                          const QString &protocolString)
{
    QSslCipher cipher;
    cipher.d->isNull = false;
    cipher.d->name = name;
    cipher.d->bits = bits;
    cipher.d->supportedBits = bits;
    cipher.d->keyExchangeMethod = keyExchangeMethod;
    cipher.d->encryptionMethod = encryptionMethod;
    cipher.d->authenticationMethod = authenticationMethod;
    cipher.d->protocol = protocol;
    cipher.d->protocolString = protocolString;
    return cipher;
}

/*!
    \internal
    Returns an implementation-specific list of ciphersuites that can be used by QSslSocket.

    \sa QSslConfiguration::defaultCiphers()
*/
QList<QSslCipher> QTlsBackend::defaultCiphers()
{
    return QSslSocketPrivate::defaultCiphers();
}

/*!
    \internal
    Returns an implementation-specific list of ciphersuites that can be used by QDtls.
*/
QList<QSslCipher> QTlsBackend::defaultDtlsCiphers()
{
    return QSslSocketPrivate::defaultDtlsCiphers();
}

/*!
    \internal
    Sets \a ciphers as defaults ciphers that QSslSocket can use.

    \sa defaultCiphers()
*/
void QTlsBackend::setDefaultCiphers(const QList<QSslCipher> &ciphers)
{
    QSslSocketPrivate::setDefaultCiphers(ciphers);
}

/*!
    \internal
    Sets \a ciphers as defaults ciphers that QDtls can use.

    \sa defaultDtlsCiphers()
*/
void QTlsBackend::setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers)
{
    QSslSocketPrivate::setDefaultDtlsCiphers(ciphers);
}

/*!
    \internal
    Sets \a ciphers as a list of supported ciphers.

    \sa QSslConfiguration::supportedCiphers()
*/
void QTlsBackend::setDefaultSupportedCiphers(const QList<QSslCipher> &ciphers)
{
    QSslSocketPrivate::setDefaultSupportedCiphers(ciphers);
}

/*!
    \internal
    Sets the list of QSslEllipticCurve objects, that QSslConfiguration::supportedEllipticCurves()
    returns, to ones that are supported by this backend.
*/
void QTlsBackend::resetDefaultEllipticCurves()
{
    QSslSocketPrivate::resetDefaultEllipticCurves();
}

/*!
    Sets \a certs as a list of certificates, that QSslConfiguration::caCertificates()
    reports.

    \sa QSslConfiguration::defaultConfiguration(), QSslConfiguration::caCertificates()
*/
void QTlsBackend::setDefaultCaCertificates(const QList<QSslCertificate> &certs)
{
    QSslSocketPrivate::setDefaultCaCertificates(certs);
}

/*!
    \internal
    Returns true if \a configuration allows loading root certificates on demand.
*/
bool QTlsBackend::rootLoadingOnDemandAllowed(const QSslConfiguration &configuration)
{
    return configuration.d->allowRootCertOnDemandLoading;
}

/*!
    \internal
    Stores \a peerCert in the \a configuration.
*/
void QTlsBackend::storePeerCertificate(QSslConfiguration &configuration,
                                       const QSslCertificate &peerCert)
{
    configuration.d->peerCertificate = peerCert;
}

/*!
    \internal
    Stores \a peerChain in the \a configuration.
*/
void QTlsBackend::storePeerCertificateChain(QSslConfiguration &configuration,
                                            const QList<QSslCertificate> &peerChain)
{
    configuration.d->peerCertificateChain = peerChain;
}

/*!
    \internal
    Clears the peer certificate chain in \a configuration.
*/
void QTlsBackend::clearPeerCertificates(QSslConfiguration &configuration)
{
    configuration.d->peerCertificate.clear();
    configuration.d->peerCertificateChain.clear();
}

/*!
    \internal
    Clears the peer certificate chain in \a d.
*/
void QTlsBackend::clearPeerCertificates(QSslSocketPrivate *d)
{
    Q_ASSERT(d);
    d->configuration.peerCertificate.clear();
    d->configuration.peerCertificateChain.clear();
}

/*!
    \internal
    Updates the configuration in \a d with \a shared value.
*/
void QTlsBackend::setPeerSessionShared(QSslSocketPrivate *d, bool shared)
{
    Q_ASSERT(d);
    d->configuration.peerSessionShared = shared;
}

/*!
    \internal
    Sets TLS session in \a d to \a asn1.
*/
void QTlsBackend::setSessionAsn1(QSslSocketPrivate *d, const QByteArray &asn1)
{
    Q_ASSERT(d);
    d->configuration.sslSession = asn1;
}

/*!
    \internal
    Sets TLS session lifetime hint in \a d to \a hint.
*/
void QTlsBackend::setSessionLifetimeHint(QSslSocketPrivate *d, int hint)
{
    Q_ASSERT(d);
    d->configuration.sslSessionTicketLifeTimeHint = hint;
}

/*!
    \internal
    Sets application layer protocol negotiation status in \a d to \a st.
*/
void QTlsBackend::setAlpnStatus(QSslSocketPrivate *d, AlpnNegotiationStatus st)
{
    Q_ASSERT(d);
    d->configuration.nextProtocolNegotiationStatus = st;
}

/*!
    \internal
    Sets \a protocol in \a d as a negotiated application layer protocol.
*/
void QTlsBackend::setNegotiatedProtocol(QSslSocketPrivate *d, const QByteArray &protocol)
{
    Q_ASSERT(d);
    d->configuration.nextNegotiatedProtocol = protocol;
}

/*!
    \internal
    Stores \a peerCert in the TLS configuration of \a d.
*/
void QTlsBackend::storePeerCertificate(QSslSocketPrivate *d, const QSslCertificate &peerCert)
{
    Q_ASSERT(d);
    d->configuration.peerCertificate = peerCert;
}

/*!
    \internal

    Stores \a peerChain in the TLS configuration of \a d.

    \note This is a helper function that TlsCryptograph and DtlsCryptograph
    call during a handshake.
*/
void QTlsBackend::storePeerCertificateChain(QSslSocketPrivate *d,
                                            const QList<QSslCertificate> &peerChain)
{
    Q_ASSERT(d);
    d->configuration.peerCertificateChain = peerChain;
}

/*!
    \internal

    Adds \a rootCert to the list of trusted root certificates in \a d.

    \note In Qt 6.1 it's only used on Windows, during so called 'CA fetch'.
*/
void QTlsBackend::addTustedRoot(QSslSocketPrivate *d, const QSslCertificate &rootCert)
{
    Q_ASSERT(d);
    if (!d->configuration.caCertificates.contains(rootCert))
        d->configuration.caCertificates += rootCert;
}

/*!
    \internal

    Saves ephemeral \a key in \a d.

    \sa QSslConfiguration::ephemeralKey()
*/
void QTlsBackend::setEphemeralKey(QSslSocketPrivate *d, const QSslKey &key)
{
    Q_ASSERT(d);
    d->configuration.ephemeralServerKey = key;
}

/*!
    \internal

    Implementation-specific. Sets the security level suitable for Qt's
    auto-tests.
*/
void QTlsBackend::forceAutotestSecurityLevel()
{
}

#endif // QT_CONFIG(ssl)

namespace QTlsPrivate {

/*!
    \internal (Network-private)
    \namespace QTlsPrivate
    \brief Namespace containing onternal types that TLS backends implement.

    This namespace is private to Qt and the backends that implement its TLS support.
*/

/*!
    \class TlsKey
    \internal (Network-private)
    \brief TlsKey is an abstract class, that allows a TLS plugin to provide
    an underlying implementation for the class QSslKey.

    Most functions in the class TlsKey are pure virtual and thus have to be
    reimplemented by a TLS backend that supports QSslKey. In many cases an
    empty implementation as an overrider is sufficient, albeit with some
    of QSslKey's functionality missing.

    \sa QTlsBackend::createKey(), QTlsBackend::implementedClasses(), QSslKey
*/

/*!
    \fn void TlsKey::decodeDer(KeyType type, KeyAlgorithm algorithm, const QByteArray &der, const QByteArray &passPhrase, bool deepClear)
    \internal

    If a support of public and private keys in DER format is required, this function
    must be overridden and should initialize this key using the \a type, \a algorithm, \a der
    and \a passPhrase. If this key was initialized previously, \a deepClear
    has an implementation-specific meaning (e.g., if an implementation is using
    reference-counting and can share internally some data structures, a value \c true may
    trigger decrementing a reference counter on some implementation-specific object).

    \note An empty overrider is sufficient, but then reading keys in QSsl::Der format
    will not be supported.

    \sa isNull(), QSsl::KeyType, QSsl::EncodingFormat, QSsl::KeyAlgorithm
*/

/*!
    \fn void TlsKey::decodePem(KeyType type, KeyAlgorithm algorithm, const QByteArray &pem, const QByteArray &passPhrase, bool deepClear)
    \internal

    If a support of public and private keys in PEM format is required, this function must
    be overridden and should initialize this key using the \a type, \a algorithm, \a pem and
    \a passPhrase. If this key was initialized previously, \a deepClear has an
    implementation-specific meaning (e.g., in an implementation using reference-counting,
    a value \c true may trigger decrementing a reference counter on some implementation-specific
    object).

    \note An empty overrider is sufficient, but then reading keys in QSsl::Pem format
    will not be supported.

    \sa isNull(), QSsl::KeyType, QSsl::EncodingFormat, QSsl::KeyAlgorithm
*/

/*!
    \fn QByteArray TlsKey::toPem(const QByteArray &passPhrase) const
    \internal

    This function must be overridden, if converting a key to PEM format, potentially with
    encryption, is needed (e.g. to save a QSslKey into a file). If this key is
    private and \a passPhrase is not empty, the key's data is expected to be encrypted using
    some conventional encryption algorithm (e.g. DES or AES - the one that different tools
    or even the class QSslKey can understand later).

    \note If this particular functionality is not needed, an overrider returning an
    empty QByteArray is sufficient.

    \sa QSslKey::toPem()
*/

/*!
    \fn QByteArray TlsKey::derFromPem(const QByteArray &pem, QMap<QByteArray, QByteArray> *headers) const
    \internal

    Converts \a pem to DER format, using this key's type and algorithm. The parameter \a headers
    must be a valid, non-null pointer. When parsing \a pem, the headers found there will be saved
    into \a headers.

    \note An overrider returning an empty QByteArray is sufficient, if QSslKey::toDer() is not
    needed.

    \note This function is very implementation-specific. A backend, that already has this key's
    non-empty DER data, may simply return this data.

    \sa QSslKey::toDer()
*/

/*!
    \fn QByteArray TlsKey::pemFromDer(const QByteArray &der, const QMap<QByteArray, QByteArray> &headers) const
    \internal

    If overridden, this function is expected to convert \a der, using \a headers, to PEM format.

    \note This function is very implementation-specific. As of now (Qt 6.1), it is only required by
    Qt's own non-OpenSSL backends, that internally use DER and implement QSslKey::toPem()
    via pemFromDer().
*/

/*!
    \fn void TlsKey::fromHandle(Qt::HANDLE handle, KeyType type)
    \internal

    Initializes this key using the \a handle and \a type, taking the ownership
    of the \a handle.

    \note The meaning of the \a handle is implementation-specific.

    \note If a TLS backend does not support such keys, it must provide an
    empty implementation.

    \sa handle(), QSslKey::QSslKey(), QSslKet::handle()
*/

/*!
    \fn TlsKey::handle() const
    \internal

    If a TLS backend supports opaque keys, returns a native handle that
    this key was initialized with.

    \sa fromHandle(), QSslKey::handle()
*/

/*!
    \fn bool TlsKey::isNull() const
    \internal

    Returns \c true if this is a null key, \c false otherwise.

    \note A null key corresponds to the default-constructed
    QSslKey or the one, that was cleared via QSslKey::clear().

    \sa QSslKey::isNull()
*/

/*!
    \fn QSsl::KeyType TlsKey::type() const
    \internal

    Returns the type of this key (public or private).
*/

/*!
    \fn QSsl::KeyAlgorithm TlsKey::algorithm() const
    \internal

    Return this key's algorithm.
*/

/*!
    \fn int TlsKey::length() const
    \internal

    Returns the length of the key in bits, or -1 if the key is null.
*/

/*!
    \fn void TlsKey::clear(bool deep)
    \internal

    Clears the contents of this key, making it a null key. The meaning
    of \a deep is implementation-specific (e.g. if some internal objects
    representing a key can be shared using reference counting, \a deep equal
    to \c true would imply decrementing a reference count).

    \sa isNull()
*/

/*!
    \fn bool TlsKey::isPkcs8() const
    \internal

    This function is internally used only by Qt's own TLS plugins and affects
    the way PEM file is generated by TlsKey. It's sufficient to override it
    and return \c false in case a new TLS backend is not using Qt's plugin
    as a base.
*/

/*!
    \fn QByteArray TlsKey::decrypt(Cipher cipher, const QByteArray &data, const QByteArray &passPhrase, const QByteArray &iv) const
    \internal

    This function allows to decrypt \a data (for example, a private key read from a file), using
    \a passPhrase, initialization vector \a iv. \a cipher is describing a block cipher and its
    mode (for example, AES256 + CBC). decrypt() is needed to implement QSslKey's constructor.

    \note A TLS backend may provide an empty implementation, but as a result QSslKey will not be able
    to work with private encrypted keys.

    \sa QSslKey
*/

/*!
    \fn QByteArray TlsKey::encrypt(Cipher cipher, const QByteArray &data, const QByteArray &passPhrase, const QByteArray &iv) const
    \internal

    This function is needed to implement QSslKey::toPem() with encryption (for a private
    key). \a cipher names a block cipher to use to encrypt \a data, using
    \a passPhrase and initialization vector \a iv.

    \note An empty implementation is sufficient, but QSslKey::toPem() will fail for
    a private key and non-empty passphrase.

    \sa QSslKey
*/

/*!
    \internal

    Destroys this key.
*/
TlsKey::~TlsKey() = default;

/*!
    \internal

    A convenience function that returns a string, corresponding to the
    key type or algorithm, which can be used as a header in a PEM file.
*/
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

    Q_UNREACHABLE_RETURN({});
}

/*!
    \internal
    A convenience function that returns a string, corresponding to the
    key type or algorithm, which can be used as a footer in a PEM file.
*/
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

    Q_UNREACHABLE_RETURN({});
}

/*!
    \class X509Certificate
    \internal (Network-private)
    \brief X509Certificate is an abstract class that allows a TLS backend to
    provide an implementation of the QSslCertificate class.

    This class provides an interface that must be reimplemented by a TLS plugin,
    that supports QSslCertificate. Most functions are pure virtual, and thus
    have to be overridden. For some of them, an empty overrider is acceptable,
    though a part of functionality in QSslCertificate will be missing.

    \sa QTlsBackend::createCertificate(), QTlsBackend::X509PemReader(), QTlsBackend::X509DerReader()
*/

/*!
    \fn bool X509Certificate::isEqual(const X509Certificate &other) const
    \internal

    This function is expected to return \c true if this certificate is the same as
    the \a other, \c false otherwise. Used by QSslCertificate's comparison operators.
*/

/*!
    \fn bool X509Certificate::isNull() const
    \internal

    Returns true if this certificate was default-constructed and not initialized yet.
    This function is called by QSslCertificate::isNull().

    \sa QSslCertificate::isNull()
*/

/*!
    \fn bool X509Certificate::isSelfSigned() const
    \internal

    This function is needed to implement QSslCertificate::isSelfSigned()

    \sa QSslCertificate::isSelfSigned()
*/

/*!
    \fn QByteArray X509Certificate::version() const
    \internal

    Implements QSslCertificate::version().

    \sa QSslCertificate::version()
*/

/*!
    \fn QByteArray X509Certificate::serialNumber() const
    \internal

    This function is expected to return the certificate's serial number string in
    hexadecimal format.

    \sa QSslCertificate::serialNumber()
*/

/*!
    \fn QStringList X509Certificate::issuerInfo(QSslCertificate::SubjectInfo subject) const
    \internal

    This function is expected to return the issuer information for the \a subject
    from the certificate, or an empty list if there is no information for subject
    in the certificate. There can be more than one entry of each type.

    \sa QSslCertificate::issuerInfo().
*/

/*!
    \fn QStringList X509Certificate::issuerInfo(const QByteArray &attribute) const
    \internal

    This function is expected to return the issuer information for attribute from
    the certificate, or an empty list if there is no information for \a attribute
    in the certificate. There can be more than one entry for an attribute.

    \sa QSslCertificate::issuerInfo().
*/

/*!
    \fn QStringList X509Certificate::subjectInfo(QSslCertificate::SubjectInfo subject) const
    \internal

    This function is expected to return the information for the \a subject, or an empty list
    if there is no information for subject in the certificate. There can be more than one
    entry of each type.

    \sa QSslCertificate::subjectInfo().
*/

/*!
    \fn QStringList X509Certificate::subjectInfo(const QByteArray &attribute) const
    \internal

    This function is expected to return the subject information for \a attribute, or
    an empty list if there is no information for attribute in the certificate.
    There can be more than one entry for an attribute.

    \sa QSslCertificate::subjectInfo().
*/

/*!
    \fn QList<QByteArray> X509Certificate::subjectInfoAttributes() const
    \internal

    This function is expected to return a list of the attributes that have values
    in the subject information of this certificate. The information associated
    with a given attribute can be accessed using the subjectInfo() method. Note
    that this list may include the OIDs for any elements that are not known by
    the TLS backend.

    \note This function is needed for QSslCertificate:::subjectInfoAttributes().

    \sa subjectInfo()
*/

/*!
    \fn QList<QByteArray> X509Certificate::issuerInfoAttributes() const
    \internal

    This function is expected to return a list of the attributes that have
    values in the issuer information of this certificate. The information
    associated with a given attribute can be accessed using the issuerInfo()
    method. Note that this list may include the OIDs for any
    elements that are not known by the TLS backend.

    \note This function implements QSslCertificate::issuerInfoAttributes().

    \sa issuerInfo()
*/

/*!
    \fn QMultiMap<QSsl::AlternativeNameEntryType, QString> X509Certificate::subjectAlternativeNames() const
    \internal

    This function is expected to return the list of alternative subject names for
    this certificate. The alternative names typically contain host names, optionally
    with wildcards, that are valid for this certificate.

    \sa subjectInfo()
*/

/*!
    \fn QDateTime X509Certificate::effectiveDate() const
    \internal

    This function is expected to return the date-time that the certificate
    becomes valid, or an empty QDateTime if this is a null certificate.

    \sa expiryDate()
*/

/*!
    \fn QDateTime X509Certificate::expiryDate() const
    \internal

    This function is expected to return the date-time that the certificate expires,
    or an empty QDateTime if this is a null certificate.

    \sa effectiveDate()
*/

/*!
    \fn qsizetype X509Certificate::numberOfExtensions() const
    \internal

    This function is expected to return the number of X509 extensions of
    this certificate.
*/

/*!
    \fn QString X509Certificate::oidForExtension(qsizetype i) const
    \internal

    This function is expected to return the ASN.1 OID for the extension
    with index \a i.

    \sa numberOfExtensions()
*/

/*!
    \fn QString X509Certificate::nameForExtension(qsizetype i) const
    \internal

    This function is expected to return the name for the extension
    with index \a i. If no name is known for the extension then the
    OID will be returned.

    \sa numberOfExtensions(), oidForExtension()
*/

/*!
    \fn QVariant X509Certificate::valueForExtension(qsizetype i) const
    \internal

    This function is expected to return the value of the extension
    with index \a i. The structure of the value returned depends on
    the extension type

    \sa numberOfExtensions()
*/

/*!
    \fn bool X509Certificate::isExtensionCritical(qsizetype i) const
    \internal

    This function is expected to return the criticality of the extension
    with index \a i.

    \sa numberOfExtensions()
*/

/*!
    \fn bool X509Certificate::isExtensionSupported(qsizetype i) const
    \internal

    This function is expected to return \c true if this extension is supported.
    In this case, supported simply means that the structure of the QVariant returned
    by the valueForExtension() accessor will remain unchanged between versions.

    \sa numberOfExtensions()
*/

/*!
    \fn QByteArray X509Certificate::toPem() const
    \internal

    This function is expected to return this certificate converted to a PEM (Base64)
    encoded representation.
*/

/*!
    \fn QByteArray X509Certificate::toDer() const
    \internal

    This function is expected to return this certificate converted to a DER (binary)
    encoded representation.
*/

/*!
    \fn QString X509Certificate::toText() const
    \internal

    This function is expected to return this certificate converted to a human-readable
    text representation.
*/

/*!
    \fn Qt::HANDLE X509Certificate::handle() const
    \internal

    This function is expected to return a pointer to the native certificate handle,
    if there is one, else nullptr.
*/

/*!
    \fn size_t X509Certificate::hash(size_t seed) const
    \internal

    This function is expected to return the hash value for this certificate,
    using \a seed to seed the calculation.
*/

/*!
    \internal

    Destroys this certificate.
*/
X509Certificate::~X509Certificate() = default;

/*!
    \internal

    Returns the certificate subject's public key.
*/
TlsKey *X509Certificate::publicKey() const
{
    return nullptr;
}

#if QT_CONFIG(ssl)

/*!
    \class TlsCryptograph
    \internal (Network-private)
    \brief TlsCryptograph is an abstract class, that allows a TLS plugin to implement QSslSocket.

    This abstract base class provides an interface that must be reimplemented by a TLS plugin,
    that supports QSslSocket. A class, implementing TlsCryptograph's interface, is responsible
    for TLS handshake, reading and writing encryped application data; it is expected
    to work with QSslSocket and it's private implementation - QSslSocketPrivate.
    QSslSocketPrivate provides access to its read/write buffers, QTcpSocket it
    internally uses for connecting, reading and writing. QSslSocketPrivate
    can also be used for reporting errors and storing the certificates received
    during the handshake phase.

    \note Most of the functions in this class are pure virtual and have no actual implementation
    in the QtNetwork module. This documentation is mostly conceptual and only describes what those
    functions are expected to do, but not how they must be implemented.

    \sa QTlsBackend::createTlsCryptograph()
*/

/*!
    \fn void TlsCryptograph::init(QSslSocket *q, QSslSocketPrivate *d)
    \internal

    When initializing this TlsCryptograph, QSslSocket will pass a pointer to self and
    its d-object using this function.
*/

/*!
    \fn QList<QSslError> TlsCryptograph::tlsErrors() const
    \internal

    Returns a list of QSslError, describing errors encountered during
    the TLS handshake.

    \sa QSslSocket::sslHandshakeErrors()
*/

/*!
    \fn void TlsCryptograph::startClientEncryption()
    \internal

    A client-side QSslSocket calls this function after its internal TCP socket
    establishes a connection with a remote host, or from QSslSocket::startClientEncryption().
    This TlsCryptograph is expected to initialize some implementation-specific TLS context,
    if needed, and then start the client side of the TLS handshake (for example, by calling
    transmit()), using TCP socket from QSslSocketPrivate.

    \sa init(), transmit(), QSslSocket::startClientEncryption(), QSslSocket::connectToHostEncrypted()
*/

/*!
    \fn void TlsCryptograph::startServerEncryption()
    \internal

    This function is called by QSslSocket::startServerEncryption(). The TlsCryptograph
    is expected to initialize some implementation-specific TLS context, if needed,
    and then try to read the ClientHello message and continue the TLS handshake
    (for example, by calling transmit()).

    \sa transmit(), QSslSocket::startServerEncryption()
*/

/*!
    \fn void TlsCryptograph::continueHandshake()
    \internal

    QSslSocket::resume() calls this function if its pause mode is QAbstractSocket::PauseOnSslErrors,
    and errors, found during the handshake, were ignored. If implemented, this function is expected
    to emit QSslSocket::encrypted().

    \sa QAbstractSocket::pauseMode(), QSslSocket::sslHandshakeErrors(), QSslSocket::ignoreSslErrors(), QSslSocket::resume()
*/

/*!
    \fn void TlsCryptograph::disconnectFromHost()
    \internal

    This function is expected to call disconnectFromHost() on the TCP socket
    that can be obtained from QSslSocketPrivate. Any additional actions
    are implementation-specific (e.g., sending shutdown alert message).

*/

/*!
    \fn void TlsCryptograph::disconnected()
    \internal

    This function is called when the remote has disconnected. If there
    is data left to be read you may ignore the maxReadBufferSize restriction
    and read it all now.
*/

/*!
    \fn QSslCipher TlsCryptograph::sessionCipher() const
    \internal

    This function returns a QSslCipher object describing the ciphersuite negotiated
    during the handshake.
*/

/*!
    \fn QSsl::SslProtocol TlsCryptograph::sessionProtocol() const
    \internal

    This function returns the version of TLS (or DTLS) protocol negotiated during the handshake.
*/

/*!
    \fn void TlsCryptograph::transmit()
    \internal

    This function is responsible for reading and writing data. The meaning of these I/O
    operations depends on an implementation-specific TLS state machine. These read and write
    operations can be reading and writing parts of a TLS handshake (e.g. by calling handshake-specific
    functions), or reading and writing application data (if encrypted connection was already
    established). transmit() is expected to use the QSslSocket's TCP socket (accessible via
    QSslSocketPrivate) to read the incoming data and write the outgoing data. When in encrypted
    state, transmit() is also using QSslSocket's internal read and write buffers: the read buffer
    to fill with decrypted incoming data; the write buffer - for the data to encrypt and send.
    This TlsCryptograph can also use QSslSocketPrivate to check which TLS errors were ignored during
    the handshake.

    \note This function is responsible for emitting QSslSocket's signals, that occur during the
    handshake (e.g. QSslSocket::sslErrors() or QSslSocket::encrypted()), and also read/write signals,
    e.g. QSslSocket::bytesWritten() and QSslSocket::readyRead().

    \sa init()
*/

/*!
    \internal

    Destroys this object.
*/
TlsCryptograph::~TlsCryptograph() = default;

/*!
    \internal

    This function allows to share QSslContext between several QSslSocket objects.
    The default implementation does nothing.

    \note The definition of the class QSslContext is implementation-specific.

    \sa sslContext()
*/
void TlsCryptograph::checkSettingSslContext(std::shared_ptr<QSslContext> tlsContext)
{
    Q_UNUSED(tlsContext);
}

/*!
    \internal

    Returns the context previously set by checkSettingSslContext() or \nullptr,
    if no context was set. The default implementation returns \nullptr.

    \sa checkSettingSslContext()
*/
std::shared_ptr<QSslContext> TlsCryptograph::sslContext() const
{
    return {};
}

/*!
    \internal

    If this TLS backend supports reporting errors before handshake is finished,
    e.g. from a verification callback function, enableHandshakeContinuation()
    allows this object to continue handshake. The default implementation does
    nothing.

    \sa QSslSocket::handshakeInterruptedOnError(), QSslConfiguration::setHandshakeMustInterruptOnError()
*/
void TlsCryptograph::enableHandshakeContinuation()
{
}

/*!
    \internal

    Windows and OpenSSL-specific, only used internally by Qt's OpenSSL TLS backend.

    \note The default empty implementation is sufficient.
*/
void TlsCryptograph::cancelCAFetch()
{
}

/*!
    \internal

    Windows and Schannel-specific, only used by Qt's Schannel TLS backend, in
    general, if a backend has its own buffer where it stores undecrypted data
    then it must report true if it contains any data through this function.

    \note The default empty implementation, returning \c false is sufficient.
*/
bool TlsCryptograph::hasUndecryptedData() const
{
    return false;
}

/*!
    \internal

    Returns the list of OCSP (Online Certificate Status Protocol) responses,
    received during the handshake. The default implementation returns an empty
    list.
*/
QList<QOcspResponse> TlsCryptograph::ocsps() const
{
    return {};
}

/*!
    \internal

    A helper function that can be used during a handshake. Returns \c true if the \a peerName
    matches one of subject alternative names or common names found in the \a certificate.
*/
bool TlsCryptograph::isMatchingHostname(const QSslCertificate &certificate, const QString &peerName)
{
    return QSslSocketPrivate::isMatchingHostname(certificate, peerName);
}

/*!
    \internal
    Calls QAbstractSocketPrivate::setErrorAndEmit() for \a d, passing \a errorCode and
    \a errorDescription as parameters.
*/
void TlsCryptograph::setErrorAndEmit(QSslSocketPrivate *d, QAbstractSocket::SocketError errorCode,
                                     const QString &errorDescription) const
{
    Q_ASSERT(d);
    d->setErrorAndEmit(errorCode, errorDescription);
}

#if QT_CONFIG(dtls)
/*!
    \class DtlsBase
    \internal (Network-private)
    \brief DtlsBase is a base class for the classes DtlsCryptograph and DtlsCookieVerifier.

    DtlsBase is the base class for the classes DtlsCryptograph and DtlsCookieVerifier. It's
    an abstract class, an interface that these before-mentioned classes share. It allows to
    set, get and clear the last error that occurred, set and get cookie generation parameters,
    set and get QSslConfiguration.

    \note This class is not supposed to be inherited directly, it's only needed by DtlsCryptograph
    and DtlsCookieVerifier.

    \sa QDtls, QDtlsClientVerifier, DtlsCryptograph, DtlsCookieVerifier
*/

/*!
    \fn void DtlsBase::setDtlsError(QDtlsError code, const QString &description)
    \internal

    Sets the last error to \a code and its textual description to \a description.

    \sa QDtlsError, error(), errorString()
*/

/*!
    \fn QDtlsError DtlsBase::error() const
    \internal

    This function, when overridden, is expected to return the code for the last error that occurred.
    If no error occurred it should return QDtlsError::NoError.

    \sa QDtlsError, errorString(), setDtlsError()
*/

/*!
    \fn QDtlsError DtlsBase::errorString() const
    \internal

    This function, when overridden, is expected to return the textual description for the last error
    that occurred or an empty string if no error occurred.

    \sa QDtlsError, error(), setDtlsError()
*/

/*!
    \fn void DtlsBase::clearDtlsError()
    \internal

    This function is expected to set the error code for the last error to QDtlsError::NoError and
    its textual description to an empty string.

    \sa QDtlsError, setDtlsError(), error(), errorString()
*/

/*!
    \fn void DtlsBase::setConfiguration(const QSslConfiguration &configuration)
    \internal

    Sets a TLS configuration that an object of a class inheriting from DtlsCookieVerifier or
    DtlsCryptograph will use, to \a configuration.

    \sa configuration()
*/

/*!
    \fn QSslConfiguration DtlsBase::configuration() const
    \internal

    Returns TLS configuration this object is using (either set by setConfiguration()
    previously, or the default DTLS configuration).

    \sa setConfiguration(), QSslConfiguration::defaultDtlsConfiguration()
*/

/*!
    \fn bool DtlsBase::setCookieGeneratorParameters(const QDtlsClientVerifier::GeneratorParameters &params)
    \internal

    Sets the DTLS cookie generation parameters that DtlsCookieVerifier or DtlsCryptograph will use to
    \a params.

    \note This function returns \c false if parameters were invalid - if the secret was empty. Otherwise,
    this function must return true.

    \sa QDtlsClientVerifier::GeneratorParameters, cookieGeneratorParameters()
*/

/*!
    \fn QDtlsClientVerifier::GeneratorParameters DtlsBase::cookieGeneratorParameters() const
    \internal

    Returns DTLS cookie generation parameters that were either previously set by setCookieGeneratorParameters(),
    or default parameters.

    \sa setCookieGeneratorParameters()
*/

/*!
    \internal

    Destroys this object.
*/
DtlsBase::~DtlsBase() = default;

/*!
    \class DtlsCookieVerifier
    \internal (Network-private)
    \brief DtlsCookieVerifier is an interface that allows a TLS plugin to support the class QDtlsClientVerifier.

    DtlsCookieVerifier is an interface, an abstract class, that has to be implemented by
    a TLS plugin that supports DTLS cookie verification.

    \sa QDtlsClientVerifier
*/

/*!
    \fn bool DtlsCookieVerifier::verifyClient(QUdpSocket *socket, const QByteArray &dgram, const QHostAddress &address, quint16 port)
    \internal

    This function is expected to verify a ClientHello message, found in \a dgram, using \a address,
    \a port, and cookie generator parameters. The function returns \c true if such cookie was found
    and \c false otherwise. If no valid cookie was found in the \a dgram, this verifier should use
    \a socket to send a HelloVerifyRequest message, using \a address and \a port as the destination
    and a source material for cookie generation, see also
    \l {RFC 6347, section 4.2.1}

    \sa QDtlsClientVerifier
*/

/*!
    \fn QByteArray DtlsCookieVerifier::verifiedHello() const
    \internal

    Returns the last ClientHello message containing the DTLS cookie that this verifier was
    able to verify as correct, or an empty byte array.

    \sa verifyClient()
*/

/*!
    \class DtlsCryptograph
    \internal (Network-private)
    \brief DtlsCryptograph is an interface that allows a TLS plugin to implement the class QDtls.

    DtlsCryptograph is an abstract class; a TLS plugin can provide a class, inheriting from
    DtlsCryptograph and implementing its pure virtual functions, thus implementing the class
    QDtls and enabling DTLS over UDP.

    To write DTLS datagrams, a class, inheriting DtlsCryptograph, is expected to use
    QUdpSocket. In general, all reading is done externally, so DtlsCryptograph is
    expected to only write into QUdpSocket, check possible socket errors, change socket
    options if needed.

    \note All functions in this class are pure virtual and have no actual implementation
    in the QtNetwork module. This documentation is mostly conceptual and only describes
    what those functions are expected to do, but not how they must be implemented.

    \sa QDtls, QUdpSocket
*/

/*!
    \fn QSslSocket::SslMode DtlsCryptograph::cryptographMode() const
    \internal

    Returns the mode (client or server) this object operates in.

    \note This mode is set once when a new DtlsCryptograph is created
    by QTlsBackend and cannot change.

    \sa QTlsBackend::createDtlsCryptograph()
*/

/*!
    \fn void DtlsCryptograph::setPeer(const QHostAddress &addr, quint16 port, const QString &name)
    \internal

    Sets the remote peer's address to \a addr and remote port to \a port. \a name,
    if not empty, is to be used when validating the peer's certificate.

    \sa peerAddress(), peerPort(), peerVerificationName()
*/

/*!
    \fn QHostAddress DtlsCryptograph::peerAddress() const
    \internal

    Returns the remote peer's address previously set by setPeer() or,
    if no address was set, an empty address.

    \sa setPeer()
*/

/*!
    \fn quint16 DtlsCryptograph::peerPort() const
    \internal

    Returns the remote peer's port previously set by setPeer() or
    0 if no port was set.

    \sa setPeer(), peerAddress()
*/

/*!
    \fn void DtlsCryptograph::setPeerVerificationName(const QString &name)
    \internal

    Sets the host name to use during certificate validation to \a name.

    \sa peerVerificationName(), setPeer()
*/

/*!
    \fn QString DtlsCryptograph::peerVerificationName() const
    \internal

    Returns the name that this object is using during the certificate validation,
    previously set by setPeer() or setPeerVerificationName(). Returns an empty string
    if no peer verification name was set.

    \sa setPeer(), setPeerVerificationName()
*/

/*!
    \fn void DtlsCryptograph::setDtlsMtuHint(quint16 mtu)
    \internal

    Sets the maximum transmission unit (MTU), if it is supported by a TLS implementation, to \a mtu.

    \sa dtlsMtuHint()
*/

/*!
    \fn quint16 DtlsCryptograph::dtlsMtuHint() const
    \internal

    Returns the value of the maximum transmission unit either previously set by setDtlsMtuHint(),
    or some implementation-specific value (guessed or somehow known to this DtlsCryptograph).

    \sa setDtlsMtuHint()
*/

/*!
    \fn QDtls::HandshakeState DtlsCryptograph::state() const
    \internal

    Returns the current handshake state for this DtlsCryptograph (not started, in progress,
    peer verification error found, complete).

    \sa isConnectionEncrypted(), startHandshake()
*/

/*!
    \fn bool DtlsCryptograph::isConnectionEncrypted() const
    \internal

    Returns \c true if this DtlsCryptograph has completed a handshake without validation
    errors (or these errors were ignored). Returns \c false otherwise.
*/

/*!
    \fn bool DtlsCryptograph::startHandshake(QUdpSocket *socket, const QByteArray &dgram)
    \internal

    This function is expected to initialize some implementation-specific context and to start a DTLS
    handshake, using \a socket to write datagrams (but not to read them). If this object is operating
    as a server, \a dgram is non-empty and contains the ClientHello message. This function returns
    \c true if no error occurred (and this DtlsCryptograph's state switching to
    QDtls::HandshakeState::HandshakeInProgress), \c false otherwise.

    \sa continueHandshake(), handleTimeout(), resumeHandshake(), abortHandshake(), state()
*/

/*!
    \fn bool DtlsCryptograph::handleTimeout(QUdpSocket *socket)
    \internal

    In case a timeout occurred during the handshake, allows to re-transmit the last message,
    using \a socket to write the datagram. Returns \c true if no error occurred, \c false otherwise.

    \sa QDtls::handshakeTimeout(), QDtls::handleTimeout()
*/

/*!
    \fn bool DtlsCryptograph::continueHandshake(QUdpSocket *socket, const QByteArray &dgram)
    \internal

    Continues the handshake, using \a socket to write datagrams (a handshake-specific message).
    \a dgram contains the peer's handshake-specific message. Returns \c false in case some error
    was encountered (this can include socket-related errors and errors found during the certificate
    validation). Returns \c true if the handshake was complete successfully, or is still in progress.

    This function, depending on the implementation-specific state machine, may leave the handshake
    state in QDtls::HandshakeState::HandshakeInProgress, or switch to QDtls::HandshakeState::HandshakeComplete
    or QDtls::HandshakeState::PeerVerificationFailed.

    This function may store the peer's certificate (or chain of certificates), extract and store
    the information about the negotiated session protocol and ciphersuite.

    \sa startHandshake()
*/

/*!
    \fn bool DtlsCryptograph::resumeHandshake(QUdpSocket *socket)
    \internal

    If peer validation errors were found duing the handshake, this function tries to
    continue and complete the handshake. If errors were ignored, the function switches
    this object's state to QDtls::HandshakeState::HandshakeComplete and returns \c true.

    \sa abortHandshake()
*/

/*!
    \fn void DtlsCryptograph::abortHandshake(QUdpSocket *socket)
    \internal

    Aborts the handshake if it's in progress or in the state QDtls::HandshakeState::PeerVerificationFailed.
    The use of \a socket is implementation-specific (for example, this DtlsCryptograph may send
    ShutdownAlert message).

    \sa resumeHandshake()
*/

/*!
    \fn void DtlsCryptograph::sendShutdownAlert(QUdpSocket *socket)
    \internal

    If the underlying TLS library provides the required functionality, this function
    may sent ShutdownAlert message using \a socket.
*/

/*!
    \fn QList<QSslError> DtlsCryptograph::peerVerificationErrors() const
    \internal

    Returns the list of errors that this object encountered during DTLS handshake
    and certificate validation.

    \sa ignoreVerificationErrors()
*/

/*!
    \fn void DtlsCryptograph::ignoreVerificationErrors(const QList<QSslError> &errorsToIgnore)
    \internal

    Tells this object to ignore errors from \a errorsToIgnore when they are found during
    DTLS handshake.

    \sa peerVerificationErrors()
*/

/*!
    \fn QSslCipher DtlsCryptograph::dtlsSessionCipher() const
    \internal

    If such information is available, returns the ciphersuite, negotiated during
    the handshake.

    \sa continueHandshake(), dtlsSessionProtocol()
*/

/*!
    \fn QSsl::SslProtocol DtlsCryptograph::dtlsSessionProtocol() const
    \internal

    Returns the version of the session protocol that was negotiated during the handshake or
    QSsl::UnknownProtocol if the handshake is incomplete or no information about the session
    protocol is available.

    \sa continueHandshake(), dtlsSessionCipher()
*/

/*!
    \fn qint64 DtlsCryptograph::writeDatagramEncrypted(QUdpSocket *socket, const QByteArray &dgram)
    \internal

    If this DtlsCryptograph is in the QDtls::HandshakeState::HandshakeComplete state, this function
    encrypts \a dgram and writes this encrypted data into \a socket.

    Returns the number of bytes (of \a dgram) written, or -1 in case of error. This function should
    set the error code and description if some error was encountered.

    \sa decryptDatagram()
*/

/*!
    \fn QByteArray DtlsCryptograph::decryptDatagram(QUdpSocket *socket, const QByteArray &dgram)
    \internal

    If this DtlsCryptograph is in the QDtls::HandshakeState::HandshakeComplete state, decrypts \a dgram.
    The use of \a socket is implementation-specific. This function should return an empty byte array
    and set the error code and description if some error was encountered.
*/

#endif // QT_CONFIG(dtls)
#endif // QT_CONFIG(ssl)

} // namespace QTlsPrivate

#if QT_CONFIG(ssl)
/*!
    \internal
*/
Q_NETWORK_EXPORT void qt_ForceTlsSecurityLevel()
{
    if (auto *backend = QSslSocketPrivate::tlsBackendInUse())
        backend->forceAutotestSecurityLevel();
}

#endif // QT_CONFIG(ssl)

QT_END_NAMESPACE

#include "moc_qtlsbackend_p.cpp"
