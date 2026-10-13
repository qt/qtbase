// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

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

