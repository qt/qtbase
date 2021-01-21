/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#include "qsslcertificate.h"
#include "qsslcertificate_p.h"

#include <wincrypt.h>

QT_BEGIN_NAMESPACE

QSslCertificate QSslCertificatePrivate::QSslCertificate_from_CERT_CONTEXT(const CERT_CONTEXT *certificateContext)
{
    QByteArray derData = QByteArray((const char *)certificateContext->pbCertEncoded,
                                    certificateContext->cbCertEncoded);

    QSslCertificate certificate(derData, QSsl::Der);
    certificate.d->certificateContext = CertDuplicateCertificateContext(certificateContext);
    return certificate;
}

Qt::HANDLE QSslCertificate::handle() const
{
    return Qt::HANDLE(d->certificateContext);
}

QT_END_NAMESPACE
