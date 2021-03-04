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

#include <private/qtnetworkglobal_p.h>

#include <private/qopenssl_p.h>

#include <private/qtlsbackend_p.h>
#include <private/qx509_base_p.h>

#include <QtCore/qvariant.h>
#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

#include <openssl/x509.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

// TLSTODO: This class is essentially what qsslcertificate_openssl.cpp
// contains - OpenSSL-based version of QSslCertificatePrivate. Remove
// this comment when plugins are ready.
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

    // TLSTODO: these are needed by qsslsocket_openssl and later, by
    // TLS code inside OpenSSL plugin. Remove this comment when
    // plugins are ready.
    static QSslCertificate certificateFromX509(X509 *x);
    static QList<QSslCertificate> stackOfX509ToQSslCertificates(STACK_OF(X509) *x509);
    static QSslErrorEntry errorEntryFromStoreContext(X509_STORE_CTX *ctx);

    // TLSTODO: remove this comment when plugins are in place. This is what QSslSocketPrivate::verify()
    // in qsslsocket_openssl.cpp is (was) doing (in the past).
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
