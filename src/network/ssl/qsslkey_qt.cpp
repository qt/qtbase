/****************************************************************************
**
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsslkey.h"
#include "qsslkey_p.h"
#include "qasn1element_p.h"

QT_USE_NAMESPACE

static const quint8 bits_table[256] = {
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
};

static int numberOfBits(const QByteArray &modulus)
{
    int bits = modulus.size() * 8;
    for (int i = 0; i < modulus.size(); ++i) {
        quint8 b = modulus[i];
        bits -= 8;
        if (b != 0) {
            bits += bits_table[b];
            break;
        }
    }
    return bits;
}

void QSslKeyPrivate::clear(bool deep)
{
    Q_UNUSED(deep);
    isNull = true;
    derData.clear();
    keyLength = -1;
}

void QSslKeyPrivate::decodeDer(const QByteArray &der, bool deepClear)
{
    clear(deepClear);

    if (der.isEmpty())
        return;

    QAsn1Element elem;
    if (!elem.read(der) || elem.type() != QAsn1Element::SequenceType)
        return;

    if (type == QSsl::PublicKey) {
        // key info
        QDataStream keyStream(elem.value());
        if (!elem.read(keyStream) || elem.type() != QAsn1Element::SequenceType)
            return;
        QVector<QAsn1Element> infoItems = elem.toVector();
        if (infoItems.size() < 2 || infoItems[0].type() != QAsn1Element::ObjectIdentifierType)
            return;
        if (algorithm == QSsl::Rsa) {
            if (infoItems[0].toObjectId() != "1.2.840.113549.1.1.1")
                return;
            // key data
            if (!elem.read(keyStream) || elem.type() != QAsn1Element::BitStringType || elem.value().isEmpty())
                return;
            if (!elem.read(elem.value().mid(1)) || elem.type() != QAsn1Element::SequenceType)
                return;
            if (!elem.read(elem.value()) || elem.type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(elem.value());
        } else if (algorithm == QSsl::Dsa) {
            if (infoItems[0].toObjectId() != "1.2.840.10040.4.1")
                return;
            if (infoItems[1].type() != QAsn1Element::SequenceType)
                return;
            // key params
            QVector<QAsn1Element> params = infoItems[1].toVector();
            if (params.isEmpty() || params[0].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(params[0].value());
        }

    } else {
        QVector<QAsn1Element> items = elem.toVector();
        if (items.isEmpty())
            return;

        // version
        if (items[0].type() != QAsn1Element::IntegerType || items[0].value().toHex() != "00")
            return;

        if (algorithm == QSsl::Rsa) {
            if (items.size() != 9 || items[1].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(items[1].value());
        } else if (algorithm == QSsl::Dsa) {
            if (items.size() != 6 || items[1].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(items[1].value());
        }
    }

    derData = der;
    isNull = false;
}

void QSslKeyPrivate::decodePem(const QByteArray &pem, const QByteArray &passPhrase,
                               bool deepClear)
{
    if (type == QSsl::PrivateKey && !passPhrase.isEmpty()) {
        Q_UNIMPLEMENTED();
        return;
    }

    decodeDer(derFromPem(pem), deepClear);
}

int QSslKeyPrivate::length() const
{
    return keyLength;
}

QByteArray QSslKeyPrivate::toPem(const QByteArray &passPhrase) const
{
    if (type == QSsl::PrivateKey && !passPhrase.isEmpty()) {
        Q_UNIMPLEMENTED();
        return QByteArray();
    }

    return pemFromDer(derData);
}

Qt::HANDLE QSslKeyPrivate::handle() const
{
    return opaque;
}
