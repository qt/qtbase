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

#include "qsslsocket_openssl_symbols_p.h"
#include "qtlskey_openssl_p.h"

#include <QtNetwork/private/qsslkey_p.h>

#include <QtNetwork/qsslsocket.h>

#include <QtCore/qscopeguard.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

void TlsKeyOpenSSL::decodeDer(QSsl::KeyType type, QSsl::KeyAlgorithm algorithm, const QByteArray &der,
                              const QByteArray &passPhrase, bool deepClear)
{
    if (der.isEmpty())
        return;

    keyType = type;
    keyAlgorithm = algorithm;

    QMap<QByteArray, QByteArray> headers;
    const auto pem = pemFromDer(der, headers);

    decodePem(type, algorithm, pem, passPhrase, deepClear);
}

void TlsKeyOpenSSL::decodePem(KeyType type, KeyAlgorithm algorithm, const QByteArray &pem,
                              const QByteArray &passPhrase, bool deepClear)
{
    if (pem.isEmpty())
        return;

    keyType = type;
    keyAlgorithm = algorithm;

    clear(deepClear);

    BIO *bio = q_BIO_new_mem_buf(const_cast<char *>(pem.data()), pem.size());
    if (!bio)
        return;

    const auto bioRaii = qScopeGuard([bio]{q_BIO_free(bio);});

    void *phrase = const_cast<char *>(passPhrase.data());

    if (algorithm == QSsl::Rsa) {
        RSA *result = (type == QSsl::PublicKey)
            ? q_PEM_read_bio_RSA_PUBKEY(bio, &rsa, nullptr, phrase)
            : q_PEM_read_bio_RSAPrivateKey(bio, &rsa, nullptr, phrase);
        if (rsa && rsa == result)
            keyIsNull = false;
    } else if (algorithm == QSsl::Dsa) {
        DSA *result = (type == QSsl::PublicKey)
            ? q_PEM_read_bio_DSA_PUBKEY(bio, &dsa, nullptr, phrase)
            : q_PEM_read_bio_DSAPrivateKey(bio, &dsa, nullptr, phrase);
        if (dsa && dsa == result)
            keyIsNull = false;
    } else if (algorithm == QSsl::Dh) {
        EVP_PKEY *result = (type == QSsl::PublicKey)
            ? q_PEM_read_bio_PUBKEY(bio, nullptr, nullptr, phrase)
            : q_PEM_read_bio_PrivateKey(bio, nullptr, nullptr, phrase);
        if (result)
            dh = q_EVP_PKEY_get1_DH(result);
        if (dh)
            keyIsNull = false;
        q_EVP_PKEY_free(result);
#ifndef OPENSSL_NO_EC
    } else if (algorithm == QSsl::Ec) {
        EC_KEY *result = (type == QSsl::PublicKey)
            ? q_PEM_read_bio_EC_PUBKEY(bio, &ec, nullptr, phrase)
            : q_PEM_read_bio_ECPrivateKey(bio, &ec, nullptr, phrase);
        if (ec && ec == result)
            keyIsNull = false;
#endif
    }
}

QByteArray TlsKeyOpenSSL::derFromPem(const QByteArray &pem, QMap<QByteArray, QByteArray> *headers) const
{
    QByteArray header = pemHeader();
    QByteArray footer = pemFooter();

    QByteArray der(pem);

    int headerIndex = der.indexOf(header);
    int footerIndex = der.indexOf(footer, headerIndex + header.length());
    if (type() != QSsl::PublicKey) {
        if (headerIndex == -1 || footerIndex == -1) {
            header = pkcs8Header(true);
            footer = pkcs8Footer(true);
            headerIndex = der.indexOf(header);
            footerIndex = der.indexOf(footer, headerIndex + header.length());
        }
        if (headerIndex == -1 || footerIndex == -1) {
            header = pkcs8Header(false);
            footer = pkcs8Footer(false);
            headerIndex = der.indexOf(header);
            footerIndex = der.indexOf(footer, headerIndex + header.length());
        }
    }
    if (headerIndex == -1 || footerIndex == -1)
        return QByteArray();

    der = der.mid(headerIndex + header.size(), footerIndex - (headerIndex + header.size()));

    if (der.contains("Proc-Type:")) {
        // taken from QHttpNetworkReplyPrivate::parseHeader
        int i = 0;
        while (i < der.count()) {
            int j = der.indexOf(':', i); // field-name
            if (j == -1)
                break;
            const QByteArray field = der.mid(i, j - i).trimmed();
            j++;
            // any number of LWS is allowed before and after the value
            QByteArray value;
            do {
                i = der.indexOf('\n', j);
                if (i == -1)
                    break;
                if (!value.isEmpty())
                    value += ' ';
                // check if we have CRLF or only LF
                bool hasCR = (i && der[i-1] == '\r');
                int length = i -(hasCR ? 1: 0) - j;
                value += der.mid(j, length).trimmed();
                j = ++i;
            } while (i < der.count() && (der.at(i) == ' ' || der.at(i) == '\t'));
            if (i == -1)
                break; // something is wrong

            headers->insert(field, value);
        }
        der = der.mid(i);
    }

    return QByteArray::fromBase64(der); // ignores newlines
}

