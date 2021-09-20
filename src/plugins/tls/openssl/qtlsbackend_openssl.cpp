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

#include "qsslsocket_openssl_symbols_p.h"
#include "qtlsbackend_openssl_p.h"
#include "qtlskey_openssl_p.h"
#include "qx509_openssl_p.h"
#include "qtls_openssl_p.h"

#if QT_CONFIG(dtls)
#include "qdtls_openssl_p.h"
#endif // QT_CONFIG(dtls)

#include <QtNetwork/private/qsslcipher_p.h>

#include <QtNetwork/qsslcipher.h>
#include <QtNetwork/qssl.h>

#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qscopeguard.h>

#include "qopenssl_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTlsBackend, "qt.tlsbackend.ossl");

Q_GLOBAL_STATIC(QRecursiveMutex, qt_opensslInitMutex)

static void q_loadCiphersForConnection(SSL *connection, QList<QSslCipher> &ciphers,
                                       QList<QSslCipher> &defaultCiphers)
{
    Q_ASSERT(connection);

    STACK_OF(SSL_CIPHER) *supportedCiphers = q_SSL_get_ciphers(connection);
    for (int i = 0; i < q_sk_SSL_CIPHER_num(supportedCiphers); ++i) {
        if (SSL_CIPHER *cipher = q_sk_SSL_CIPHER_value(supportedCiphers, i)) {
            const auto ciph = QTlsBackendOpenSSL::qt_OpenSSL_cipher_to_QSslCipher(cipher);
            if (!ciph.isNull()) {
                // Unconditionally exclude ADH and AECDH ciphers since they offer no MITM protection
                if (!ciph.name().toLower().startsWith(QLatin1String("adh")) &&
                    !ciph.name().toLower().startsWith(QLatin1String("exp-adh")) &&
                    !ciph.name().toLower().startsWith(QLatin1String("aecdh"))) {
                    ciphers << ciph;

                    if (ciph.usedBits() >= 128)
                        defaultCiphers << ciph;
                }
            }
        }
    }
}

bool QTlsBackendOpenSSL::s_libraryLoaded = false;
bool QTlsBackendOpenSSL::s_loadedCiphersAndCerts = false;
int QTlsBackendOpenSSL::s_indexForSSLExtraData = -1;

QString QTlsBackendOpenSSL::getErrorsFromOpenSsl()
{
    QString errorString;
    char buf[256] = {}; // OpenSSL docs claim both 120 and 256; use the larger.
    unsigned long errNum;
    while ((errNum = q_ERR_get_error())) {
        if (!errorString.isEmpty())
            errorString.append(QLatin1String(", "));
        q_ERR_error_string_n(errNum, buf, sizeof buf);
        errorString.append(QString::fromLatin1(buf)); // error is ascii according to man ERR_error_string
    }
    return errorString;
}

void QTlsBackendOpenSSL::logAndClearErrorQueue()
{
    const auto errors = getErrorsFromOpenSsl();
    if (errors.size())
        qCWarning(lcTlsBackend) << "Discarding errors:" << errors;
}

void QTlsBackendOpenSSL::clearErrorQueue()
{
    const auto errs = getErrorsFromOpenSsl();
    Q_UNUSED(errs);
}

bool QTlsBackendOpenSSL::ensureLibraryLoaded()
{
    if (!q_resolveOpenSslSymbols())
        return false;

    const QMutexLocker locker(qt_opensslInitMutex());

    if (!s_libraryLoaded) {
        // Initialize OpenSSL.
        if (q_OPENSSL_init_ssl(0, nullptr) != 1)
            return false;

        if (q_OpenSSL_version_num() < 0x10101000L) {
            qCWarning(lcTlsBackend, "QSslSocket: OpenSSL >= 1.1.1 is required; %s was found instead", q_OpenSSL_version(OPENSSL_VERSION));
            return false;
        }

        q_SSL_load_error_strings();
        q_OpenSSL_add_all_algorithms();

        s_indexForSSLExtraData = q_CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_SSL, 0L, nullptr, nullptr,
                                                           nullptr, nullptr);

        // Initialize OpenSSL's random seed.
        if (!q_RAND_status()) {
            qWarning("Random number generator not seeded, disabling SSL support");
            return false;
        }

        s_libraryLoaded = true;
    }

    return true;
}

