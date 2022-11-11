// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetworklistmanagerevents.h"

#include <QtCore/qpointer.h>

#include <mutex>

#ifdef SUPPORTS_WINRT
#include <winrt/base.h>
#include <QtCore/private/qfactorycacheregistration_p.h>
// Workaround for Windows SDK bug.
// See https://github.com/microsoft/Windows.UI.Composition-Win32-Samples/issues/47
namespace winrt::impl
{
    template <typename Async>
    auto wait_for(Async const& async, Windows::Foundation::TimeSpan const& timeout);
}

#include <winrt/Windows.Networking.Connectivity.h>
#endif

QT_BEGIN_NAMESPACE

namespace {
template<typename T>
bool QueryInterfaceImpl(IUnknown *from, REFIID riid, void **ppvObject)
{
    if (riid == __uuidof(T)) {
        *ppvObject = static_cast<T *>(from);
        from->AddRef();
        return true;
    }
    return false;
}
}

QNetworkListManagerEvents::QNetworkListManagerEvents() : QObject(nullptr)
{
    auto hr = CoCreateInstance(CLSID_NetworkListManager, nullptr, CLSCTX_INPROC_SERVER,
                               IID_INetworkListManager, &networkListManager);
    if (FAILED(hr)) {
        qCWarning(lcNetInfoNLM) << "Could not get a NetworkListManager instance:"
                                << errorStringFromHResult(hr);
        return;
    }

    ComPtr<IConnectionPointContainer> connectionPointContainer;
    hr = networkListManager.As(&connectionPointContainer);
    if (SUCCEEDED(hr)) {
        hr = connectionPointContainer->FindConnectionPoint(IID_INetworkListManagerEvents,
                                                           &connectionPoint);
    }
    if (FAILED(hr)) {
        qCWarning(lcNetInfoNLM) << "Failed to get connection point for network list manager events:"
                                << errorStringFromHResult(hr);
    }
}

QNetworkListManagerEvents::~QNetworkListManagerEvents()
{
    Q_ASSERT(ref == 0);
}

HRESULT STDMETHODCALLTYPE QNetworkListManagerEvents::QueryInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
        return E_INVALIDARG;

    return QueryInterfaceImpl<IUnknown>(this, riid, ppvObject)
                    || QueryInterfaceImpl<INetworkListManagerEvents>(this, riid, ppvObject)
            ? S_OK
            : E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE
QNetworkListManagerEvents::ConnectivityChanged(NLM_CONNECTIVITY newConnectivity)
{
    // This function is run on a different thread than 'monitor' is created on, so we need to run
    // it on that thread
    emit connectivityChanged(newConnectivity);
    return S_OK;
}

bool QNetworkListManagerEvents::start()
{
    if (!connectionPoint) {
        qCWarning(lcNetInfoNLM, "Initialization failed, can't start!");
        return false;
    }
    auto hr = connectionPoint->Advise(this, &cookie);
    if (FAILED(hr)) {
        qCWarning(lcNetInfoNLM) << "Failed to subscribe to network connectivity events:"
                                << errorStringFromHResult(hr);
        return false;
    }

    // Update connectivity since it might have changed since this class was constructed
    NLM_CONNECTIVITY connectivity;
    hr = networkListManager->GetConnectivity(&connectivity);
    if (FAILED(hr))
        qCWarning(lcNetInfoNLM) << "Could not get connectivity:" << errorStringFromHResult(hr);
    else
        emit connectivityChanged(connectivity);

#ifdef SUPPORTS_WINRT
    using namespace winrt::Windows::Networking::Connectivity;
    using winrt::Windows::Foundation::IInspectable;
    // Register for changes in the network and store a token to unregister later:
    token = NetworkInformation::NetworkStatusChanged(
            [owner = QPointer(this)](const IInspectable sender) {
                Q_UNUSED(sender);
                if (owner) {
                    std::scoped_lock locker(owner->winrtLock);
                    if (owner->token)
                        owner->emitWinRTUpdates();
                }
            });
    // Emit initial state
    emitWinRTUpdates();
#endif

    return true;
}

void QNetworkListManagerEvents::stop()
{
    Q_ASSERT(connectionPoint);
    auto hr = connectionPoint->Unadvise(cookie);
    if (FAILED(hr)) {
        qCWarning(lcNetInfoNLM) << "Failed to unsubscribe from network connectivity events:"
                                << errorStringFromHResult(hr);
    } else {
        cookie = 0;
    }
    // Even if we fail we should still try to unregister from winrt events:

#ifdef SUPPORTS_WINRT
    // Try to synchronize unregistering with potentially in-progress callbacks
    std::scoped_lock locker(winrtLock);
    if (token) {
        using namespace winrt::Windows::Networking::Connectivity;
        // Pass the token we stored earlier to unregister:
        NetworkInformation::NetworkStatusChanged(token);
        token = {};
    }
#endif
}

