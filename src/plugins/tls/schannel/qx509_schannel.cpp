/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