QString QTlsBackendOpenSSL::backendName() const
{
    return builtinBackendNames[nameIndexOpenSSL];
}

bool QTlsBackendOpenSSL::isValid() const
{
    return ensureLibraryLoaded();
}

long QTlsBackendOpenSSL::tlsLibraryVersionNumber() const
{
    return q_OpenSSL_version_num();
}

QString QTlsBackendOpenSSL::tlsLibraryVersionString() const
{
    const char *versionString = q_OpenSSL_version(OPENSSL_VERSION);
    if (!versionString)
        return QString();

    return QString::fromLatin1(versionString);
}

long QTlsBackendOpenSSL::tlsLibraryBuildVersionNumber() const
{
    return OPENSSL_VERSION_NUMBER;
}

QString QTlsBackendOpenSSL::tlsLibraryBuildVersionString() const
{
    // Using QStringLiteral to store the version string as unicode and
    // avoid false positives from Google searching the playstore for old
    // SSL versions. See QTBUG-46265
    return QStringLiteral(OPENSSL_VERSION_TEXT);
}

void QTlsBackendOpenSSL::ensureInitialized() const
{
    // Old qsslsocket_openssl calls supportsSsl() (which means
    // library found and symbols resolved, this already assured
    // by the fact we end up in this function (isValid() returned
    // true for the backend, see its code). The qsslsocket_openssl
    // proceedes with loading certificate, ciphers and elliptic
    // curves.
    ensureCiphersAndCertsLoaded();
}

void QTlsBackendOpenSSL::ensureCiphersAndCertsLoaded() const
{
    const QMutexLocker locker(qt_opensslInitMutex());

    if (s_loadedCiphersAndCerts)
        return;
    s_loadedCiphersAndCerts = true;

    resetDefaultCiphers();
    resetDefaultEllipticCurves();

#if QT_CONFIG(library)
    //load symbols needed to receive certificates from system store
#if defined(Q_OS_QNX)
    QSslSocketPrivate::setRootCertOnDemandLoadingSupported(true);
#elif defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    // check whether we can enable on-demand root-cert loading (i.e. check whether the sym links are there)
    QList<QByteArray> dirs = QSslSocketPrivate::unixRootCertDirectories();
    QStringList symLinkFilter;
    symLinkFilter << QLatin1String("[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f].[0-9]");
    for (int a = 0; a < dirs.count(); ++a) {
        QDirIterator iterator(QLatin1String(dirs.at(a)), symLinkFilter, QDir::Files);
        if (iterator.hasNext()) {
            QSslSocketPrivate::setRootCertOnDemandLoadingSupported(true);
            break;
        }
    }
#endif
#endif // QT_CONFIG(library)
    // if on-demand loading was not enabled, load the certs now
    if (!QSslSocketPrivate::rootCertOnDemandLoadingSupported())
        setDefaultCaCertificates(systemCaCertificates());
#ifdef Q_OS_WIN
    //Enabled for fetching additional root certs from windows update on windows.
    //This flag is set false by setDefaultCaCertificates() indicating the app uses
    //its own cert bundle rather than the system one.
    //Same logic that disables the unix on demand cert loading.
    //Unlike unix, we do preload the certificates from the cert store.
    QSslSocketPrivate::setRootCertOnDemandLoadingSupported(true);
#endif
}

void QTlsBackendOpenSSL::resetDefaultCiphers()
{
    SSL_CTX *myCtx = q_SSL_CTX_new(q_TLS_client_method());
    // Note, we assert, not just silently return/bail out early:
    // this should never happen and problems with OpenSSL's initialization
    // must be caught before this (see supportsSsl()).
    Q_ASSERT(myCtx);
    SSL *mySsl = q_SSL_new(myCtx);
    Q_ASSERT(mySsl);

    QList<QSslCipher> ciphers;
    QList<QSslCipher> defaultCiphers;

    q_loadCiphersForConnection(mySsl, ciphers, defaultCiphers);

    q_SSL_CTX_free(myCtx);
    q_SSL_free(mySsl);

    setDefaultSupportedCiphers(ciphers);
    setDefaultCiphers(defaultCiphers);

#if QT_CONFIG(dtls)
    ciphers.clear();
    defaultCiphers.clear();
    myCtx = q_SSL_CTX_new(q_DTLS_client_method());
    if (myCtx) {
        mySsl = q_SSL_new(myCtx);
        if (mySsl) {
            q_loadCiphersForConnection(mySsl, ciphers, defaultCiphers);
            setDefaultDtlsCiphers(defaultCiphers);
            q_SSL_free(mySsl);
        }
        q_SSL_CTX_free(myCtx);
    }
#endif // dtls
}