bool QNetworkListManagerEvents::checkBehindCaptivePortal()
{
    if (!networkListManager)
        return false;
    ComPtr<IEnumNetworks> networks;
    HRESULT hr =
            networkListManager->GetNetworks(NLM_ENUM_NETWORK_CONNECTED, networks.GetAddressOf());
    if (FAILED(hr) || networks == nullptr)
        return false;

    // @note: This checks all connected networks, but that might not be necessary
    ComPtr<INetwork> network;
    hr = networks->Next(1, network.GetAddressOf(), nullptr);
    while (SUCCEEDED(hr) && network != nullptr) {
        ComPtr<IPropertyBag> propertyBag;
        hr = network.As(&propertyBag);
        if (SUCCEEDED(hr) && propertyBag != nullptr) {
            VARIANT variant;
            VariantInit(&variant);
            const auto scopedVariantClear = qScopeGuard([&variant]() { VariantClear(&variant); });

            const wchar_t *versions[] = { NA_InternetConnectivityV6, NA_InternetConnectivityV4 };
            for (const auto version : versions) {
                hr = propertyBag->Read(version, &variant, nullptr);
                if (SUCCEEDED(hr)
                    && (V_UINT(&variant) & NLM_INTERNET_CONNECTIVITY_WEBHIJACK)
                            == NLM_INTERNET_CONNECTIVITY_WEBHIJACK) {
                    return true;
                }
            }
        }

        hr = networks->Next(1, network.GetAddressOf(), nullptr);
    }

    return false;
}

#ifdef SUPPORTS_WINRT
namespace {
using namespace winrt::Windows::Networking::Connectivity;
// NB: this isn't part of "network list manager", but sadly NLM doesn't have an
// equivalent API (at least not that I've found...)!
[[nodiscard]]
QNetworkInformation::TransportMedium getTransportMedium(const ConnectionProfile &profile)
{
    if (profile.IsWwanConnectionProfile())
        return QNetworkInformation::TransportMedium::Cellular;
    if (profile.IsWlanConnectionProfile())
        return QNetworkInformation::TransportMedium::WiFi;

    NetworkAdapter adapter(nullptr);
    try {
        adapter = profile.NetworkAdapter();
    } catch (...) {
        // pass, we will return Unknown anyway
    }
    if (adapter == nullptr)
        return QNetworkInformation::TransportMedium::Unknown;

    // Note: Bluetooth is given an iana iftype of 6, which is the same as Ethernet.
    // In Windows itself there is clearly a distinction between a Bluetooth PAN
    // and an Ethernet LAN, though it is not clear how they make this distinction.
    auto fromIanaId = [](quint32 ianaId) -> QNetworkInformation::TransportMedium {
        // https://www.iana.org/assignments/ianaiftype-mib/ianaiftype-mib
        switch (ianaId) {
        case 6:
            return QNetworkInformation::TransportMedium::Ethernet;
        case 71: // Should be handled before entering this lambda
            return QNetworkInformation::TransportMedium::WiFi;
        }
        return QNetworkInformation::TransportMedium::Unknown;
    };

    return fromIanaId(adapter.IanaInterfaceType());
}

[[nodiscard]] bool getMetered(const ConnectionProfile &profile)
{
    ConnectionCost cost(nullptr);
    try {
        cost = profile.GetConnectionCost();
    } catch (...) {
        // pass, we return false if we get an empty object back anyway
    }
    if (cost == nullptr)
        return false;
    NetworkCostType type = cost.NetworkCostType();
    return type == NetworkCostType::Fixed || type == NetworkCostType::Variable;
}
} // unnamed namespace

void QNetworkListManagerEvents::emitWinRTUpdates()
{
    using namespace winrt::Windows::Networking::Connectivity;
    ConnectionProfile profile = nullptr;
    try {
        profile = NetworkInformation::GetInternetConnectionProfile();
    } catch (...) {
        // pass, we would just return early if we get an empty object back anyway
    }
    if (profile == nullptr)
        return;
    emit transportMediumChanged(getTransportMedium(profile));
    emit isMeteredChanged(getMetered(profile));
}
#endif

QT_END_NAMESPACE
