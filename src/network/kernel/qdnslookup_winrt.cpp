/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qdnslookup_p.h"

#include <qfunctions_winrt.h>
#include <qurl.h>
#include <qdebug.h>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.networking.h>
#include <windows.networking.sockets.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Networking;
using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::Networking::Sockets;

#define E_NO_SUCH_HOST 0x80072af9

QT_BEGIN_NAMESPACE

void QDnsLookupRunnable::query(const int requestType, const QByteArray &requestName, const QHostAddress &nameserver, QDnsLookupReply *reply)
{
    // TODO: Add nameserver support for winRT
    if (!nameserver.isNull())
        qWarning("Ignoring nameserver as its currently not supported on WinRT");

    // TODO: is there any way to do "proper" dns lookup?
    if (requestType != QDnsLookup::A && requestType != QDnsLookup::AAAA
            && requestType != QDnsLookup::ANY) {
        reply->error = QDnsLookup::InvalidRequestError;
        reply->errorString = QLatin1String("WinRT only supports IPv4 and IPv6 requests");
        return;
    }

    QString aceHostname = QUrl::fromAce(requestName);
    if (aceHostname.isEmpty()) {
        reply->error = QDnsLookup::InvalidRequestError;
        reply->errorString = requestName.isEmpty() ? tr("No hostname given") : tr("Invalid hostname");
        return;
    }

    ComPtr<IHostNameFactory> hostnameFactory;
    HRESULT hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                                        IID_PPV_ARGS(&hostnameFactory));
    if (FAILED(hr)) {
        reply->error = QDnsLookup::ResolverError;
        reply->errorString = QLatin1String("Could not obtain hostname factory");
        return;
    }
    ComPtr<IHostName> host;
    HStringReference hostNameRef((const wchar_t*)aceHostname.utf16());
    hr = hostnameFactory->CreateHostName(hostNameRef.Get(), &host);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<IDatagramSocketStatics> datagramSocketStatics;
    hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_DatagramSocket).Get(), &datagramSocketStatics);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<IAsyncOperation<IVectorView<EndpointPair *> *>> op;
    hr = datagramSocketStatics->GetEndpointPairsAsync(host.Get(),
                                                 HString::MakeReference(L"0").Get(),
                                                 &op);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<IVectorView<EndpointPair *>> endpointPairs;
    hr = QWinRTFunctions::await(op, endpointPairs.GetAddressOf(), QWinRTFunctions::YieldThread, 60 * 1000);
    if (hr == E_NO_SUCH_HOST || !endpointPairs) {
        reply->error = QDnsLookup::NotFoundError;
        reply->errorString = tr("Host %1 could not be found.").arg(aceHostname);
        return;
    }
    if (FAILED(hr)) {
        reply->error = QDnsLookup::ServerFailureError;
        reply->errorString = tr("Unknown error");
        return;
    }

    unsigned int size;
    hr = endpointPairs->get_Size(&size);
    Q_ASSERT_SUCCEEDED(hr);
    // endpoint pairs might contain duplicates so we temporarily store addresses in a QSet
    QSet<QHostAddress> addresses;
    for (unsigned int i = 0; i < size; ++i) {
        ComPtr<IEndpointPair> endpointpair;
        hr = endpointPairs->GetAt(i, &endpointpair);
        Q_ASSERT_SUCCEEDED(hr);
        ComPtr<IHostName> remoteHost;
        hr = endpointpair->get_RemoteHostName(&remoteHost);
        Q_ASSERT_SUCCEEDED(hr);
        HostNameType type;
        hr = remoteHost->get_Type(&type);
        Q_ASSERT_SUCCEEDED(hr);
        if (type == HostNameType_Bluetooth || type == HostNameType_DomainName
                || (requestType != QDnsLookup::ANY
                && ((type == HostNameType_Ipv4 && requestType == QDnsLookup::AAAA)
                || (type == HostNameType_Ipv6 && requestType == QDnsLookup::A))))
            continue;

        HString name;
        hr = remoteHost->get_CanonicalName(name.GetAddressOf());
        Q_ASSERT_SUCCEEDED(hr);
        UINT32 length;
        PCWSTR rawString = name.GetRawBuffer(&length);
        addresses.insert(QHostAddress(QString::fromWCharArray(rawString, length)));
    }
    for (const QHostAddress &address : qAsConst(addresses)) {
        QDnsHostAddressRecord record;
        record.d->name = aceHostname;
        record.d->value = address;
        reply->hostAddressRecords.append(record);
    }
}

QT_END_NAMESPACE