QList<QSsl::SslProtocol> QTlsBackendOpenSSL::supportedProtocols() const
{
    QList<QSsl::SslProtocol> protocols;

    protocols << QSsl::AnyProtocol;
    protocols << QSsl::SecureProtocols;
    protocols << QSsl::TlsV1_0;
    protocols << QSsl::TlsV1_0OrLater;
    protocols << QSsl::TlsV1_1;
    protocols << QSsl::TlsV1_1OrLater;
    protocols << QSsl::TlsV1_2;
    protocols << QSsl::TlsV1_2OrLater;

#ifdef TLS1_3_VERSION
    protocols << QSsl::TlsV1_3;
    protocols << QSsl::TlsV1_3OrLater;
#endif // TLS1_3_VERSION

#if QT_CONFIG(dtls)
    protocols << QSsl::DtlsV1_0;
    protocols << QSsl::DtlsV1_0OrLater;
    protocols << QSsl::DtlsV1_2;
    protocols << QSsl::DtlsV1_2OrLater;
#endif // dtls

    return protocols;
}

QList<QSsl::SupportedFeature> QTlsBackendOpenSSL::supportedFeatures() const
{
    QList<QSsl::SupportedFeature> features;

    features << QSsl::SupportedFeature::CertificateVerification;

#if !defined(OPENSSL_NO_TLSEXT)
    features << QSsl::SupportedFeature::ClientSideAlpn;
    features << QSsl::SupportedFeature::ServerSideAlpn;
#endif // !OPENSSL_NO_TLSEXT

    features << QSsl::SupportedFeature::Ocsp;
    features << QSsl::SupportedFeature::Psk;
    features << QSsl::SupportedFeature::SessionTicket;
    features << QSsl::SupportedFeature::Alerts;

    return features;
}

QList<QSsl::ImplementedClass> QTlsBackendOpenSSL::implementedClasses() const
{
    QList<QSsl::ImplementedClass> classes;

    classes << QSsl::ImplementedClass::Key;
    classes << QSsl::ImplementedClass::Certificate;
    classes << QSsl::ImplementedClass::Socket;
#if QT_CONFIG(dtls)
    classes << QSsl::ImplementedClass::Dtls;
    classes << QSsl::ImplementedClass::DtlsCookie;
#endif
    classes << QSsl::ImplementedClass::EllipticCurve;
    classes << QSsl::ImplementedClass::DiffieHellman;

    return classes;
}

QTlsPrivate::TlsKey *QTlsBackendOpenSSL::createKey() const
{
    return new QTlsPrivate::TlsKeyOpenSSL;
}

QTlsPrivate::X509Certificate *QTlsBackendOpenSSL::createCertificate() const
{
    return new QTlsPrivate::X509CertificateOpenSSL;
}

