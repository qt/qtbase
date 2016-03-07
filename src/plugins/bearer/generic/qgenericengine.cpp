/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qgenericengine.h"
#include "../qnetworksession_impl.h"

#include <QtNetwork/private/qnetworkconfiguration_p.h>

#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qstringlist.h>

#include <QtCore/qdebug.h>
#include <QtCore/private/qcoreapplication_p.h>

#if defined(Q_OS_WIN32)
#include "../platformdefs_win.h"
#endif

#ifdef Q_OS_WINRT
#include <qfunctions_winrt.h>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.networking.connectivity.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Networking;
using namespace ABI::Windows::Networking::Connectivity;
// needed as interface is used as parameter name in qGetInterfaceType
#undef interface
#endif // Q_OS_WINRT

#ifdef Q_OS_LINUX
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <unistd.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_NETWORKINTERFACE
static QNetworkConfiguration::BearerType qGetInterfaceType(const QString &interface)
{
#if defined(Q_OS_WIN32)
    DWORD bytesWritten;
    NDIS_MEDIUM medium;
    NDIS_PHYSICAL_MEDIUM physicalMedium;

    unsigned long oid;
    HANDLE handle = CreateFile((TCHAR *)QString::fromLatin1("\\\\.\\%1").arg(interface).utf16(), 0,
                               FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (handle == INVALID_HANDLE_VALUE)
        return QNetworkConfiguration::BearerUnknown;

    bytesWritten = 0;

    oid = OID_GEN_MEDIA_SUPPORTED;
    bool result = DeviceIoControl(handle, IOCTL_NDIS_QUERY_GLOBAL_STATS, &oid, sizeof(oid),
                                  &medium, sizeof(medium), &bytesWritten, 0);
    if (!result) {
        CloseHandle(handle);
        return QNetworkConfiguration::BearerUnknown;
    }

    bytesWritten = 0;

    oid = OID_GEN_PHYSICAL_MEDIUM;
    result = DeviceIoControl(handle, IOCTL_NDIS_QUERY_GLOBAL_STATS, &oid, sizeof(oid),
                             &physicalMedium, sizeof(physicalMedium), &bytesWritten, 0);

    if (!result) {
        CloseHandle(handle);

        if (medium == NdisMedium802_3)
            return QNetworkConfiguration::BearerEthernet;
        else
            return QNetworkConfiguration::BearerUnknown;
    }

    CloseHandle(handle);

    if (medium == NdisMedium802_3) {
        switch (physicalMedium) {
        case NdisPhysicalMediumWirelessLan:
            return QNetworkConfiguration::BearerWLAN;
        case NdisPhysicalMediumBluetooth:
            return QNetworkConfiguration::BearerBluetooth;
        case NdisPhysicalMediumWiMax:
            return QNetworkConfiguration::BearerWiMAX;
        default:
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug() << "Physical Medium" << physicalMedium;
#endif
            return QNetworkConfiguration::BearerEthernet;
        }
    }

#ifdef BEARER_MANAGEMENT_DEBUG
    qDebug() << medium << physicalMedium;
#endif
#elif defined(Q_OS_LINUX)
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    ifreq request;
    strncpy(request.ifr_name, interface.toLocal8Bit().data(), sizeof(request.ifr_name));
    request.ifr_name[sizeof(request.ifr_name) - 1] = '\0';
    int result = ioctl(sock, SIOCGIFHWADDR, &request);
    close(sock);

    if (result >= 0 && request.ifr_hwaddr.sa_family == ARPHRD_ETHER)
        return QNetworkConfiguration::BearerEthernet;
#elif defined(Q_OS_WINRT)
    ComPtr<INetworkInformationStatics> networkInfoStatics;
    HRESULT hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(), &networkInfoStatics);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IVectorView<ConnectionProfile *>> connectionProfiles;
    hr = networkInfoStatics->GetConnectionProfiles(&connectionProfiles);
    Q_ASSERT_SUCCEEDED(hr);
    if (!connectionProfiles)
        return QNetworkConfiguration::BearerUnknown;

    unsigned int size;
    hr = connectionProfiles->get_Size(&size);
    Q_ASSERT_SUCCEEDED(hr);
    for (unsigned int i = 0; i < size; ++i) {
        ComPtr<IConnectionProfile> profile;
        hr = connectionProfiles->GetAt(i, &profile);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<INetworkAdapter> adapter;
        hr = profile->get_NetworkAdapter(&adapter);
        // Indicates that no internet connection is available/the device is in airplane mode
        if (hr == E_INVALIDARG)
            return QNetworkConfiguration::BearerUnknown;
        Q_ASSERT_SUCCEEDED(hr);
        GUID id;
        hr = adapter->get_NetworkAdapterId(&id);
        Q_ASSERT_SUCCEEDED(hr);
        OLECHAR adapterName[39]={0};
        int length = StringFromGUID2(id, adapterName, 39);
        // "length - 1" as we have to remove the null terminator from it in order to compare
        if (!length
                || QString::fromRawData(reinterpret_cast<const QChar *>(adapterName), length - 1) != interface)
            continue;

        ComPtr<IConnectionProfile2> profile2;
        hr = profile.As(&profile2);
        Q_ASSERT_SUCCEEDED(hr);
        boolean isWLan;
        hr = profile2->get_IsWlanConnectionProfile(&isWLan);
        Q_ASSERT_SUCCEEDED(hr);
        if (isWLan)
            return QNetworkConfiguration::BearerWLAN;

        boolean isWWan;
        hr = profile2->get_IsWwanConnectionProfile(&isWWan);
        Q_ASSERT_SUCCEEDED(hr);
        if (isWWan) {
            ComPtr<IWwanConnectionProfileDetails> details;
            hr = profile2->get_WwanConnectionProfileDetails(&details);
            Q_ASSERT_SUCCEEDED(hr);
            WwanDataClass dataClass;
            hr = details->GetCurrentDataClass(&dataClass);
            Q_ASSERT_SUCCEEDED(hr);
            switch (dataClass) {
            case WwanDataClass_Edge:
            case WwanDataClass_Gprs:
                return QNetworkConfiguration::Bearer2G;
            case WwanDataClass_Umts:
                return QNetworkConfiguration::BearerWCDMA;
            case WwanDataClass_LteAdvanced:
                return QNetworkConfiguration::BearerLTE;
            case WwanDataClass_Hsdpa:
            case WwanDataClass_Hsupa:
                return QNetworkConfiguration::BearerHSPA;
            case WwanDataClass_Cdma1xRtt:
            case WwanDataClass_Cdma3xRtt:
            case WwanDataClass_CdmaUmb:
                return QNetworkConfiguration::BearerCDMA2000;
            case WwanDataClass_Cdma1xEvdv:
            case WwanDataClass_Cdma1xEvdo:
            case WwanDataClass_Cdma1xEvdoRevA:
            case WwanDataClass_Cdma1xEvdoRevB:
                return QNetworkConfiguration::BearerEVDO;
            case WwanDataClass_Custom:
            case WwanDataClass_None:
            default:
                return QNetworkConfiguration::BearerUnknown;
            }
        }
        return QNetworkConfiguration::BearerEthernet;
    }
#else
    Q_UNUSED(interface);
#endif

    return QNetworkConfiguration::BearerUnknown;
}
#endif

