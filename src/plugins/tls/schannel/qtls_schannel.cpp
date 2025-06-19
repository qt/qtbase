// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:cryptography

// #define QSSLSOCKET_DEBUG

#include "qtlsbackend_schannel_p.h"
#include "qtlskey_schannel_p.h"
#include "qx509_schannel_p.h"
#include "qtls_schannel_p.h"
#include "../shared/qasn1element_p.h"

#include <QtNetwork/private/qsslcertificate_p.h>
#include <QtNetwork/private/qsslcipher_p.h>
#include <QtNetwork/private/qssl_p.h>

#include <QtNetwork/qsslcertificate.h>
#include <QtNetwork/qsslcertificateextension.h>
#include <QtNetwork/qsslsocket.h>

#include <QtCore/qscopeguard.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qmutex.h>

#define SECURITY_WIN32
#include <security.h>
#include <schnlsp.h>

#if NTDDI_VERSION >= NTDDI_WINBLUE && defined(SECBUFFER_APPLICATION_PROTOCOLS)
// ALPN = Application Layer Protocol Negotiation
#define SUPPORTS_ALPN 1
#endif

// Not defined in MinGW
#ifndef SECBUFFER_ALERT
#define SECBUFFER_ALERT 17
#endif
#ifndef SECPKG_ATTR_APPLICATION_PROTOCOL
#define SECPKG_ATTR_APPLICATION_PROTOCOL 35
#endif

// Another missing MinGW define
#ifndef SEC_E_APPLICATION_PROTOCOL_MISMATCH
#define SEC_E_APPLICATION_PROTOCOL_MISMATCH _HRESULT_TYPEDEF_(0x80090367L)
#endif

// Also not defined in MinGW.......
#ifndef SP_PROT_TLS1_SERVER
#define SP_PROT_TLS1_SERVER 0x00000040
#endif
#ifndef SP_PROT_TLS1_CLIENT
#define SP_PROT_TLS1_CLIENT 0x00000080
#endif
#ifndef SP_PROT_TLS1_0_SERVER
#define SP_PROT_TLS1_0_SERVER SP_PROT_TLS1_SERVER
#endif
#ifndef SP_PROT_TLS1_0_CLIENT
#define SP_PROT_TLS1_0_CLIENT SP_PROT_TLS1_CLIENT
#endif
#ifndef SP_PROT_TLS1_0
#define SP_PROT_TLS1_0 (SP_PROT_TLS1_0_CLIENT | SP_PROT_TLS1_0_SERVER)
#endif
#ifndef SP_PROT_TLS1_1_SERVER
#define SP_PROT_TLS1_1_SERVER 0x00000100
#endif
#ifndef SP_PROT_TLS1_1_CLIENT
#define SP_PROT_TLS1_1_CLIENT 0x00000200
#endif
#ifndef SP_PROT_TLS1_1
#define SP_PROT_TLS1_1 (SP_PROT_TLS1_1_CLIENT | SP_PROT_TLS1_1_SERVER)
#endif
#ifndef SP_PROT_TLS1_2_SERVER
#define SP_PROT_TLS1_2_SERVER 0x00000400
#endif
#ifndef SP_PROT_TLS1_2_CLIENT
#define SP_PROT_TLS1_2_CLIENT 0x00000800
#endif
#ifndef SP_PROT_TLS1_2
#define SP_PROT_TLS1_2 (SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_2_SERVER)
#endif
#ifndef SP_PROT_TLS1_3_SERVER
#define SP_PROT_TLS1_3_SERVER 0x00001000
#endif
#ifndef SP_PROT_TLS1_3_CLIENT
#define SP_PROT_TLS1_3_CLIENT 0x00002000
#endif
#ifndef SP_PROT_TLS1_3
#define SP_PROT_TLS1_3 (SP_PROT_TLS1_3_CLIENT | SP_PROT_TLS1_3_SERVER)
#endif
#ifndef BCRYPT_ECDH_ALGORITHM
#define BCRYPT_ECDH_ALGORITHM L"ECDH"
#endif
#ifndef BCRYPT_ECDSA_ALGORITHM
#define BCRYPT_ECDSA_ALGORITHM L"ECDSA"
#endif

/*
    @future!:

    - Transmitting intermediate certificates
        - Look for a way to avoid putting intermediate certificates in the certificate store
        - No documentation on how to send the chain
        - A stackoverflow question on this from 3 years ago implies schannel only sends intermediate
            certificates if it's "in the system or user certificate store".
                - https://stackoverflow.com/q/30156584/2493610
                - This can be done by users, but we shouldn't add any and all local intermediate
                    certs to the stores automatically.
    - PSK support
        - Was added in Windows 10 (it seems), documentation at time of writing is sparse/non-existent.
            - Specifically about how to supply credentials when they're requested.
            - Or how to recognize that they're requested in the first place.
        - Skip certificate verification.
        - Check if "PSK-only" is still required to do PSK _at all_ (all-around bad solution).
        - Check if SEC_I_INCOMPLETE_CREDENTIALS is still returned for both "missing certificate" and
            "missing PSK" when calling InitializeSecurityContext in "performHandshake".

    Low priority:
    - Possibly make RAII wrappers for SecBuffer (which I commonly create QScopeGuards for)

*/

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcTlsBackendSchannel, "qt.tlsbackend.schannel");

// Defined in qsslsocket_qt.cpp.
extern QByteArray _q_makePkcs12(const QList<QSslCertificate> &certs, const QSslKey &key,
                         const QString &passPhrase);
extern QAsn1Element _q_PKCS12_key(const QSslKey &key);

namespace {
bool supportsTls13();
}

