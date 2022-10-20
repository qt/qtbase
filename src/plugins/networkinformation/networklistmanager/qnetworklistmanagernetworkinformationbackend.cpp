/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtNetwork/private/qnetworkinformation_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/private/qobject_p.h>
#include <QtCore/qscopeguard.h>

#include <objbase.h>
#include <netlistmgr.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <comdef.h>
#include <iphlpapi.h>
using namespace Microsoft::WRL;

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoNLM)
Q_LOGGING_CATEGORY(lcNetInfoNLM, "qt.network.info.netlistmanager");

static const QString backendName = QStringLiteral("networklistmanager");

namespace {
QString errorStringFromHResult(HRESULT hr)
{
    _com_error error(hr);
    return QString::fromWCharArray(error.ErrorMessage());
}

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

bool testCONNECTIVITY(NLM_CONNECTIVITY connectivity, NLM_CONNECTIVITY flag)
{
    return (connectivity & flag) == flag;
}

QNetworkInformation::Reachability reachabilityFromNLM_CONNECTIVITY(NLM_CONNECTIVITY connectivity)
{
    if (connectivity == NLM_CONNECTIVITY_DISCONNECTED)
        return QNetworkInformation::Reachability::Disconnected;
    if (testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV6_INTERNET)
        || testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV4_INTERNET)) {
        return QNetworkInformation::Reachability::Online;
    }
    if (testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV6_SUBNET)
        || testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV4_SUBNET)) {
        return QNetworkInformation::Reachability::Site;
    }
    if (testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV6_LOCALNETWORK)
        || testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV4_LOCALNETWORK)) {
        return QNetworkInformation::Reachability::Local;
    }
    if (testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV6_NOTRAFFIC)
        || testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV4_NOTRAFFIC)) {
        return QNetworkInformation::Reachability::Unknown;
    }

    return QNetworkInformation::Reachability::Unknown;
}
}

class QNetworkListManagerEvents;
class QNetworkListManagerNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QNetworkListManagerNetworkInformationBackend();
    ~QNetworkListManagerNetworkInformationBackend();

    QString name() const override { return backendName; }
    QNetworkInformation::Features featuresSupported() const override
    {
        return featuresSupportedStatic();
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        return QNetworkInformation::Features(QNetworkInformation::Feature::Reachability
                                             | QNetworkInformation::Feature::CaptivePortal);
    }

    [[nodiscard]] bool start();
    void stop();

private:
    friend class QNetworkListManagerEvents;

    bool event(QEvent *event) override;
    void setConnectivity(NLM_CONNECTIVITY newConnectivity);
    void checkCaptivePortal();

    ComPtr<QNetworkListManagerEvents> managerEvents;

    NLM_CONNECTIVITY connectivity = NLM_CONNECTIVITY_DISCONNECTED;

    bool monitoring = false;
    bool comInitFailed = false;
};

class QNetworkListManagerNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QNetworkListManagerNetworkInformationBackendFactory() = default;
    ~QNetworkListManagerNetworkInformationBackendFactory() = default;
    QString name() const override { return backendName; }
    QNetworkInformation::Features featuresSupported() const override
    {
        return QNetworkListManagerNetworkInformationBackend::featuresSupportedStatic();
    }

    QNetworkInformationBackend *
    create(QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        auto backend = new QNetworkListManagerNetworkInformationBackend();
        if (!backend->start()) {
            qCWarning(lcNetInfoNLM) << "Failed to start listening to events";
            delete backend;
            backend = nullptr;
        }
        return backend;
    }
};

class QNetworkListManagerEvents : public QObject, public INetworkListManagerEvents
{
    Q_OBJECT
public:
    QNetworkListManagerEvents();
    virtual ~QNetworkListManagerEvents();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--ref == 0) {
            delete this;
            return 0;
        }
        return ref;
    }

    HRESULT STDMETHODCALLTYPE ConnectivityChanged(NLM_CONNECTIVITY newConnectivity) override;

    [[nodiscard]] bool start();
    bool stop();

    [[nodiscard]] bool checkBehindCaptivePortal();

