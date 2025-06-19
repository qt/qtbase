// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:cryptography

#include "qpassworddigestor.h"

#include <QtCore/QDebug>
#include <QtCore/QMessageAuthenticationCode>
#include <QtCore/QtEndian>
#include <QtCore/QList>

#include "qtcore-config_p.h"

#include <limits>

#if QT_CONFIG(opensslv30) && QT_CONFIG(openssl_linked)
#define USING_OPENSSL30
#include <openssl/core_names.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/provider.h>
#endif

QT_BEGIN_NAMESPACE
namespace QPasswordDigestor {

/*!
    \namespace QPasswordDigestor
    \inmodule QtNetwork

    \brief The QPasswordDigestor namespace contains functions which you can use
    to generate hashes or keys.
*/

/*!
    \since 5.12

    Returns a hash computed using the PBKDF1-algorithm as defined in
    \l {RFC 8018, section 5.1}.

    The function takes the \a data and \a salt, and then hashes it repeatedly
    for \a iterations iterations using the specified hash \a algorithm. If the
    resulting hash is longer than \a dkLen then it is truncated before it is
    returned.

    This function only supports SHA-1 and MD5! The max output size is 160 bits
    (20 bytes) when using SHA-1, or 128 bits (16 bytes) when using MD5.
    Specifying a value for \a dkLen which is greater than this results in a
    warning and an empty QByteArray is returned. To programmatically check this
    limit you can use \l {QCryptographicHash::hashLength}. Furthermore: the
    \a salt must always be 8 bytes long!

    \note This function is provided for use with legacy applications and all
    new applications are recommended to use \l {deriveKeyPbkdf2} {PBKDF2}.

    \sa deriveKeyPbkdf2, QCryptographicHash, QCryptographicHash::hashLength
*/
Q_NETWORK_EXPORT QByteArray deriveKeyPbkdf1(QCryptographicHash::Algorithm algorithm,
                                            const QByteArray &data, const QByteArray &salt,
                                            int iterations, quint64 dkLen)
{
    // https://tools.ietf.org/html/rfc8018#section-5.1

    if (algorithm != QCryptographicHash::Sha1
        && algorithm != QCryptographicHash::Md5
    ) {
        qWarning("The only supported algorithms for pbkdf1 are SHA-1 and MD5!");
        return QByteArray();
    }

    if (salt.size() != 8) {
        qWarning("The salt must be 8 bytes long!");
        return QByteArray();
    }
    if (iterations < 1 || dkLen < 1)
        return QByteArray();

    if (dkLen > quint64(QCryptographicHash::hashLength(algorithm))) {
        qWarning() << "Derived key too long:\n"
                   << algorithm << "was chosen which produces output of length"
                   << QCryptographicHash::hashLength(algorithm) << "but" << dkLen
                   << "was requested.";
        return QByteArray();
    }

    QCryptographicHash hash(algorithm);
    hash.addData(data);
    hash.addData(salt);
    QByteArray key = hash.result();

    for (int i = 1; i < iterations; i++) {
        hash.reset();
        hash.addData(key);
        key = hash.result();
    }
    return key.left(dkLen);
}

#ifdef USING_OPENSSL30
// Copied from QCryptographicHashPrivate
static constexpr const char * methodToName(QCryptographicHash::Algorithm method) noexcept
{
    switch (method) {
#define CASE(Enum, Name) \
    case QCryptographicHash:: Enum : \
        return Name \
    /*end*/
    CASE(Sha1, "SHA1");
    CASE(Md4, "MD4");
    CASE(Md5, "MD5");
    CASE(Sha224, "SHA224");
    CASE(Sha256, "SHA256");
    CASE(Sha384, "SHA384");
    CASE(Sha512, "SHA512");
    CASE(RealSha3_224, "SHA3-224");
    CASE(RealSha3_256, "SHA3-256");
    CASE(RealSha3_384, "SHA3-384");
    CASE(RealSha3_512, "SHA3-512");
    CASE(Keccak_224, "SHA3-224");
    CASE(Keccak_256, "SHA3-256");
    CASE(Keccak_384, "SHA3-384");
    CASE(Keccak_512, "SHA3-512");
    CASE(Blake2b_512, "BLAKE2B512");
    CASE(Blake2s_256, "BLAKE2S256");
#undef CASE
    default: return nullptr;
    }
}

static QByteArray opensslDeriveKeyPbkdf2(QCryptographicHash::Algorithm algorithm,
                                         const QByteArray &data, const QByteArray &salt,
                                         uint64_t iterations, quint64 dkLen)
{
    EVP_KDF *kdf = EVP_KDF_fetch(nullptr, "PBKDF2", nullptr);

    if (!kdf)
        return QByteArray();

    auto cleanUpKdf = qScopeGuard([kdf] {
        EVP_KDF_free(kdf);
    });

    EVP_KDF_CTX *ctx = EVP_KDF_CTX_new(kdf);

    if (!ctx)
        return QByteArray();

    auto cleanUpCtx = qScopeGuard([ctx] {
        EVP_KDF_CTX_free(ctx);
    });

    // Do not enable SP800-132 compliance check, otherwise we will require:
    // - the iteration count is at least 1000
    // - the salt length is at least 128 bits
    // - the derived key length is at least 112 bits
    // This would be a different behavior from the original implementation.
    int checkDisabled = 1;
    QList<OSSL_PARAM> params;
    params.append(OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, const_cast<char*>(methodToName(algorithm)), 0));
    params.append(OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, const_cast<char*>(salt.data()), salt.size()));
    params.append(OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD, const_cast<char*>(data.data()), data.size()));
    params.append(OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_ITER, &iterations));
    params.append(OSSL_PARAM_construct_int(OSSL_KDF_PARAM_PKCS5, &checkDisabled));
    params.append(OSSL_PARAM_construct_end());

    if (EVP_KDF_CTX_set_params(ctx, params.data()) <= 0)
        return QByteArray();

    QByteArray derived(dkLen, '\0');

    if (!EVP_KDF_derive(ctx, reinterpret_cast<unsigned char*>(derived.data()), derived.size(), nullptr))
        return QByteArray();

    return derived;
}
#endif

