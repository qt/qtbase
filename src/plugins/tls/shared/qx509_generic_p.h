// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QX509_GENERIC_P_H
#define QX509_GENERIC_P_H

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

#include <QtNetwork/private/qtlsbackend_p.h>

#include "qx509_base_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

// A part of SecureTransport and Schannel plugin.
class X509CertificateGeneric : public X509CertificateBase
{
public:
    bool isEqual(const X509Certificate &rhs) const override;
    bool isSelfSigned() const override;

    QMultiMap<QSsl::AlternativeNameEntryType, QString> subjectAlternativeNames() const override;
    QByteArray toPem() const override;
    QByteArray toDer() const override;
    QString toText() const override;
    Qt::HANDLE handle() const override;

    size_t hash(size_t seed) const noexcept override;

    static QList<QSslCertificate> certificatesFromPem(const QByteArray &pem, int count);
    static QList<QSslCertificate> certificatesFromDer(const QByteArray &der, int count);

protected:

    bool subjectMatchesIssuer = false;
    QSsl::KeyAlgorithm publicKeyAlgorithm = QSsl::Rsa;
    QByteArray publicKeyDerData;

    QMultiMap<QSsl::AlternativeNameEntryType, QString> saNames;
    QByteArray derData;

    bool parse(const QByteArray &data);
    bool parseExtension(const QByteArray &data, X509CertificateExtension &extension);
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QX509_GENERIC_P_H
