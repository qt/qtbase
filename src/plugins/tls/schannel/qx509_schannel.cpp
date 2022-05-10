// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtlskey_schannel_p.h"
#include "qx509_schannel_p.h"

#include <QtNetwork/private/qsslcertificate_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

X509CertificateSchannel::X509CertificateSchannel() = default;

X509CertificateSchannel::~X509CertificateSchannel()
{
    if (certificateContext)
        CertFreeCertificateContext(certificateContext);
}

TlsKey *X509CertificateSchannel::publicKey() const
{
    auto key = std::make_unique<TlsKeySchannel>(QSsl::PublicKey);
    if (publicKeyAlgorithm != QSsl::Opaque)
        key->decodeDer(QSsl::PublicKey, publicKeyAlgorithm, publicKeyDerData, {}, false);

    return key.release();
}

Qt::HANDLE X509CertificateSchannel::handle() const
{
    return Qt::HANDLE(certificateContext);
}

QSslCertificate X509CertificateSchannel::QSslCertificate_from_CERT_CONTEXT(const CERT_CONTEXT *certificateContext)
{
    QByteArray derData = QByteArray((const char *)certificateContext->pbCertEncoded,
                                    certificateContext->cbCertEncoded);
    QSslCertificate certificate(derData, QSsl::Der);

    auto *certBackend = QTlsBackend::backend<X509CertificateSchannel>(certificate);
    Q_ASSERT(certBackend);
    certBackend->certificateContext = CertDuplicateCertificateContext(certificateContext);
    return certificate;
}

} // namespace QTlsPrivate

QT_END_NAMESPACE