/*!
    \since 5.12

    Derive a key using the PBKDF2-algorithm as defined in
    \l {RFC 8018, section 5.2}.

    This function takes the \a data and \a salt, and then applies HMAC-X, where
    the X is \a algorithm, repeatedly. It internally concatenates intermediate
    results to the final output until at least \a dkLen amount of bytes have
    been computed and it will execute HMAC-X \a iterations times each time a
    concatenation is required. The total number of times it will execute HMAC-X
    depends on \a iterations, \a dkLen and \a algorithm and can be calculated
    as
    \c{iterations * ceil(dkLen / QCryptographicHash::hashLength(algorithm))}.

    \sa deriveKeyPbkdf1, QMessageAuthenticationCode, QCryptographicHash
*/
Q_NETWORK_EXPORT QByteArray deriveKeyPbkdf2(QCryptographicHash::Algorithm algorithm,
                                            const QByteArray &data, const QByteArray &salt,
                                            int iterations, quint64 dkLen)
{
    // The RFC recommends checking that 'dkLen' is not greater than '(2^32 - 1) * hLen'
    int hashLen = QCryptographicHash::hashLength(algorithm);
    const quint64 maxLen = quint64(std::numeric_limits<quint32>::max() - 1) * hashLen;
    if (dkLen > maxLen) {
        qWarning().nospace() << "Derived key too long:\n"
                             << algorithm << " was chosen which produces output of length "
                             << maxLen << " but " << dkLen << " was requested.";
        return QByteArray();
    }

    if (iterations < 1 || dkLen < 1)
        return QByteArray();

#ifdef USING_OPENSSL30
    if (methodToName(algorithm))
        return opensslDeriveKeyPbkdf2(algorithm, data, salt, iterations, dkLen);
#endif

    // https://tools.ietf.org/html/rfc8018#section-5.2
    QByteArray key;
    quint32 currentIteration = 1;
    QMessageAuthenticationCode hmac(algorithm, data);
    QByteArray index(4, Qt::Uninitialized);
    while (quint64(key.size()) < dkLen) {
        hmac.addData(salt);

        qToBigEndian(currentIteration, index.data());
        hmac.addData(index);

        QByteArray u = hmac.result();
        hmac.reset();
        QByteArray tkey = u;
        for (int iter = 1; iter < iterations; iter++) {
            hmac.addData(u);
            u = hmac.result();
            hmac.reset();
            std::transform(tkey.cbegin(), tkey.cend(), u.cbegin(), tkey.begin(),
                           std::bit_xor<char>());
        }
        key += tkey;
        currentIteration++;
    }
    return key.left(dkLen);
}
} // namespace QPasswordDigestor
QT_END_NAMESPACE
