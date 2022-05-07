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

#include "qtlskey_st_p.h"
#include "qx509_st_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

TlsKey *X509CertificateSecureTransport::publicKey() const
{
    auto key = std::make_unique<TlsKeySecureTransport>(QSsl::PublicKey);
    if (publicKeyAlgorithm != QSsl::Opaque)
        key->decodeDer(QSsl::PublicKey, publicKeyAlgorithm, publicKeyDerData, {}, false);

    return key.release();
}

} // namespace QTlsPrivate

QT_END_NAMESPACE

