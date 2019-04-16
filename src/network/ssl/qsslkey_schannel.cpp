/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qssl_p.h"
#include "qsslkey.h"
#include "qsslkey_p.h"
#include "qsslcertificate_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qscopeguard.h>

QT_BEGIN_NAMESPACE

namespace {
const wchar_t *getName(QSslKeyPrivate::Cipher cipher)
{
    switch (cipher) {
    case QSslKeyPrivate::Cipher::DesCbc:
        return BCRYPT_DES_ALGORITHM;
    case QSslKeyPrivate::Cipher::DesEde3Cbc:
        return BCRYPT_3DES_ALGORITHM;
    case QSslKeyPrivate::Cipher::Rc2Cbc:
        return BCRYPT_RC2_ALGORITHM;
    case QSslKeyPrivate::Cipher::Aes128Cbc:
    case QSslKeyPrivate::Cipher::Aes192Cbc:
    case QSslKeyPrivate::Cipher::Aes256Cbc:
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
        qCWarning(lcSsl, "Failed to open algorithm handle (%ld)!", status);
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
        qCWarning(lcSsl, "Failed to generate symmetric key (%ld)!", status);
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
        qCWarning(lcSsl, "Failed to change the symmetric key's chaining mode (%ld)!", status);
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
            qCWarning(lcSsl, "%s failed (%ld)!", encrypt ? "Encrypt" : "Decrypt", status);
            return {};
        }
    }

    return QByteArray(reinterpret_cast<const char *>(output.constData()), int(sizeNeeded));
}
} // anonymous namespace

QByteArray QSslKeyPrivate::decrypt(Cipher cipher, const QByteArray &data, const QByteArray &key,
                                   const QByteArray &iv)
{
    return doCrypt(cipher, data, key, iv, false);
}

QByteArray QSslKeyPrivate::encrypt(Cipher cipher, const QByteArray &data, const QByteArray &key,
                                   const QByteArray &iv)
{
    return doCrypt(cipher, data, key, iv, true);
}

QT_END_NAMESPACE