namespace QTlsPrivate {

QList<QSslCipher> defaultCiphers();

struct SchannelCipherInfo {
    const char *openSslCipherSuite;
    const char *schannelCipherSuite;
    const char *keyExchangeMethod;
    const char *authenticationMethod;
    const char *encryptionMethod;
    int encryptionBits;
    const char *hashMethod;
    QList<QSsl::SslProtocol> protocols;
};

// The list of supported ciphers according to
//   https://learn.microsoft.com/en-us/windows/win32/secauthn/tls-cipher-suites-in-windows-server-2022
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
std::array<SchannelCipherInfo, 44> schannelCipherInfo = {{
    {"TLS_AES_256_GCM_SHA384",        "TLS_AES_256_GCM_SHA384",                  "",     "",      "AES",  256, "SHA384", {QSsl::TlsV1_3}},
    {"TLS_AES_128_GCM_SHA256",        "TLS_AES_128_GCM_SHA256",                  "",     "",      "AES",  128, "SHA256", {QSsl::TlsV1_3}},
    {"ECDHE-ECDSA-AES256-GCM-SHA384", "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384", "ECDH", "ECDSA", "AES",  256, "SHA384", {QSsl::TlsV1_2}},
    {"ECDHE-ECDSA-AES128-GCM-SHA256", "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256", "ECDH", "ECDSA", "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"ECDHE-RSA-AES256-GCM-SHA384",   "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",   "ECDH", "RSA",   "AES",  256, "SHA384", {QSsl::TlsV1_2}},
    {"ECDHE-RSA-AES128-GCM-SHA256",   "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",   "ECDH", "RSA",   "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"DHE-RSA-AES256-GCM-SHA384",     "TLS_DHE_RSA_WITH_AES_256_GCM_SHA384",     "DH",   "RSA",   "AES",  256, "SHA384", {QSsl::TlsV1_2}},
    {"DHE-RSA-AES128-GCM-SHA256",     "TLS_DHE_RSA_WITH_AES_128_GCM_SHA256",     "DH",   "RSA",   "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"ECDHE-ECDSA-AES256-SHA384",     "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384", "ECDH", "ECDSA", "AES",  256, "SHA384", {QSsl::TlsV1_2}},
    {"ECDHE-ECDSA-AES128-SHA256",     "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256", "ECDH", "ECDSA", "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"ECDHE-RSA-AES256-SHA384",       "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384",   "ECDH", "RSA",   "AES",  256, "SHA384", {QSsl::TlsV1_2}},
    {"ECDHE-RSA-AES128-SHA256",       "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256",   "ECDH", "RSA",   "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"ECDHE-ECDSA-AES256-SHA",        "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA",    "ECDH", "ECDSA", "AES",  256, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"ECDHE-ECDSA-AES128-SHA",        "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA",    "ECDH", "ECDSA", "AES",  128, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"ECDHE-RSA-AES256-SHA",          "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA",      "ECDH", "RSA",   "AES",  256, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"ECDHE-RSA-AES128-SHA",          "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA",      "ECDH", "RSA",   "AES",  128, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"AES256-GCM-SHA384",             "TLS_RSA_WITH_AES_256_GCM_SHA384",         "RSA",  "RSA",   "AES",  256, "SHA384", {QSsl::TlsV1_2}},
    {"AES128-GCM-SHA256",             "TLS_RSA_WITH_AES_128_GCM_SHA256",         "RSA",  "RSA",   "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"AES256-SHA256",                 "TLS_RSA_WITH_AES_256_CBC_SHA256",         "RSA",  "RSA",   "AES",  256, "SHA256", {QSsl::TlsV1_2}},
    {"AES128-SHA256",                 "TLS_RSA_WITH_AES_128_CBC_SHA256",         "RSA",  "RSA",   "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"AES256-SHA",                    "TLS_RSA_WITH_AES_256_CBC_SHA",            "RSA",  "RSA",   "AES",  256, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"AES128-SHA",                    "TLS_RSA_WITH_AES_128_CBC_SHA",            "RSA",  "RSA",   "AES",  128, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"DES-CBC3-SHA",                  "TLS_RSA_WITH_3DES_EDE_CBC_SHA",           "RSA",  "RSA",   "3DES", 168, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"NULL-SHA256",                   "TLS_RSA_WITH_NULL_SHA256",                "RSA",  "RSA",   "",     0,   "SHA256", {QSsl::TlsV1_2}},
    {"NULL-SHA",                      "TLS_RSA_WITH_NULL_SHA",                   "RSA",  "RSA",   "",     0,   "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},

    // the following cipher suites are not enabled by default in schannel provider
    {"TLS_CHACHA20_POLY1305_SHA256",  "TLS_CHACHA20_POLY1305_SHA256",            "",     "",      "CHACHA20_POLY1305", 0, "", {QSsl::TlsV1_3}},
    {"DHE-RSA-AES256-SHA",            "TLS_DHE_RSA_WITH_AES_256_CBC_SHA",        "DH",   "RSA",   "AES",  256, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"DHE-RSA-AES128-SHA",            "TLS_DHE_RSA_WITH_AES_128_CBC_SHA",        "DH",   "RSA",   "AES",  128, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"DHE-DSS-AES256-SHA256",         "TLS_DHE_DSS_WITH_AES_256_CBC_SHA256",     "DH",   "DSA",   "AES",  256, "SHA256", {QSsl::TlsV1_2}},
    {"DHE-DSS-AES128-SHA256",         "TLS_DHE_DSS_WITH_AES_128_CBC_SHA256",     "DH",   "DSA",   "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"DHE-DSS-AES256-SHA",            "TLS_DHE_DSS_WITH_AES_256_CBC_SHA",        "DH",   "DSA",   "AES",  256, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"DHE-DSS-AES128-SHA",            "TLS_DHE_DSS_WITH_AES_128_CBC_SHA",        "DH",   "DSA",   "AES",  128, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"EDH-DSS-DES-CBC3-SHA",          "TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA",       "DH",   "DSA",   "3DES", 168, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"RC4-SHA",                       "TLS_RSA_WITH_RC4_128_SHA",                "RSA",  "RSA",   "RC4",  128, "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"RC4-MD5",                       "TLS_RSA_WITH_RC4_128_MD5",                "RSA",  "RSA",   "RC4",  128, "MD5",    {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"DES-CBC-SHA",                   "TLS_RSA_WITH_DES_CBC_SHA",                "RSA",  "RSA",   "DES",  56,  "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"EDH-DSS-DES-CBC-SHA",           "TLS_DHE_DSS_WITH_DES_CBC_SHA",            "DH",   "DSA",   "DES",  56,  "SHA1",   {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},
    {"NULL-MD5",                      "TLS_RSA_WITH_NULL_MD5",                   "RSA",  "RSA",   "",     0,   "MD5",    {QSsl::TlsV1_2, QSsl::TlsV1_1, QSsl::TlsV1_0}},

    // PSK cipher suites
    {"PSK-AES256-GCM-SHA384",         "TLS_PSK_WITH_AES_256_GCM_SHA384",         "PSK",  "",      "AES",  256, "SHA384", {QSsl::TlsV1_2}},
    {"PSK-AES128-GCM-SHA256",         "TLS_PSK_WITH_AES_128_GCM_SHA256",         "PSK",  "",      "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"PSK-AES256-CBC-SHA384",         "TLS_PSK_WITH_AES_256_CBC_SHA384",         "PSK",  "",      "AES",  256, "SHA384", {QSsl::TlsV1_2}},
    {"PSK-AES128-CBC-SHA256",         "TLS_PSK_WITH_AES_128_CBC_SHA256",         "PSK",  "",      "AES",  128, "SHA256", {QSsl::TlsV1_2}},
    {"PSK-NULL-SHA384",               "TLS_PSK_WITH_NULL_SHA384",                "PSK",  "",      "",     0,   "SHA384", {QSsl::TlsV1_2}},
    {"PSK-NULL-SHA256",               "TLS_PSK_WITH_NULL_SHA256",                "PSK",  "",      "",     0,   "SHA256", {QSsl::TlsV1_2}},
}};
QT_WARNING_POP

const SchannelCipherInfo *cipherInfoByOpenSslName(const QString &name)
{
    for (const auto &cipherInfo : schannelCipherInfo) {
        if (name == QLatin1StringView(cipherInfo.openSslCipherSuite))
            return &cipherInfo;
    }

    return nullptr;
}

UNICODE_STRING cbcChainingMode = {
    sizeof(BCRYPT_CHAIN_MODE_CBC) - 2,
    sizeof(BCRYPT_CHAIN_MODE_CBC),
    const_cast<PWSTR>(BCRYPT_CHAIN_MODE_CBC)
};

UNICODE_STRING gcmChainingMode = {
    sizeof(BCRYPT_CHAIN_MODE_GCM) - 2,
    sizeof(BCRYPT_CHAIN_MODE_GCM),
    const_cast<PWSTR>(BCRYPT_CHAIN_MODE_GCM)
};

/**
 Determines which algorithms are not used by the requested ciphers to build
 up a black list that can be passed to SCH_CREDENTIALS.
 */
QList<CRYPTO_SETTINGS> cryptoSettingsForCiphers(const QList<QSslCipher> &ciphers)
{
    static const QList<QSslCipher> defaultCipherList = defaultCiphers();

    if (defaultCipherList == ciphers) {
        // the ciphers have not been restricted for this session, so no black listing needed
        return {};
    }

    QList<const SchannelCipherInfo*> cipherInfo;

    for (const auto &cipher : ciphers) {
        if (cipher.isNull())
            continue;

        const auto *info = cipherInfoByOpenSslName(cipher.name());
        if (!cipherInfo.contains(info))
            cipherInfo.append(info);
    }

    QList<CRYPTO_SETTINGS> cryptoSettings;

    const auto assignUnicodeString = [](UNICODE_STRING &unicodeString, const wchar_t *characters) {
        unicodeString.Length = static_cast<USHORT>(wcslen(characters) * sizeof(WCHAR));
        unicodeString.MaximumLength = unicodeString.Length + sizeof(UNICODE_NULL);
        unicodeString.Buffer = const_cast<wchar_t*>(characters);
    };

    // black list of key exchange algorithms
    const auto allKeyExchangeAlgorithms = {BCRYPT_RSA_ALGORITHM,
                                           BCRYPT_ECDH_ALGORITHM,
                                           BCRYPT_DH_ALGORITHM};

    for (const auto &algorithm : allKeyExchangeAlgorithms) {
        const auto method = QStringView(algorithm);

        const auto usesMethod = [method](const SchannelCipherInfo *info) {
            return QLatin1StringView(info->keyExchangeMethod) == method;
        };

        const bool exclude = std::none_of(cipherInfo.cbegin(), cipherInfo.cend(), usesMethod);

        if (exclude) {
            CRYPTO_SETTINGS settings = {};
            settings.eAlgorithmUsage = TlsParametersCngAlgUsageKeyExchange;
            assignUnicodeString(settings.strCngAlgId, algorithm);
            cryptoSettings.append(settings);
        }
    }

    // black list of authentication algorithms
    const auto allAuthenticationAlgorithms = {BCRYPT_RSA_ALGORITHM,
                                              BCRYPT_DSA_ALGORITHM,
                                              BCRYPT_ECDSA_ALGORITHM,
                                              BCRYPT_DH_ALGORITHM};

    for (const auto &algorithm : allAuthenticationAlgorithms) {
        const auto method = QStringView(algorithm);

        const auto usesMethod = [method](const SchannelCipherInfo *info) {
            return QLatin1StringView(info->authenticationMethod) == method;
        };

        const bool exclude = std::none_of(cipherInfo.begin(), cipherInfo.end(), usesMethod);

        if (exclude) {
            CRYPTO_SETTINGS settings = {};
            settings.eAlgorithmUsage = TlsParametersCngAlgUsageSignature;
            assignUnicodeString(settings.strCngAlgId, algorithm);
            cryptoSettings.append(settings);
        }
    }


    // black list of encryption algorithms
    const auto allEncryptionAlgorithms = {BCRYPT_AES_ALGORITHM,
                                          BCRYPT_RC4_ALGORITHM,
                                          BCRYPT_DES_ALGORITHM,
                                          BCRYPT_3DES_ALGORITHM};

    for (const auto &algorithm : allEncryptionAlgorithms) {
        const auto method = QStringView(algorithm);

        if (method == QLatin1StringView("AES")) {
            bool uses128Bit = false;
            bool uses256Bit = false;
            bool usesGcm = false;
            bool usesCbc = false;
            for (const auto *info : cipherInfo) {
                if (QLatin1StringView(info->encryptionMethod) == method) {
                    uses128Bit = uses128Bit || (info->encryptionBits == 128);
                    uses256Bit = uses256Bit || (info->encryptionBits == 256);
                    usesGcm = usesGcm ||
                              QLatin1StringView(info->schannelCipherSuite).contains("_GCM_"_L1);
                    usesCbc = usesCbc ||
                              QLatin1StringView(info->schannelCipherSuite).contains("_CBC_"_L1);
                }
            }

            CRYPTO_SETTINGS settings = {};
            settings.eAlgorithmUsage = TlsParametersCngAlgUsageCipher;
            assignUnicodeString(settings.strCngAlgId, algorithm);

            if (usesGcm && !usesCbc) {
                settings.cChainingModes = 1;
                settings.rgstrChainingModes = &cbcChainingMode;
            } else if (!usesGcm && usesCbc) {
                settings.cChainingModes = 1;
                settings.rgstrChainingModes = &gcmChainingMode;
            }

            if (!uses128Bit && uses256Bit) {
                settings.dwMinBitLength = 256;
                cryptoSettings.append(settings);
            } else if (uses128Bit && !uses256Bit) {
                settings.dwMaxBitLength = 128;
                cryptoSettings.append(settings);
            } else if (!uses128Bit && !uses256Bit) {
                cryptoSettings.append(settings);
            }
        } else {
            const auto usesMethod = [method](const SchannelCipherInfo *info) {
                return QLatin1StringView(info->encryptionMethod) == method;
            };

            const bool exclude = std::none_of(cipherInfo.begin(), cipherInfo.end(), usesMethod);

            if (exclude) {
                CRYPTO_SETTINGS settings = {};
                settings.eAlgorithmUsage = TlsParametersCngAlgUsageCipher;
                assignUnicodeString(settings.strCngAlgId, algorithm);
                cryptoSettings.append(settings);
            }
        }
    }

    // black list of hash algorithms
    const auto allHashAlgorithms = {BCRYPT_MD5_ALGORITHM,
                                    BCRYPT_SHA1_ALGORITHM,
                                    BCRYPT_SHA256_ALGORITHM,
                                    BCRYPT_SHA384_ALGORITHM};

    for (const auto &algorithm : allHashAlgorithms) {
        const auto method = QStringView(algorithm);

        const auto usesMethod = [method](const SchannelCipherInfo *info) {
            return QLatin1StringView(info->hashMethod) == method;
        };

        const bool exclude = std::none_of(cipherInfo.begin(), cipherInfo.end(), usesMethod);

        if (exclude) {
            CRYPTO_SETTINGS settings = {};
            settings.eAlgorithmUsage = TlsParametersCngAlgUsageDigest;
            assignUnicodeString(settings.strCngAlgId, algorithm);
            cryptoSettings.append(settings);
        }
    }

    return cryptoSettings;
}

QList<QSslCipher> ciphersByName(QStringView schannelSuiteName)
{
    QList<QSslCipher> ciphers;

    for (const auto &cipher : schannelCipherInfo) {
        if (QLatin1StringView(cipher.schannelCipherSuite) == schannelSuiteName) {
            for (const auto &protocol : cipher.protocols) {
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
                const QString protocolName = (
                                              protocol == QSsl::TlsV1_0 ? QStringLiteral("TLSv1.0") :
                                              protocol == QSsl::TlsV1_1 ? QStringLiteral("TLSv1.1") :
                                              protocol == QSsl::TlsV1_2 ? QStringLiteral("TLSv1.2") :
                                              protocol == QSsl::TlsV1_3 ? QStringLiteral("TLSv1.3") :
                                                                          QString());
QT_WARNING_POP

                ciphers.append(QTlsBackend::createCiphersuite(QLatin1StringView(cipher.openSslCipherSuite),
                                                              QLatin1StringView(cipher.keyExchangeMethod),
                                                              QLatin1StringView(cipher.encryptionMethod),
                                                              QLatin1StringView(cipher.authenticationMethod),
                                                              cipher.encryptionBits,
                                                              protocol, protocolName));
            }
        }
    }

    return ciphers;
}

QList<QSslCipher> defaultCiphers()
{
    ULONG contextFunctionsCount = {};
    PCRYPT_CONTEXT_FUNCTIONS contextFunctions = {};

    const auto status = BCryptEnumContextFunctions(CRYPT_LOCAL, L"SSL", NCRYPT_SCHANNEL_INTERFACE,
                                                   &contextFunctionsCount, &contextFunctions);
    if (!NT_SUCCESS(status)) {
        qCWarning(lcTlsBackendSchannel, "Failed to enumerate ciphers");
        return {};
    }

    const bool supportsV13 = supportsTls13();

    QList<QSslCipher> ciphers;

    for (ULONG index = 0; index < contextFunctions->cFunctions; ++index) {
        const auto suiteName = QStringView(contextFunctions->rgpszFunctions[index]);

        const QList<QSslCipher> allCiphers = ciphersByName(suiteName);

        for (const auto &cipher : allCiphers) {
            if (!supportsV13 && (cipher.protocol() == QSsl::TlsV1_3))
                continue;

            ciphers.append(cipher);
        }
    }

    BCryptFreeBuffer(contextFunctions);

    return ciphers;
}

bool containsTls13Cipher(const QList<QSslCipher> &ciphers)
{
    return std::any_of(ciphers.cbegin(), ciphers.cend(),
                       [](const QSslCipher &cipher) { return cipher.protocol() == QSsl::TlsV1_3; });
}

} // namespace QTlsPrivate

bool QSchannelBackend::s_loadedCiphersAndCerts = false;
Q_GLOBAL_STATIC(QRecursiveMutex, qt_schannel_mutex)

long QSchannelBackend::tlsLibraryVersionNumber() const
{
    const auto os = QOperatingSystemVersion::current();
    return (os.majorVersion() << 24) | ((os.minorVersion() & 0xFF) << 16) | (os.microVersion() & 0xFFFF);
}

QString QSchannelBackend::tlsLibraryVersionString() const
{
    const auto os = QOperatingSystemVersion::current();
    return "Secure Channel, %1 %2.%3.%4"_L1
            .arg(os.name(),
                 QString::number(os.majorVersion()),
                 QString::number(os.minorVersion()),
                 QString::number(os.microVersion()));
}

long QSchannelBackend::tlsLibraryBuildVersionNumber() const
{
    return NTDDI_VERSION;
}

QString QSchannelBackend::tlsLibraryBuildVersionString() const
{
    return "Secure Channel (NTDDI: 0x%1)"_L1
            .arg(QString::number(NTDDI_VERSION, 16).toUpper());
}

void QSchannelBackend::ensureInitialized() const
{
    ensureInitializedImplementation();
}

void QSchannelBackend::ensureInitializedImplementation()
{
    const QMutexLocker<QRecursiveMutex> locker(qt_schannel_mutex);
    if (s_loadedCiphersAndCerts)
        return;
    s_loadedCiphersAndCerts = true;

    setDefaultCaCertificates(systemCaCertificatesImplementation());
    // setDefaultCaCertificates sets it to false, re-enable it:
    QSslSocketPrivate::setRootCertOnDemandLoadingSupported(true);

    resetDefaultCiphers();
}

void QSchannelBackend::resetDefaultCiphers()
{
    setDefaultSupportedCiphers(QTlsPrivate::defaultCiphers());
    setDefaultCiphers(QTlsPrivate::defaultCiphers());
}

QString QSchannelBackend::backendName() const
{
    return builtinBackendNames[nameIndexSchannel];
}

QList<QSsl::SslProtocol> QSchannelBackend::supportedProtocols() const
{
    QList<QSsl::SslProtocol> protocols;

    protocols << QSsl::AnyProtocol;
    protocols << QSsl::SecureProtocols;
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    protocols << QSsl::TlsV1_0;
    protocols << QSsl::TlsV1_0OrLater;
    protocols << QSsl::TlsV1_1;
    protocols << QSsl::TlsV1_1OrLater;
QT_WARNING_POP
    protocols << QSsl::TlsV1_2;
    protocols << QSsl::TlsV1_2OrLater;

    if (supportsTls13()) {
        protocols << QSsl::TlsV1_3;
        protocols << QSsl::TlsV1_3OrLater;
    }

    return protocols;
}

QList<QSsl::SupportedFeature> QSchannelBackend::supportedFeatures() const
{
    QList<QSsl::SupportedFeature> features;

#ifdef SUPPORTS_ALPN
    features << QSsl::SupportedFeature::ClientSideAlpn;
    features << QSsl::SupportedFeature::ServerSideAlpn;
#endif

    return features;
}

QList<QSsl::ImplementedClass> QSchannelBackend::implementedClasses() const
{
    QList<QSsl::ImplementedClass> classes;

    classes << QSsl::ImplementedClass::Socket;
    classes << QSsl::ImplementedClass::Certificate;
    classes << QSsl::ImplementedClass::Key;

    return classes;
}

QTlsPrivate::TlsKey *QSchannelBackend::createKey() const
{
    return new QTlsPrivate::TlsKeySchannel;
}

QTlsPrivate::X509Certificate *QSchannelBackend::createCertificate() const
{
    return new QTlsPrivate::X509CertificateSchannel;
}

QList<QSslCertificate> QSchannelBackend::systemCaCertificates() const
{
    return systemCaCertificatesImplementation();
}

QTlsPrivate::TlsCryptograph *QSchannelBackend::createTlsCryptograph() const
{
    return new QTlsPrivate::TlsCryptographSchannel;
}

QList<QSslCertificate> QSchannelBackend::systemCaCertificatesImplementation()
{
    // Similar to non-Darwin version found in qtlsbackend_openssl.cpp,
    // QTlsPrivate::systemCaCertificates function.
    QList<QSslCertificate> systemCerts;

    auto hSystemStore = QHCertStorePointer(
            CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
                          CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_CURRENT_USER, L"ROOT"));

    if (hSystemStore) {
        PCCERT_CONTEXT pc = nullptr;
        while ((pc = CertFindCertificateInStore(hSystemStore.get(), X509_ASN_ENCODING, 0,
                                                CERT_FIND_ANY, nullptr, pc))) {
            systemCerts.append(QTlsPrivate::X509CertificateSchannel::QSslCertificate_from_CERT_CONTEXT(pc));
        }
    }
    return systemCerts;
}

QTlsPrivate::X509PemReaderPtr QSchannelBackend::X509PemReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromPem;
}

QTlsPrivate::X509DerReaderPtr QSchannelBackend::X509DerReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromDer;
}

QTlsPrivate::X509Pkcs12ReaderPtr QSchannelBackend::X509Pkcs12Reader() const
{
    return QTlsPrivate::X509CertificateSchannel::importPkcs12;
}

namespace {

SecBuffer createSecBuffer(void *ptr, unsigned long length, unsigned long bufferType)
{
    return SecBuffer{ length, bufferType, ptr };
}

SecBuffer createSecBuffer(QByteArray &buffer, unsigned long bufferType)
{
    return createSecBuffer(buffer.data(), static_cast<unsigned long>(buffer.length()), bufferType);
}

QString schannelErrorToString(qint32 status)
{
    switch (status) {
    case SEC_E_INSUFFICIENT_MEMORY:
        return QSslSocket::tr("Insufficient memory");
    case SEC_E_INTERNAL_ERROR:
        return QSslSocket::tr("Internal error");
    case SEC_E_INVALID_HANDLE:
        return QSslSocket::tr("An internal handle was invalid");
    case SEC_E_INVALID_TOKEN:
        return QSslSocket::tr("An internal token was invalid");
    case SEC_E_LOGON_DENIED:
        // According to the link below we get this error when Schannel receives TLS1_ALERT_ACCESS_DENIED
        // https://docs.microsoft.com/en-us/windows/desktop/secauthn/schannel-error-codes-for-tls-and-ssl-alerts
        return QSslSocket::tr("Access denied");
    case SEC_E_NO_AUTHENTICATING_AUTHORITY:
        return QSslSocket::tr("No authority could be contacted for authorization");
    case SEC_E_NO_CREDENTIALS:
        return QSslSocket::tr("No credentials");
    case SEC_E_TARGET_UNKNOWN:
        return QSslSocket::tr("The target is unknown or unreachable");
    case SEC_E_UNSUPPORTED_FUNCTION:
        return QSslSocket::tr("An unsupported function was requested");
    case SEC_E_WRONG_PRINCIPAL:
        // SNI error
        return QSslSocket::tr("The hostname provided does not match the one received from the peer");
    case SEC_E_APPLICATION_PROTOCOL_MISMATCH:
        return QSslSocket::tr("No common protocol exists between the client and the server");
    case SEC_E_ILLEGAL_MESSAGE:
        return QSslSocket::tr("Unexpected or badly-formatted message received");
    case SEC_E_ENCRYPT_FAILURE:
        return QSslSocket::tr("The data could not be encrypted");
    case SEC_E_DECRYPT_FAILURE:
        return QSslSocket::tr("The data could not be decrypted");
    case SEC_E_ALGORITHM_MISMATCH:
        return QSslSocket::tr("No cipher suites in common");
    case SEC_E_UNKNOWN_CREDENTIALS:
        // This can mean "invalid argument" in some cases...
        return QSslSocket::tr("The credentials were not recognized / Invalid argument");
    case SEC_E_MESSAGE_ALTERED:
        // According to the Internet it also triggers for messages that are out of order.
        // https://microsoft.public.platformsdk.security.narkive.com/4JAvlMvD/help-please-schannel-security-contexts-and-decryptmessage
        return QSslSocket::tr("The message was tampered with, damaged or out of sequence.");
    case SEC_E_OUT_OF_SEQUENCE:
        return QSslSocket::tr("A message was received out of sequence.");
    case SEC_E_CONTEXT_EXPIRED:
        return QSslSocket::tr("The TLS/SSL connection has been closed");
    default:
        return QSslSocket::tr("Unknown error occurred: %1").arg(status);
    }
}

bool supportsTls13()
{
    static bool supported = []() {
        const auto current = QOperatingSystemVersion::current();
        // 20221 just happens to be the preview version I run on my laptop where I tested TLS 1.3.
        const auto minimum =
                QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 20221);
        return current >= minimum;
    }();

    return supported;
}

DWORD toSchannelProtocol(QSsl::SslProtocol protocol)
{
    DWORD protocols = SP_PROT_NONE;
    switch (protocol) {
    case QSsl::UnknownProtocol:
        return DWORD(-1);
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::DtlsV1_0:
    case QSsl::DtlsV1_0OrLater:
QT_WARNING_POP
    case QSsl::DtlsV1_2:
    case QSsl::DtlsV1_2OrLater:
        return DWORD(-1); // Not supported at the moment (@future)
    case QSsl::AnyProtocol:
        protocols = SP_PROT_TLS1_0 | SP_PROT_TLS1_1 | SP_PROT_TLS1_2;
        if (supportsTls13())
            protocols |= SP_PROT_TLS1_3;
        break;
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::TlsV1_0:
        protocols = SP_PROT_TLS1_0;
        break;
    case QSsl::TlsV1_1:
        protocols = SP_PROT_TLS1_1;
        break;
QT_WARNING_POP
    case QSsl::TlsV1_2:
        protocols = SP_PROT_TLS1_2;
        break;
    case QSsl::TlsV1_3:
        if (supportsTls13())
            protocols = SP_PROT_TLS1_3;
        else
            protocols = DWORD(-1);
        break;
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::TlsV1_0OrLater:
        // For the "OrLater" protocols we fall through from one to the next, adding all of them
        // in ascending order
        protocols = SP_PROT_TLS1_0;
        Q_FALLTHROUGH();
    case QSsl::TlsV1_1OrLater:
        protocols |= SP_PROT_TLS1_1;
        Q_FALLTHROUGH();
QT_WARNING_POP
    case QSsl::SecureProtocols: // TLS v1.2 and later is currently considered secure
    case QSsl::TlsV1_2OrLater:
        protocols |= SP_PROT_TLS1_2;
        Q_FALLTHROUGH();
    case QSsl::TlsV1_3OrLater:
        if (supportsTls13())
            protocols |= SP_PROT_TLS1_3;
        else if (protocol == QSsl::TlsV1_3OrLater)
            protocols = DWORD(-1); // if TlsV1_3OrLater was specifically chosen we should fail
        break;
    }
    return protocols;
}

// In the new API that descended down upon us we are not asked which protocols we want
// but rather which protocols we don't want. So now we have this function to disable
// anything that is not enabled.
DWORD negatedSchannelProtocols(DWORD wantedProtocols)
{
    DWORD protocols = SP_PROT_ALL; // all protocols
    protocols &= ~wantedProtocols; // minus the one(s) we want
    return protocols;
}

/*!
    \internal
    Used when converting the established session's \a protocol back to
    Qt's own SslProtocol type.

    Only one protocol should be passed in at a time.
*/
QSsl::SslProtocol toQtSslProtocol(DWORD protocol)
{
#define MAP_PROTOCOL(sp_protocol, q_protocol) \
    if (protocol & sp_protocol) {             \
        Q_ASSERT(!(protocol & ~sp_protocol)); \
        return q_protocol;                    \
    }

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    MAP_PROTOCOL(SP_PROT_TLS1_0, QSsl::TlsV1_0)
    MAP_PROTOCOL(SP_PROT_TLS1_1, QSsl::TlsV1_1)
QT_WARNING_POP
    MAP_PROTOCOL(SP_PROT_TLS1_2, QSsl::TlsV1_2)
    MAP_PROTOCOL(SP_PROT_TLS1_3, QSsl::TlsV1_3)
#undef MAP_PROTOCOL
    Q_UNREACHABLE();
    return QSsl::UnknownProtocol;
}

/*!
    \internal
    Used by verifyCertContext to check if a client cert is used by a server or vice versa.
*/
bool netscapeWrongCertType(const QList<QSslCertificateExtension> &extensions, bool isClient, bool isLeaf)
{
    const auto netscapeIt = std::find_if(
            extensions.cbegin(), extensions.cend(),
            [](const QSslCertificateExtension &extension) {
                return extension.oid() == u"2.16.840.1.113730.1.1";
            });
    if (netscapeIt != extensions.cend()) {
        const QByteArray netscapeCertTypeByte = netscapeIt->value().toByteArray();
        int netscapeCertType = 0;
        QDataStream dataStream(netscapeCertTypeByte);
        dataStream >> netscapeCertType;
        if (dataStream.status() != QDataStream::Status::Ok)
            return true;
        const int expectedPeerCertType = [&]() {
            if (isLeaf) {
                return isClient ? NETSCAPE_SSL_SERVER_AUTH_CERT_TYPE
                                : NETSCAPE_SSL_CLIENT_AUTH_CERT_TYPE;
            }
            return NETSCAPE_SSL_CA_CERT_TYPE;
        }();
        if ((netscapeCertType & expectedPeerCertType) == 0)
            return true;
    }
    return false;
}

/*!
    \internal
    Used by verifyCertContext to check the basicConstraints certificate
    extension to see if the certificate is a certificate authority.
    Returns false if the certificate does not have the basicConstraints
    extension or if it is not a certificate authority.
*/
bool isCertificateAuthority(const QList<QSslCertificateExtension> &extensions)
{
    auto it = std::find_if(extensions.cbegin(), extensions.cend(),
                           [](const QSslCertificateExtension &extension) {
                               return extension.name() == "basicConstraints"_L1;
                           });
    if (it != extensions.cend()) {
        QVariantMap basicConstraints = it->value().toMap();
        return basicConstraints.value("ca"_L1, false).toBool();
    }
    return false;
}

/*!
    \internal
    Returns true if the attributes we requested from the context/handshake have
    been given.
*/
bool matchesContextRequirements(DWORD attributes, DWORD requirements,
                                QSslSocket::PeerVerifyMode verifyMode,
                                bool isClient)
{
#ifdef QSSLSOCKET_DEBUG
#define DEBUG_WARN(message) qCWarning(lcTlsBackendSchannel, message)
#else
#define DEBUG_WARN(message)
#endif

#define CHECK_ATTRIBUTE(attributeName)                                                                 \
    do {                                                                                               \
        const DWORD req##attributeName = isClient ? ISC_REQ_##attributeName : ASC_REQ_##attributeName; \
        const DWORD ret##attributeName = isClient ? ISC_RET_##attributeName : ASC_RET_##attributeName; \
        if (!(requirements & req##attributeName) != !(attributes & ret##attributeName)) {              \
            DEBUG_WARN("Missing attribute \"" #attributeName "\"");                                    \
            return false;                                                                              \
        }                                                                                              \
    } while (false)

    CHECK_ATTRIBUTE(CONFIDENTIALITY);
    CHECK_ATTRIBUTE(REPLAY_DETECT);
    CHECK_ATTRIBUTE(SEQUENCE_DETECT);
    CHECK_ATTRIBUTE(STREAM);
    if (verifyMode == QSslSocket::PeerVerifyMode::VerifyPeer)
        CHECK_ATTRIBUTE(MUTUAL_AUTH);

    // This one is manual because there is no server / ASC_ version
    if (isClient) {
        const auto reqManualCredValidation = ISC_REQ_MANUAL_CRED_VALIDATION;
        const auto retManualCredValidation = ISC_RET_MANUAL_CRED_VALIDATION;
        if (!(requirements & reqManualCredValidation) != !(attributes & retManualCredValidation)) {
            DEBUG_WARN("Missing attribute \"MANUAL_CRED_VALIDATION\"");
            return false;
        }
    }

    return true;
#undef CHECK_ATTRIBUTE
#undef DEBUG_WARN
}

template<typename Required, typename Actual>
Required const_reinterpret_cast(Actual *p)
{
    return Required(p);
}

#ifdef SUPPORTS_ALPN
QByteArray createAlpnString(const QByteArrayList &nextAllowedProtocols)
{
    QByteArray alpnString;
    if (!nextAllowedProtocols.isEmpty()) {
        const QByteArray names = [&nextAllowedProtocols]() {
            QByteArray protocolString;
            for (QByteArray proto : nextAllowedProtocols) {
                if (proto.size() > 255) {
                    qCWarning(lcTlsBackendSchannel)
                            << "TLS ALPN extension" << proto << "is too long and will be ignored.";
                    continue;
                } else if (proto.isEmpty()) {
                    continue;
                }
                protocolString += char(proto.length()) + proto;
            }
            return protocolString;
        }();
        if (names.isEmpty())
            return alpnString;

        const quint16 namesSize = names.size();
        const quint32 alpnId = SecApplicationProtocolNegotiationExt_ALPN;
        const quint32 totalSize = sizeof(alpnId) + sizeof(namesSize) + namesSize;
        alpnString = QByteArray::fromRawData(reinterpret_cast<const char *>(&totalSize), sizeof(totalSize))
                + QByteArray::fromRawData(reinterpret_cast<const char *>(&alpnId), sizeof(alpnId))
                + QByteArray::fromRawData(reinterpret_cast<const char *>(&namesSize), sizeof(namesSize))
                + names;
    }
    return alpnString;
}
#endif // SUPPORTS_ALPN

qint64 readToBuffer(QByteArray &buffer, QTcpSocket *plainSocket)
{
    Q_ASSERT(plainSocket);
    static const qint64 shrinkCutoff = 1024 * 12;
    static const qint64 defaultRead = 1024 * 16;
    qint64 bytesRead = 0;

    const auto toRead = std::min(defaultRead, plainSocket->bytesAvailable());
    if (toRead > 0) {
        const auto bufferSize = buffer.size();
        buffer.reserve(bufferSize + toRead); // avoid growth strategy kicking in
        buffer.resize(bufferSize + toRead);
        bytesRead = plainSocket->read(buffer.data() + bufferSize, toRead);
        buffer.resize(bufferSize + bytesRead);
        // In case of excessive memory usage we shrink:
        if (buffer.size() < shrinkCutoff && buffer.capacity() > defaultRead)
            buffer.shrink_to_fit();
    }

    return bytesRead;
}

void retainExtraData(QByteArray &buffer, const SecBuffer &secBuffer)
{
    Q_ASSERT(secBuffer.BufferType == SECBUFFER_EXTRA);
    if (int(secBuffer.cbBuffer) >= buffer.size())
        return;

#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcTlsBackendSchannel, "We got SECBUFFER_EXTRA, will retain %lu bytes",
            secBuffer.cbBuffer);
#endif
    std::move(buffer.end() - secBuffer.cbBuffer, buffer.end(), buffer.begin());
    buffer.resize(secBuffer.cbBuffer);
}

qint64 checkIncompleteData(const SecBuffer &secBuffer)
{
    if (secBuffer.BufferType == SECBUFFER_MISSING) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcTlsBackendSchannel, "Need %lu more bytes.", secBuffer.cbBuffer);
#endif
        return secBuffer.cbBuffer;
}
    return 0;
}

DWORD defaultCredsFlag()
{
    return qEnvironmentVariableIsSet("QT_SCH_DEFAULT_CREDS") ? 0 : SCH_CRED_NO_DEFAULT_CREDS;
}
} // anonymous namespace


namespace QTlsPrivate {

TlsCryptographSchannel::TlsCryptographSchannel()
{
    SecInvalidateHandle(&credentialHandle);
    SecInvalidateHandle(&contextHandle);
    QSchannelBackend::ensureInitializedImplementation();
}

TlsCryptographSchannel::~TlsCryptographSchannel()
{
    closeCertificateStores();
    deallocateContext();
    freeCredentialsHandle();
}

void TlsCryptographSchannel::init(QSslSocket *qObj, QSslSocketPrivate *dObj)
{
    Q_ASSERT(qObj);
    Q_ASSERT(dObj);

    q = qObj;
    d = dObj;

    reset();
}

bool TlsCryptographSchannel::sendToken(void *token, unsigned long tokenLength, bool emitError)
{
    if (tokenLength == 0)
        return true;

    Q_ASSERT(d);
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);
    if (plainSocket->state() == QAbstractSocket::UnconnectedState || !plainSocket->isValid()
        || !plainSocket->isOpen()) {
        return false;
    }

    const qint64 written = plainSocket->write(static_cast<const char *>(token), tokenLength);
    if (written != qint64(tokenLength)) {
        // Failed to write/buffer everything or an error occurred
        if (emitError)
            setErrorAndEmit(d, plainSocket->error(), plainSocket->errorString());
        return false;
    }
    return true;
}

QString TlsCryptographSchannel::targetName() const
{
    // Used for SNI extension
    Q_ASSERT(q);
    Q_ASSERT(d);

    const auto verificationPeerName = d->verificationName();
    return verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName;
}

ULONG TlsCryptographSchannel::getContextRequirements()
{
    Q_ASSERT(d);
    Q_ASSERT(q);

    const bool isClient = d->tlsMode() == QSslSocket::SslClientMode;
    ULONG req = 0;

    req |= ISC_REQ_ALLOCATE_MEMORY; // Allocate memory for buffers automatically
    req |= ISC_REQ_CONFIDENTIALITY; // Encrypt messages
    req |= ISC_REQ_REPLAY_DETECT; // Detect replayed messages
    req |= ISC_REQ_SEQUENCE_DETECT; // Detect out of sequence messages
    req |= ISC_REQ_STREAM; // Support a stream-oriented connection

    if (isClient) {
        req |= ISC_REQ_MANUAL_CRED_VALIDATION; // Manually validate certificate
    } else {
        switch (q->peerVerifyMode()) {
        case QSslSocket::PeerVerifyMode::VerifyNone:
        // There doesn't seem to be a way to ask for an optional client cert :-(
        case QSslSocket::PeerVerifyMode::AutoVerifyPeer:
        case QSslSocket::PeerVerifyMode::QueryPeer:
            break;
        case QSslSocket::PeerVerifyMode::VerifyPeer:
            req |= ISC_REQ_MUTUAL_AUTH;
            break;
        }
    }

    return req;
}

bool TlsCryptographSchannel::acquireCredentialsHandle()
{
    Q_ASSERT(d);
    Q_ASSERT(q);
    const auto &configuration = q->sslConfiguration();

    Q_ASSERT(schannelState == SchannelState::InitializeHandshake);

    const bool isClient = d->tlsMode() == QSslSocket::SslClientMode;
    DWORD protocols = toSchannelProtocol(configuration.protocol());
    if (protocols == DWORD(-1)) {
        setErrorAndEmit(d, QAbstractSocket::SslInvalidUserDataError,
                        QSslSocket::tr("Invalid protocol chosen"));
        return false;
    }

    DWORD certsCount = 0;
    // Set up our certificate stores before trying to use one...
    initializeCertificateStores();

    // Check if user has specified a certificate chain but it could not be loaded.
    // This happens if there was something wrong with the certificate chain or there was no private
    // key.
    if (!configuration.localCertificateChain().isEmpty() && !localCertificateStore)
        return true; // 'true' because "tst_QSslSocket::setEmptyKey" expects us to not disconnect

    PCCERT_CONTEXT localCertificate = nullptr;
    if (localCertificateStore != nullptr) {
        certsCount = 1;
        localCertificate = static_cast<PCCERT_CONTEXT>(configuration.localCertificate().handle());
        Q_ASSERT(localCertificate);
    }

    const QList<QSslCipher> ciphers = configuration.ciphers();
    if (!ciphers.isEmpty() && !containsTls13Cipher(ciphers))
        protocols &= ~SP_PROT_TLS1_3;

    QList<CRYPTO_SETTINGS> cryptoSettings;
    if (!ciphers.isEmpty())
        cryptoSettings = cryptoSettingsForCiphers(ciphers);

    TLS_PARAMETERS tlsParameters = {
        0,
        nullptr,
        negatedSchannelProtocols(protocols), // what protocols to disable
        static_cast<DWORD>(cryptoSettings.size()),
        (cryptoSettings.isEmpty() ? nullptr : cryptoSettings.data()),
        0
    };

    SCH_CREDENTIALS credentials = {
        SCH_CREDENTIALS_VERSION,
        0,
        certsCount,
        &localCertificate,
        nullptr,
        0,
        nullptr,
        0,
        SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT | defaultCredsFlag(),
        1,
        &tlsParameters
    };

    TimeStamp expiration{};
    auto status = AcquireCredentialsHandle(nullptr, // pszPrincipal (unused)
                                           const_cast<wchar_t *>(UNISP_NAME), // pszPackage
                                           isClient ? SECPKG_CRED_OUTBOUND : SECPKG_CRED_INBOUND, // fCredentialUse
                                           nullptr, // pvLogonID (unused)
                                           &credentials, // pAuthData
                                           nullptr, // pGetKeyFn (unused)
                                           nullptr, // pvGetKeyArgument (unused)
                                           &credentialHandle, // phCredential
                                           &expiration // ptsExpir
    );

    if (status != SEC_E_OK) {
        setErrorAndEmit(d, QAbstractSocket::SslInternalError, schannelErrorToString(status));
        return false;
    }
    return true;
}

void TlsCryptographSchannel::deallocateContext()
{
    if (SecIsValidHandle(&contextHandle)) {
        DeleteSecurityContext(&contextHandle);
        SecInvalidateHandle(&contextHandle);
    }
}

void TlsCryptographSchannel::freeCredentialsHandle()
{
    if (SecIsValidHandle(&credentialHandle)) {
        FreeCredentialsHandle(&credentialHandle);
        SecInvalidateHandle(&credentialHandle);
    }
}

void TlsCryptographSchannel::closeCertificateStores()
{
    localCertificateStore.reset();
    peerCertificateStore.reset();
    caCertificateStore.reset();
}

bool TlsCryptographSchannel::createContext()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    Q_ASSERT(SecIsValidHandle(&credentialHandle));
    Q_ASSERT(schannelState == SchannelState::InitializeHandshake);
    Q_ASSERT(d->tlsMode() == QSslSocket::SslClientMode);
    ULONG contextReq = getContextRequirements();

    SecBuffer outBuffers[3];
    outBuffers[0] = createSecBuffer(nullptr, 0, SECBUFFER_TOKEN);
    outBuffers[1] = createSecBuffer(nullptr, 0, SECBUFFER_ALERT);
    outBuffers[2] = createSecBuffer(nullptr, 0, SECBUFFER_EMPTY);
    auto freeBuffers = qScopeGuard([&outBuffers]() {
        for (auto i = 0ull; i < ARRAYSIZE(outBuffers); i++) {
            if (outBuffers[i].pvBuffer)
                FreeContextBuffer(outBuffers[i].pvBuffer);
        }
    });
    SecBufferDesc outputBufferDesc{
        SECBUFFER_VERSION,
        ARRAYSIZE(outBuffers),
        outBuffers
    };

    TimeStamp expiry;

    SecBufferDesc alpnBufferDesc;
    bool useAlpn = false;
#ifdef SUPPORTS_ALPN
    QTlsBackend::setAlpnStatus(d, QSslConfiguration::NextProtocolNegotiationNone);
    QByteArray alpnString = createAlpnString(q->sslConfiguration().allowedNextProtocols());
    useAlpn = !alpnString.isEmpty();
    SecBuffer alpnBuffers[1];
    alpnBuffers[0] = createSecBuffer(alpnString, SECBUFFER_APPLICATION_PROTOCOLS);
    alpnBufferDesc = {
        SECBUFFER_VERSION,
        ARRAYSIZE(alpnBuffers),
        alpnBuffers
    };
#endif

    auto status = InitializeSecurityContext(&credentialHandle, // phCredential
                                            nullptr, // phContext
                                            const_reinterpret_cast<SEC_WCHAR *>(targetName().utf16()), // pszTargetName
                                            contextReq, // fContextReq
                                            0, // Reserved1
                                            0, // TargetDataRep (unused)
                                            useAlpn ? &alpnBufferDesc : nullptr, // pInput
                                            0, // Reserved2
                                            &contextHandle, // phNewContext
                                            &outputBufferDesc, // pOutput
                                            &contextAttributes, // pfContextAttr
                                            &expiry // ptsExpiry
    );

    // This is the first call to InitializeSecurityContext, so theoretically "CONTINUE_NEEDED"
    // should be the only non-error return-code here.
    if (status != SEC_I_CONTINUE_NEEDED) {
        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Error creating SSL context (%1)").arg(schannelErrorToString(status)));
        return false;
    }

    if (!sendToken(outBuffers[0].pvBuffer, outBuffers[0].cbBuffer))
        return false;
    schannelState = SchannelState::PerformHandshake;
    return true;
}

bool TlsCryptographSchannel::acceptContext()
{
    Q_ASSERT(d);
    Q_ASSERT(q);

    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    Q_ASSERT(SecIsValidHandle(&credentialHandle));
    Q_ASSERT(schannelState == SchannelState::InitializeHandshake);
    Q_ASSERT(d->tlsMode() == QSslSocket::SslServerMode);
    ULONG contextReq = getContextRequirements();

    if (missingData > plainSocket->bytesAvailable())
        return true;

    missingData = 0;
    readToBuffer(intermediateBuffer, plainSocket);
    if (intermediateBuffer.isEmpty())
        return true; // definitely need more data..

    SecBuffer inBuffers[2];
    inBuffers[0] = createSecBuffer(intermediateBuffer, SECBUFFER_TOKEN);

#ifdef SUPPORTS_ALPN
    QTlsBackend::setAlpnStatus(d, QSslConfiguration::NextProtocolNegotiationNone);
    // The string must be alive when we call AcceptSecurityContext
    QByteArray alpnString = createAlpnString(q->sslConfiguration().allowedNextProtocols());
    if (!alpnString.isEmpty()) {
        inBuffers[1] = createSecBuffer(alpnString, SECBUFFER_APPLICATION_PROTOCOLS);
    } else
#endif
    {
        inBuffers[1] = createSecBuffer(nullptr, 0, SECBUFFER_EMPTY);
    }

    SecBufferDesc inputBufferDesc{
        SECBUFFER_VERSION,
        ARRAYSIZE(inBuffers),
        inBuffers
    };

    SecBuffer outBuffers[3];
    outBuffers[0] = createSecBuffer(nullptr, 0, SECBUFFER_TOKEN);
    outBuffers[1] = createSecBuffer(nullptr, 0, SECBUFFER_ALERT);
    outBuffers[2] = createSecBuffer(nullptr, 0, SECBUFFER_EMPTY);
    auto freeBuffers = qScopeGuard([&outBuffers]() {
        for (auto i = 0ull; i < ARRAYSIZE(outBuffers); i++) {
            if (outBuffers[i].pvBuffer)
                FreeContextBuffer(outBuffers[i].pvBuffer);
        }
    });
    SecBufferDesc outputBufferDesc{
        SECBUFFER_VERSION,
        ARRAYSIZE(outBuffers),
        outBuffers
    };

    TimeStamp expiry;
    auto status = AcceptSecurityContext(
            &credentialHandle, // phCredential
            nullptr, // phContext
            &inputBufferDesc, // pInput
            contextReq, // fContextReq
            0, // TargetDataRep (unused)
            &contextHandle, // phNewContext
            &outputBufferDesc, // pOutput
            &contextAttributes, // pfContextAttr
            &expiry // ptsTimeStamp
    );

    if (status == SEC_E_INCOMPLETE_MESSAGE) {
        // Need more data
        missingData = checkIncompleteData(outBuffers[0]);
        return true;
    }

    if (inBuffers[1].BufferType == SECBUFFER_EXTRA) {
        // https://docs.microsoft.com/en-us/windows/desktop/secauthn/extra-buffers-returned-by-schannel
        // inBuffers[1].cbBuffer indicates the amount of bytes _NOT_ processed, the rest need to
        // be stored.
        retainExtraData(intermediateBuffer, inBuffers[1]);
    } else { /* No 'extra' data, message not incomplete */
        intermediateBuffer.resize(0);
    }

    if (status != SEC_I_CONTINUE_NEEDED) {
        setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                        QSslSocket::tr("Error creating SSL context (%1)").arg(schannelErrorToString(status)));
        return false;
    }
    if (!sendToken(outBuffers[0].pvBuffer, outBuffers[0].cbBuffer))
        return false;
    schannelState = SchannelState::PerformHandshake;
    return true;
}

bool TlsCryptographSchannel::performHandshake()
{
    Q_ASSERT(d);
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    if (plainSocket->state() == QAbstractSocket::UnconnectedState || !plainSocket->isValid()
        || !plainSocket->isOpen()) {
        setErrorAndEmit(d, QAbstractSocket::RemoteHostClosedError,
                        QSslSocket::tr("The TLS/SSL connection has been closed"));
        return false;
    }
    Q_ASSERT(SecIsValidHandle(&credentialHandle));
    Q_ASSERT(SecIsValidHandle(&contextHandle));
    Q_ASSERT(schannelState == SchannelState::PerformHandshake);

#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcTlsBackendSchannel, "Bytes available from socket: %lld",
            plainSocket->bytesAvailable());
    qCDebug(lcTlsBackendSchannel, "intermediateBuffer size: %d", intermediateBuffer.size());
#endif

    if (missingData > plainSocket->bytesAvailable())
        return true;

    missingData = 0;
    readToBuffer(intermediateBuffer, plainSocket);
    if (intermediateBuffer.isEmpty())
        return true; // no data, will fail

    SecBuffer outBuffers[3] = {};
    const auto freeOutBuffers = [&outBuffers]() {
        for (auto i = 0ull; i < ARRAYSIZE(outBuffers); i++) {
            if (outBuffers[i].pvBuffer)
                FreeContextBuffer(outBuffers[i].pvBuffer);
        }
    };
    const auto outBuffersGuard = qScopeGuard(freeOutBuffers);
    // For this call to InitializeSecurityContext we may need to call it twice.
    // In some cases us not having a certificate isn't actually an error, but just a request.
    // With Schannel, to ignore this warning, we need to call InitializeSecurityContext again
    // when we get SEC_I_INCOMPLETE_CREDENTIALS! As far as I can tell it's not documented anywhere.
    // https://stackoverflow.com/a/47479968/2493610
    SECURITY_STATUS status;
    short attempts = 2;
    do {
        SecBuffer inputBuffers[2];
        inputBuffers[0] = createSecBuffer(intermediateBuffer, SECBUFFER_TOKEN);
        inputBuffers[1] = createSecBuffer(nullptr, 0, SECBUFFER_EMPTY);
        SecBufferDesc inputBufferDesc{
            SECBUFFER_VERSION,
            ARRAYSIZE(inputBuffers),
            inputBuffers
        };

        freeOutBuffers(); // free buffers from any previous attempt
        outBuffers[0] = createSecBuffer(nullptr, 0, SECBUFFER_TOKEN);
        outBuffers[1] = createSecBuffer(nullptr, 0, SECBUFFER_ALERT);
        outBuffers[2] = createSecBuffer(nullptr, 0, SECBUFFER_EMPTY);
        SecBufferDesc outputBufferDesc{
            SECBUFFER_VERSION,
            ARRAYSIZE(outBuffers),
            outBuffers
        };

        ULONG contextReq = getContextRequirements();
        TimeStamp expiry;
        status = InitializeSecurityContext(
                &credentialHandle, // phCredential
                &contextHandle, // phContext
                const_reinterpret_cast<SEC_WCHAR *>(targetName().utf16()), // pszTargetName
                contextReq, // fContextReq
                0, // Reserved1
                0, // TargetDataRep (unused)
                &inputBufferDesc, // pInput
                0, // Reserved2
                nullptr, // phNewContext (we already have one)
                &outputBufferDesc, // pOutput
                &contextAttributes, // pfContextAttr
                &expiry // ptsExpiry
        );

        if (inputBuffers[1].BufferType == SECBUFFER_EXTRA) {
            // https://docs.microsoft.com/en-us/windows/desktop/secauthn/extra-buffers-returned-by-schannel
            // inputBuffers[1].cbBuffer indicates the amount of bytes _NOT_ processed, the rest need
            // to be stored.
            retainExtraData(intermediateBuffer, inputBuffers[1]);
        } else if (status != SEC_E_INCOMPLETE_MESSAGE) {
            // Clear the buffer if we weren't asked for more data
            intermediateBuffer.resize(0);
        }

        --attempts;
    } while (status == SEC_I_INCOMPLETE_CREDENTIALS && attempts > 0);

    switch (status) {
    case SEC_E_OK:
        // Need to transmit a final token in the handshake if 'cbBuffer' is non-zero.
        if (!sendToken(outBuffers[0].pvBuffer, outBuffers[0].cbBuffer))
            return false;
        schannelState = SchannelState::VerifyHandshake;
        return true;
    case SEC_I_CONTINUE_NEEDED:
        if (!sendToken(outBuffers[0].pvBuffer, outBuffers[0].cbBuffer))
            return false;
        // Must call InitializeSecurityContext again later (done through continueHandshake)
        return true;
    case SEC_I_INCOMPLETE_CREDENTIALS:
        // Schannel takes care of picking certificate to send (other than the one we can specify),
        // so if we get here then that means we don't have a certificate the server accepts.
        setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                        QSslSocket::tr("Server did not accept any certificate we could present."));
        return false;
    case SEC_I_CONTEXT_EXPIRED:
        // "The message sender has finished using the connection and has initiated a shutdown."
        if (outBuffers[0].BufferType == SECBUFFER_TOKEN) {
            if (!sendToken(outBuffers[0].pvBuffer, outBuffers[0].cbBuffer))
                return false;
        }
        if (!shutdown) { // we did not initiate this
            setErrorAndEmit(d, QAbstractSocket::RemoteHostClosedError,
                            QSslSocket::tr("The TLS/SSL connection has been closed"));
        }
        return true;
    case SEC_E_INCOMPLETE_MESSAGE:
        // Simply incomplete, wait for more data
        missingData = checkIncompleteData(outBuffers[0]);
        return true;
    case SEC_E_ALGORITHM_MISMATCH:
        setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                        QSslSocket::tr("Algorithm mismatch"));
        shutdown = true; // skip sending the "Shutdown" alert
        return false;
    }

    // Note: We can get here if the connection is using TLS 1.2 and the server certificate uses
    // MD5, which is not allowed in Schannel. This causes an "invalid token" error during handshake.
    // (If you came here investigating an error: md5 is insecure, update your certificate)
    setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                    QSslSocket::tr("Handshake failed: %1").arg(schannelErrorToString(status)));
    return false;
}

