// Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:cryptography

#include "qasn1element_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qmessageauthenticationcode.h>
#include <QtCore/qrandom.h>

#include <QtNetwork/private/qsslsocket_p.h>
#include <QtNetwork/private/qsslkey_p.h>

QT_BEGIN_NAMESPACE

/*
    PKCS12 helpers.
*/

static QAsn1Element wrap(quint8 type, const QAsn1Element &child)
{
    QByteArray value;
    QDataStream stream(&value, QIODevice::WriteOnly);
    child.write(stream);
    return QAsn1Element(type, value);
}

static QAsn1Element _q_PKCS7_data(const QByteArray &data)
{
    QList<QAsn1Element> items;
    items << QAsn1Element::fromObjectId("1.2.840.113549.1.7.1");
    items << wrap(QAsn1Element::Context0Type,
                  QAsn1Element(QAsn1Element::OctetStringType, data));
    return QAsn1Element::fromVector(items);
}

/*!
    PKCS #12 key derivation.

    Some test vectors:
    http://www.drh-consultancy.demon.co.uk/test.txt
    \internal
*/
static QByteArray _q_PKCS12_keygen(char id, const QByteArray &salt, const QString &passPhrase, int n, int r)
{
    const int u = 20;
    const int v = 64;

    // password formatting
    QByteArray passUnicode(passPhrase.size() * 2 + 2, '\0');
    char *p = passUnicode.data();
    for (int i = 0; i < passPhrase.size(); ++i) {
        quint16 ch = passPhrase[i].unicode();
        *(p++) = (ch & 0xff00) >> 8;
        *(p++) = (ch & 0xff);
    }

    // prepare I
    QByteArray D(64, id);
    QByteArray S, P;
    const int sSize = v * ((salt.size() + v - 1) / v);
    S.resize(sSize);
    for (int i = 0; i < sSize; ++i)
        S[i] = salt[i % salt.size()];
    const int pSize = v * ((passUnicode.size() + v - 1) / v);
    P.resize(pSize);
    for (int i = 0; i < pSize; ++i)
        P[i] = passUnicode[i % passUnicode.size()];
    QByteArray I = S + P;

    // apply hashing
    const int c = (n + u - 1) / u;
    QByteArray A;
    QByteArray B;
    B.resize(v);
    for (int i = 0; i < c; ++i) {
        // hash r iterations
        QByteArray Ai = D + I;
        for (int j = 0; j < r; ++j)
            Ai = QCryptographicHash::hash(Ai, QCryptographicHash::Sha1);

        for (int j = 0; j < v; ++j)
            B[j] = Ai[j % u];

        // modify I as Ij = (Ij + B + 1) modulo 2^v
        for (int p = 0; p < I.size(); p += v) {
            quint8 carry = 1;
            for (int j = v - 1; j >= 0; --j) {
                quint16 v = quint8(I[p + j]) + quint8(B[j]) + carry;
                I[p + j] = v & 0xff;
                carry = (v & 0xff00) >> 8;
            }
        }
        A += Ai;
    }
    return A.left(n);
}

static QByteArray _q_PKCS12_salt()
{
    QByteArray salt;
    salt.resize(8);
    for (int i = 0; i < salt.size(); ++i)
        salt[i] = (QRandomGenerator::global()->generate() & 0xff);
    return salt;
}

static QByteArray _q_PKCS12_certBag(const QSslCertificate &cert)
{
    QList<QAsn1Element> items;
    items << QAsn1Element::fromObjectId("1.2.840.113549.1.12.10.1.3");

    // certificate
    QList<QAsn1Element> certItems;
    certItems << QAsn1Element::fromObjectId("1.2.840.113549.1.9.22.1");
    certItems << wrap(QAsn1Element::Context0Type,
                      QAsn1Element(QAsn1Element::OctetStringType, cert.toDer()));
    items << wrap(QAsn1Element::Context0Type,
                  QAsn1Element::fromVector(certItems));

    // local key id
    const QByteArray localKeyId = cert.digest(QCryptographicHash::Sha1);
    QList<QAsn1Element> idItems;
    idItems << QAsn1Element::fromObjectId("1.2.840.113549.1.9.21");
    idItems << wrap(QAsn1Element::SetType,
                    QAsn1Element(QAsn1Element::OctetStringType, localKeyId));
    items << wrap(QAsn1Element::SetType, QAsn1Element::fromVector(idItems));

    // dump
    QAsn1Element root = wrap(QAsn1Element::SequenceType, QAsn1Element::fromVector(items));
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    root.write(stream);
    return ba;
}

QAsn1Element _q_PKCS12_key(const QSslKey &key)
{
    Q_ASSERT(key.algorithm() == QSsl::Rsa || key.algorithm() == QSsl::Dsa);

    QList<QAsn1Element> keyItems;
    keyItems << QAsn1Element::fromInteger(0);
    QList<QAsn1Element> algoItems;
    if (key.algorithm() == QSsl::Rsa)
        algoItems << QAsn1Element::fromObjectId(RSA_ENCRYPTION_OID);
    else if (key.algorithm() == QSsl::Dsa)
        algoItems << QAsn1Element::fromObjectId(DSA_ENCRYPTION_OID);
    algoItems << QAsn1Element(QAsn1Element::NullType);
    keyItems << QAsn1Element::fromVector(algoItems);
    keyItems << QAsn1Element(QAsn1Element::OctetStringType, key.toDer());
    return QAsn1Element::fromVector(keyItems);
}