void TlsKeyOpenSSL::clear(bool deep)
{
    keyIsNull = true;

    if (algorithm() == QSsl::Rsa && rsa) {
        if (deep)
            q_RSA_free(rsa);
        rsa = nullptr;
    }
    if (algorithm() == QSsl::Dsa && dsa) {
        if (deep)
            q_DSA_free(dsa);
        dsa = nullptr;
    }
    if (algorithm() == QSsl::Dh && dh) {
        if (deep)
            q_DH_free(dh);
        dh = nullptr;
    }
#ifndef OPENSSL_NO_EC
    if (algorithm() == QSsl::Ec && ec) {
       if (deep)
            q_EC_KEY_free(ec);
       ec = nullptr;
    }
#endif
    if (algorithm() == QSsl::Opaque && opaque) {
        if (deep)
            q_EVP_PKEY_free(opaque);
        opaque = nullptr;
    }
}

Qt::HANDLE TlsKeyOpenSSL::handle() const
{
    switch (keyAlgorithm) {
    case QSsl::Opaque:
        return Qt::HANDLE(opaque);
    case QSsl::Rsa:
        return Qt::HANDLE(rsa);
    case QSsl::Dsa:
        return Qt::HANDLE(dsa);
    case QSsl::Dh:
        return Qt::HANDLE(dh);
#ifndef OPENSSL_NO_EC
    case QSsl::Ec:
        return Qt::HANDLE(ec);
#endif
    default:
        return Qt::HANDLE(nullptr);
    }
}

int TlsKeyOpenSSL::length() const
{
    if (isNull() || algorithm() == QSsl::Opaque)
        return -1;

    switch (algorithm()) {
    case QSsl::Rsa:
        return q_RSA_bits(rsa);
    case QSsl::Dsa:
        return q_DSA_bits(dsa);
    case QSsl::Dh:
        return q_DH_bits(dh);
#ifndef OPENSSL_NO_EC
    case QSsl::Ec:
        return q_EC_GROUP_get_degree(q_EC_KEY_get0_group(ec));
#endif
    default:
        return -1;
    }
}

