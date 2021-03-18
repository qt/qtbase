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

#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"

#ifndef QT_NO_NETWORKINTERFACE

#include <qfunctions_winrt.h>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.networking.h>
#include <windows.networking.connectivity.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Networking;
using namespace ABI::Windows::Networking::Connectivity;

#include <qhostinfo.h>

QT_BEGIN_NAMESPACE

struct HostNameInfo {
    GUID adapterId;
    unsigned char prefixLength;
    QString address;
};

uint QNetworkInterfaceManager::interfaceIndexFromName(const QString &name)
{
    // TBD - may not be possible
    Q_UNUSED(name);
    return 0;
}

QString QNetworkInterfaceManager::interfaceNameFromIndex(uint index)
{
    // TBD - may not be possible
    return QString::number(index);
}

static QNetworkInterfacePrivate *interfaceFromProfile(IConnectionProfile *profile, QList<HostNameInfo> *hostList)
{
    if (!profile)
        return 0;

    QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;

    NetworkConnectivityLevel connectivityLevel;
    HRESULT hr = profile->GetNetworkConnectivityLevel(&connectivityLevel);
    Q_ASSERT_SUCCEEDED(hr);
    if (connectivityLevel != NetworkConnectivityLevel_None)
        iface->flags = QNetworkInterface::IsUp | QNetworkInterface::IsRunning;
    iface->flags |= QNetworkInterface::CanBroadcast;

    ComPtr<INetworkAdapter> adapter;
    hr = profile->get_NetworkAdapter(&adapter);
    // Indicates that no internet connection is available/the device is in airplane mode
    if (hr == E_INVALIDARG)
        return 0;
    Q_ASSERT_SUCCEEDED(hr);
    UINT32 type;
    hr = adapter->get_IanaInterfaceType(&type);
    Q_ASSERT_SUCCEEDED(hr);
    if (type == 23)
        iface->flags |= QNetworkInterface::IsPointToPoint;
    GUID id;
    hr = adapter->get_NetworkAdapterId(&id);
    Q_ASSERT_SUCCEEDED(hr);
    OLECHAR adapterName[39]={0};
    StringFromGUID2(id, adapterName, 39);
    iface->name = QString::fromWCharArray(adapterName);

    // According to http://stackoverflow.com/questions/12936193/how-unique-is-the-ethernet-network-adapter-id-in-winrt-it-is-derived-from-the-m
    // obtaining the MAC address using WinRT API is impossible
    // iface->hardwareAddress = ?

    for (int i = 0; i < hostList->length(); ++i) {
        const HostNameInfo hostInfo = hostList->at(i);
        if (id != hostInfo.adapterId)
            continue;

        QNetworkAddressEntry entry;
        entry.setIp(QHostAddress(hostInfo.address));
        entry.setPrefixLength(hostInfo.prefixLength);
        iface->addressEntries << entry;

        hostList->takeAt(i);
        --i;
    }
    return iface;
}

static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    QList<QNetworkInterfacePrivate *> interfaces;

    QList<HostNameInfo> hostList;

    ComPtr<INetworkInformationStatics> hostNameStatics;
    HRESULT hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(), &hostNameStatics);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<IVectorView<HostName *>> hostNames;
    hr = hostNameStatics->GetHostNames(&hostNames);
    Q_ASSERT_SUCCEEDED(hr);
    if (!hostNames)
        return interfaces;

    unsigned int hostNameCount;
    hr = hostNames->get_Size(&hostNameCount);
    Q_ASSERT_SUCCEEDED(hr);
    for (unsigned i = 0; i < hostNameCount; ++i) {
        HostNameInfo hostInfo;
        ComPtr<IHostName> hostName;
        hr = hostNames->GetAt(i, &hostName);
        Q_ASSERT_SUCCEEDED(hr);

        HostNameType type;
        hr = hostName->get_Type(&type);
        Q_ASSERT_SUCCEEDED(hr);
        if (type == HostNameType_DomainName)
            continue;

        ComPtr<IIPInformation> ipInformation;
        hr = hostName->get_IPInformation(&ipInformation);
        Q_ASSERT_SUCCEEDED(hr);
        ComPtr<INetworkAdapter> currentAdapter;
        hr = ipInformation->get_NetworkAdapter(&currentAdapter);
        Q_ASSERT_SUCCEEDED(hr);

        hr = currentAdapter->get_NetworkAdapterId(&hostInfo.adapterId);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<IReference<unsigned char>> prefixLengthReference;
        hr = ipInformation->get_PrefixLength(&prefixLengthReference);
        Q_ASSERT_SUCCEEDED(hr);

        hr = prefixLengthReference->get_Value(&hostInfo.prefixLength);
        Q_ASSERT_SUCCEEDED(hr);

        // invalid prefixes
        if ((type == HostNameType_Ipv4 && hostInfo.prefixLength > 32)
                || (type == HostNameType_Ipv6 && hostInfo.prefixLength > 128))
            continue;

        HString name;
        hr = hostName->get_CanonicalName(name.GetAddressOf());
        Q_ASSERT_SUCCEEDED(hr);
        UINT32 length;
        PCWSTR rawString = name.GetRawBuffer(&length);
        hostInfo.address = QString::fromWCharArray(rawString, length);

        hostList << hostInfo;
    }

    INetworkInformationStatics *networkInfoStatics;
    hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(), &networkInfoStatics);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IConnectionProfile> connectionProfile;
    hr = networkInfoStatics->GetInternetConnectionProfile(&connectionProfile);
    Q_ASSERT_SUCCEEDED(hr);
    QNetworkInterfacePrivate *iface = interfaceFromProfile(connectionProfile.Get(), &hostList);
    if (iface) {
        iface->index = 0;
        interfaces << iface;
    }

    ComPtr<IVectorView<ConnectionProfile *>> connectionProfiles;
    hr = networkInfoStatics->GetConnectionProfiles(&connectionProfiles);
    Q_ASSERT_SUCCEEDED(hr);
    if (!connectionProfiles)
        return interfaces;

    unsigned int size;
    hr = connectionProfiles->get_Size(&size);
    Q_ASSERT_SUCCEEDED(hr);
    for (unsigned int i = 0; i < size; ++i) {
        ComPtr<IConnectionProfile> profile;
        hr = connectionProfiles->GetAt(i, &profile);
        Q_ASSERT_SUCCEEDED(hr);

        iface = interfaceFromProfile(profile.Get(), &hostList);
        if (iface) {
            iface->index = i + 1;
            interfaces << iface;
        }
    }
    return interfaces;
}

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    return interfaceListing();
}

QString QHostInfo::localDomainName()
{
    return QString();
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE
