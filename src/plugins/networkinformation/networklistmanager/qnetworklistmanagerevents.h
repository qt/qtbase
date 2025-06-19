// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKLISTMANAGEREVENTS_H
#define QNETWORKLISTMANAGEREVENTS_H

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtNetwork/qnetworkinformation.h>

#include <QtCore/qstring.h>
#include <QtCore/qobject.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmutex.h>

#include <objbase.h>
#include <ocidl.h>
#include <netlistmgr.h>
#include <QtCore/private/qcomptr_p.h>
#include <wrl/wrappers/corewrappers.h>

#if QT_CONFIG(cpp_winrt)
#include <QtCore/private/qt_winrtbase_p.h>
#endif

using namespace Microsoft::WRL;

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoNLM)

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
    void stop();

    [[nodiscard]] bool checkBehindCaptivePortal();

signals:
    void connectivityChanged(NLM_CONNECTIVITY);
    void transportMediumChanged(QNetworkInformation::TransportMedium);
    void isMeteredChanged(bool);

private:
    ComPtr<INetworkListManager> networkListManager = nullptr;
    ComPtr<IConnectionPoint> connectionPoint = nullptr;

#if QT_CONFIG(cpp_winrt)
    void emitWinRTUpdates();

    winrt::event_token token;
    QMutex winrtLock;
#endif

    QAtomicInteger<ULONG> ref = 0;
    DWORD cookie = 0;
};

QT_END_NAMESPACE

#endif // QNETWORKLISTMANAGEREVENTS_H