namespace QTlsPrivate {

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
QList<QByteArray> fetchSslCertificateData();
#endif

QList<QSslCertificate> systemCaCertificates();

#ifndef Q_OS_DARWIN
QList<QSslCertificate> systemCaCertificates()
{
#ifdef QSSLSOCKET_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    QList<QSslCertificate> systemCerts;
#if defined(Q_OS_WIN)
    HCERTSTORE hSystemStore;
    hSystemStore = CertOpenSystemStoreW(0, L"ROOT");
    if (hSystemStore) {
        PCCERT_CONTEXT pc = nullptr;
        while (1) {
            pc = CertFindCertificateInStore(hSystemStore, X509_ASN_ENCODING, 0, CERT_FIND_ANY, nullptr, pc);
            if (!pc)
                break;
            QByteArray der(reinterpret_cast<const char *>(pc->pbCertEncoded),
                            static_cast<int>(pc->cbCertEncoded));
            QSslCertificate cert(der, QSsl::Der);
            systemCerts.append(cert);
        }
        CertCloseStore(hSystemStore, 0);
    }
#elif defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
    const QList<QByteArray> certData = fetchSslCertificateData();
    for (auto certDatum : certData)
        systemCerts.append(QSslCertificate::fromData(certDatum, QSsl::Der));
#elif defined(Q_OS_UNIX)
    QSet<QString> certFiles;
    QDir currentDir;
    QStringList nameFilters;
    QList<QByteArray> directories;
    QSsl::EncodingFormat platformEncodingFormat;
# ifndef Q_OS_ANDROID
    directories = QSslSocketPrivate::unixRootCertDirectories();
    nameFilters << QLatin1String("*.pem") << QLatin1String("*.crt");
    platformEncodingFormat = QSsl::Pem;
# endif //Q_OS_ANDROID
    {
        currentDir.setNameFilters(nameFilters);
        for (int a = 0; a < directories.count(); a++) {
            currentDir.setPath(QLatin1String(directories.at(a)));
            QDirIterator it(currentDir);
            while (it.hasNext()) {
                it.next();
                // use canonical path here to not load the same certificate twice if symlinked
                certFiles.insert(it.fileInfo().canonicalFilePath());
            }
        }
        for (const QString& file : qAsConst(certFiles))
            systemCerts.append(QSslCertificate::fromPath(file, platformEncodingFormat));
# ifndef Q_OS_ANDROID
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/etc/pki/tls/certs/ca-bundle.crt"), QSsl::Pem)); // Fedora, Mandriva
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/usr/local/share/certs/ca-root-nss.crt"), QSsl::Pem)); // FreeBSD's ca_root_nss
# endif
    }
#endif
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcTlsBackend) << "systemCaCertificates retrieval time " << timer.elapsed() << "ms";
    qCDebug(lcTlsBackend) << "imported " << systemCerts.count() << " certificates";
#endif

    return systemCerts;
}
#endif // !Q_OS_DARWIN
} // namespace QTlsPrivate

QList<QSslCertificate> QTlsBackendOpenSSL::systemCaCertificates() const
{
    return QTlsPrivate::systemCaCertificates();
}

QTlsPrivate::DtlsCookieVerifier *QTlsBackendOpenSSL::createDtlsCookieVerifier() const
{
#if QT_CONFIG(dtls)
    return new QDtlsClientVerifierOpenSSL;
#else
    qCWarning(lcTlsBackend, "Feature 'dtls' is disabled, cannot verify DTLS cookies");
    return nullptr;
#endif // QT_CONFIG(dtls)
}

QTlsPrivate::TlsCryptograph *QTlsBackendOpenSSL::createTlsCryptograph() const
{
    return new QTlsPrivate::TlsCryptographOpenSSL;
}

QTlsPrivate::DtlsCryptograph *QTlsBackendOpenSSL::createDtlsCryptograph(QDtls *q, int mode) const
{
#if QT_CONFIG(dtls)
    return new QDtlsPrivateOpenSSL(q, QSslSocket::SslMode(mode));
#else
    Q_UNUSED(q);
    Q_UNUSED(mode);
    qCWarning(lcTlsBackend, "Feature 'dtls' is disabled, cannot encrypt UDP datagrams");
    return nullptr;
#endif // QT_CONFIG(dtls)
}

QTlsPrivate::X509ChainVerifyPtr QTlsBackendOpenSSL::X509Verifier() const
{
    return QTlsPrivate::X509CertificateOpenSSL::verify;
}

QTlsPrivate::X509PemReaderPtr QTlsBackendOpenSSL::X509PemReader() const
{
    return QTlsPrivate::X509CertificateOpenSSL::certificatesFromPem;
}

QTlsPrivate::X509DerReaderPtr QTlsBackendOpenSSL::X509DerReader() const
{
    return QTlsPrivate::X509CertificateOpenSSL::certificatesFromDer;
}

QTlsPrivate::X509Pkcs12ReaderPtr QTlsBackendOpenSSL::X509Pkcs12Reader() const
{
    return QTlsPrivate::X509CertificateOpenSSL::importPkcs12;
}

