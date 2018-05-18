/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2016 Richard J. Moore <rich@kde.org>
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


#include "qsslkey.h"
#include "qsslkey_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslsocket.h"
#include "qsslsocket_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qiodevice.h>
#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

QT_BEGIN_NAMESPACE

void QSslKeyPrivate::clear(bool deep)
{
    isNull = true;
    if (!QSslSocket::supportsSsl())
        return;
    if (algorithm == QSsl::Rsa && rsa) {
        if (deep)
            q_RSA_free(rsa);
        rsa = 0;
    }
    if (algorithm == QSsl::Dsa && dsa) {
        if (deep)
            q_DSA_free(dsa);
        dsa = 0;
    }
#ifndef OPENSSL_NO_EC
    if (algorithm == QSsl::Ec && ec) {
       if (deep)
            q_EC_KEY_free(ec);
       ec = 0;
    }
#endif
    if (algorithm == QSsl::Opaque && opaque) {
        if (deep)
            q_EVP_PKEY_free(opaque);
        opaque = 0;
    }
}

bool QSslKeyPrivate::fromEVP_PKEY(EVP_PKEY *pkey)
{
    if (pkey == nullptr)
        return false;

#if QT_CONFIG(opensslv11)
    const int keyType = q_EVP_PKEY_type(q_EVP_PKEY_base_id(pkey));
#else
    const int keyType = pkey->type;
#endif
    if (keyType == EVP_PKEY_RSA) {
        isNull = false;
        algorithm = QSsl::Rsa;
        type = QSsl::PrivateKey;
        rsa = q_EVP_PKEY_get1_RSA(pkey);
        return true;
    } else if (keyType == EVP_PKEY_DSA) {
        isNull = false;
        algorithm = QSsl::Dsa;
        type = QSsl::PrivateKey;
        dsa = q_EVP_PKEY_get1_DSA(pkey);
        return true;
    }
#ifndef OPENSSL_NO_EC
    else if (keyType == EVP_PKEY_EC) {
        isNull = false;
        algorithm = QSsl::Ec;
        type = QSsl::PrivateKey;
        ec = q_EVP_PKEY_get1_EC_KEY(pkey);
        return true;
    }
#endif
    else {
        // Unknown key type. This could be handled as opaque, but then
        // we'd eventually leak memory since we wouldn't be able to free
        // the underlying EVP_PKEY structure. For now, we won't support
        // this.
    }

    return false;
}

void QSslKeyPrivate::decodeDer(const QByteArray &der, bool deepClear)
{
    QMap<QByteArray, QByteArray> headers;
    decodePem(pemFromDer(der, headers), QByteArray(), deepClear);
}

void QSslKeyPrivate::decodePem(const QByteArray &pem, const QByteArray &passPhrase,
                               bool deepClear)
{
    if (pem.isEmpty())
        return;

    clear(deepClear);

    if (!QSslSocket::supportsSsl())
        return;

    BIO *bio = q_BIO_new_mem_buf(const_cast<char *>(pem.data()), pem.size());
    if (!bio)
        return;

    void *phrase = const_cast<char *>(passPhrase.constData());

    if (algorithm == QSsl::Rsa) {
        RSA *result = (type == QSsl::PublicKey)
            ? q_PEM_read_bio_RSA_PUBKEY(bio, &rsa, 0, phrase)
            : q_PEM_read_bio_RSAPrivateKey(bio, &rsa, 0, phrase);
        if (rsa && rsa == result)
            isNull = false;
    } else if (algorithm == QSsl::Dsa) {
        DSA *result = (type == QSsl::PublicKey)
            ? q_PEM_read_bio_DSA_PUBKEY(bio, &dsa, 0, phrase)
            : q_PEM_read_bio_DSAPrivateKey(bio, &dsa, 0, phrase);
        if (dsa && dsa == result)
            isNull = false;
#ifndef OPENSSL_NO_EC
    } else if (algorithm == QSsl::Ec) {
        EC_KEY *result = (type == QSsl::PublicKey)
            ? q_PEM_read_bio_EC_PUBKEY(bio, &ec, 0, phrase)
            : q_PEM_read_bio_ECPrivateKey(bio, &ec, 0, phrase);
        if (ec && ec == result)
            isNull = false;
#endif
    }

    q_BIO_free(bio);
}

int QSslKeyPrivate::length() const
{
    if (isNull || algorithm == QSsl::Opaque)
        return -1;

    switch (algorithm) {
        case QSsl::Rsa: return q_RSA_bits(rsa);
        case QSsl::Dsa: return q_DSA_bits(dsa);
#ifndef OPENSSL_NO_EC
        case QSsl::Ec: return q_EC_GROUP_get_degree(q_EC_KEY_get0_group(ec));
#endif
        default: return -1;
    }
}

