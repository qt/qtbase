// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:cryptography

#include <QtNetwork/private/qssl_p.h>

#include "qtlsbackend_schannel_p.h"
#include "qtlskey_schannel_p.h"

#include "../shared/qwincrypt_p.h"

#include <QtNetwork/private/qtlsbackend_p.h>
#include <QtNetwork/private/qsslkey_p.h>

#include <QtNetwork/qsslkey.h>

#include <QtCore/qscopeguard.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

namespace {
const wchar_t *getName(QSslKeyPrivate::Cipher cipher)
{
    switch (cipher) {
    case QTlsPrivate::Cipher::DesCbc:
        return BCRYPT_DES_ALGORITHM;
    case QTlsPrivate::Cipher::DesEde3Cbc:
        return BCRYPT_3DES_ALGORITHM;
    case QTlsPrivate::Cipher::Rc2Cbc:
        return BCRYPT_RC2_ALGORITHM;
    case QTlsPrivate::Cipher::Aes128Cbc:
    case QTlsPrivate::Cipher::Aes192Cbc:
    case QTlsPrivate::Cipher::Aes256Cbc:
        return BCRYPT_AES_ALGORITHM;
    }
    Q_UNREACHABLE();
}

BCRYPT_ALG_HANDLE getHandle(QSslKeyPrivate::Cipher cipher)
{
    BCRYPT_ALG_HANDLE handle;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
            &handle, // phAlgorithm
            getName(cipher), // pszAlgId
            nullptr, // pszImplementation
            0 // dwFlags
    );
    if (status < 0) {
        qCWarning(lcTlsBackendSchannel, "Failed to open algorithm handle (%ld)!", status);
        return nullptr;
    }

    return handle;
}

BCRYPT_KEY_HANDLE generateSymmetricKey(BCRYPT_ALG_HANDLE handle,
                                       const QByteArray &key)
{
    BCRYPT_KEY_HANDLE keyHandle;
    NTSTATUS status = BCryptGenerateSymmetricKey(
            handle, // hAlgorithm
            &keyHandle, // phKey
            nullptr, // pbKeyObject (can ignore)
            0, // cbKeyObject (also ignoring)
            reinterpret_cast<unsigned char *>(const_cast<char *>(key.data())), // pbSecret
            ULONG(key.length()), // cbSecret
            0 // dwFlags
    );
    if (status < 0) {
        qCWarning(lcTlsBackendSchannel, "Failed to generate symmetric key (%ld)!", status);
        return nullptr;
    }

    status = BCryptSetProperty(
            keyHandle, // hObject
            BCRYPT_CHAINING_MODE, // pszProperty
            reinterpret_cast<UCHAR *>(const_cast<wchar_t *>(BCRYPT_CHAIN_MODE_CBC)), // pbInput
            ARRAYSIZE(BCRYPT_CHAIN_MODE_CBC), // cbInput
            0 // dwFlags
    );
    if (status < 0) {
        BCryptDestroyKey(keyHandle);
        qCWarning(lcTlsBackendSchannel, "Failed to change the symmetric key's chaining mode (%ld)!",
                  status);
        return nullptr;
    }
    return keyHandle;
}

QByteArray doCrypt(QSslKeyPrivate::Cipher cipher, const QByteArray &data, const QByteArray &key,
                   const QByteArray &iv, bool encrypt)
{
    BCRYPT_ALG_HANDLE handle = getHandle(cipher);
    if (!handle)
        return {};
    auto handleDealloc = qScopeGuard([&handle]() {
        BCryptCloseAlgorithmProvider(handle, 0);
    });

    BCRYPT_KEY_HANDLE keyHandle = generateSymmetricKey(handle, key);
    if (!keyHandle)
        return {};
    auto keyHandleDealloc = qScopeGuard([&keyHandle]() {
        BCryptDestroyKey(keyHandle);
    });

    QByteArray ivCopy = iv; // This gets modified, so we take a copy

    ULONG sizeNeeded = 0;
    QVarLengthArray<unsigned char> output;
    auto cryptFunction = encrypt ? BCryptEncrypt : BCryptDecrypt;
    for (int i = 0; i < 2; i++) {
        output.resize(int(sizeNeeded));
        auto input = reinterpret_cast<unsigned char *>(const_cast<char *>(data.data()));
        // Need to call it twice because the first iteration lets us know the size needed.
        NTSTATUS status = cryptFunction(
                keyHandle, // hKey
                input, // pbInput
                ULONG(data.length()), // cbInput
                nullptr, // pPaddingInfo
                reinterpret_cast<unsigned char *>(ivCopy.data()), // pbIV
                ULONG(ivCopy.length()), // cbIV
                sizeNeeded ? output.data() : nullptr, // pbOutput
                ULONG(output.length()), // cbOutput
                &sizeNeeded, // pcbResult
                BCRYPT_BLOCK_PADDING // dwFlags
        );
        if (status < 0) {
            qCWarning(lcTlsBackendSchannel, "%s failed (%ld)!", encrypt ? "Encrypt" : "Decrypt",
                      status);
            return {};
        }
    }

    return QByteArray(reinterpret_cast<const char *>(output.constData()), int(sizeNeeded));
}
} // anonymous namespace

namespace QTlsPrivate {

QByteArray TlsKeySchannel::decrypt(Cipher cipher, const QByteArray &data, const QByteArray &key,
                                   const QByteArray &iv) const
{
    return doCrypt(cipher, data, key, iv, false);
}

QByteArray TlsKeySchannel::encrypt(Cipher cipher, const QByteArray &data, const QByteArray &key,
                                   const QByteArray &iv) const
{
    return doCrypt(cipher, data, key, iv, true);
}

} // namespace QTlsPrivate

QT_END_NAMESPACE