QList<int> QTlsBackendOpenSSL::ellipticCurvesIds() const
{
    QList<int> ids;

#ifndef OPENSSL_NO_EC
    const size_t curveCount = q_EC_get_builtin_curves(nullptr, 0);
    QVarLengthArray<EC_builtin_curve> builtinCurves(static_cast<int>(curveCount));

    if (q_EC_get_builtin_curves(builtinCurves.data(), curveCount) == curveCount) {
        ids.reserve(curveCount);
        for (const auto &ec : builtinCurves)
            ids.push_back(ec.nid);
    }
#endif // OPENSSL_NO_EC

    return ids;
}

 int QTlsBackendOpenSSL::curveIdFromShortName(const QString &name) const
 {
     int nid = 0;
     if (name.isEmpty())
         return nid;

     ensureInitialized(); // TLSTODO: check if it's needed!
#ifndef OPENSSL_NO_EC
     const QByteArray curveNameLatin1 = name.toLatin1();
     nid = q_OBJ_sn2nid(curveNameLatin1.data());

     if (nid == 0)
         nid = q_EC_curve_nist2nid(curveNameLatin1.data());
#endif // !OPENSSL_NO_EC

     return nid;
 }

 int QTlsBackendOpenSSL::curveIdFromLongName(const QString &name) const
 {
     int nid = 0;
     if (name.isEmpty())
         return nid;

     ensureInitialized();

#ifndef OPENSSL_NO_EC
     const QByteArray curveNameLatin1 = name.toLatin1();
     nid = q_OBJ_ln2nid(curveNameLatin1.data());
#endif

     return nid;
 }

 QString QTlsBackendOpenSSL::shortNameForId(int id) const
 {
     QString result;

#ifndef OPENSSL_NO_EC
     if (id != 0)
         result = QString::fromLatin1(q_OBJ_nid2sn(id));
#endif

     return result;
 }

QString QTlsBackendOpenSSL::longNameForId(int id) const
{
    QString result;

#ifndef OPENSSL_NO_EC
    if (id != 0)
        result = QString::fromLatin1(q_OBJ_nid2ln(id));
#endif

    return result;
}

// NIDs of named curves allowed in TLS as per RFCs 4492 and 7027,
// see also https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-8
static const int tlsNamedCurveNIDs[] = {
    // RFC 4492
    NID_sect163k1,
    NID_sect163r1,
    NID_sect163r2,
    NID_sect193r1,
    NID_sect193r2,
    NID_sect233k1,
    NID_sect233r1,
    NID_sect239k1,
    NID_sect283k1,
    NID_sect283r1,
    NID_sect409k1,
    NID_sect409r1,
    NID_sect571k1,
    NID_sect571r1,

    NID_secp160k1,
    NID_secp160r1,
    NID_secp160r2,
    NID_secp192k1,
    NID_X9_62_prime192v1, // secp192r1
    NID_secp224k1,
    NID_secp224r1,
    NID_secp256k1,
    NID_X9_62_prime256v1, // secp256r1
    NID_secp384r1,
    NID_secp521r1,

    // RFC 7027
    NID_brainpoolP256r1,
    NID_brainpoolP384r1,
    NID_brainpoolP512r1
};

const size_t tlsNamedCurveNIDCount = sizeof(tlsNamedCurveNIDs) / sizeof(tlsNamedCurveNIDs[0]);

bool QTlsBackendOpenSSL::isTlsNamedCurve(int id) const
{
    const int *const tlsNamedCurveNIDsEnd = tlsNamedCurveNIDs + tlsNamedCurveNIDCount;
    return std::find(tlsNamedCurveNIDs, tlsNamedCurveNIDsEnd, id) != tlsNamedCurveNIDsEnd;
}

QString QTlsBackendOpenSSL::msgErrorsDuringHandshake()
{
    return QSslSocket::tr("Error during SSL handshake: %1").arg(getErrorsFromOpenSsl());
}

QSslCipher QTlsBackendOpenSSL::qt_OpenSSL_cipher_to_QSslCipher(const SSL_CIPHER *cipher)
{
    Q_ASSERT(cipher);
    char buf [256] = {};
    const QString desc = QString::fromLatin1(q_SSL_CIPHER_description(cipher, buf, sizeof(buf)));
    int supportedBits = 0;
    const int bits = q_SSL_CIPHER_get_bits(cipher, &supportedBits);
    return createCiphersuite(desc, bits, supportedBits);
}

void QTlsBackendOpenSSL::forceAutotestSecurityLevel()
{
    QSslContext::forceAutoTestSecurityLevel();
}

QT_END_NAMESPACE