QByteArray QSslKeyPrivate::toPem(const QByteArray &passPhrase) const
{
    if (!QSslSocket::supportsSsl() || isNull || algorithm == QSsl::Opaque)
        return QByteArray();

    // ### the cipher should be selectable in the API:
    const EVP_CIPHER *cipher = nullptr;
    if (type == QSsl::PrivateKey && !passPhrase.isEmpty()) {
#ifndef OPENSSL_NO_DES
        cipher = q_EVP_des_ede3_cbc();
#else
        return QByteArray();
#endif
    }

    BIO *bio = q_BIO_new(q_BIO_s_mem());
    if (!bio)
        return QByteArray();

    bool fail = false;

    if (algorithm == QSsl::Rsa) {
        if (type == QSsl::PublicKey) {
            if (!q_PEM_write_bio_RSA_PUBKEY(bio, rsa))
                fail = true;
        } else {
            if (!q_PEM_write_bio_RSAPrivateKey(
                    bio, rsa, cipher,
                    const_cast<uchar *>((const uchar *)passPhrase.data()), passPhrase.size(), 0, 0)) {
                fail = true;
            }
        }
    } else if (algorithm == QSsl::Dsa) {
        if (type == QSsl::PublicKey) {
            if (!q_PEM_write_bio_DSA_PUBKEY(bio, dsa))
                fail = true;
        } else {
            if (!q_PEM_write_bio_DSAPrivateKey(
                    bio, dsa, cipher,
                    const_cast<uchar *>((const uchar *)passPhrase.data()), passPhrase.size(), 0, 0)) {
                fail = true;
            }
        }
#ifndef OPENSSL_NO_EC
    } else if (algorithm == QSsl::Ec) {
        if (type == QSsl::PublicKey) {
            if (!q_PEM_write_bio_EC_PUBKEY(bio, ec))
                fail = true;
        } else {
            if (!q_PEM_write_bio_ECPrivateKey(
                    bio, ec, cipher,
                    const_cast<uchar *>((const uchar *)passPhrase.data()), passPhrase.size(), 0, 0)) {
                fail = true;
            }
        }
#endif
    } else {
        fail = true;
    }

    QByteArray pem;
    if (!fail) {
        char *data;
        long size = q_BIO_get_mem_data(bio, &data);
        pem = QByteArray(data, size);
    }
    q_BIO_free(bio);
    return pem;
}

Qt::HANDLE QSslKeyPrivate::handle() const
{
    switch (algorithm) {
    case QSsl::Opaque:
        return Qt::HANDLE(opaque);
    case QSsl::Rsa:
        return Qt::HANDLE(rsa);
    case QSsl::Dsa:
        return Qt::HANDLE(dsa);
#ifndef OPENSSL_NO_EC
    case QSsl::Ec:
        return Qt::HANDLE(ec);
#endif
    default:
        return Qt::HANDLE(NULL);
    }
}

static QByteArray doCrypt(QSslKeyPrivate::Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv, int enc)
{
    const EVP_CIPHER* type = 0;
    int i = 0, len = 0;

    switch (cipher) {
    case QSslKeyPrivate::DesCbc:
#ifndef OPENSSL_NO_DES
        type = q_EVP_des_cbc();
#endif
        break;
    case QSslKeyPrivate::DesEde3Cbc:
#ifndef OPENSSL_NO_DES
        type = q_EVP_des_ede3_cbc();
#endif
        break;
    case QSslKeyPrivate::Rc2Cbc:
#ifndef OPENSSL_NO_RC2
        type = q_EVP_rc2_cbc();
#endif
        break;
    }

    if (type == nullptr)
        return QByteArray();

    QByteArray output;
    output.resize(data.size() + EVP_MAX_BLOCK_LENGTH);

#if QT_CONFIG(opensslv11)
    EVP_CIPHER_CTX *ctx = q_EVP_CIPHER_CTX_new();
    q_EVP_CIPHER_CTX_reset(ctx);
#else
    EVP_CIPHER_CTX evpCipherContext;
    EVP_CIPHER_CTX *ctx = &evpCipherContext;
    q_EVP_CIPHER_CTX_init(ctx);
#endif

    q_EVP_CipherInit(ctx, type, NULL, NULL, enc);
    q_EVP_CIPHER_CTX_set_key_length(ctx, key.size());
    if (cipher == QSslKeyPrivate::Rc2Cbc)
        q_EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_SET_RC2_KEY_BITS, 8 * key.size(), NULL);

#if QT_CONFIG(opensslv11)
    // EVP_CipherInit in 1.1 resets the context thus making the calls above useless.
    // We call EVP_CipherInit_ex instead.
    q_EVP_CipherInit_ex(ctx, nullptr, nullptr,
                        reinterpret_cast<const unsigned char *>(key.constData()),
                        reinterpret_cast<const unsigned char *>(iv.constData()),
                        enc);
#else
    q_EVP_CipherInit(ctx, NULL,
        reinterpret_cast<const unsigned char *>(key.constData()),
        reinterpret_cast<const unsigned char *>(iv.constData()), enc);
#endif // opensslv11

    q_EVP_CipherUpdate(ctx,
        reinterpret_cast<unsigned char *>(output.data()), &len,
        reinterpret_cast<const unsigned char *>(data.constData()), data.size());
    q_EVP_CipherFinal(ctx,
        reinterpret_cast<unsigned char *>(output.data()) + len, &i);
    len += i;

#if QT_CONFIG(opensslv11)
    q_EVP_CIPHER_CTX_reset(ctx);
    q_EVP_CIPHER_CTX_free(ctx);
#else
    q_EVP_CIPHER_CTX_cleanup(ctx);
#endif

    return output.left(len);
}

QByteArray QSslKeyPrivate::decrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    return doCrypt(cipher, data, key, iv, 0);
}

QByteArray QSslKeyPrivate::encrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    return doCrypt(cipher, data, key, iv, 1);
}

QT_END_NAMESPACE
