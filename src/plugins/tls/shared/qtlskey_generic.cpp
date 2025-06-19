// Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qtlskey_generic_p.h"
#include "qasn1element_p.h"

#include <QtNetwork/private/qsslkey_p.h>

#include <QtNetwork/qpassworddigestor.h>

#include <QtCore/QMessageAuthenticationCode>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qrandom.h>

#include <QtCore/qdatastream.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmap.h>

#include <cstring>

QT_BEGIN_NAMESPACE

// The code here is essentially what we had in qsslkey_qt.cpp before, with
// minimal changes/restructure.

namespace QTlsPrivate {

// OIDs of named curves allowed in TLS as per RFCs 4492 and 7027,
// see also https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-8
namespace {

const quint8 bits_table[256] = {
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

using OidLengthMap = QMap<QByteArray, int>;

OidLengthMap createOidMap()
{
    OidLengthMap oids;
    oids.insert(oids.cend(), QByteArrayLiteral("1.2.840.10045.3.1.1"), 192); // secp192r1 a.k.a prime192v1
    oids.insert(oids.cend(), QByteArrayLiteral("1.2.840.10045.3.1.7"), 256); // secp256r1 a.k.a prime256v1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.1"), 193); // sect193r2
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.10"), 256); // secp256k1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.16"), 283); // sect283k1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.17"), 283); // sect283r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.26"), 233); // sect233k1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.27"), 233); // sect233r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.3"), 239); // sect239k1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.30"), 160); // secp160r2
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.31"), 192); // secp192k1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.32"), 224); // secp224k1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.33"), 224); // secp224r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.34"), 384); // secp384r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.35"), 521); // secp521r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.36"), 409); // sect409k1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.37"), 409); // sect409r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.38"), 571); // sect571k1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.39"), 571); // sect571r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.8"), 160); // secp160r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.132.0.9"), 160); // secp160k1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.36.3.3.2.8.1.1.11"), 384); // brainpoolP384r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.36.3.3.2.8.1.1.13"), 512); // brainpoolP512r1
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.36.3.3.2.8.1.1.7"), 256); // brainpoolP256r1
    return oids;
}

} // Unnamed namespace.

Q_GLOBAL_STATIC_WITH_ARGS(OidLengthMap, oidLengthMap, (createOidMap()))

namespace {

// Maps OIDs to the encryption cipher they specify
const QMap<QByteArray, QSslKeyPrivate::Cipher> oidCipherMap {
    {DES_CBC_ENCRYPTION_OID, QSslKeyPrivate::Cipher::DesCbc},
    {DES_EDE3_CBC_ENCRYPTION_OID, QSslKeyPrivate::Cipher::DesEde3Cbc},
    // {PKCS5_MD2_DES_CBC_OID, QSslKeyPrivate::Cipher::DesCbc}, // No MD2
    {PKCS5_MD5_DES_CBC_OID, QSslKeyPrivate::Cipher::DesCbc},
    {PKCS5_SHA1_DES_CBC_OID, QSslKeyPrivate::Cipher::DesCbc},
    // {PKCS5_MD2_RC2_CBC_OID, QSslKeyPrivate::Cipher::Rc2Cbc}, // No MD2
    {PKCS5_MD5_RC2_CBC_OID, QSslKeyPrivate::Cipher::Rc2Cbc},
    {PKCS5_SHA1_RC2_CBC_OID, QSslKeyPrivate::Cipher::Rc2Cbc},
    {RC2_CBC_ENCRYPTION_OID, QSslKeyPrivate::Cipher::Rc2Cbc}
    // {RC5_CBC_ENCRYPTION_OID, QSslKeyPrivate::Cipher::Rc5Cbc}, // No RC5
    // {AES128_CBC_ENCRYPTION_OID, QSslKeyPrivate::Cipher::Aes128}, // no AES
    // {AES192_CBC_ENCRYPTION_OID, QSslKeyPrivate::Cipher::Aes192},
    // {AES256_CBC_ENCRYPTION_OID, QSslKeyPrivate::Cipher::Aes256}
};

struct EncryptionData
{
    EncryptionData() = default;

