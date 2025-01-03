// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTLSKEY_OPENSSL_H
#define QTLSKEY_OPENSSL_H

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

#include "../shared/qtlskey_base_p.h"

#include <QtNetwork/private/qtlsbackend_p.h>
#include <QtNetwork/private/qsslkey_p.h>

#include <QtNetwork/qssl.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qglobal.h>

#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/dh.h>

#ifdef OPENSSL_NO_DEPRECATED_3_0
typedef struct evp_pkey_st EVP_PKEY;
typedef struct dsa_st DSA;
typedef struct rsa_st RSA;
typedef struct dh_st DH;
typedef struct ec_key_st EC_KEY;
#endif // OPENSSL_NO_DEPRECATED_3_0

QT_BEGIN_NAMESPACE

QT_REQUIRE_CONFIG(ssl);

namespace QTlsPrivate {

class TlsKeyOpenSSL final : public TlsKeyBase
{
public:
    TlsKeyOpenSSL()
        : opaque(nullptr)
    {
        clear(false);
    }
    ~TlsKeyOpenSSL()
    {
        clear(true);
    }

    void decodeDer(KeyType type, KeyAlgorithm algorithm, const QByteArray &der,
                   const QByteArray &passPhrase, bool deepClear) override;
    void decodePem(KeyType type, KeyAlgorithm algorithm, const QByteArray &pem,
                   const QByteArray &passPhrase, bool deepClear) override;

    QByteArray toPem(const QByteArray &passPhrase) const override;
    QByteArray derFromPem(const QByteArray &pem, QMap<QByteArray, QByteArray> *headers) const override;

    void fromHandle(Qt::HANDLE opaque, KeyType expectedType) override;

    void clear(bool deep) override;
    Qt::HANDLE handle() const override;
    int length() const override;

    QByteArray decrypt(Cipher cipher, const QByteArray &data,
                       const QByteArray &key, const QByteArray &iv) const override;
    QByteArray encrypt(Cipher cipher, const QByteArray &data,
                       const QByteArray &key, const QByteArray &iv) const override;

    static TlsKeyOpenSSL *publicKeyFromX509(X509 *x);

    union {
        EVP_PKEY *opaque;
        RSA *rsa;
        DSA *dsa;
        DH *dh;
#ifndef OPENSSL_NO_EC
        EC_KEY *ec;
#endif
        EVP_PKEY *genericKey;
    };

    bool fromEVP_PKEY(EVP_PKEY *pkey);
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLSKEY_OPENSSL_H