QByteArray TlsKeyOpenSSL::toPem(const QByteArray &passPhrase) const
{
    if (!QSslSocket::supportsSsl() || isNull() || algorithm() == QSsl::Opaque)
        return {};

    const EVP_CIPHER *cipher = nullptr;
    if (type() == QSsl::PrivateKey && !passPhrase.isEmpty()) {
#ifndef OPENSSL_NO_DES
        cipher = q_EVP_des_ede3_cbc();
#else
        return {};
#endif
    }

    BIO *bio = q_BIO_new(q_BIO_s_mem());
    if (!bio)
        return {};

    const auto bioRaii = qScopeGuard([bio]{q_BIO_free(bio);});

    bool fail = false;

    if (algorithm() == QSsl::Rsa) {
        if (type() == QSsl::PublicKey) {
            if (!q_PEM_write_bio_RSA_PUBKEY(bio, rsa))
                fail = true;
        } else {
            if (!q_PEM_write_bio_RSAPrivateKey(
                    bio, rsa, cipher, (uchar *)passPhrase.data(),
                    passPhrase.size(), nullptr, nullptr)) {
                fail = true;
            }
        }
    } else if (algorithm() == QSsl::Dsa) {
        if (type() == QSsl::PublicKey) {
            if (!q_PEM_write_bio_DSA_PUBKEY(bio, dsa))
                fail = true;
        } else {
            if (!q_PEM_write_bio_DSAPrivateKey(
                    bio, dsa, cipher, (uchar *)passPhrase.data(),
                    passPhrase.size(), nullptr, nullptr)) {
                fail = true;
            }
        }
    } else if (algorithm() == QSsl::Dh) {
        EVP_PKEY *result = q_EVP_PKEY_new();
        if (!result || !q_EVP_PKEY_set1_DH(result, dh)) {
            fail = true;
        } else if (type() == QSsl::PublicKey) {
            if (!q_PEM_write_bio_PUBKEY(bio, result))
                fail = true;
        } else if (!q_PEM_write_bio_PrivateKey(
                bio, result, cipher, (uchar *)passPhrase.data(),
                passPhrase.size(), nullptr, nullptr)) {
            fail = true;
        }
        q_EVP_PKEY_free(result);
#ifndef OPENSSL_NO_EC
    } else if (algorithm() == QSsl::Ec) {
        if (type() == QSsl::PublicKey) {
            if (!q_PEM_write_bio_EC_PUBKEY(bio, ec))
                fail = true;
        } else {
            if (!q_PEM_write_bio_ECPrivateKey(
                    bio, ec, cipher, (uchar *)passPhrase.data(),
                    passPhrase.size(), nullptr, nullptr)) {
                fail = true;
            }
        }
#endif
    } else {
        fail = true;
    }

    QByteArray pem;
    if (!fail) {
        char *data = nullptr;
        const long size = q_BIO_get_mem_data(bio, &data);
        if (size > 0 && data)
            pem = QByteArray(data, size);
    }

    return pem;
}

void TlsKeyOpenSSL::fromHandle(Qt::HANDLE handle, QSsl::KeyType expectedType)
{
    EVP_PKEY *evpKey = reinterpret_cast<EVP_PKEY *>(handle);
    if (!evpKey || !fromEVP_PKEY(evpKey)) {
        opaque = evpKey;
        keyAlgorithm = QSsl::Opaque;
    } else {
        q_EVP_PKEY_free(evpKey);
    }

    keyType = expectedType;
    keyIsNull = !opaque;
}

bool TlsKeyOpenSSL::fromEVP_PKEY(EVP_PKEY *pkey)
{
    if (!pkey)
        return false;

    switch (q_EVP_PKEY_type(q_EVP_PKEY_base_id(pkey))) {
    case EVP_PKEY_RSA:
        keyIsNull = false;
        keyAlgorithm = QSsl::Rsa;
        keyType = QSsl::PrivateKey;
        rsa = q_EVP_PKEY_get1_RSA(pkey);

        return true;
    case EVP_PKEY_DSA:
        keyIsNull = false;
        keyAlgorithm = QSsl::Dsa;
        keyType = QSsl::PrivateKey;
        dsa = q_EVP_PKEY_get1_DSA(pkey);

        return true;
    case EVP_PKEY_DH:
        keyIsNull = false;
        keyAlgorithm = QSsl::Dh;
        keyType = QSsl::PrivateKey;
        dh = q_EVP_PKEY_get1_DH(pkey);
        return true;
#ifndef OPENSSL_NO_EC
    case EVP_PKEY_EC:
        keyIsNull = false;
        keyAlgorithm = QSsl::Ec;
        keyType = QSsl::PrivateKey;
        ec = q_EVP_PKEY_get1_EC_KEY(pkey);

        return true;
#endif
    default:;
        // Unknown key type. This could be handled as opaque, but then
        // we'd eventually leak memory since we wouldn't be able to free
        // the underlying EVP_PKEY structure. For now, we won't support
        // this.
    }

    return false;
}