signals:
    void connectivityChanged(NLM_CONNECTIVITY);

private:
    ComPtr<INetworkListManager> networkListManager = nullptr;
    ComPtr<IConnectionPoint> connectionPoint = nullptr;

    QAtomicInteger<ULONG> ref = 0;
    DWORD cookie = 0;
};

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
    connectivityChanged(newConnectivity);
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
    if (FAILED(hr)) {
        qCWarning(lcNetInfoNLM) << "Could not get connectivity:" << errorStringFromHResult(hr);
    } else {
        emit connectivityChanged(connectivity);
    }
    return true;
}

bool QNetworkListManagerEvents::stop()
{
    Q_ASSERT(connectionPoint);
    auto hr = connectionPoint->Unadvise(cookie);
    if (FAILED(hr)) {
        qCWarning(lcNetInfoNLM) << "Failed to unsubscribe from network connectivity events:"
                                << errorStringFromHResult(hr);
        return false;
    }
    cookie = 0;
    return true;
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

QNetworkListManagerNetworkInformationBackend::QNetworkListManagerNetworkInformationBackend()
{
    auto hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        qCWarning(lcNetInfoNLM) << "Failed to initialize COM:" << errorStringFromHResult(hr);
        comInitFailed = true;
        return;
    }
    managerEvents = new QNetworkListManagerEvents();
    connect(managerEvents.Get(), &QNetworkListManagerEvents::connectivityChanged, this,
            &QNetworkListManagerNetworkInformationBackend::setConnectivity);
}

QNetworkListManagerNetworkInformationBackend::~QNetworkListManagerNetworkInformationBackend()
{
    if (comInitFailed)
        return;
    stop();
}

void QNetworkListManagerNetworkInformationBackend::setConnectivity(NLM_CONNECTIVITY newConnectivity)
{
    if (reachabilityFromNLM_CONNECTIVITY(connectivity)
        != reachabilityFromNLM_CONNECTIVITY(newConnectivity)) {
        connectivity = newConnectivity;
        setReachability(reachabilityFromNLM_CONNECTIVITY(newConnectivity));

        // @future: only check if signal is connected
        checkCaptivePortal();
    }
}

void QNetworkListManagerNetworkInformationBackend::checkCaptivePortal()
{
    setBehindCaptivePortal(managerEvents->checkBehindCaptivePortal());
}

bool QNetworkListManagerNetworkInformationBackend::event(QEvent *event)
{
    if (event->type() == QEvent::ThreadChange && monitoring) {
        stop();
        QMetaObject::invokeMethod(this, &QNetworkListManagerNetworkInformationBackend::start,
                                  Qt::QueuedConnection);
    }

    return QObject::event(event);
}

bool QNetworkListManagerNetworkInformationBackend::start()
{
    Q_ASSERT(!monitoring);

    if (comInitFailed) {
        auto hr = CoInitialize(nullptr);
        if (FAILED(hr)) {
            qCWarning(lcNetInfoNLM) << "Failed to initialize COM:" << errorStringFromHResult(hr);
            comInitFailed = true;
            return false;
        }
        comInitFailed = false;
    }
    if (!managerEvents)
        managerEvents = new QNetworkListManagerEvents();

    if (managerEvents->start())
        monitoring = true;
    return monitoring;
}

void QNetworkListManagerNetworkInformationBackend::stop()
{
    if (monitoring) {
        Q_ASSERT(managerEvents);
        // Can return false but realistically shouldn't since that would break everything:
        managerEvents->stop();
        monitoring = false;
        managerEvents.Reset();
    }

    CoUninitialize();
    comInitFailed = true; // we check this value in start() to see if we need to re-initialize
}

QT_END_NAMESPACE

#include "qnetworklistmanagernetworkinformationbackend.moc"