bool TlsCryptographSchannel::verifyHandshake()
{
    Q_ASSERT(d);
    Q_ASSERT(q);
    const auto &configuration = q->sslConfiguration();

    sslErrors.clear();

    const bool isClient = d->tlsMode() == QSslSocket::SslClientMode;
#define CHECK_STATUS(status)                                                  \
    if (status != SEC_E_OK) {                                                 \
        setErrorAndEmit(d, QAbstractSocket::SslInternalError,                 \
                        QSslSocket::tr("Failed to query the TLS context: %1") \
                                .arg(schannelErrorToString(status)));         \
        return false;                                                         \
    }

    // Everything is set up, now make sure there's nothing wrong and query some attributes...
    if (!matchesContextRequirements(contextAttributes, getContextRequirements(),
                                    configuration.peerVerifyMode(), isClient)) {
        setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                        QSslSocket::tr("Did not get the required attributes for the connection."));
        return false;
    }

    // Get stream sizes (to know the max size of a message and the size of the header and trailer)
    auto status = QueryContextAttributes(&contextHandle,
                                         SECPKG_ATTR_STREAM_SIZES,
                                         &streamSizes);
    CHECK_STATUS(status);

    // Get session cipher info
    status = QueryContextAttributes(&contextHandle,
                                    SECPKG_ATTR_CIPHER_INFO,
                                    &cipherInfo);
    CHECK_STATUS(status);

    status = QueryContextAttributes(&contextHandle,
                                    SECPKG_ATTR_CONNECTION_INFO,
                                    &connectionInfo);
    CHECK_STATUS(status);