QByteArray doCrypt(QSslKeyPrivate::Cipher cipher, const QByteArray &data,
                   const QByteArray &key, const QByteArray &iv, bool enc)
{
    const EVP_CIPHER *type = nullptr;
    int i = 0, len = 0;

    switch (cipher) {
    case Cipher::DesCbc:
#ifndef OPENSSL_NO_DES
        type = q_EVP_des_cbc();
#endif
        break;
    case Cipher::DesEde3Cbc:
#ifndef OPENSSL_NO_DES
        type = q_EVP_des_ede3_cbc();
#endif
        break;
    case Cipher::Rc2Cbc:
#ifndef OPENSSL_NO_RC2
        type = q_EVP_rc2_cbc();
#endif
        break;
    case Cipher::Aes128Cbc:
        type = q_EVP_aes_128_cbc();
        break;
    case Cipher::Aes192Cbc:
        type = q_EVP_aes_192_cbc();
        break;
    case Cipher::Aes256Cbc:
        type = q_EVP_aes_256_cbc();
        break;
    }

    if (type == nullptr)
        return {};

    QByteArray output;
    output.resize(data.size() + EVP_MAX_BLOCK_LENGTH);

    EVP_CIPHER_CTX *ctx = q_EVP_CIPHER_CTX_new();
    q_EVP_CIPHER_CTX_reset(ctx);
    q_EVP_CipherInit(ctx, type, nullptr, nullptr, enc);
    q_EVP_CIPHER_CTX_set_key_length(ctx, key.size());
    if (cipher == Cipher::Rc2Cbc)
        q_EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_SET_RC2_KEY_BITS, 8 * key.size(), nullptr);

    q_EVP_CipherInit_ex(ctx, nullptr, nullptr,
                        reinterpret_cast<const unsigned char *>(key.constData()),
                        reinterpret_cast<const unsigned char *>(iv.constData()),
                        enc);
    q_EVP_CipherUpdate(ctx,
        reinterpret_cast<unsigned char *>(output.data()), &len,
        reinterpret_cast<const unsigned char *>(data.constData()), data.size());
    q_EVP_CipherFinal(ctx,
        reinterpret_cast<unsigned char *>(output.data()) + len, &i);
    len += i;

    q_EVP_CIPHER_CTX_reset(ctx);
    q_EVP_CIPHER_CTX_free(ctx);

    return output.left(len);
}

QByteArray TlsKeyOpenSSL::decrypt(Cipher cipher, const QByteArray &data,
                                  const QByteArray &key, const QByteArray &iv) const
{
    return doCrypt(cipher, data, key, iv, false);
}

QByteArray TlsKeyOpenSSL::encrypt(Cipher cipher, const QByteArray &data,
                                  const QByteArray &key, const QByteArray &iv) const
{
    return doCrypt(cipher, data, key, iv, true);
}

TlsKeyOpenSSL *TlsKeyOpenSSL::publicKeyFromX509(X509 *x)
{
    TlsKeyOpenSSL *tlsKey = new TlsKeyOpenSSL;
    std::unique_ptr<TlsKeyOpenSSL> keyRaii(tlsKey);

    tlsKey->keyType = QSsl::PublicKey;

    EVP_PKEY *pkey = q_X509_get_pubkey(x);
    Q_ASSERT(pkey);
    const int keyType = q_EVP_PKEY_type(q_EVP_PKEY_base_id(pkey));

    if (keyType == EVP_PKEY_RSA) {
        tlsKey->rsa = q_EVP_PKEY_get1_RSA(pkey);
        tlsKey->keyAlgorithm = QSsl::Rsa;
        tlsKey->keyIsNull = false;
    } else if (keyType == EVP_PKEY_DSA) {
        tlsKey->dsa = q_EVP_PKEY_get1_DSA(pkey);
        tlsKey->keyAlgorithm = QSsl::Dsa;
        tlsKey->keyIsNull = false;
#ifndef OPENSSL_NO_EC
    } else if (keyType == EVP_PKEY_EC) {
        tlsKey->ec = q_EVP_PKEY_get1_EC_KEY(pkey);
        tlsKey->keyAlgorithm = QSsl::Ec;
        tlsKey->keyIsNull = false;
#endif
    } else if (keyType == EVP_PKEY_DH) {
        // DH unsupported (key is null)
    } else {
        // error? (key is null)
    }

    q_EVP_PKEY_free(pkey);
    return keyRaii.release();
}

} // namespace QTlsPrivate

QT_END_NAMESPACE
