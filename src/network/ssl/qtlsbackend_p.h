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

#ifndef QTLSBACKEND_P_H
#define QTLSBACKEND_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include "qsslconfiguration.h"
#include "qsslerror.h"
#include "qssl_p.h"

#if QT_CONFIG(dtls)
#include "qdtls.h"
#endif

#include <QtNetwork/qsslcertificate.h>
#include <QtNetwork/qsslcipher.h>
#include <QtNetwork/qsslkey.h>
#include <QtNetwork/qssl.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qobject.h>
#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QSslPreSharedKeyAuthenticator;
class QSslSocketPrivate;
class QHostAddress;
class QSslContext;

class QSslSocket;
class QByteArray;
class QSslCipher;
class QUdpSocket;
class QIODevice;
class QSslError;
class QSslKey;

namespace QTlsPrivate {

// The class TlsKey encapsulates key's data (DER) or backend-specific
// data-structure, like RSA/DSA/DH structs in OpenSSL.
// TLSTODO: Interface is mostly what QSslKeyPrivate is now. Names,
// however strange they are, for now preserved to ease the transition
// (this may change in future - for example, 'decodeDer' is not just
// decoding DER, it's initializing a key from DER. Note, QSslKey requires
// a real TLS library because private keys tend to be encrypted.
class Q_NETWORK_PRIVATE_EXPORT TlsKey {
public:
    virtual ~TlsKey();

    using KeyType = QSsl::KeyType;
    using KeyAlgorithm = QSsl::KeyAlgorithm;

    virtual void decodeDer(KeyType type, KeyAlgorithm algorithm, const QByteArray &der,
                           const QByteArray &passPhrase, bool deepClear) = 0;
    virtual void decodePem(KeyType type, KeyAlgorithm algorithm, const QByteArray &pem,
                           const QByteArray &passPhrase, bool deepClear) = 0;

    virtual QByteArray toPem(const QByteArray &passPhrase) const = 0;
    virtual QByteArray derFromPem(const QByteArray &pem, QMap<QByteArray, QByteArray> *headers) const = 0;
    virtual QByteArray pemFromDer(const QByteArray &der, const QMap<QByteArray, QByteArray> &headers) const = 0;

    virtual void fromHandle(Qt::HANDLE opaque, KeyType type) = 0;
    virtual Qt::HANDLE handle() const = 0;

    virtual bool isNull() const = 0;
    virtual KeyType type() const = 0;
    virtual KeyAlgorithm algorithm() const = 0;
    virtual int length() const = 0;

    virtual void clear(bool deepClear) = 0;

    // Needed by QSslKeyPrivate::pemFromDer() for non-OpenSSL backends.
    virtual bool isPkcs8() const = 0;

    virtual QByteArray decrypt(Cipher cipher, const QByteArray &data,
                               const QByteArray &key, const QByteArray &iv) const = 0;
    virtual QByteArray encrypt(Cipher cipher, const QByteArray &data,
                               const QByteArray &key, const QByteArray &iv) const = 0;

    // Those two are non-virtual, always the same and only depend on the key type
    // and algorithm:
    QByteArray pemHeader() const;
    QByteArray pemFooter() const;
};

// An abstraction hiding OpenSSL's X509 or our generic
// 'derData'-based code.
class Q_NETWORK_PRIVATE_EXPORT X509Certificate
{
public:
    virtual ~X509Certificate();

    virtual bool isEqual(const X509Certificate &rhs) const = 0;
    virtual bool isNull() const = 0;
    virtual bool isSelfSigned() const = 0;
    virtual QByteArray version() const = 0;
    virtual QByteArray serialNumber() const = 0;
    virtual QStringList issuerInfo(QSslCertificate::SubjectInfo info) const = 0;
    virtual QStringList issuerInfo(const QByteArray &attribute) const = 0;
    virtual QStringList subjectInfo(QSslCertificate::SubjectInfo info) const = 0;
    virtual QStringList subjectInfo(const QByteArray &attribute) const = 0;

    virtual QList<QByteArray> subjectInfoAttributes() const = 0;
    virtual QList<QByteArray> issuerInfoAttributes() const = 0;
    virtual QMultiMap<QSsl::AlternativeNameEntryType, QString> subjectAlternativeNames() const = 0;
    virtual QDateTime effectiveDate() const = 0;
    virtual QDateTime expiryDate() const = 0;

