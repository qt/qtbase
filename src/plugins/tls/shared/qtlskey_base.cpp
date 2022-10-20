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


