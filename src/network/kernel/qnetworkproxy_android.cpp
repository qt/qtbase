// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:trusted-data

#include "qnetworkproxy.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qcoreapplication_platform.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>

#ifndef QT_NO_NETWORKPROXY

QT_BEGIN_NAMESPACE

struct ProxyInfoObject
{
public:
    ProxyInfoObject();
    ~ProxyInfoObject();
};

using namespace QNativeInterface;
using namespace QtJniTypes;

Q_APPLICATION_STATIC(ProxyInfoObject, proxyInfoInstance)

Q_DECLARE_JNI_CLASS(QtNetwork, "org/qtproject/qt/android/network/QtNetwork")
Q_DECLARE_JNI_CLASS(ProxyInfo, "android/net/ProxyInfo")

ProxyInfoObject::ProxyInfoObject()
{
    QtNetwork::callStaticMethod<void>("registerReceiver", QAndroidApplication::context());
}

ProxyInfoObject::~ProxyInfoObject()
{
    QtNetwork::callStaticMethod<void>("unregisterReceiver", QAndroidApplication::context());
}

// nonProxyHosts entries, as ignoreProxyFor() in qnetworkproxy_generic.cpp.
static bool isExcluded(QStringView host, QStringView entry)
{
    QStringView token = entry.trimmed();
    if (token.isEmpty())
        return false;
    if (token.size() == 1 && token.front() == u'*')
        return true;

    if (token.startsWith(u'*'))
        token = token.sliced(1);
    if (token.endsWith(u'.') && !host.endsWith(u'.'))
        token = token.chopped(1);
    if (token.startsWith(u'.'))
        token = token.sliced(1);

    if (token.isEmpty() || !host.endsWith(token, Qt::CaseInsensitive))
        return false;

    // Keep "example.com" from matching "notexample.com".
    return (host.size() == token.size()) || (host[host.size() - token.size() - 1] == u'.');
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxyList;
    if (!proxyInfoInstance)
        return proxyList;

    QJniObject proxyInfo = QtNetwork::callStaticMethod<ProxyInfo>("getProxyInfo",
                                                                  QAndroidApplication::context());
    if (proxyInfo.isValid()) {
        const QJniArray exclusionList = proxyInfo.callMethod<String[]>("getExclusionList");
        bool exclude = false;
        if (exclusionList.isValid()) {
            const QString host = query.peerHostName();
            for (const auto &entry : exclusionList) {
                if (isExcluded(host, entry.toString())) {
                    exclude = true;
                    break;
                }
            }
        }
        if (!exclude) {
            const QString hostName = proxyInfo.callMethod<QString>("getHost");
            const int port = proxyInfo.callMethod<jint>("getPort");
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, hostName, port);
            proxyList << proxy;
        }
    }
    if (proxyList.isEmpty())
        proxyList << QNetworkProxy::NoProxy;

    return proxyList;
}

QT_END_NAMESPACE

#endif
