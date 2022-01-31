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

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtNetwork/qnetworkinformation.h>

#include <QtCore/qstring.h>
#include <QtCore/qobject.h>
#include <QtCore/qloggingcategory.h>

#include <objbase.h>
#include <netlistmgr.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <comdef.h>

#if QT_CONFIG(cpp_winrt) && !defined(Q_CC_CLANG)
#define SUPPORTS_WINRT 1
#endif

#ifdef SUPPORTS_WINRT
#include <winrt/base.h>
#endif

using namespace Microsoft::WRL;

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoNLM)

inline QString errorStringFromHResult(HRESULT hr)
{
    _com_error error(hr);
    return QString::fromWCharArray(error.ErrorMessage());
}

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
    void transportMediumChanged(QNetworkInformation::TransportMedium);
    void isMeteredChanged(bool);

private:
    ComPtr<INetworkListManager> networkListManager = nullptr;
    ComPtr<IConnectionPoint> connectionPoint = nullptr;

#ifdef SUPPORTS_WINRT
    void emitWinRTUpdates();

    winrt::event_token token;
#endif

    QAtomicInteger<ULONG> ref = 0;
    DWORD cookie = 0;
};

QT_END_NAMESPACE