    EncryptionData(QSslKeyPrivate::Cipher cipher, QByteArray key, QByteArray iv)
        : initialized(true), cipher(cipher), key(key), iv(iv)
    {
    }
    bool initialized = false;
    QSslKeyPrivate::Cipher cipher;
    QByteArray key;
    QByteArray iv;
};

EncryptionData readPbes2(const QList<QAsn1Element> &element, const QByteArray &passPhrase)
{
    // RFC 8018: https://tools.ietf.org/html/rfc8018#section-6.2
    /*** Scheme: ***
     * Sequence (scheme-specific info..)
      * Sequence (key derivation info)
       * Object Identifier (Key derivation algorithm (e.g. PBKDF2))
       * Sequence (salt)
        * CHOICE (this entry can be either of the types it contains)
         * Octet string (actual salt)
         * Object identifier (Anything using this is deferred to a later version of PKCS #5)
        * Integer (iteration count)
      * Sequence (encryption algorithm info)
       * Object identifier (identifier for the algorithm)
       * Algorithm dependent, is covered in the switch further down
    */

    static const QMap<QByteArray, QCryptographicHash::Algorithm> pbes2OidHashFunctionMap {
        // PBES2/PBKDF2
        {HMAC_WITH_SHA1, QCryptographicHash::Sha1},
        {HMAC_WITH_SHA224, QCryptographicHash::Sha224},
        {HMAC_WITH_SHA256, QCryptographicHash::Sha256},
        {HMAC_WITH_SHA512, QCryptographicHash::Sha512},
        {HMAC_WITH_SHA512_224, QCryptographicHash::Sha512},
        {HMAC_WITH_SHA512_256, QCryptographicHash::Sha512},
        {HMAC_WITH_SHA384, QCryptographicHash::Sha384}
    };

    // Values from their respective sections here: https://tools.ietf.org/html/rfc8018#appendix-B.2
    static const QMap<QSslKeyPrivate::Cipher, int> cipherKeyLengthMap {
        {QSslKeyPrivate::Cipher::DesCbc, 8},
        {QSslKeyPrivate::Cipher::DesEde3Cbc, 24},
        // @note: variable key-length (https://tools.ietf.org/html/rfc8018#appendix-B.2.3)
        {QSslKeyPrivate::Cipher::Rc2Cbc, 4}
        // @todo: AES(, rc5?)
    };

    const QList<QAsn1Element> keyDerivationContainer = element[0].toList();
    if (keyDerivationContainer.size() != 2
        || keyDerivationContainer[0].type() != QAsn1Element::ObjectIdentifierType
        || keyDerivationContainer[1].type() != QAsn1Element::SequenceType) {
        return {};
    }

    const QByteArray keyDerivationAlgorithm = keyDerivationContainer[0].toObjectId();
    const auto keyDerivationParams = keyDerivationContainer[1].toList();

    const auto encryptionAlgorithmContainer = element[1].toList();
    if (encryptionAlgorithmContainer.size() != 2
        || encryptionAlgorithmContainer[0].type() != QAsn1Element::ObjectIdentifierType) {
        return {};
    }

    auto iterator = oidCipherMap.constFind(encryptionAlgorithmContainer[0].toObjectId());
    if (iterator == oidCipherMap.cend()) {
        qWarning()
            << "QSslKey: Unsupported encryption cipher OID:" << encryptionAlgorithmContainer[0].toObjectId()
            << "\nFile a bug report to Qt (include the line above).";
        return {};
    }

    QSslKeyPrivate::Cipher cipher = *iterator;
    QByteArray key;
    QByteArray iv;
    switch (cipher) {
    case QSslKeyPrivate::Cipher::DesCbc:
    case QSslKeyPrivate::Cipher::DesEde3Cbc:
        // https://tools.ietf.org/html/rfc8018#appendix-B.2.1 (DES-CBC-PAD)
        // https://tools.ietf.org/html/rfc8018#appendix-B.2.2 (DES-EDE3-CBC-PAD)
        // @todo https://tools.ietf.org/html/rfc8018#appendix-B.2.5 (AES-CBC-PAD)
        /*** Scheme: ***
         * Octet string (IV)
        */
        if (encryptionAlgorithmContainer[1].type() != QAsn1Element::OctetStringType)
            return {};

        // @note: All AES identifiers should be able to use this branch!!
        iv = encryptionAlgorithmContainer[1].value();

        if (iv.size() != 8) // @note: AES needs 16 bytes
            return {};
        break;
    case QSslKeyPrivate::Cipher::Rc2Cbc: {
        // https://tools.ietf.org/html/rfc8018#appendix-B.2.3
        /*** Scheme: ***
         * Sequence (rc2 parameters)
          * Integer (rc2 parameter version)
          * Octet string (IV)
        */
        if (encryptionAlgorithmContainer[1].type() != QAsn1Element::SequenceType)
            return {};
        const auto rc2ParametersContainer = encryptionAlgorithmContainer[1].toList();
        if ((rc2ParametersContainer.size() != 1 && rc2ParametersContainer.size() != 2)
            || rc2ParametersContainer.back().type() != QAsn1Element::OctetStringType) {
            return {};
        }
        iv = rc2ParametersContainer.back().value();
        if (iv.size() != 8)
            return {};
        break;
    } // @todo(?): case (RC5 , AES)
    case QSslKeyPrivate::Cipher::Aes128Cbc:
    case QSslKeyPrivate::Cipher::Aes192Cbc:
    case QSslKeyPrivate::Cipher::Aes256Cbc:
        Q_UNREACHABLE();
    }

    if (Q_LIKELY(keyDerivationAlgorithm == PKCS5_PBKDF2_ENCRYPTION_OID)) {
        // Definition: https://tools.ietf.org/html/rfc8018#appendix-A.2
        QByteArray salt;
        if (keyDerivationParams[0].type() == QAsn1Element::OctetStringType) {
            salt = keyDerivationParams[0].value();
        } else if (keyDerivationParams[0].type() == QAsn1Element::ObjectIdentifierType) {
            Q_UNIMPLEMENTED();
            /* See paragraph from https://tools.ietf.org/html/rfc8018#appendix-A.2
               which ends with: "such facilities are deferred to a future version of PKCS #5"
            */
            return {};
        } else {
            return {};
        }

        // Iterations needed to derive the key
        int iterationCount = keyDerivationParams[1].toInteger();
        // Optional integer
        int keyLength = -1;
        int vectorPos = 2;
        if (keyDerivationParams.size() > vectorPos
            && keyDerivationParams[vectorPos].type() == QAsn1Element::IntegerType) {
            keyLength = keyDerivationParams[vectorPos].toInteger(nullptr);
            ++vectorPos;
        } else {
            keyLength = cipherKeyLengthMap[cipher];
        }

        // Optional algorithm identifier (default: HMAC-SHA-1)
        QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Sha1;
        if (keyDerivationParams.size() > vectorPos
            && keyDerivationParams[vectorPos].type() == QAsn1Element::SequenceType) {
            const auto hashAlgorithmContainer = keyDerivationParams[vectorPos].toList();
            hashAlgorithm = pbes2OidHashFunctionMap[hashAlgorithmContainer.front().toObjectId()];
            Q_ASSERT(hashAlgorithmContainer[1].type() == QAsn1Element::NullType);
            ++vectorPos;
        }
        Q_ASSERT(keyDerivationParams.size() == vectorPos);

        key = QPasswordDigestor::deriveKeyPbkdf2(hashAlgorithm, passPhrase, salt, iterationCount, keyLength);
    } else {
        qWarning()
            << "QSslKey: Unsupported key derivation algorithm OID:" << keyDerivationAlgorithm
            << "\nFile a bugreport to Qt (include the line above).";
        return {};
    }
    return {cipher, key, iv};
}

// Maps OIDs to the hash function it specifies
const QMap<QByteArray, QCryptographicHash::Algorithm> pbes1OidHashFunctionMap {
    // PKCS5
    //{PKCS5_MD2_DES_CBC_OID, QCryptographicHash::Md2}, No MD2
    //{PKCS5_MD2_RC2_CBC_OID, QCryptographicHash::Md2},
    {PKCS5_MD5_DES_CBC_OID, QCryptographicHash::Md5},
    {PKCS5_MD5_RC2_CBC_OID, QCryptographicHash::Md5},
    {PKCS5_SHA1_DES_CBC_OID, QCryptographicHash::Sha1},
    {PKCS5_SHA1_RC2_CBC_OID, QCryptographicHash::Sha1},
    // PKCS12 (unimplemented)
    // {PKCS12_SHA1_RC4_128_OID, QCryptographicHash::Sha1}, // No RC4
    // {PKCS12_SHA1_RC4_40_OID, QCryptographicHash::Sha1},
    // @todo: lacking support. @note: there might be code to do this inside qsslsocket_mac...
    // further note that more work may be required for the 3DES variations listed to be available.
    // {PKCS12_SHA1_3KEY_3DES_CBC_OID, QCryptographicHash::Sha1},
    // {PKCS12_SHA1_2KEY_3DES_CBC_OID, QCryptographicHash::Sha1},
    // {PKCS12_SHA1_RC2_128_CBC_OID, QCryptographicHash::Sha1},
    // {PKCS12_SHA1_RC2_40_CBC_OID, QCryptographicHash::Sha1}
};

EncryptionData readPbes1(const QList<QAsn1Element> &element, const QByteArray &encryptionScheme,
                         const QByteArray &passPhrase)
{
    // RFC 8018: https://tools.ietf.org/html/rfc8018#section-6.1
    // Steps refer to this section: https://tools.ietf.org/html/rfc8018#section-6.1.2
    /*** Scheme: ***
     * Sequence (PBE Parameter)
      * Octet string (salt)
      * Integer (iteration counter)
    */
    // Step 1
    if (element.size() != 2
        || element[0].type() != QAsn1Element::ElementType::OctetStringType
        || element[1].type() != QAsn1Element::ElementType::IntegerType) {
        return {};
    }
    QByteArray salt = element[0].value();
    if (salt.size() != 8)
        return {};

    int iterationCount = element[1].toInteger();
    if (iterationCount < 0)
        return {};

    // Step 2
    auto iterator = pbes1OidHashFunctionMap.constFind(encryptionScheme);
    if (iterator == pbes1OidHashFunctionMap.cend()) {
        // Qt was compiled with ONLY_SHA1 (or it's MD2)
        return {};
    }
    QCryptographicHash::Algorithm hashAlgorithm = *iterator;
    QByteArray key = QPasswordDigestor::deriveKeyPbkdf1(hashAlgorithm, passPhrase, salt, iterationCount, 16);
    if (key.size() != 16)
        return {};

    // Step 3
    QByteArray iv = key.right(8); // last 8 bytes are used as IV
    key.truncate(8); // first 8 bytes are used for the key

    QSslKeyPrivate::Cipher cipher = oidCipherMap[encryptionScheme];
    // Steps 4-6 are done after returning
    return {cipher, key, iv};
}

int curveBits(const QByteArray &oid)
{
    const int length = oidLengthMap->value(oid);
    return length ? length : -1;
}

int numberOfBits(const QByteArray &modulus)
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

QByteArray deriveAesKey(QSslKeyPrivate::Cipher cipher, const QByteArray &passPhrase,
                        const QByteArray &iv)
{
    // This is somewhat simplified and shortened version of what OpenSSL does.
    // See, for example, EVP_BytesToKey for the "algorithm" itself and elsewhere
    // in their code for what they pass as arguments to EVP_BytesToKey when
    // deriving encryption keys (when reading/writing pems files with encrypted
    // keys).

    Q_ASSERT(iv.size() >= 8);

    QCryptographicHash hash(QCryptographicHash::Md5);

    QByteArray data(passPhrase);
    data.append(iv.data(), 8); // AKA PKCS5_SALT_LEN in OpenSSL.

    hash.addData(data);

    if (cipher == Cipher::Aes128Cbc)
        return hash.result();

    QByteArray key(hash.result());
    hash.reset();
    hash.addData(key);
    hash.addData(data);

    if (cipher == Cipher::Aes192Cbc)
        return key.append(hash.resultView().first(8));

    return key.append(hash.resultView());
}

QByteArray deriveKey(QSslKeyPrivate::Cipher cipher, const QByteArray &passPhrase,
                     const QByteArray &iv)
{
    QByteArray key;
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(passPhrase);
    hash.addData(iv);
    switch (cipher) {
    case Cipher::DesCbc:
        key = hash.result().left(8);
        break;
    case Cipher::DesEde3Cbc:
        key = hash.result();
        hash.reset();
        hash.addData(key);
        hash.addData(passPhrase);
        hash.addData(iv);
        key += hash.result().left(8);
        break;
    case Cipher::Rc2Cbc:
        key = hash.result();
        break;
    case Cipher::Aes128Cbc:
    case Cipher::Aes192Cbc:
    case Cipher::Aes256Cbc:
        return deriveAesKey(cipher, passPhrase, iv);
    }
    return key;
}

int extractPkcs8KeyLength(const QList<QAsn1Element> &items, TlsKey *that)
{
    Q_ASSERT(items.size() == 3);
    Q_ASSERT(that);

    int keyLength = -1;

    auto getName = [](QSsl::KeyAlgorithm algorithm) {
        switch (algorithm){
        case QSsl::Rsa: return "RSA";
        case QSsl::Dsa: return "DSA";
        case QSsl::Dh: return "DH";
        case QSsl::Ec: return "EC";
        case QSsl::Opaque: return "Opaque";
        }
        Q_UNREACHABLE();
    };

    const auto pkcs8Info = items[1].toList();
    if (pkcs8Info.size() != 2 || pkcs8Info[0].type() != QAsn1Element::ObjectIdentifierType)
        return -1;
    const QByteArray value = pkcs8Info[0].toObjectId();
    if (value == RSA_ENCRYPTION_OID) {
        if (Q_UNLIKELY(that->algorithm() != QSsl::Rsa)) {
            // We could change the 'algorithm' of QSslKey here and continue loading, but
            // this is not supported in the openssl back-end, so we'll fail here and give
            // the user some feedback.
            qWarning() << "QSslKey: Found RSA key when asked to use" << getName(that->algorithm())
                        << "\nLoading will fail.";
            return -1;
        }
        // Luckily it contains the 'normal' RSA-key format inside, so we can just recurse
        // and read the key's info.
        that->decodeDer(that->type(), that->algorithm(), items[2].value(), {}, true);
        // The real info has been filled out in the call above, so return as if it was invalid
        // to avoid overwriting the data.
        return -1;
    } else if (value == EC_ENCRYPTION_OID) {
        if (Q_UNLIKELY(that->algorithm() != QSsl::Ec)) {
            // As above for RSA.
            qWarning() << "QSslKey: Found EC key when asked to use" << getName(that->algorithm())
                        << "\nLoading will fail.";
            return -1;
        }
        // I don't know where this is documented, but the elliptic-curve identifier has been
        // moved into the "pkcs#8 wrapper", which is what we're interested in.
        if (pkcs8Info[1].type() != QAsn1Element::ObjectIdentifierType)
            return -1;
        keyLength = curveBits(pkcs8Info[1].toObjectId());
    } else if (value == DSA_ENCRYPTION_OID) {
        if (Q_UNLIKELY(that->algorithm() != QSsl::Dsa)) {
            // As above for RSA.
            qWarning() << "QSslKey: Found DSA when asked to use" << getName(that->algorithm())
                        << "\nLoading will fail.";
            return -1;
        }
        // DSA's structure is documented here:
        // https://www.cryptsoft.com/pkcs11doc/STANDARD/v201-95.pdf in section 11.9.
        if (pkcs8Info[1].type() != QAsn1Element::SequenceType)
            return -1;
        const auto dsaInfo = pkcs8Info[1].toList();
        if (dsaInfo.size() != 3 || dsaInfo[0].type() != QAsn1Element::IntegerType)
            return -1;
        keyLength = numberOfBits(dsaInfo[0].value());
    } else if (value == DH_ENCRYPTION_OID) {
        if (Q_UNLIKELY(that->algorithm() != QSsl::Dh)) {
            // As above for RSA.
            qWarning() << "QSslKey: Found DH when asked to use" << getName(that->algorithm())
                        << "\nLoading will fail.";
            return -1;
        }
        // DH's structure is documented here:
        // https://www.cryptsoft.com/pkcs11doc/STANDARD/v201-95.pdf in section 11.9.
        if (pkcs8Info[1].type() != QAsn1Element::SequenceType)
            return -1;
        const auto dhInfo = pkcs8Info[1].toList();
        if (dhInfo.size() < 2 || dhInfo.size() > 3 || dhInfo[0].type() != QAsn1Element::IntegerType)
            return -1;
        keyLength = numberOfBits(dhInfo[0].value());
    } else {
        // in case of unexpected formats:
        qWarning() << "QSslKey: Unsupported PKCS#8 key algorithm:" << value
                    << "\nFile a bugreport to Qt (include the line above).";
        return -1;
    }

    return keyLength;
}

} // Unnamed namespace