static QByteArray _q_PKCS12_shroudedKeyBag(const QSslKey &key, const QString &passPhrase, const QByteArray &localKeyId)
{
    const int iterations = 2048;
    QByteArray salt = _q_PKCS12_salt();
    QByteArray cKey = _q_PKCS12_keygen(1, salt, passPhrase, 24, iterations);
    QByteArray cIv = _q_PKCS12_keygen(2, salt, passPhrase, 8, iterations);

    // prepare and encrypt data
    QByteArray plain;
    QDataStream plainStream(&plain, QIODevice::WriteOnly);
    _q_PKCS12_key(key).write(plainStream);
    QByteArray crypted = QSslKeyPrivate::encrypt(QTlsPrivate::Cipher::DesEde3Cbc,
                                                 plain, cKey, cIv);

    QList<QAsn1Element> items;
    items << QAsn1Element::fromObjectId("1.2.840.113549.1.12.10.1.2");

    // key
    QList<QAsn1Element> keyItems;
    QList<QAsn1Element> algoItems;
    algoItems << QAsn1Element::fromObjectId("1.2.840.113549.1.12.1.3");
    QList<QAsn1Element> paramItems;
    paramItems << QAsn1Element(QAsn1Element::OctetStringType, salt);
    paramItems << QAsn1Element::fromInteger(iterations);
    algoItems << QAsn1Element::fromVector(paramItems);
    keyItems << QAsn1Element::fromVector(algoItems);
    keyItems << QAsn1Element(QAsn1Element::OctetStringType, crypted);
    items << wrap(QAsn1Element::Context0Type,
                  QAsn1Element::fromVector(keyItems));

    // local key id
    QList<QAsn1Element> idItems;
    idItems << QAsn1Element::fromObjectId("1.2.840.113549.1.9.21");
    idItems << wrap(QAsn1Element::SetType,
                    QAsn1Element(QAsn1Element::OctetStringType, localKeyId));
    items << wrap(QAsn1Element::SetType,
                  QAsn1Element::fromVector(idItems));

    // dump
    QAsn1Element root = wrap(QAsn1Element::SequenceType, QAsn1Element::fromVector(items));
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    root.write(stream);
    return ba;
}

static QByteArray _q_PKCS12_bag(const QList<QSslCertificate> &certs, const QSslKey &key, const QString &passPhrase)
{
    QList<QAsn1Element> items;

    // certs
    for (int i = 0; i < certs.size(); ++i)
        items << _q_PKCS7_data(_q_PKCS12_certBag(certs[i]));

    // key
    if (!key.isNull()) {
        const QByteArray localKeyId = certs.first().digest(QCryptographicHash::Sha1);
        items << _q_PKCS7_data(_q_PKCS12_shroudedKeyBag(key, passPhrase, localKeyId));
    }

    // dump
    QAsn1Element root = QAsn1Element::fromVector(items);
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    root.write(stream);
    return ba;
}

static QAsn1Element _q_PKCS12_mac(const QByteArray &data, const QString &passPhrase)
{
    const int iterations = 2048;

    // salt generation
    QByteArray macSalt = _q_PKCS12_salt();
    QByteArray key = _q_PKCS12_keygen(3, macSalt, passPhrase, 20, iterations);

    // HMAC calculation
    QMessageAuthenticationCode hmac(QCryptographicHash::Sha1, key);
    hmac.addData(data);

    QList<QAsn1Element> algoItems;
    algoItems << QAsn1Element::fromObjectId("1.3.14.3.2.26");
    algoItems << QAsn1Element(QAsn1Element::NullType);

    QList<QAsn1Element> digestItems;
    digestItems << QAsn1Element::fromVector(algoItems);
    digestItems << QAsn1Element(QAsn1Element::OctetStringType, hmac.result());

    QList<QAsn1Element> macItems;
    macItems << QAsn1Element::fromVector(digestItems);
    macItems << QAsn1Element(QAsn1Element::OctetStringType, macSalt);
    macItems << QAsn1Element::fromInteger(iterations);
    return QAsn1Element::fromVector(macItems);
}

QByteArray _q_makePkcs12(const QList<QSslCertificate> &certs, const QSslKey &key, const QString &passPhrase)
{
    QList<QAsn1Element> items;

    // version
    items << QAsn1Element::fromInteger(3);

    // auth safe
    const QByteArray data = _q_PKCS12_bag(certs, key, passPhrase);
    items << _q_PKCS7_data(data);

    // HMAC
    items << _q_PKCS12_mac(data, passPhrase);

    // dump
    QAsn1Element root = QAsn1Element::fromVector(items);
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    root.write(stream);
    return ba;
}

QT_END_NAMESPACE