#ifdef SUPPORTS_ALPN
    const auto allowedProtos = configuration.allowedNextProtocols();
    if (!allowedProtos.isEmpty()) {
        SecPkgContext_ApplicationProtocol alpn;
        status = QueryContextAttributes(&contextHandle,
                                        SECPKG_ATTR_APPLICATION_PROTOCOL,
                                        &alpn);
        CHECK_STATUS(status);
        if (alpn.ProtoNegoStatus == SecApplicationProtocolNegotiationStatus_Success) {
            QByteArray negotiatedProto = QByteArray((const char *)alpn.ProtocolId,
                                                    alpn.ProtocolIdSize);
            if (!allowedProtos.contains(negotiatedProto)) {
                setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                                QSslSocket::tr("Unwanted protocol was negotiated"));
                return false;
            }
            QTlsBackend::setNegotiatedProtocol(d, negotiatedProto);
            QTlsBackend::setAlpnStatus(d, QSslConfiguration::NextProtocolNegotiationNegotiated);
        } else {
            QTlsBackend::setNegotiatedProtocol(d, {});
            QTlsBackend::setAlpnStatus(d, QSslConfiguration::NextProtocolNegotiationUnsupported);
        }
    }
#endif // supports ALPN

#undef CHECK_STATUS

    // Verify certificate
    CERT_CONTEXT *certificateContext = nullptr;
    auto freeCertificate = qScopeGuard([&certificateContext]() {
        if (certificateContext)
            CertFreeCertificateContext(certificateContext);
    });
    status = QueryContextAttributes(&contextHandle,
                                    SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                    &certificateContext);

    // QueryPeer can (currently) not work in Schannel since Schannel itself doesn't have a way to
    // ask for a certificate and then still be OK if it's not received.
    // To work around this we don't request a certificate at all for QueryPeer.
    // For servers AutoVerifyPeer is supposed to be treated the same as QueryPeer.
    // This means that servers using Schannel will only request client certificate for "VerifyPeer".
    if ((!isClient && configuration.peerVerifyMode() == QSslSocket::PeerVerifyMode::VerifyPeer)
        || (isClient && configuration.peerVerifyMode() != QSslSocket::PeerVerifyMode::VerifyNone
            && configuration.peerVerifyMode() != QSslSocket::PeerVerifyMode::QueryPeer)) {
        if (status != SEC_E_OK) {
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcTlsBackendSchannel) << "Couldn't retrieve peer certificate, status:"
                                          << schannelErrorToString(status);
#endif
            const QSslError error{ QSslError::NoPeerCertificate };
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    // verifyCertContext returns false if the user disconnected while it was checking errors.
    if (certificateContext && !verifyCertContext(certificateContext))
        return false;

    if (!checkSslErrors() || q->state() != QAbstractSocket::ConnectedState) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcTlsBackendSchannel) << __func__ << "was unsuccessful. Paused:" << d->isPaused();
