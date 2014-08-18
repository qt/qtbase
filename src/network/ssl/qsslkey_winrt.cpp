/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsslkey.h"
#include "qsslkey_p.h"

#include <QtCore/qfunctions_winrt.h>

#include <wrl.h>
#include <windows.security.cryptography.h>
#include <windows.security.cryptography.core.h>
#include <windows.security.cryptography.certificates.h>
#include <windows.storage.streams.h>
#include <robuffer.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Security::Cryptography;
using namespace ABI::Windows::Security::Cryptography::Certificates;
using namespace ABI::Windows::Security::Cryptography::Core;
using namespace ABI::Windows::Storage::Streams;

QT_BEGIN_NAMESPACE

struct SslKeyGlobal
{
    SslKeyGlobal()
    {
        HRESULT hr;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(),
                                  &bufferFactory);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<IAsymmetricKeyAlgorithmProviderStatics> keyProviderFactory;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_AsymmetricKeyAlgorithmProvider).Get(),
                                  &keyProviderFactory);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<IAsymmetricAlgorithmNamesStatics> algorithmNames;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_AsymmetricAlgorithmNames).Get(),
                                  &algorithmNames);
        Q_ASSERT_SUCCEEDED(hr);

        HString algorithmName;
        // The algorithm name doesn't matter for imports, so just use PKCS1
        hr = algorithmNames->get_RsaPkcs1(algorithmName.GetAddressOf());
        Q_ASSERT_SUCCEEDED(hr);
        hr = keyProviderFactory->OpenAlgorithm(algorithmName.Get(), &keyProvider);
        Q_ASSERT_SUCCEEDED(hr);
    }

    ComPtr<ICryptographicBufferStatics> bufferFactory;
    ComPtr<IAsymmetricKeyAlgorithmProvider> keyProvider;
};
Q_GLOBAL_STATIC(SslKeyGlobal, g)

// Use the opaque struct for key storage
struct EVP_PKEY {
    ComPtr<ICryptographicKey> key;
};

void QSslKeyPrivate::clear(bool deep)
{
    isNull = true;

    if (opaque) {
        if (deep) {
            delete opaque;
            opaque = 0;
        } else {
            opaque->key.Reset();
        }
    }
}

void QSslKeyPrivate::decodeDer(const QByteArray &der, const QByteArray &passPhrase,
                               bool deepClear)
{
    Q_UNUSED(passPhrase);

    clear(deepClear);

    if (der.isEmpty())
        return;

    if (type != QSsl::PublicKey) {
        qWarning("The WinRT SSL backend does not support importing private keys.");
        return;
    }

    HRESULT hr;
    ComPtr<IBuffer> buffer;
    hr = g->bufferFactory->CreateFromByteArray(der.length(), (BYTE *)der.data(), &buffer);
    Q_ASSERT_SUCCEEDED(hr);

    if (!opaque)
        opaque = new EVP_PKEY;

    hr = g->keyProvider->ImportDefaultPublicKeyBlob(buffer.Get(), &opaque->key);
    RETURN_VOID_IF_FAILED("Failed to import public key");

    isNull = false;
}

void QSslKeyPrivate::decodePem(const QByteArray &pem, const QByteArray &passPhrase,
                               bool deepClear)
{
    decodeDer(derFromPem(pem), passPhrase, deepClear);
}

int QSslKeyPrivate::length() const
{
    if (isNull)
        return -1;

    Q_ASSERT(opaque && opaque->key);
    HRESULT hr;
    UINT32 keySize;
    hr = opaque->key->get_KeySize(&keySize);
    Q_ASSERT_SUCCEEDED(hr);
    return keySize;
}

QByteArray QSslKeyPrivate::toPem(const QByteArray &passPhrase) const
{
    Q_UNUSED(passPhrase);
    QByteArray result;
    if (isNull)
        return result;

    Q_ASSERT(opaque && opaque->key);
    HRESULT hr;
    ComPtr<IBuffer> buffer;
    hr = opaque->key->ExportDefaultPublicKeyBlobType(&buffer);
    RETURN_IF_FAILED("Failed to export key", return result);

    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteAccess;
    hr = buffer.As(&byteAccess);
    Q_ASSERT_SUCCEEDED(hr);
    char *data;
    hr = byteAccess->Buffer(reinterpret_cast<byte **>(&data));
    Q_ASSERT_SUCCEEDED(hr);
    UINT32 size;
    hr = buffer->get_Length(&size);
    Q_ASSERT_SUCCEEDED(hr);
    result = pemFromDer(QByteArray::fromRawData(data, size));
    return result;
}

Qt::HANDLE QSslKeyPrivate::handle() const
{
    return opaque ? opaque->key.Get() : 0;
}

QT_END_NAMESPACE
