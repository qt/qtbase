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

#include "qssl_p.h"

#include <QtNetwork/qsslcertificate.h>
#include <QtNetwork/qsslerror.h>
#include <QtNetwork/qsslkey.h>
#include <QtNetwork/qssl.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qobject.h>
#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>

#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE

class QByteArray;
class QIODevice;
class QSslKey;

namespace QSsl {

// The class TlsKey encapsulates key's data (DER) or backend-specific
// data-structure, like RSA/DSA/DH structs in OpenSSL.
// TLSTODO: Interface is mostly what QSslKeyPrivate is now. Names,
// however strange they are, for now preserved to ease the transition
// (this may change in future - for example, 'decodeDer' is not just
// decoding DER, it's initializing a key from DER. Note, QSslKey requires
// a real TLS library because private keys tend to be encrypted. This
// base class does not need a working TLS library.
class TlsKey {
public:
    virtual ~TlsKey();

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

    using Cipher = QSsl::Cipher;
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
class X509Certificate
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

using X509ChainVerifyPtr = QList<QSslError> (*)(const QList<QSslCertificate> &chain,
                                                const QString &hostName);
using X509PemReaderPtr = QList<QSslCertificate> (*)(const QByteArray &pem, int count);
using X509DerReaderPtr = X509PemReaderPtr;
using X509Pkcs12ReaderPtr = bool (*)(QIODevice *device, QSslKey *key, QSslCertificate *cert,
                                     QList<QSslCertificate> *caCertificates,
                                     const QByteArray &passPhrase);

// TLS over TCP. Handshake, encryption/decryption.
class TlsCryptograph;

// TLS over UDP. Handshake, encryption/decryption.
class DtlsCryptograph;

// DTLS cookie: generation and verification.
class DtlsCookieVerifier;

} // namespace QSsl

// Factory, creating back-end specific implementations of
// different entities QSslSocket is using.
class Q_NETWORK_EXPORT QTlsBackend : public QObject
{
    Q_OBJECT
public:
    QTlsBackend();
    ~QTlsBackend() override;

    virtual bool isValid() const;

    virtual QString backendName() const = 0;
    virtual QList<QSsl::SslProtocol> supportedProtocols() const = 0;
    virtual QList<QSsl::SupportedFeature> supportedFeatures() const = 0;
    virtual QList<QSsl::ImplementedClass> implementedClasses() const = 0;

    // X509 and keys:
    virtual QSsl::TlsKey *createKey() const;
    virtual QSsl::X509Certificate *createCertificate() const;

    // TLS and DTLS:
    virtual QSsl::TlsCryptograph *createTlsCryptograph() const;
    virtual QSsl::DtlsCryptograph *createDtlsCryptograph() const;
    virtual QSsl::DtlsCookieVerifier *createDtlsCookieVerifier() const;

    // X509 machinery:
    virtual QSsl::X509ChainVerifyPtr X509Verifier() const;
    virtual QSsl::X509PemReaderPtr X509PemReader() const;
    virtual QSsl::X509DerReaderPtr X509DerReader() const;
    virtual QSsl::X509Pkcs12ReaderPtr X509Pkcs12Reader() const;

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

    template<class DynamicType, class  TLSObject>
    static DynamicType *backend(const TLSObject &o)
    {
        return static_cast<DynamicType *>(o.backendImplementation());
    }

    static void resetBackend(QSslKey &key, QSsl::TlsKey *keyBackend);

    Q_DISABLE_COPY_MOVE(QTlsBackend)
};

Q_DECLARE_LOGGING_CATEGORY(lcTlsBackend)

#define QTlsBackend_iid "org.qt-project.Qt.QTlsBackend"
Q_DECLARE_INTERFACE(QTlsBackend, QTlsBackend_iid);

QT_END_NAMESPACE

#endif // QTLSBACKEND_P_H