#endif
        // If we're paused then checkSslErrors returned false, but it's not an error
        return d->isPaused() && q->state() == QAbstractSocket::ConnectedState;
    }

    schannelState = SchannelState::Done;
    return true;
}

bool TlsCryptographSchannel::renegotiate()
{
    Q_ASSERT(d);

    SecBuffer outBuffers[3];
    outBuffers[0] = createSecBuffer(nullptr, 0, SECBUFFER_TOKEN);
    outBuffers[1] = createSecBuffer(nullptr, 0, SECBUFFER_ALERT);
    outBuffers[2] = createSecBuffer(nullptr, 0, SECBUFFER_EMPTY);
    auto freeBuffers = qScopeGuard([&outBuffers]() {
        for (auto i = 0ull; i < ARRAYSIZE(outBuffers); i++) {
            if (outBuffers[i].pvBuffer)
                FreeContextBuffer(outBuffers[i].pvBuffer);
        }
    });
    SecBufferDesc outputBufferDesc{
        SECBUFFER_VERSION,
        ARRAYSIZE(outBuffers),
        outBuffers
    };

    ULONG contextReq = getContextRequirements();
    TimeStamp expiry;
    SECURITY_STATUS status;
    if (d->tlsMode() == QSslSocket::SslClientMode) {
        status = InitializeSecurityContext(&credentialHandle, // phCredential
                                           &contextHandle, // phContext
                                           const_reinterpret_cast<SEC_WCHAR *>(targetName().utf16()), // pszTargetName
                                           contextReq, // fContextReq
                                           0, // Reserved1
                                           0, // TargetDataRep (unused)
                                           nullptr, // pInput (nullptr for renegotiate)
                                           0, // Reserved2
                                           nullptr, // phNewContext (we already have one)
                                           &outputBufferDesc, // pOutput
                                           &contextAttributes, // pfContextAttr
                                           &expiry // ptsExpiry
        );
    } else {
        status = AcceptSecurityContext(
                &credentialHandle, // phCredential
                &contextHandle, // phContext
                nullptr, // pInput
                contextReq, // fContextReq
                0, // TargetDataRep (unused)
                nullptr, // phNewContext
                &outputBufferDesc, // pOutput
                &contextAttributes, // pfContextAttr,
                &expiry // ptsTimeStamp
        );
    }
    if (status == SEC_I_CONTINUE_NEEDED) {
        schannelState = SchannelState::PerformHandshake;
        return sendToken(outBuffers[0].pvBuffer, outBuffers[0].cbBuffer);
    } else if (status == SEC_E_OK) {
        schannelState = SchannelState::PerformHandshake;
        return true;
    }
    setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                    QSslSocket::tr("Renegotiation was unsuccessful: %1").arg(schannelErrorToString(status)));
    return false;
}