QGenericEngine::QGenericEngine(QObject *parent)
:   QBearerEngineImpl(parent)
{
    //workaround for deadlock in __cxa_guard_acquire with webkit on macos x
    //initialise the Q_GLOBAL_STATIC in same thread as the AtomicallyInitializedStatic
    (void)QNetworkInterface::interfaceFromIndex(0);
}

QGenericEngine::~QGenericEngine()
{
}

QString QGenericEngine::getInterfaceFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    return configurationInterface.value(id);
}

bool QGenericEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);

    return configurationInterface.contains(id);
}

void QGenericEngine::connectToId(const QString &id)
{
    emit connectionError(id, OperationNotSupported);
}

void QGenericEngine::disconnectFromId(const QString &id)
{
    emit connectionError(id, OperationNotSupported);
}

void QGenericEngine::initialize()
{
    doRequestUpdate();
}

void QGenericEngine::requestUpdate()
{
    doRequestUpdate();
}

void QGenericEngine::doRequestUpdate()
{
#ifndef QT_NO_NETWORKINTERFACE
    QMutexLocker locker(&mutex);

    // Immediately after connecting with a wireless access point
    // QNetworkInterface::allInterfaces() will sometimes return an empty list. Calling it again a
    // second time results in a non-empty list. If we loose interfaces we will end up removing
    // network configurations which will break current sessions.
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    if (interfaces.isEmpty())
        interfaces = QNetworkInterface::allInterfaces();

    QStringList previous = accessPointConfigurations.keys();

    // create configuration for each interface
    while (!interfaces.isEmpty()) {
        QNetworkInterface interface = interfaces.takeFirst();

        if (!interface.isValid())
            continue;

        // ignore loopback interface
        if (interface.flags() & QNetworkInterface::IsLoopBack)
            continue;

#ifndef Q_OS_WINRT
        // ignore WLAN interface handled in separate engine
        if (qGetInterfaceType(interface.name()) == QNetworkConfiguration::BearerWLAN)
            continue;
#endif

        uint identifier;
        if (interface.index())
            identifier = qHash(QLatin1String("generic:") + QString::number(interface.index()));
        else
            identifier = qHash(QLatin1String("generic:") + interface.hardwareAddress());

        const QString id = QString::number(identifier);

        previous.removeAll(id);

        QString name = interface.humanReadableName();
        if (name.isEmpty())
            name = interface.name();

        QNetworkConfiguration::StateFlags state = QNetworkConfiguration::Defined;
        if ((interface.flags() & QNetworkInterface::IsRunning) && !interface.addressEntries().isEmpty())
            state |= QNetworkConfiguration::Active;

        if (accessPointConfigurations.contains(id)) {
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

            bool changed = false;

            ptr->mutex.lock();

            if (!ptr->isValid) {
                ptr->isValid = true;
                changed = true;
            }

            if (ptr->name != name) {
                ptr->name = name;
                changed = true;
            }

            if (ptr->id != id) {
                ptr->id = id;
                changed = true;
            }

            if (ptr->state != state) {
                ptr->state = state;
                changed = true;
            }

            ptr->mutex.unlock();

            if (changed) {
                locker.unlock();
                emit configurationChanged(ptr);
                locker.relock();
            }
        } else {
            QNetworkConfigurationPrivatePointer ptr(new QNetworkConfigurationPrivate);

            ptr->name = name;
            ptr->isValid = true;
            ptr->id = id;
            ptr->state = state;
            ptr->type = QNetworkConfiguration::InternetAccessPoint;
            ptr->bearerType = qGetInterfaceType(interface.name());

            accessPointConfigurations.insert(id, ptr);
            configurationInterface.insert(id, interface.name());

            locker.unlock();
            emit configurationAdded(ptr);
            locker.relock();
        }
    }

    while (!previous.isEmpty()) {
        QNetworkConfigurationPrivatePointer ptr =
            accessPointConfigurations.take(previous.takeFirst());

        configurationInterface.remove(ptr->id);

        locker.unlock();
        emit configurationRemoved(ptr);
        locker.relock();
    }

    locker.unlock();
#endif

    emit updateCompleted();
}

QNetworkSession::State QGenericEngine::sessionStateForId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

    if (!ptr)
        return QNetworkSession::Invalid;

    QMutexLocker configLocker(&ptr->mutex);

    if (!ptr->isValid) {
        return QNetworkSession::Invalid;
    } else if ((ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        return QNetworkSession::Connected;
    } else if ((ptr->state & QNetworkConfiguration::Discovered) ==
                QNetworkConfiguration::Discovered) {
        return QNetworkSession::Disconnected;
    } else if ((ptr->state & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined) {
        return QNetworkSession::NotAvailable;
    } else if ((ptr->state & QNetworkConfiguration::Undefined) ==
                QNetworkConfiguration::Undefined) {
        return QNetworkSession::NotAvailable;
    }

    return QNetworkSession::Invalid;
}

QNetworkConfigurationManager::Capabilities QGenericEngine::capabilities() const
{
    return QNetworkConfigurationManager::ForcedRoaming;
}

QNetworkSessionPrivate *QGenericEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl;
}

QNetworkConfigurationPrivatePointer QGenericEngine::defaultConfiguration()
{
    return QNetworkConfigurationPrivatePointer();
}


bool QGenericEngine::requiresPolling() const
{
    return true;
}

QT_END_NAMESPACE
