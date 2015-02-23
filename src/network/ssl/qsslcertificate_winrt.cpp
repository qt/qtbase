/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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
        RETURN_IF_FAILED("Failed to create the certificate data buffer", return 0);

        hr = g->certificateFactory->CreateCertificate(buffer.Get(), &d->certificate);
        RETURN_IF_FAILED("Failed to create the certificate handle from the data buffer", return 0);
    }

    return d->certificate.Get();
}