/*!
    \internal
    reset the state in preparation for reuse of socket
*/
void TlsCryptographSchannel::reset()
{
    Q_ASSERT(d);

    closeCertificateStores(); // certificate stores could've changed
    deallocateContext();
    freeCredentialsHandle(); // in case we already had one (@future: session resumption requires re-use)

    cipherInfo = {};
    connectionInfo = {};
    streamSizes = {};

    contextAttributes = 0;
    intermediateBuffer.clear();
    schannelState = SchannelState::InitializeHandshake;


    d->setEncrypted(false);
    shutdown = false;
    renegotiating = false;

    missingData = 0;
}

void TlsCryptographSchannel::startClientEncryption()
{
    Q_ASSERT(q);

    if (q->isEncrypted())
        return; // let's not mess up the connection...
    reset();
    continueHandshake();
}

void TlsCryptographSchannel::startServerEncryption()
{
    Q_ASSERT(q);

    if (q->isEncrypted())
        return; // let's not mess up the connection...
    reset();
    continueHandshake();
}

auto TlsCryptographSchannel::getNextEncryptedMessage() -> MessageBufferResult
{
    MessageBufferResult result;
    QByteArray &fullMessage = result.messageBuffer;

    Q_ASSERT(d);
    auto &writeBuffer = d->tlsWriteBuffer();

    auto allocateMessage = [&fullMessage](qsizetype size) -> QSpan<char> {
        auto targetSize = fullMessage.size() + size;
        if (fullMessage.capacity() < targetSize) {
            qsizetype newSize = fullMessage.capacity() * 2;
            if (newSize < targetSize)
                newSize = targetSize;
            fullMessage.reserve(newSize);
        }
        fullMessage.resizeForOverwrite(targetSize);
        return QSpan(fullMessage).subspan(fullMessage.size() - size);
    };

    const int headerSize = int(streamSizes.cbHeader);
    const int trailerSize = int(streamSizes.cbTrailer);
    constexpr qsizetype MessageBufferThreshold = 128 * 1024;

    qint64 writeBufferSize = 0;
    while ((writeBufferSize = writeBuffer.size()) > 0
           && fullMessage.size() < MessageBufferThreshold) {
        // Try to read 'cbMaximumMessage' bytes from buffer before encrypting.
        const int bodySize = int(std::min(writeBufferSize, qint64(streamSizes.cbMaximumMessage)));
        const qsizetype messageSize = headerSize + bodySize + trailerSize;
        QSpan buffer = allocateMessage(messageSize);
        char *header = buffer.data();
        char *body = header + headerSize;
        char *trailer = body + bodySize;
        {
            // Use peek() here instead of read() so we don't lose data if encryption fails.
            qint64 copied = writeBuffer.peek(body, bodySize);
            Q_ASSERT(copied == bodySize);
        }

        SecBuffer inputBuffers[] = {
            createSecBuffer(header, headerSize, SECBUFFER_STREAM_HEADER),
            createSecBuffer(body, bodySize, SECBUFFER_DATA),
            createSecBuffer(trailer, trailerSize, SECBUFFER_STREAM_TRAILER),
            createSecBuffer(nullptr, 0, SECBUFFER_EMPTY)
        };
        SecBufferDesc message = {
            SECBUFFER_VERSION,
            ARRAYSIZE(inputBuffers),
            inputBuffers
        };

        if (auto status = EncryptMessage(&contextHandle, 0, &message, 0); status != SEC_E_OK) {
            setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                            QSslSocket::tr("Schannel failed to encrypt data: %1")
                                    .arg(schannelErrorToString(status)));
            result.messageBuffer.chop(messageSize);
            return result;
        }
        // Data was encrypted successfully, so we free() what we peek()ed earlier
        writeBuffer.free(bodySize);

        // The trailer's size is not final, so resize fullMessage to not send trailing junk
        auto finalSize = qsizetype(inputBuffers[0].cbBuffer + inputBuffers[1].cbBuffer
                                   + inputBuffers[2].cbBuffer);
        fullMessage.chop(messageSize - finalSize);
    }
    result.ok = true;
    return result;
}

void TlsCryptographSchannel::transmit()
{
    Q_ASSERT(q);
    Q_ASSERT(d);
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    if (d->tlsMode() == QSslSocket::UnencryptedMode)
        return; // This function should not have been called

    // Can happen if called through QSslSocket::abort->QSslSocket::close->QSslSocket::flush->here
    if (plainSocket->state() == QAbstractSocket::UnconnectedState || !plainSocket->isValid()
        || !plainSocket->isOpen()) {
        return;
    }

    if (schannelState != SchannelState::Done) {
        continueHandshake();
        return;
    }

    auto &buffer = d->tlsBuffer();
    if (q->isEncrypted()) { // encrypt data in writeBuffer and write it to plainSocket
        qint64 totalBytesWritten = 0;
        while (d->tlsWriteBuffer().size() > 0) {
            MessageBufferResult r = getNextEncryptedMessage();
            if (r.messageBuffer.isEmpty() && !r.ok)
                return;
            QByteArray fullMessage = std::move(r.messageBuffer);
            const qint64 bytesWritten = plainSocket->write(fullMessage);
            if (!r.ok && bytesWritten < 0)
                break; // We might have to emit bytesWritten, so break instead of return
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcTlsBackendSchannel, "Wrote %lld of total %d bytes", bytesWritten,
                    fullMessage.size());
#endif
            if (bytesWritten >= 0) {
                totalBytesWritten += bytesWritten;
            } else {
                setErrorAndEmit(d, plainSocket->error(), plainSocket->errorString());
                return;
            }
        }

        if (totalBytesWritten > 0) {
            // Don't emit bytesWritten() recursively.
            bool &emittedBytesWritten = d->tlsEmittedBytesWritten();
            if (!emittedBytesWritten) {
                emittedBytesWritten = true;
                emit q->bytesWritten(totalBytesWritten);
                emittedBytesWritten = false;
            }
            emit q->channelBytesWritten(0, totalBytesWritten);
        }
    }

    int totalRead = 0;
    bool hadIncompleteData = false;
    const auto readBufferMaxSize = d->maxReadBufferSize();
    while (!readBufferMaxSize || buffer.size() < readBufferMaxSize) {
        if (missingData > plainSocket->bytesAvailable()
            && (!readBufferMaxSize || readBufferMaxSize >= missingData)) {
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcTlsBackendSchannel, "We're still missing %lld bytes, will check later.",
                    missingData);
#endif
            break;
        }

        missingData = 0;
        const qint64 bytesRead = readToBuffer(intermediateBuffer, plainSocket);
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcTlsBackendSchannel, "Read %lld encrypted bytes from the socket", bytesRead);
#endif
        if (intermediateBuffer.length() == 0 || (hadIncompleteData && bytesRead == 0)) {
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcTlsBackendSchannel,
                    hadIncompleteData ? "No new data received, leaving loop!"
                                      : "Nothing to decrypt, leaving loop!");
#endif
            break;
        }
        hadIncompleteData = false;
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcTlsBackendSchannel, "Total amount of bytes to decrypt: %d",
                intermediateBuffer.length());
#endif

        SecBuffer dataBuffer[4]{
            createSecBuffer(intermediateBuffer, SECBUFFER_DATA),
            createSecBuffer(nullptr, 0, SECBUFFER_EMPTY),
            createSecBuffer(nullptr, 0, SECBUFFER_EMPTY),
            createSecBuffer(nullptr, 0, SECBUFFER_EMPTY)
        };
        SecBufferDesc message{
            SECBUFFER_VERSION,
            ARRAYSIZE(dataBuffer),
            dataBuffer
        };
        auto status = DecryptMessage(&contextHandle, &message, 0, nullptr);
        if (status == SEC_E_OK || status == SEC_I_RENEGOTIATE || status == SEC_I_CONTEXT_EXPIRED) {
            // There can still be 0 output even if it succeeds, this is fine
            if (dataBuffer[1].cbBuffer > 0) {
                // It is always decrypted in-place.
                // But [0] is the STREAM_HEADER, [1] is the DATA and [2] is the STREAM_TRAILER.
                // The pointers in all of those still point into 'intermediateBuffer'.
                buffer.append(static_cast<char *>(dataBuffer[1].pvBuffer),
                                dataBuffer[1].cbBuffer);
                totalRead += dataBuffer[1].cbBuffer;
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcTlsBackendSchannel, "Decrypted %lu bytes. New read buffer size: %d",
                        dataBuffer[1].cbBuffer, buffer.size());
#endif
            }
            if (dataBuffer[3].BufferType == SECBUFFER_EXTRA) {
                // https://docs.microsoft.com/en-us/windows/desktop/secauthn/extra-buffers-returned-by-schannel
                // dataBuffer[3].cbBuffer indicates the amount of bytes _NOT_ processed,
                // the rest need to be stored.
                retainExtraData(intermediateBuffer, dataBuffer[3]);
            } else {
                intermediateBuffer.resize(0);
            }
        }

        if (status == SEC_E_INCOMPLETE_MESSAGE) {
            missingData = checkIncompleteData(dataBuffer[0]);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcTlsBackendSchannel, "We didn't have enough data to decrypt anything, will try again!");
#endif
            // We try again, but if we don't get any more data then we leave
            hadIncompleteData = true;
        } else if (status == SEC_E_INVALID_HANDLE) {
            // I don't think this should happen, if it does we're done...
            qCWarning(lcTlsBackendSchannel, "The internal SSPI handle is invalid!");
            Q_UNREACHABLE();
        } else if (status == SEC_E_INVALID_TOKEN) {
            // Supposedly we have an invalid token, it's under-documented what
            // this means, so to be safe we disconnect.
            shutdown = true;
            disconnectFromHost();
            setErrorAndEmit(d, QAbstractSocket::SslInternalError, schannelErrorToString(status));
            break;
        } else if (status == SEC_E_MESSAGE_ALTERED) {
            // The message has been altered, disconnect now.
            shutdown = true; // skips sending the shutdown alert
            disconnectFromHost();
            setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                            schannelErrorToString(status));
            break;
        } else if (status == SEC_E_OUT_OF_SEQUENCE) {
            // @todo: I don't know if this one is actually "fatal"..
            // This path might never be hit as it seems this is for connection-oriented connections,
            // while SEC_E_MESSAGE_ALTERED is for stream-oriented ones (what we use).
            shutdown = true; // skips sending the shutdown alert
            disconnectFromHost();
            setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                            schannelErrorToString(status));
            break;
        } else if (status == SEC_E_DECRYPT_FAILURE) {
            // It's not documented as a possible return value for DecryptMessage,
            // but we see that this may happen - supposed to be a bug in Schannel (with TLS 1.3?)
            shutdown = true; // skips sending the shutdown alert
            disconnectFromHost();
            setErrorAndEmit(d, QAbstractSocket::SslInternalError, schannelErrorToString(status));
            break;
        } else if (status == SEC_I_CONTEXT_EXPIRED) {
            // 'remote' has initiated a shutdown
            disconnectFromHost();
            break;
        } else if (status == SEC_I_RENEGOTIATE) {
            // 'remote' wants to renegotiate
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcTlsBackendSchannel, "The peer wants to renegotiate.");
#endif
            schannelState = SchannelState::Renegotiate;
            renegotiating = true;

            // We need to call 'continueHandshake' or else there's no guarantee it ever gets called
            continueHandshake();
            break;
        }
    }

    if (totalRead) {
        if (bool *readyReadEmittedPointer = d->readyReadPointer())
            *readyReadEmittedPointer = true;
        emit q->readyRead();
        emit q->channelReadyRead(0);
    }
}

void TlsCryptographSchannel::sendShutdown()
{
    Q_ASSERT(d);

    const bool isClient = d->tlsMode() == QSslSocket::SslClientMode;
    DWORD shutdownToken = SCHANNEL_SHUTDOWN;
    SecBuffer buffer = createSecBuffer(&shutdownToken, sizeof(DWORD), SECBUFFER_TOKEN);
    SecBufferDesc token{
        SECBUFFER_VERSION,
        1,
        &buffer
    };
    auto status = ApplyControlToken(&contextHandle, &token);

    if (status != SEC_E_OK) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcTlsBackendSchannel)
                << "Failed to apply shutdown control token:" << schannelErrorToString(status);