void TlsKeyGeneric::decodeDer(QSsl::KeyType type, QSsl::KeyAlgorithm algorithm, const QByteArray &der,
                              const QByteArray &passPhrase, bool deepClear)
{
    keyType = type;
    keyAlgorithm = algorithm;

    clear(deepClear);

    if (der.isEmpty())
        return;
    // decryptPkcs8 decrypts if necessary or returns 'der' unaltered
    QByteArray decryptedDer = decryptPkcs8(der, passPhrase);

    QAsn1Element elem;
    if (!elem.read(decryptedDer) || elem.type() != QAsn1Element::SequenceType)
        return;

    if (type == QSsl::PublicKey) {
        // key info
        QDataStream keyStream(elem.value());
        if (!elem.read(keyStream) || elem.type() != QAsn1Element::SequenceType)
            return;
        const auto infoItems = elem.toList();
        if (infoItems.size() < 2 || infoItems[0].type() != QAsn1Element::ObjectIdentifierType)
            return;
        if (algorithm == QSsl::Rsa) {
            if (infoItems[0].toObjectId() != RSA_ENCRYPTION_OID)
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
            if (infoItems[0].toObjectId() != DSA_ENCRYPTION_OID)
                return;
            if (infoItems[1].type() != QAsn1Element::SequenceType)
                return;
            // key params
            const auto params = infoItems[1].toList();
            if (params.isEmpty() || params[0].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(params[0].value());
        } else if (algorithm == QSsl::Dh) {
            if (infoItems[0].toObjectId() != DH_ENCRYPTION_OID)
                return;
            if (infoItems[1].type() != QAsn1Element::SequenceType)
                return;
            // key params
            const auto params = infoItems[1].toList();
            if (params.isEmpty() || params[0].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(params[0].value());
        } else if (algorithm == QSsl::Ec) {
            if (infoItems[0].toObjectId() != EC_ENCRYPTION_OID)
                return;
            if (infoItems[1].type() != QAsn1Element::ObjectIdentifierType)
                return;
            keyLength = curveBits(infoItems[1].toObjectId());
        }

    } else {
        const auto items = elem.toList();
        if (items.isEmpty())
            return;

        // version
        if (items[0].type() != QAsn1Element::IntegerType)
            return;
        const QByteArray versionHex = items[0].value().toHex();

        if (items.size() == 3 && items[1].type() == QAsn1Element::SequenceType
            && items[2].type() == QAsn1Element::OctetStringType) {
            if (versionHex != "00" && versionHex != "01")
                return;
            int pkcs8KeyLength = extractPkcs8KeyLength(items, this);
            if (pkcs8KeyLength == -1)
                return;
            pkcs8 = true;
            keyLength = pkcs8KeyLength;
        } else if (algorithm == QSsl::Rsa) {
            if (versionHex != "00")
                return;
            if (items.size() != 9 || items[1].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(items[1].value());
        } else if (algorithm == QSsl::Dsa) {
            if (versionHex != "00")
                return;
            if (items.size() != 6 || items[1].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(items[1].value());
        } else if (algorithm == QSsl::Dh) {
            if (versionHex != "00")
                return;
            if (items.size() < 5 || items.size() > 6 || items[1].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(items[1].value());
        } else if (algorithm == QSsl::Ec) {
            if (versionHex != "01")
                return;
            if (items.size() != 4
               || items[1].type() != QAsn1Element::OctetStringType
               || items[2].type() != QAsn1Element::Context0Type
               || items[3].type() != QAsn1Element::Context1Type)
                return;
            QAsn1Element oidElem;
            if (!oidElem.read(items[2].value())
                || oidElem.type() != QAsn1Element::ObjectIdentifierType)
                return;
            keyLength = curveBits(oidElem.toObjectId());
        }
    }

    derData = decryptedDer;
    keyIsNull = false;
}

void TlsKeyGeneric::decodePem(QSsl::KeyType type, QSsl::KeyAlgorithm algorithm, const QByteArray &pem,
                              const QByteArray &passPhrase, bool deepClear)
{
    keyType = type;
    keyAlgorithm = algorithm;

    QMap<QByteArray, QByteArray> headers;
    QByteArray data = derFromPem(pem, &headers);

    if (headers.value("Proc-Type") == "4,ENCRYPTED") {
        const QList<QByteArray> dekInfo = headers.value("DEK-Info").split(',');
        if (dekInfo.size() != 2) {
            clear(deepClear);
            return;
        }

        QSslKeyPrivate::Cipher cipher;
        if (dekInfo.first() == "DES-CBC") {
            cipher = Cipher::DesCbc;
        } else if (dekInfo.first() == "DES-EDE3-CBC") {
            cipher = Cipher::DesEde3Cbc;
        } else if (dekInfo.first() == "RC2-CBC") {
            cipher = Cipher::Rc2Cbc;
        } else if (dekInfo.first() == "AES-128-CBC") {
            cipher = Cipher::Aes128Cbc;
        } else if (dekInfo.first() == "AES-192-CBC") {
            cipher = Cipher::Aes192Cbc;
        } else if (dekInfo.first() == "AES-256-CBC") {
            cipher = Cipher::Aes256Cbc;
        } else {
            clear(deepClear);
            return;
        }

        const QByteArray iv = QByteArray::fromHex(dekInfo.last());
        const QByteArray key = deriveKey(cipher, passPhrase, iv);
        data = decrypt(cipher, data, key, iv);
    }

    decodeDer(keyType, keyAlgorithm, data, passPhrase, deepClear);
}

QByteArray TlsKeyGeneric::toPem(const QByteArray &passPhrase) const
{
    QByteArray data;
    QMap<QByteArray, QByteArray> headers;

    if (type() == QSsl::PrivateKey && !passPhrase.isEmpty()) {
        // ### use a cryptographically secure random number generator
        quint64 random = QRandomGenerator::system()->generate64();
        QByteArray iv = QByteArray::fromRawData(reinterpret_cast<const char *>(&random), sizeof(random));

        auto cipher = Cipher::DesEde3Cbc;
        const QByteArray key = deriveKey(cipher, passPhrase, iv);
        data = encrypt(cipher, derData, key, iv);

        headers.insert("Proc-Type", "4,ENCRYPTED");
        headers.insert("DEK-Info", "DES-EDE3-CBC," + iv.toHex());
    } else {
        data = derData;
    }

    return pemFromDer(data, headers);
}

QByteArray TlsKeyGeneric::derFromPem(const QByteArray &pem, QMap<QByteArray, QByteArray> *headers) const
{
    if (derData.size())
        return derData;

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
        while (i < der.length()) {
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
            } while (i < der.length() && (der.at(i) == ' ' || der.at(i) == '\t'));
            if (i == -1)
                break; // something is wrong

            headers->insert(field, value);
        }
        der = der.mid(i);
    }

    return QByteArray::fromBase64(der); // ignores newlines
}

void TlsKeyGeneric::fromHandle(Qt::HANDLE handle, KeyType expectedType)
{
    opaque = handle;
    keyType = expectedType;
}

void TlsKeyGeneric::clear(bool deep)
{
    keyIsNull = true;
    if (deep)
        std::memset(derData.data(), 0, derData.size());
    derData.clear();
    keyLength = -1;
}

QByteArray TlsKeyGeneric::decryptPkcs8(const QByteArray &encrypted, const QByteArray &passPhrase)
{
    // RFC 5958: https://tools.ietf.org/html/rfc5958
    /*** Scheme: ***
     * Sequence
      * Sequence
       * Object Identifier (encryption scheme (currently PBES2, PBES1, @todo PKCS12))
       * Sequence (scheme parameters)
      * Octet String (the encrypted data)
    */
    QAsn1Element elem;
    if (!elem.read(encrypted) || elem.type() != QAsn1Element::SequenceType)
        return encrypted;

    const auto items = elem.toList();
    if (items.size() != 2
        || items[0].type() != QAsn1Element::SequenceType
        || items[1].type() != QAsn1Element::OctetStringType) {
        return encrypted;
    }

    const auto encryptionSchemeContainer = items[0].toList();

    if (encryptionSchemeContainer.size() != 2
        || encryptionSchemeContainer[0].type() != QAsn1Element::ObjectIdentifierType
        || encryptionSchemeContainer[1].type() != QAsn1Element::SequenceType) {
        return encrypted;
    }

    const QByteArray encryptionScheme = encryptionSchemeContainer[0].toObjectId();
    const auto schemeParameterContainer = encryptionSchemeContainer[1].toList();

    if (schemeParameterContainer.size() != 2
        && schemeParameterContainer[0].type() != QAsn1Element::SequenceType
        && schemeParameterContainer[1].type() != QAsn1Element::SequenceType) {
        return encrypted;
    }

    EncryptionData data;
    if (encryptionScheme == PKCS5_PBES2_ENCRYPTION_OID) {
        data = readPbes2(schemeParameterContainer, passPhrase);
    } else if (pbes1OidHashFunctionMap.contains(encryptionScheme)) {
        data = readPbes1(schemeParameterContainer, encryptionScheme, passPhrase);
    } else if (encryptionScheme.startsWith(PKCS12_OID)) {
        Q_UNIMPLEMENTED(); // this isn't some 'unknown', I know these aren't implemented
        return encrypted;
    } else {
        qWarning()
            << "QSslKey: Unsupported encryption scheme OID:" << encryptionScheme
            << "\nFile a bugreport to Qt (include the line above).";
        return encrypted;
    }

    if (!data.initialized) {
        // something went wrong, return
        return encrypted;
    }

    QByteArray decryptedKey = decrypt(data.cipher, items[1].value(), data.key, data.iv);
    // The data is still wrapped in a octet string, so let's unwrap it
    QAsn1Element decryptedKeyElement(QAsn1Element::ElementType::OctetStringType, decryptedKey);
    return decryptedKeyElement.value();
}

} // namespace QTlsPrivate

QT_END_NAMESPACE
