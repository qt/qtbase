// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QX509_OPENSSL_P_H
#define QX509_OPENSSL_P_H

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

#include "../shared/qx509_base_p.h"

#include <QtNetwork/private/qtlsbackend_p.h>

#include <QtCore/qvariant.h>
#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

#include "qopenssl_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class X509CertificateOpenSSL final : public X509CertificateBase
{
public:
    X509CertificateOpenSSL();
    ~X509CertificateOpenSSL();

    // TLSTODO: in future may become movable/copyable (ref-counted based
    // OpenSSL's X509 implementation).

    bool isEqual(const X509Certificate &rhs) const override;
    bool isSelfSigned() const override;
    QMultiMap<QSsl::AlternativeNameEntryType, QString> subjectAlternativeNames() const override;
    TlsKey *publicKey() const override;

    QByteArray toPem() const override;
    QByteArray toDer() const override;
    QString toText() const override;
    Qt::HANDLE handle() const override;

    size_t hash(size_t seed) const noexcept override;

    static QSslCertificate certificateFromX509(X509 *x);
    static QList<QSslCertificate> stackOfX509ToQSslCertificates(STACK_OF(X509) *x509);
    static QSslErrorEntry errorEntryFromStoreContext(X509_STORE_CTX *ctx);

    static QList<QSslError> verify(const QList<QSslCertificate> &chain, const QString &hostName);
    static QList<QSslError> verify(const QList<QSslCertificate> &caCertificates,
                                   const QList<QSslCertificate> &certificateChain,
                                   const QString &hostName);

    static QList<QSslCertificate> certificatesFromPem(const QByteArray &pem, int count);
    static QList<QSslCertificate> certificatesFromDer(const QByteArray &der, int count);
    static bool importPkcs12(QIODevice *device, QSslKey *key, QSslCertificate *cert,
                             QList<QSslCertificate> *caCertificates,
                             const QByteArray &passPhrase);

    static QSslError openSSLErrorToQSslError(int errorCode, const QSslCertificate &cert);
private:
    void parseExtensions();
    static X509CertificateExtension convertExtension(X509_EXTENSION *ext);

    X509 *x509 = nullptr;

    Q_DISABLE_COPY_MOVE(X509CertificateOpenSSL)
};

extern "C" int qt_X509Callback(int ok, X509_STORE_CTX *ctx);

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QX509_OPENSSL_P_H