#endif
        return;
    }

    SecBuffer outBuffers[3];
    outBuffers[0] = createSecBuffer(nullptr, 0, SECBUFFER_TOKEN);
    outBuffers[1] = createSecBuffer(nullptr, 0, SECBUFFER_ALERT);
    outBuffers[2] = createSecBuffer(nullptr, 0, SECBUFFER_EMPTY);
    auto freeBuffers = qScopeGuard([&outBuffers]() {
        for (auto i = 0ull; i < ARRAYSIZE(outBuffers); i++) {
            if (outBuffers[i].pvBuffer)
                FreeContextBuffer(outBuffers[i].pvBuffer);
        }
    });
    SecBufferDesc outputBufferDesc{
        SECBUFFER_VERSION,
        ARRAYSIZE(outBuffers),
        outBuffers
    };

    ULONG contextReq = getContextRequirements();
    TimeStamp expiry;
    if (isClient) {
        status = InitializeSecurityContext(&credentialHandle, // phCredential
                                           &contextHandle, // phContext
                                           const_reinterpret_cast<SEC_WCHAR *>(targetName().utf16()), // pszTargetName
                                           contextReq, // fContextReq
                                           0, // Reserved1
                                           0, // TargetDataRep (unused)
                                           nullptr, // pInput
                                           0, // Reserved2
                                           nullptr, // phNewContext (we already have one)
                                           &outputBufferDesc, // pOutput
                                           &contextAttributes, // pfContextAttr
                                           &expiry // ptsExpiry
        );
    } else {
        status = AcceptSecurityContext(
                &credentialHandle, // phCredential
                &contextHandle, // phContext
                nullptr, // pInput
                contextReq, // fContextReq
                0, // TargetDataRep (unused)
                nullptr, // phNewContext
                &outputBufferDesc, // pOutput
                &contextAttributes, // pfContextAttr,
                &expiry // ptsTimeStamp
        );
    }
    if (status == SEC_E_OK || status == SEC_I_CONTEXT_EXPIRED) {
        if (!sendToken(outBuffers[0].pvBuffer, outBuffers[0].cbBuffer, false)) {
            // We failed to send the shutdown message, but it's not that important since we're
            // shutting down anyway.
            return;
        }
    } else {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcTlsBackendSchannel)
                << "Failed to initialize shutdown:" << schannelErrorToString(status);
#endif
    }
}

void TlsCryptographSchannel::disconnectFromHost()
{
    Q_ASSERT(q);
    Q_ASSERT(d);
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    if (SecIsValidHandle(&contextHandle)) {
        if (!shutdown) {
            shutdown = true;
            if (plainSocket->state() != QAbstractSocket::UnconnectedState && q->isEncrypted()) {
                sendShutdown();
                transmit();
            }
        }
    }
    plainSocket->disconnectFromHost();
}

void TlsCryptographSchannel::disconnected()
{
    Q_ASSERT(d);
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);
    d->setEncrypted(false);

    shutdown = true;
    if (plainSocket->bytesAvailable() > 0 || hasUndecryptedData()) {
        // Read as much as possible because this is likely our last chance
        qint64 tempMax = d->maxReadBufferSize();
        d->setMaxReadBufferSize(0); // Unlimited
        transmit();
        d->setMaxReadBufferSize(tempMax);
        // Since there were bytes still available we don't want to deallocate
        // our context yet. It will happen later, when the socket is re-used or
        // destroyed.
    } else {
        deallocateContext();
        freeCredentialsHandle();
    }
}

QSslCipher TlsCryptographSchannel::sessionCipher() const
{
    Q_ASSERT(q);

    if (!q->isEncrypted())
        return {};

    const auto sessionProtocol = toQtSslProtocol(connectionInfo.dwProtocol);

    const auto ciphers = ciphersByName(QStringView(cipherInfo.szCipherSuite));
    for (const auto& cipher : ciphers) {
        if (cipher.protocol() == sessionProtocol)
            return cipher;
    }

    return {};
}

QSsl::SslProtocol TlsCryptographSchannel::sessionProtocol() const
{
    if (!q->isEncrypted())
        return QSsl::SslProtocol::UnknownProtocol;
    return toQtSslProtocol(connectionInfo.dwProtocol);
}

void TlsCryptographSchannel::continueHandshake()
{
    Q_ASSERT(q);
    Q_ASSERT(d);
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    const bool isServer = d->tlsMode() == QSslSocket::SslServerMode;
    switch (schannelState) {
    case SchannelState::InitializeHandshake:
        if (!SecIsValidHandle(&credentialHandle) && !acquireCredentialsHandle()) {
            disconnectFromHost();
            return;
        }
        if (!SecIsValidHandle(&credentialHandle)) // Needed to support tst_QSslSocket::setEmptyKey
            return;
        if (!SecIsValidHandle(&contextHandle) && !(isServer ? acceptContext() : createContext())) {
            disconnectFromHost();
            return;
        }
        if (schannelState != SchannelState::PerformHandshake)
            break;
        Q_FALLTHROUGH();
    case SchannelState::PerformHandshake:
        if (!performHandshake()) {
            disconnectFromHost();
            return;
        }
        if (schannelState != SchannelState::VerifyHandshake)
            break;
        Q_FALLTHROUGH();
    case SchannelState::VerifyHandshake:
        // if we're in shutdown or renegotiating then we might not need to verify
        // (since we already did)
        if (!verifyHandshake()) {
            shutdown = true; // Skip sending shutdown alert
            q->abort(); // We don't want to send buffered data
            disconnectFromHost();
            return;
        }
        if (schannelState != SchannelState::Done)
            break;
        Q_FALLTHROUGH();
    case SchannelState::Done:
        // connectionEncrypted is already true if we come here from a renegotiation
        if (!q->isEncrypted()) {
            d->setEncrypted(true); // all is done
            emit q->encrypted();
        }
        renegotiating = false;
        if (d->isPendingClose()) {
            d->setPendingClose(false);
            disconnectFromHost();
        } else {
            transmit();
        }
        break;
    case SchannelState::Renegotiate:
        if (!renegotiate()) {
            disconnectFromHost();
            return;
        } else if (intermediateBuffer.size() || plainSocket->bytesAvailable()) {
            continueHandshake();
        }
        break;
    }
}

QList<QSslError> TlsCryptographSchannel::tlsErrors() const
{
    return sslErrors;
}

/*
    Copied from qsslsocket_mac.cpp, which was copied from qsslsocket_openssl.cpp
*/
bool TlsCryptographSchannel::checkSslErrors()
{
    if (sslErrors.isEmpty())
        return true;

    Q_ASSERT(q);
    Q_ASSERT(d);
    const auto &configuration = q->sslConfiguration();
    auto *plainSocket = d->plainTcpSocket();

    emit q->sslErrors(sslErrors);

    const bool doVerifyPeer = configuration.peerVerifyMode() == QSslSocket::VerifyPeer
            || (configuration.peerVerifyMode() == QSslSocket::AutoVerifyPeer
                && d->tlsMode() == QSslSocket::SslClientMode);
    const bool doEmitSslError = !d->verifyErrorsHaveBeenIgnored();
    // check whether we need to emit an SSL handshake error
    if (doVerifyPeer && doEmitSslError) {
        if (q->pauseMode() & QAbstractSocket::PauseOnSslErrors) {
            QSslSocketPrivate::pauseSocketNotifiers(q);
            d->setPaused(true);
        } else {
            setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                            sslErrors.constFirst().errorString());
            plainSocket->disconnectFromHost();
        }
        return false;
    }

    return true;
}

static void attachPrivateKeyToCertificate(const QSslCertificate &certificate,
                                          const QSslKey &privateKey)
{
    QAsn1Element elem = _q_PKCS12_key(privateKey);
    QByteArray buffer;
    QDataStream stream(&buffer, QDataStream::WriteOnly);
    elem.write(stream);
    NCRYPT_PROV_HANDLE provider = 0;
    SECURITY_STATUS status = NCryptOpenStorageProvider(&provider, MS_KEY_STORAGE_PROVIDER, 0);
    if (status != SEC_E_OK) {
        qCWarning(lcTlsBackendSchannel())
                << "Failed to open ncrypt storage provider:" << schannelErrorToString(status);
        return;
    }
    const auto freeProvider = qScopeGuard([provider]() { NCryptFreeObject(provider); });

    const QString certName = certificate.subjectInfo(QSslCertificate::CommonName).front();
    QSpan<const QChar> nameSpan(certName);
    NCryptBuffer nbuffer{ ULONG(nameSpan.size_bytes() + sizeof(char16_t)),
                          NCRYPTBUFFER_PKCS_KEY_NAME,
                          const_reinterpret_cast<void *>(nameSpan.data()) };
    NCryptBufferDesc bufferDesc{ NCRYPTBUFFER_VERSION, 1, &nbuffer };
    NCRYPT_KEY_HANDLE ncryptKey = 0;
    status = NCryptImportKey(provider, 0, NCRYPT_PKCS8_PRIVATE_KEY_BLOB, &bufferDesc, &ncryptKey,
                             PBYTE(buffer.data()), buffer.size(), 0);
    if (status != SEC_E_OK) {
        qCWarning(lcTlsBackendSchannel())
                << "Failed to import private key:" << schannelErrorToString(status);
        return;
    }
    const auto freeKey = qScopeGuard([ncryptKey]() { NCryptFreeObject(ncryptKey); });

    CERT_CONTEXT *context = PCERT_CONTEXT(certificate.handle());
    Q_ASSERT(context);

    CRYPT_DATA_BLOB keyBlob = { sizeof(ncryptKey), PBYTE(&ncryptKey) };
    BOOL ok =
            CertSetCertificateContextProperty(context, CERT_NCRYPT_KEY_HANDLE_PROP_ID, 0, &keyBlob);
    if (!ok) {
        auto error = GetLastError();
        if (lcTlsBackendSchannel().isWarningEnabled())
            qErrnoWarning(int(error), "Failed to set ncrypt handle property.");
        return;
    }

    CRYPT_KEY_PROV_INFO provInfo{
        const_reinterpret_cast<LPWSTR>(certName.constData()),
        const_cast<LPWSTR>(MS_KEY_STORAGE_PROVIDER),
        0,
        CERT_SET_KEY_PROV_HANDLE_PROP_ID | CERT_SET_KEY_CONTEXT_PROP_ID,
        0,
        nullptr,
        0,
    };
    ok = CertSetCertificateContextProperty(context, CERT_KEY_PROV_INFO_PROP_ID,
                                           CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG, &provInfo);
    if (!ok) {
        auto error = GetLastError();
        if (lcTlsBackendSchannel().isWarningEnabled())
            qErrnoWarning(int(error), "Failed to set key provider info property.");
        return;
    }
}

void TlsCryptographSchannel::initializeCertificateStores()
{
    //// helper function which turns a chain into a certificate store
    Q_ASSERT(d);
    Q_ASSERT(q);
    const auto &configuration = q->sslConfiguration();

    auto createStoreFromCertificateChain = [](const QList<QSslCertificate> certChain, const QSslKey &privateKey) {
        const wchar_t *passphrase = L"";
        // Need to embed the private key in the certificate
        QByteArray pkcs12 = _q_makePkcs12(certChain,
                                          privateKey,
                                          QString::fromWCharArray(passphrase, 0));
        CRYPT_DATA_BLOB pfxBlob;
        pfxBlob.cbData = DWORD(pkcs12.length());
        pfxBlob.pbData = reinterpret_cast<unsigned char *>(pkcs12.data());
        // ALWAYS_CNG to import using "Cryptography API: Next Generation (CNG)"
        // NO_PERSIST_KEY to request not persisting anything imported to disk
        constexpr DWORD flags = PKCS12_ALWAYS_CNG_KSP | PKCS12_NO_PERSIST_KEY;
        return QHCertStorePointer(PFXImportCertStore(&pfxBlob, passphrase, flags));
    };

    if (!configuration.localCertificateChain().isEmpty()) {
        if (configuration.privateKey().isNull()) {
            setErrorAndEmit(d, QAbstractSocket::SslInvalidUserDataError,
                            QSslSocket::tr("Cannot provide a certificate with no key"));
            return;
        }
        if (localCertificateStore == nullptr) {
            localCertificateStore =
                    createStoreFromCertificateChain(configuration.localCertificateChain(), {});
            if (localCertificateStore) {
                const CERT_CONTEXT *certificateContext = CertFindCertificateInStore(
                        localCertificateStore.get(), X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0,
                        CERT_FIND_ANY, nullptr, nullptr);
                if (certificateContext) {
                    auto *backend = QTlsBackend::backend<X509CertificateSchannel>(
                            configuration.localCertificate());
                    backend->certificateContext.reset(
                            CertDuplicateCertificateContext(certificateContext));

                    DWORD keySpec = 0;
                    BOOL mustFree = FALSE;
                    NCRYPT_KEY_HANDLE testKey = 0;
                    BOOL ok = CryptAcquireCertificatePrivateKey(
                            certificateContext, CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG, nullptr,
                            &testKey, &keySpec, &mustFree);
                    if (mustFree)
                        NCryptFreeObject(testKey);
                    if (!ok) {
                        attachPrivateKeyToCertificate(configuration.localCertificate(),
                                                      configuration.privateKey());
                    }
                }
            } else {
                qCWarning(lcTlsBackendSchannel, "Failed to load certificate chain!");
            }
        }
    }

    if (!configuration.caCertificates().isEmpty() && !caCertificateStore) {
        caCertificateStore = createStoreFromCertificateChain(configuration.caCertificates(),
                                                             {}); // No private key for the CA certs
    }
}