    virtual TlsKey *publicKey() const;

    // Extensions. Plugins do not expose internal representation
    // and cannot rely on QSslCertificate's internals.
    virtual qsizetype numberOfExtensions() const = 0;
    virtual QString oidForExtension(qsizetype index) const = 0;
    virtual QString nameForExtension(qsizetype index) const = 0;
    virtual QVariant valueForExtension(qsizetype index) const = 0;
    virtual bool isExtensionCritical(qsizetype index) const = 0;
    virtual bool isExtensionSupported(qsizetype index) const = 0;

    virtual QByteArray toPem() const = 0;
    virtual QByteArray toDer() const = 0;
    virtual QString toText() const = 0;

    virtual Qt::HANDLE handle() const = 0;

    virtual size_t hash(size_t seed) const noexcept = 0;
};

// TLSTODO: consider making those into virtuals in QTlsBackend. After all, we ask the backend
// to return those pointers if the functionality is supported, but it's a bit odd to have
// this level of indirection. They are not parts of the classes above because ...
// you'd then have to ask backend to create a certificate to ... call those
// functions on a certificate.
using X509ChainVerifyPtr = QList<QSslError> (*)(const QList<QSslCertificate> &chain,
                                                const QString &hostName);
using X509PemReaderPtr = QList<QSslCertificate> (*)(const QByteArray &pem, int count);
using X509DerReaderPtr = X509PemReaderPtr;
using X509Pkcs12ReaderPtr = bool (*)(QIODevice *device, QSslKey *key, QSslCertificate *cert,
                                     QList<QSslCertificate> *caCertificates,
                                     const QByteArray &passPhrase);

#if QT_CONFIG(ssl)
// TLS over TCP. Handshake, encryption/decryption.
class Q_NETWORK_PRIVATE_EXPORT TlsCryptograph : public QObject
{
public:
    virtual ~TlsCryptograph();

    virtual void init(QSslSocket *q, QSslSocketPrivate *d) = 0;
    virtual void checkSettingSslContext(QSharedPointer<QSslContext> tlsContext);
    virtual QSharedPointer<QSslContext> sslContext() const;

    virtual QList<QSslError> tlsErrors() const = 0;

    virtual void startClientEncryption() = 0;
    virtual void startServerEncryption() = 0;
    virtual void continueHandshake() = 0;
    virtual void enableHandshakeContinuation();
    virtual void disconnectFromHost() = 0;
    virtual void disconnected() = 0;
    virtual void cancelCAFetch();
    virtual QSslCipher sessionCipher() const = 0;
    virtual QSsl::SslProtocol sessionProtocol() const = 0;

    virtual void transmit() = 0;
    virtual bool hasUndecryptedData() const;
    virtual QList<QOcspResponse> ocsps() const;

    static bool isMatchingHostname(const QSslCertificate &cert, const QString &peerName);
    static bool isMatchingHostname(const QString &cn, const QString &hostname);
};
#else
class TlsCryptograph;
#endif // QT_CONFIG(ssl)

#if QT_CONFIG(dtls)

class Q_NETWORK_PRIVATE_EXPORT DtlsBase
{
public:
    virtual ~DtlsBase();

    virtual void setDtlsError(QDtlsError code, const QString &description) = 0;

    virtual QDtlsError error() const = 0;
    virtual QString errorString() const = 0;

    virtual void clearDtlsError() = 0;

    virtual void setConfiguration(const QSslConfiguration &configuration) = 0;
    virtual QSslConfiguration configuration() const = 0;

    using GenParams = QDtlsClientVerifier::GeneratorParameters;
    virtual bool setCookieGeneratorParameters(const GenParams &params) = 0;
    virtual GenParams cookieGeneratorParameters() const = 0;
};

// DTLS cookie: generation and verification.
class Q_NETWORK_EXPORT DtlsCookieVerifier : virtual public DtlsBase
{
public:
    virtual bool verifyClient(QUdpSocket *socket, const QByteArray &dgram,
                              const QHostAddress &address, quint16 port) = 0;
    virtual QByteArray verifiedHello() const = 0;
};

// TLS over UDP. Handshake, encryption/decryption.
class Q_NETWORK_PRIVATE_EXPORT DtlsCryptograph : virtual public DtlsBase
{
public:

    virtual QSslSocket::SslMode cryptographMode() const = 0;
    virtual void setPeer(const QHostAddress &addr, quint16 port, const QString &name) = 0;
    virtual QHostAddress peerAddress() const = 0;
    virtual quint16 peerPort() const = 0;
    virtual void setPeerVerificationName(const QString &name) = 0;
    virtual QString peerVerificationName() const = 0;

    virtual void setDtlsMtuHint(quint16 mtu) = 0;
    virtual quint16 dtlsMtuHint() const = 0;

    virtual QDtls::HandshakeState state() const = 0;
    virtual bool isConnectionEncrypted() const = 0;

    virtual bool startHandshake(QUdpSocket *socket, const QByteArray &dgram) = 0;
    virtual bool handleTimeout(QUdpSocket *socket) = 0;
    virtual bool continueHandshake(QUdpSocket *socket, const QByteArray &dgram) = 0;
    virtual bool resumeHandshake(QUdpSocket *socket) = 0;
    virtual void abortHandshake(QUdpSocket *socket) = 0;
    virtual void sendShutdownAlert(QUdpSocket *socket) = 0;

    virtual QList<QSslError> peerVerificationErrors() const = 0;
    virtual void ignoreVerificationErrors(const QList<QSslError> &errorsToIgnore) = 0;

    virtual QSslCipher dtlsSessionCipher() const = 0;
    virtual QSsl::SslProtocol dtlsSessionProtocol() const = 0;

    virtual qint64 writeDatagramEncrypted(QUdpSocket *socket, const QByteArray &dgram) = 0;
    virtual QByteArray decryptDatagram(QUdpSocket *socket, const QByteArray &dgram) = 0;
};

#else

class DtlsCookieVerifier;
class DtlsCryptograph;

#endif // QT_CONFIG(dtls)

} // namespace QTlsPrivate

// Factory, creating back-end specific implementations of
// different entities QSslSocket is using.
class Q_NETWORK_EXPORT QTlsBackend : public QObject
{
    Q_OBJECT
public:
    QTlsBackend();
    ~QTlsBackend() override;

    virtual bool isValid() const;
    virtual long tlsLibraryVersionNumber() const;
    virtual QString tlsLibraryVersionString() const;
    virtual long tlsLibraryBuildVersionNumber() const;
    virtual QString tlsLibraryBuildVersionString() const;
    virtual void ensureInitialized() const;

    virtual QString backendName() const = 0;
    virtual QList<QSsl::SslProtocol> supportedProtocols() const = 0;
    virtual QList<QSsl::SupportedFeature> supportedFeatures() const = 0;
    virtual QList<QSsl::ImplementedClass> implementedClasses() const = 0;

    // X509 and keys:
    virtual QTlsPrivate::TlsKey *createKey() const;
    virtual QTlsPrivate::X509Certificate *createCertificate() const;

    virtual QList<QSslCertificate> systemCaCertificates() const;

    // TLS and DTLS:
    virtual QTlsPrivate::TlsCryptograph *createTlsCryptograph() const;
    virtual QTlsPrivate::DtlsCryptograph *createDtlsCryptograph(class QDtls *qObject, int mode) const;
    virtual QTlsPrivate::DtlsCookieVerifier *createDtlsCookieVerifier() const;

    // TLSTODO - get rid of these function pointers, make them virtuals in
    // the backend itself. X509 machinery:
    virtual QTlsPrivate::X509ChainVerifyPtr X509Verifier() const;
    virtual QTlsPrivate::X509PemReaderPtr X509PemReader() const;
    virtual QTlsPrivate::X509DerReaderPtr X509DerReader() const;
    virtual QTlsPrivate::X509Pkcs12ReaderPtr X509Pkcs12Reader() const;

    // Elliptic curves:
    virtual QList<int> ellipticCurvesIds() const;
    virtual int curveIdFromShortName(const QString &name) const;
    virtual int curveIdFromLongName(const QString &name) const;
    virtual QString shortNameForId(int cid) const;
    virtual QString longNameForId(int cid) const;
    virtual bool isTlsNamedCurve(int cid) const;

    // TLSTODO: int->enum ugliness in error reporting.
    // DH decoding:
    virtual int dhParametersFromDer(const QByteArray &derData, QByteArray *data) const;
    virtual int dhParametersFromPem(const QByteArray &pemData, QByteArray *data) const;

    static QList<QString> availableBackendNames();
    static QString defaultBackendName();
    static QTlsBackend *findBackend(const QString &backendName);
    static QTlsBackend *activeOrAnyBackend();

