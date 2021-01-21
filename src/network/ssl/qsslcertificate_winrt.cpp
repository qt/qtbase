/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qsslcertificate_p.h"

#include <QtCore/qfunctions_winrt.h>

#include <wrl.h>
#include <windows.storage.streams.h>
#include <windows.security.cryptography.h>
#include <robuffer.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Security::Cryptography;
using namespace ABI::Windows::Security::Cryptography::Certificates;
using namespace ABI::Windows::Storage::Streams;

QT_USE_NAMESPACE

struct SslCertificateGlobal
{
    SslCertificateGlobal() {
        HRESULT hr;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Certificates_Certificate).Get(),
                                  &certificateFactory);
        Q_ASSERT_SUCCEEDED(hr);
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(),
                                  &bufferFactory);
        Q_ASSERT_SUCCEEDED(hr);
    }

    ComPtr<ICertificateFactory> certificateFactory;
    ComPtr<ICryptographicBufferStatics> bufferFactory;
};
Q_GLOBAL_STATIC(SslCertificateGlobal, g)

QSslCertificate QSslCertificatePrivate::QSslCertificate_from_Certificate(ICertificate *iCertificate)
{
    Q_ASSERT(iCertificate);
    ComPtr<IBuffer> buffer;
    HRESULT hr = iCertificate->GetCertificateBlob(&buffer);
    RETURN_IF_FAILED("Could not obtain certification blob", return QSslCertificate());
    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteAccess;
    hr = buffer.As(&byteAccess);
    RETURN_IF_FAILED("Could not obtain byte access to buffer", return QSslCertificate());
    char *data;
    hr = byteAccess->Buffer(reinterpret_cast<byte **>(&data));
    RETURN_IF_FAILED("Could not obtain buffer data", return QSslCertificate());
    UINT32 size;
    hr = buffer->get_Length(&size);
    RETURN_IF_FAILED("Could not obtain buffer length ", return QSslCertificate());
    QByteArray der(data, size);

    QSslCertificate certificate;
    certificate.d->null = false;
    certificate.d->certificate = iCertificate;

    return certificatesFromDer(der, 1).at(0);
}

Qt::HANDLE QSslCertificate::handle() const
{
    if (!d->certificate) {
        HRESULT hr;
        ComPtr<IBuffer> buffer;
        hr = g->bufferFactory->CreateFromByteArray(d->derData.length(), (BYTE *)d->derData.data(), &buffer);
        RETURN_IF_FAILED("Failed to create the certificate data buffer", return nullptr);

        hr = g->certificateFactory->CreateCertificate(buffer.Get(), &d->certificate);
        RETURN_IF_FAILED("Failed to create the certificate handle from the data buffer",
                         return nullptr);
    }

    return d->certificate.Get();
}
