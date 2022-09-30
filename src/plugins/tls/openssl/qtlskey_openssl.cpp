// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsslsocket_openssl_symbols_p.h"
#include "qtlsbackend_openssl_p.h"
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

#ifdef OPENSSL_NO_DEPRECATED_3_0
    if (type == QSsl::PublicKey)
        genericKey = q_PEM_read_bio_PUBKEY(bio, nullptr, nullptr, phrase);
    else
        genericKey = q_PEM_read_bio_PrivateKey(bio, nullptr, nullptr, phrase);
    keyIsNull = !genericKey;
    if (keyIsNull)
        QTlsBackendOpenSSL::logAndClearErrorQueue();
#else

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
#endif // OPENSSL_NO_EC
    }

#endif // OPENSSL_NO_DEPRECATED_3_0
}

QByteArray TlsKeyOpenSSL::derFromPem(const QByteArray &pem, QMap<QByteArray, QByteArray> *headers) const
{
    QByteArray header = pemHeader();
    QByteArray footer = pemFooter();

    QByteArray der(pem);

    int headerIndex = der.indexOf(header);
    int footerIndex = der.indexOf(footer, headerIndex + header.size());
    if (type() != QSsl::PublicKey) {
        if (headerIndex == -1 || footerIndex == -1) {
            header = pkcs8Header(true);
            footer = pkcs8Footer(true);
            headerIndex = der.indexOf(header);
            footerIndex = der.indexOf(footer, headerIndex + header.size());
        }
        if (headerIndex == -1 || footerIndex == -1) {
            header = pkcs8Header(false);
            footer = pkcs8Footer(false);
            headerIndex = der.indexOf(header);
            footerIndex = der.indexOf(footer, headerIndex + header.size());
        }
    }
    if (headerIndex == -1 || footerIndex == -1)
        return QByteArray();

    der = der.mid(headerIndex + header.size(), footerIndex - (headerIndex + header.size()));

    if (der.contains("Proc-Type:")) {
        // taken from QHttpNetworkReplyPrivate::parseHeader
        int i = 0;
        while (i < der.size()) {
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
            } while (i < der.size() && (der.at(i) == ' ' || der.at(i) == '\t'));
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

#ifndef OPENSSL_NO_DEPRECATED_3_0
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
#endif // OPENSSL_NO_DEPRECATED_3_0

    if (algorithm() == QSsl::Opaque && opaque) {
        if (deep)
            q_EVP_PKEY_free(opaque);
        opaque = nullptr;
    }

    if (genericKey) {
        // None of the above cleared it. genericKey is either
        // initialised by PEM read operation, or from X509, and
        // we are the owners and not sharing. So we free it.
        q_EVP_PKEY_free(genericKey);
        genericKey = nullptr;
    }
}

Qt::HANDLE TlsKeyOpenSSL::handle() const
{
    if (keyAlgorithm == QSsl::Opaque)
        return Qt::HANDLE(opaque);

#ifndef OPENSSL_NO_DEPRECATED_3_0
    switch (keyAlgorithm) {
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
#else
    qCWarning(lcTlsBackend,
              "This version of OpenSSL disabled direct manipulation with RSA/DSA/DH/EC_KEY structures, consider using QSsl::Opaque instead.");
    return Qt::HANDLE(nullptr);
#endif
}

int TlsKeyOpenSSL::length() const
{
    if (isNull() || algorithm() == QSsl::Opaque)
        return -1;

#ifndef OPENSSL_NO_DEPRECATED_3_0
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
#else // OPENSSL_NO_DEPRECATED_3_0
    Q_ASSERT(genericKey);
    return q_EVP_PKEY_get_bits(genericKey);
#endif // OPENSSL_NO_DEPRECATED_3_0
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

#ifndef OPENSSL_NO_DEPRECATED_3_0

#define write_pubkey(alg, key) q_PEM_write_bio_##alg##_PUBKEY(bio, key)
#define write_privatekey(alg, key) \
        q_PEM_write_bio_##alg##PrivateKey(bio, key, cipher, (uchar *)passPhrase.data(), \
        passPhrase.size(), nullptr, nullptr)

#else

#define write_pubkey(alg, key) q_PEM_write_bio_PUBKEY(bio, genericKey)
#define write_privatekey(alg, key) \
    q_PEM_write_bio_PrivateKey_traditional(bio, genericKey, cipher, (uchar *)passPhrase.data(), passPhrase.size(), nullptr, nullptr)

#endif // OPENSSL_NO_DEPRECATED_3_0

    bool fail = false;
    if (algorithm() == QSsl::Rsa) {
        if (type() == QSsl::PublicKey) {
            if (!write_pubkey(RSA, rsa))
                fail = true;
        } else if (!write_privatekey(RSA, rsa)) {
            fail = true;
        }
    } else if (algorithm() == QSsl::Dsa) {
        if (type() == QSsl::PublicKey) {
            if (!write_pubkey(DSA, dsa))
                fail = true;
        } else if (!write_privatekey(DSA, dsa)) {
            fail = true;
        }
    } else if (algorithm() == QSsl::Dh) {
#ifdef OPENSSL_NO_DEPRECATED_3_0
        EVP_PKEY *result = genericKey;
#else
        EVP_PKEY *result = q_EVP_PKEY_new();
        const auto guard = qScopeGuard([result]{if (result) q_EVP_PKEY_free(result);});
        if (!result || !q_EVP_PKEY_set1_DH(result, dh)) {
            fail = true;
        } else
#endif
        if (type() == QSsl::PublicKey) {
            if (!q_PEM_write_bio_PUBKEY(bio, result))
                fail = true;
        } else if (!q_PEM_write_bio_PrivateKey(bio, result, cipher, (uchar *)passPhrase.data(),
                                               passPhrase.size(), nullptr, nullptr)) {
            fail = true;
        }
#ifndef OPENSSL_NO_EC
    } else if (algorithm() == QSsl::Ec) {
        if (type() == QSsl::PublicKey) {
            if (!write_pubkey(EC, ec))
                fail = true;
        } else {
            if (!write_privatekey(EC, ec))
                fail = true;
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
    } else {
        QTlsBackendOpenSSL::logAndClearErrorQueue();
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

#ifndef  OPENSSL_NO_DEPRECATED_3_0
#define get_key(key, alg) key = q_EVP_PKEY_get1_##alg(pkey)
#else
#define get_key(key, alg) q_EVP_PKEY_up_ref(pkey); genericKey = pkey;
#endif

    switch (q_EVP_PKEY_type(q_EVP_PKEY_base_id(pkey))) {
    case EVP_PKEY_RSA:
        keyIsNull = false;
        keyAlgorithm = QSsl::Rsa;
        keyType = QSsl::PrivateKey;
        get_key(rsa, RSA);
        return true;
    case EVP_PKEY_DSA:
        keyIsNull = false;
        keyAlgorithm = QSsl::Dsa;
        keyType = QSsl::PrivateKey;
        get_key(dsa, DSA);
        return true;
    case EVP_PKEY_DH:
        keyIsNull = false;
        keyAlgorithm = QSsl::Dh;
        keyType = QSsl::PrivateKey;
        get_key(dh, DH);
        return true;
#ifndef OPENSSL_NO_EC
    case EVP_PKEY_EC:
        keyIsNull = false;
        keyAlgorithm = QSsl::Ec;
        keyType = QSsl::PrivateKey;
        get_key(ec, EC_KEY);
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
    if (q_EVP_CipherInit(ctx, type, nullptr, nullptr, enc) != 1) {
        q_EVP_CIPHER_CTX_free(ctx);
        QTlsBackendOpenSSL::logAndClearErrorQueue();
        return {};
    }

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

#ifndef OPENSSL_NO_DEPRECATED_3_0

#define get_pubkey(keyName, alg) tlsKey->keyName = q_EVP_PKEY_get1_##alg(pkey)

#else

#define get_pubkey(a, b) tlsKey->genericKey = pkey

#endif

    EVP_PKEY *pkey = q_X509_get_pubkey(x);
    Q_ASSERT(pkey);
    const int keyType = q_EVP_PKEY_type(q_EVP_PKEY_base_id(pkey));

    if (keyType == EVP_PKEY_RSA) {
        get_pubkey(rsa, RSA);
        tlsKey->keyAlgorithm = QSsl::Rsa;
        tlsKey->keyIsNull = false;
    } else if (keyType == EVP_PKEY_DSA) {
        get_pubkey(dsa, DSA);
        tlsKey->keyAlgorithm = QSsl::Dsa;
        tlsKey->keyIsNull = false;
#ifndef OPENSSL_NO_EC
    } else if (keyType == EVP_PKEY_EC) {
        get_pubkey(ec, EC_KEY);
        tlsKey->keyAlgorithm = QSsl::Ec;
        tlsKey->keyIsNull = false;
#endif
    } else if (keyType == EVP_PKEY_DH) {
        // DH unsupported (key is null)
    } else {
        // error? (key is null)
    }

#ifndef OPENSSL_NO_DEPRECATED_3_0
    q_EVP_PKEY_free(pkey);
#endif

    return keyRaii.release();
}

} // namespace QTlsPrivate

QT_END_NAMESPACE
