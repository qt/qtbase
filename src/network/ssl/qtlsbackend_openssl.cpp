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

#include "qtlsbackend_openssl_p.h"
#include "qtlskey_openssl_p.h"
#include "qx509_openssl_p.h"

#if QT_CONFIG(dtls)
#include "qdtls_openssl_p.h"
#endif // QT_CONFIG(dtls)

// TLSTODO: Later, this code (ensure initialised, etc.)
// must move from the socket to backend.
#include "qsslsocket_p.h"
//
#include "qsslsocket_openssl_symbols_p.h"

#include <qssl.h>

#include <qlist.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTlsBackend, "qt.tlsbackend.ossl");

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

QString QTlsBackendOpenSSL::backendName() const
{
    return builtinBackendNames[nameIndexOpenSSL];
}

bool QTlsBackendOpenSSL::isValid() const
{
    // TLSTODO: backend should do initialization,
    // not socket.
    return QSslSocket::supportsSsl();
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
    features << QSsl::SupportedFeature::ClientSideAlpn;
    features << QSsl::SupportedFeature::ServerSideAlpn;
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
    classes << QSsl::ImplementedClass::Dtls;
    classes << QSsl::ImplementedClass::EllipticCurve;
    classes << QSsl::ImplementedClass::DiffieHellman;

    return classes;
}

QSsl::TlsKey *QTlsBackendOpenSSL::createKey() const
{
    return new QSsl::TlsKeyOpenSSL;
}

QSsl::X509Certificate *QTlsBackendOpenSSL::createCertificate() const
{
    return new QSsl::X509CertificateOpenSSL;
}

QSsl::DtlsCookieVerifier *QTlsBackendOpenSSL::createDtlsCookieVerifier() const
{
#if QT_CONFIG(dtls)
    return new QDtlsClientVerifierOpenSSL;
#else
    qCWarning(lcTlsBackend, "Feature 'dtls' is disabled, cannot verify DTLS cookies");
    return nullptr;
#endif // QT_CONFIG(dtls)
}

QSsl::DtlsCryptograph *QTlsBackendOpenSSL::createDtlsCryptograph(QDtls *q, int mode) const
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

QSsl::X509ChainVerifyPtr QTlsBackendOpenSSL::X509Verifier() const
{
    return QSsl::X509CertificateOpenSSL::verify;
}

QSsl::X509PemReaderPtr QTlsBackendOpenSSL::X509PemReader() const
{
    return QSsl::X509CertificateOpenSSL::certificatesFromPem;
}

QSsl::X509DerReaderPtr QTlsBackendOpenSSL::X509DerReader() const
{
    return QSsl::X509CertificateOpenSSL::certificatesFromDer;
}

QSsl::X509Pkcs12ReaderPtr QTlsBackendOpenSSL::X509Pkcs12Reader() const
{
    return QSsl::X509CertificateOpenSSL::importPkcs12;
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

     // TLSTODO: check if it's needed! The fact we are here,
     // means OpenSSL was loaded, symbols resolved. Is it because
     // of ensureCiphers(AndCertificates)Loaded ?
     QSslSocketPrivate::ensureInitialized();
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

     // TLSTODO: check if it's needed! The fact we are here,
     // means OpenSSL was loaded, symbols resolved. Is it because
     // of ensureCiphers(AndCertificates)Loaded ?
     QSslSocketPrivate::ensureInitialized();

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

QT_END_NAMESPACE