bool TlsCryptographSchannel::verifyCertContext(CERT_CONTEXT *certContext)
{
    Q_ASSERT(certContext);
    Q_ASSERT(q);
    Q_ASSERT(d);

    const bool isClient = d->tlsMode() == QSslSocket::SslClientMode;

    // Create a collection of stores so we can pass in multiple stores as additional locations to
    // search for the certificate chain
    auto tempCertCollection = QHCertStorePointer(CertOpenStore(CERT_STORE_PROV_COLLECTION,
                                                               X509_ASN_ENCODING,
                                                               0,
                                                               CERT_STORE_CREATE_NEW_FLAG,
                                                               nullptr));
    if (!tempCertCollection) {
#ifdef QSSLSOCKET_DEBUG
        qCWarning(lcTlsBackendSchannel, "Failed to create certificate store collection!");
#endif
        return false;
    }

    if (rootCertOnDemandLoadingAllowed()) {
        // @future(maybe): following the OpenSSL backend these certificates should be added into
        // the Ca list, not just included during verification.
        // That being said, it's not trivial to add the root certificates (if and only if they
        // came from the system root store). And I don't see this mentioned in our documentation.
        auto rootStore = QHCertStorePointer(
                CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
                              CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_CURRENT_USER, L"ROOT"));

        if (!rootStore) {
#ifdef QSSLSOCKET_DEBUG
            qCWarning(lcTlsBackendSchannel, "Failed to open the system root CA certificate store!");
#endif
            return false;
        } else if (!CertAddStoreToCollection(tempCertCollection.get(), rootStore.get(), 0, 1)) {
#ifdef QSSLSOCKET_DEBUG
            qCWarning(lcTlsBackendSchannel,
                      "Failed to add the system root CA certificate store to the certificate store "
                      "collection!");
#endif
            return false;
        }
    }
    if (caCertificateStore) {
        if (!CertAddStoreToCollection(tempCertCollection.get(), caCertificateStore.get(), 0, 1)) {
#ifdef QSSLSOCKET_DEBUG
            qCWarning(lcTlsBackendSchannel,
                      "Failed to add the user's CA certificate store to the certificate store "
                      "collection!");
#endif
            return false;
        }
    }

    if (!CertAddStoreToCollection(tempCertCollection.get(), certContext->hCertStore, 0, 0)) {
#ifdef QSSLSOCKET_DEBUG
        qCWarning(lcTlsBackendSchannel,
                  "Failed to add certificate's origin store to the certificate store collection!");
#endif
        return false;
    }

    CERT_CHAIN_PARA parameters;
    ZeroMemory(&parameters, sizeof(parameters));
    parameters.cbSize = sizeof(CERT_CHAIN_PARA);
    parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    parameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    LPSTR oid = LPSTR(isClient ? szOID_PKIX_KP_SERVER_AUTH
                               : szOID_PKIX_KP_CLIENT_AUTH);
    parameters.RequestedUsage.Usage.rgpszUsageIdentifier = &oid;

    QTlsBackend::clearPeerCertificates(d);
    const CERT_CHAIN_CONTEXT *chainContext = nullptr;
    auto freeCertChain = qScopeGuard([&chainContext]() {
        if (chainContext)
            CertFreeCertificateChain(chainContext);
    });
    BOOL status = CertGetCertificateChain(nullptr, // hChainEngine, default
                                          certContext, // pCertContext
                                          nullptr, // pTime, 'now'
                                          tempCertCollection.get(), // hAdditionalStore, additional cert store
                                          &parameters, // pChainPara
                                          CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT, // dwFlags
                                          nullptr, // reserved
                                          &chainContext // ppChainContext
    );
    if (status == FALSE || !chainContext || chainContext->cChain == 0) {
        QSslError error(QSslError::UnableToVerifyFirstCertificate);
        sslErrors += error;
        emit q->peerVerifyError(error);
        return q->state() == QAbstractSocket::ConnectedState;
    }

    // Helper-function to get a QSslCertificate given a CERT_CHAIN_ELEMENT
    static auto getCertificateFromChainElement = [](CERT_CHAIN_ELEMENT *element) {
        if (!element)
            return QSslCertificate();

        const CERT_CONTEXT *certContext = element->pCertContext;
        return QTlsPrivate::X509CertificateSchannel::QSslCertificate_from_CERT_CONTEXT(certContext);
    };

    // Pick a chain to use as the certificate chain, if multiple are available:
    // According to https://docs.microsoft.com/en-gb/windows/desktop/api/wincrypt/ns-wincrypt-_cert_chain_context
    // this seems to be the best way to get a trusted chain.
    CERT_SIMPLE_CHAIN *chain = chainContext->rgpChain[chainContext->cChain - 1];

    if (chain->TrustStatus.dwErrorStatus & CERT_TRUST_IS_PARTIAL_CHAIN) {
        auto error = QSslError(QSslError::SslError::UnableToGetIssuerCertificate,
                               getCertificateFromChainElement(chain->rgpElement[chain->cElement - 1]));
        sslErrors += error;
        emit q->peerVerifyError(error);
        if (q->state() != QAbstractSocket::ConnectedState)
            return false;
    }
    if (chain->TrustStatus.dwErrorStatus & CERT_TRUST_INVALID_BASIC_CONSTRAINTS) {
        // @Note: This is actually one of two errors:
        // "either the certificate cannot be used to issue other certificates, or the chain path length has been exceeded."
        // But here we are checking the chain's status, so we assume the "issuing" error cannot occur here.
        auto error = QSslError(QSslError::PathLengthExceeded);
        sslErrors += error;
        emit q->peerVerifyError(error);
        if (q->state() != QAbstractSocket::ConnectedState)
            return false;
    }
    static const DWORD leftoverCertChainErrorMask = CERT_TRUST_IS_CYCLIC | CERT_TRUST_INVALID_EXTENSION
            | CERT_TRUST_INVALID_POLICY_CONSTRAINTS | CERT_TRUST_INVALID_NAME_CONSTRAINTS
            | CERT_TRUST_CTL_IS_NOT_TIME_VALID | CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID
            | CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE;
    if (chain->TrustStatus.dwErrorStatus & leftoverCertChainErrorMask) {
        auto error = QSslError(QSslError::SslError::UnspecifiedError);
        sslErrors += error;
        emit q->peerVerifyError(error);
        if (q->state() != QAbstractSocket::ConnectedState)
            return false;
    }

    DWORD verifyDepth = chain->cElement;
    if (q->peerVerifyDepth() > 0 && DWORD(q->peerVerifyDepth()) < verifyDepth)
        verifyDepth = DWORD(q->peerVerifyDepth());

    const auto &caCertificates = q->sslConfiguration().caCertificates();

    if (!rootCertOnDemandLoadingAllowed()
            && !(chain->TrustStatus.dwErrorStatus & CERT_TRUST_IS_PARTIAL_CHAIN)
            && (q->peerVerifyMode() == QSslSocket::VerifyPeer
                    || (isClient && q->peerVerifyMode() == QSslSocket::AutoVerifyPeer))) {
        // When verifying a peer Windows "helpfully" builds a chain that
        // may include roots from the system store. But we don't want that if
        // the user has set their own CA certificates.
        // Since Windows claims this is not a partial chain the root is included
        // and we have to check that it is one of our configured CAs.
        CERT_CHAIN_ELEMENT *element = chain->rgpElement[chain->cElement - 1];
        QSslCertificate certificate = getCertificateFromChainElement(element);
        if (!caCertificates.contains(certificate)) {
            auto error = QSslError(QSslError::CertificateUntrusted, certificate);
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    QList<QSslCertificate> peerCertificateChain;
    for (DWORD i = 0; i < verifyDepth; i++) {
        CERT_CHAIN_ELEMENT *element = chain->rgpElement[i];
        QSslCertificate certificate = getCertificateFromChainElement(element);
        if (certificate.isNull()) {
            const auto &previousCert = !peerCertificateChain.isEmpty() ? peerCertificateChain.last()
                                                                       : QSslCertificate();
            auto error = QSslError(QSslError::SslError::UnableToGetIssuerCertificate, previousCert);
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (previousCert.isNull() || q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
        const QList<QSslCertificateExtension> extensions = certificate.extensions();

#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcTlsBackendSchannel) << "issuer:" << certificate.issuerDisplayName()
                                      << "\nsubject:" << certificate.subjectDisplayName()
                                      << "\nQSslCertificate info:" << certificate
                                      << "\nextended error info:" << element->pwszExtendedErrorInfo
                                      << "\nerror status:" << element->TrustStatus.dwErrorStatus;
#endif

        peerCertificateChain.append(certificate);
        QTlsBackend::storePeerCertificateChain(d, peerCertificateChain);

        if (certificate.isBlacklisted()) {
            const auto error = QSslError(QSslError::CertificateBlacklisted, certificate);
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }

        LONG result = CertVerifyTimeValidity(nullptr /*== now */, element->pCertContext->pCertInfo);
        if (result != 0) {
            auto error = QSslError(result == -1 ? QSslError::CertificateNotYetValid
                                                : QSslError::CertificateExpired,
                                   certificate);
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }

        //// Errors
        if (element->TrustStatus.dwErrorStatus & CERT_TRUST_IS_NOT_TIME_VALID) {
            // handled right above
            Q_ASSERT(!sslErrors.isEmpty());
        }
        if (element->TrustStatus.dwErrorStatus & CERT_TRUST_IS_REVOKED) {
            auto error = QSslError(QSslError::CertificateRevoked, certificate);
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
        if (element->TrustStatus.dwErrorStatus & CERT_TRUST_IS_NOT_SIGNATURE_VALID) {
            auto error = QSslError(QSslError::CertificateSignatureFailed, certificate);
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }

        // While netscape shouldn't be relevant now it defined an extension which is
        // still in use. Schannel does not check this automatically, so we do it here.
        // It is used to differentiate between client and server certificates.
        if (netscapeWrongCertType(extensions, isClient, i == 0))
            element->TrustStatus.dwErrorStatus |= CERT_TRUST_IS_NOT_VALID_FOR_USAGE;

        if (element->TrustStatus.dwErrorStatus & CERT_TRUST_IS_NOT_VALID_FOR_USAGE) {
            auto error = QSslError(QSslError::InvalidPurpose, certificate);
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
        if (element->TrustStatus.dwErrorStatus & CERT_TRUST_IS_UNTRUSTED_ROOT) {
            // Override this error if we have the certificate inside our trusted CAs list.
            const bool isTrustedRoot = caCertificates.contains(certificate);
            if (!isTrustedRoot) {
                auto error = QSslError(QSslError::CertificateUntrusted, certificate);
                sslErrors += error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
        static const DWORD certRevocationCheckUnavailableError = CERT_TRUST_IS_OFFLINE_REVOCATION
                | CERT_TRUST_REVOCATION_STATUS_UNKNOWN;
        if (element->TrustStatus.dwErrorStatus & certRevocationCheckUnavailableError) {
            // @future(maybe): Do something with this
        }

        // Dumping ground of errors that don't fit our specific errors
        static const DWORD leftoverCertErrorMask = CERT_TRUST_IS_CYCLIC
                | CERT_TRUST_INVALID_EXTENSION | CERT_TRUST_INVALID_NAME_CONSTRAINTS
                | CERT_TRUST_INVALID_POLICY_CONSTRAINTS
                | CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT
                | CERT_TRUST_HAS_NOT_SUPPORTED_CRITICAL_EXT
                | CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT
                | CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT
                | CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT;
        if (element->TrustStatus.dwErrorStatus & leftoverCertErrorMask) {
            auto error = QSslError(QSslError::UnspecifiedError, certificate);
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
        if (element->TrustStatus.dwErrorStatus & CERT_TRUST_INVALID_BASIC_CONSTRAINTS) {
            auto it = std::find_if(extensions.cbegin(), extensions.cend(),
                                   [](const QSslCertificateExtension &extension) {
                                       return extension.name() == "basicConstraints"_L1;
                                   });
            if (it != extensions.cend()) {
                // @Note: This is actually one of two errors:
                // "either the certificate cannot be used to issue other certificates,
                // or the chain path length has been exceeded."
                QVariantMap basicConstraints = it->value().toMap();
                QSslError error;
                if (i > 0 && !basicConstraints.value("ca"_L1, false).toBool())
                    error = QSslError(QSslError::InvalidPurpose, certificate);
                else
                    error = QSslError(QSslError::PathLengthExceeded, certificate);
                sslErrors += error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
        if (element->TrustStatus.dwErrorStatus & CERT_TRUST_IS_EXPLICIT_DISTRUST) {
            auto error = QSslError(QSslError::CertificateBlacklisted, certificate);
            sslErrors += error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }

        if (element->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED) {
            // If it's self-signed *and* a CA then we can assume it's a root CA certificate
            // and we can ignore the "self-signed" note:
            // We check the basicConstraints certificate extension when possible, but this didn't
            // exist for version 1, so we can only guess in that case
            const bool isRootCertificateAuthority = isCertificateAuthority(extensions)
                    || certificate.version() == "1";

            // Root certificate tends to be signed by themselves, so ignore self-signed status.
            if (!isRootCertificateAuthority) {
                auto error = QSslError(QSslError::SelfSignedCertificate, certificate);
                sslErrors += error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
    }

    if (!peerCertificateChain.isEmpty())
        QTlsBackend::storePeerCertificate(d, peerCertificateChain.constFirst());

    const auto &configuration = q->sslConfiguration(); // Probably, updated by QTlsBackend::storePeerCertificate etc.
    // @Note: Somewhat copied from qsslsocket_mac.cpp
    const bool doVerifyPeer = q->peerVerifyMode() == QSslSocket::VerifyPeer
            || (q->peerVerifyMode() == QSslSocket::AutoVerifyPeer
                && d->tlsMode() == QSslSocket::SslClientMode);
    // Check the peer certificate itself. First try the subject's common name
    // (CN) as a wildcard, then try all alternate subject name DNS entries the
    // same way.
    if (!configuration.peerCertificate().isNull()) {
        // but only if we're a client connecting to a server
        // if we're the server, don't check CN
        if (d->tlsMode() == QSslSocket::SslClientMode) {
            const auto verificationPeerName = d->verificationName();
            const QString peerName(verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName);
            if (!isMatchingHostname(configuration.peerCertificate(), peerName)) {
                // No matches in common names or alternate names.
                const QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate());
                sslErrors += error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
    } else if (doVerifyPeer) {
        // No peer certificate presented. Report as error if the socket
        // expected one.
        const QSslError error(QSslError::NoPeerCertificate);
        sslErrors += error;
        emit q->peerVerifyError(error);
        if (q->state() != QAbstractSocket::ConnectedState)
            return false;
    }

    return true;
}

bool TlsCryptographSchannel::rootCertOnDemandLoadingAllowed()
{
    Q_ASSERT(d);
    return d->isRootsOnDemandAllowed() && QSslSocketPrivate::rootCertOnDemandLoadingSupported();
}

} // namespace QTlsPrivate

QT_END_NAMESPACE
