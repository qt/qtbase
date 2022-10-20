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

