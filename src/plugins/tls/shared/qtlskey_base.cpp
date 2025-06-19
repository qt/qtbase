// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qtlskey_base_p.h"
#include "qasn1element_p.h"

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

QByteArray TlsKeyBase::pemFromDer(const QByteArray &der, const QMap<QByteArray, QByteArray> &headers) const
{
    QByteArray pem(der.toBase64());

    const int lineWidth = 64; // RFC 1421
    const int newLines = pem.size() / lineWidth;
    const bool rem = pem.size() % lineWidth;

    for (int i = 0; i < newLines; ++i)
        pem.insert((i + 1) * lineWidth + i, '\n');
    if (rem)
        pem.append('\n');

    QByteArray extra;
    if (!headers.isEmpty()) {
        QMap<QByteArray, QByteArray>::const_iterator it = headers.constEnd();
        do {
            --it;
            extra += it.key() + ": " + it.value() + '\n';
        } while (it != headers.constBegin());
        extra += '\n';
    }

    if (isEncryptedPkcs8(der)) {
        pem.prepend(pkcs8Header(true) + '\n' + extra);
        pem.append(pkcs8Footer(true) + '\n');
    } else if (isPkcs8()) {
        pem.prepend(pkcs8Header(false) + '\n' + extra);
        pem.append(pkcs8Footer(false) + '\n');
    } else {
        pem.prepend(pemHeader() + '\n' + extra);
        pem.append(pemFooter() + '\n');
    }

    return pem;
}

QByteArray TlsKeyBase::pkcs8Header(bool encrypted)
{
    return encrypted
        ? QByteArrayLiteral("-----BEGIN ENCRYPTED PRIVATE KEY-----")
        : QByteArrayLiteral("-----BEGIN PRIVATE KEY-----");
}

QByteArray TlsKeyBase::pkcs8Footer(bool encrypted)
{
    return encrypted
        ? QByteArrayLiteral("-----END ENCRYPTED PRIVATE KEY-----")
        : QByteArrayLiteral("-----END PRIVATE KEY-----");
}

bool TlsKeyBase::isEncryptedPkcs8(const QByteArray &der)
{
    static const QList<QByteArray> pbes1OIds {
        // PKCS5
        { PKCS5_MD2_DES_CBC_OID }, { PKCS5_MD2_RC2_CBC_OID },  { PKCS5_MD5_DES_CBC_OID },
        { PKCS5_MD5_RC2_CBC_OID }, { PKCS5_SHA1_DES_CBC_OID }, { PKCS5_SHA1_RC2_CBC_OID },
    };
    QAsn1Element elem;
    if (!elem.read(der) || elem.type() != QAsn1Element::SequenceType)
        return false;

    const auto items = elem.toList();
    if (items.size() != 2
        || items[0].type() != QAsn1Element::SequenceType
        || items[1].type() != QAsn1Element::OctetStringType) {
        return false;
    }

    const auto encryptionSchemeContainer = items[0].toList();
    if (encryptionSchemeContainer.size() != 2
        || encryptionSchemeContainer[0].type() != QAsn1Element::ObjectIdentifierType
        || encryptionSchemeContainer[1].type() != QAsn1Element::SequenceType) {
        return false;
    }

    const QByteArray encryptionScheme = encryptionSchemeContainer[0].toObjectId();
    return encryptionScheme == PKCS5_PBES2_ENCRYPTION_OID
            || pbes1OIds.contains(encryptionScheme)
            || encryptionScheme.startsWith(PKCS12_OID);
}

} // namespace QTlsPrivate

QT_END_NAMESPACE