    static QList<QSsl::SslProtocol> supportedProtocols(const QString &backendName);
    static QList<QSsl::SupportedFeature> supportedFeatures(const QString &backendName);
    static QList<QSsl::ImplementedClass> implementedClasses(const QString &backendName);

    // Built-in, this is what Qt provides out of the box (depending on OS):
    static constexpr const int nameIndexSchannel = 0;
    static constexpr const int nameIndexSecureTransport = 1;
    static constexpr const int nameIndexOpenSSL = 2;

    static const QString builtinBackendNames[];

    template<class DynamicType, class TLSObject>
    static DynamicType *backend(const TLSObject &o)
    {
        return static_cast<DynamicType *>(o.d->backend.get());
    }

    static void resetBackend(QSslKey &key, QTlsPrivate::TlsKey *keyBackend);

    static void setupClientPskAuth(QSslPreSharedKeyAuthenticator *auth, const char *hint,
                                   int hintLength, unsigned maxIdentityLen, unsigned maxPskLen);
    static void setupServerPskAuth(QSslPreSharedKeyAuthenticator *auth, const char *identity,
                                   const QByteArray &identityHint, unsigned maxPskLen);
#if QT_CONFIG(ssl)
    static QSslCipher createCiphersuite(const QString &description, int bits, int supportedBits);
    static QSslCipher createCiphersuite(const QString &suiteName, QSsl::SslProtocol protocol,
                                        const QString &protocolString);
    static QSslCipher createCipher(const QString &name, QSsl::SslProtocol protocol,
                                   const QString &protocolString);

    // Those statics are implemented using QSslSocketPrivate (which is not exported,
    // unlike QTlsBackend).
    static QList<QSslCipher> defaultCiphers();
    static QList<QSslCipher> defaultDtlsCiphers();

    static void setDefaultCiphers(const QList<QSslCipher> &ciphers);
    static void setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers);
    static void setDefaultSupportedCiphers(const QList<QSslCipher> &ciphers);

    static void resetDefaultEllipticCurves();

    static void setDefaultCaCertificates(const QList<QSslCertificate> &certs);

    // Many thanks to people who designed QSslConfiguration with hidden
    // data-members, that sneakily set by some 'friend' classes, having
    // some twisted logic.
    static bool rootLoadingOnDemandAllowed(const QSslConfiguration &configuration);
    static void storePeerCertificate(QSslConfiguration &configuration, const QSslCertificate &peerCert);
    static void storePeerCertificateChain(QSslConfiguration &configuration,
                                          const QList<QSslCertificate> &peerCertificateChain);
    static void clearPeerCertificates(QSslConfiguration &configuration);
    // And those are even worse, this is where we don't have the original configuration,
    // and can have only a copy. So instead we go to d->privateConfiguration.someMember:
    static void clearPeerCertificates(QSslSocketPrivate *d);
    static void setPeerSessionShared(QSslSocketPrivate *d, bool shared);
    static void setSessionAsn1(QSslSocketPrivate *d, const QByteArray &asn1);
    static void setSessionLifetimeHint(QSslSocketPrivate *d, int hint);
    using AlpnNegotiationStatus = QSslConfiguration::NextProtocolNegotiationStatus;
    static void setAlpnStatus(QSslSocketPrivate *d, AlpnNegotiationStatus st);
    static void setNegotiatedProtocol(QSslSocketPrivate *d, const QByteArray &protocol);
    static void storePeerCertificate(QSslSocketPrivate *d, const QSslCertificate &peerCert);
    static void storePeerCertificateChain(QSslSocketPrivate *d, const QList<QSslCertificate> &peerChain);
    static void addTustedRoot(QSslSocketPrivate *d, const QSslCertificate &rootCert);
    // The next one - is a "very important" feature! Kidding ...
    static void setEphemeralKey(QSslSocketPrivate *d, const QSslKey &key);
#endif // QT_CONFIG(ssl)

    Q_DISABLE_COPY_MOVE(QTlsBackend)
};

Q_DECLARE_LOGGING_CATEGORY(lcTlsBackend)

#define QTlsBackend_iid "org.qt-project.Qt.QTlsBackend"
Q_DECLARE_INTERFACE(QTlsBackend, QTlsBackend_iid);

QT_END_NAMESPACE

#endif // QTLSBACKEND_P_H
