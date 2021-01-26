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

#include "qsslsocket_openssl_symbols_p.h"

#include <qssl.h>

#include <qlist.h>

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

QT_END_NAMESPACE
