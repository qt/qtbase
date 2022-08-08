// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetworkproxy.h"

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

Q_GLOBAL_STATIC(ProxyInfoObject, proxyInfoInstance)

static const char networkClass[] = "org/qtproject/qt/android/network/QtNetwork";

Q_DECLARE_JNI_TYPE(ProxyInfo, "Landroid/net/ProxyInfo;")
Q_DECLARE_JNI_TYPE(JStringArray, "[Ljava/lang/String;")

ProxyInfoObject::ProxyInfoObject()
{
    QJniObject::callStaticMethod<void>(networkClass,
                                       "registerReceiver",
                                       QAndroidApplication::context());
}

ProxyInfoObject::~ProxyInfoObject()
{
    QJniObject::callStaticMethod<void>(networkClass,
                                       "unregisterReceiver",
                                       QAndroidApplication::context());
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxyList;
    if (!proxyInfoInstance)
        return proxyList;

    QJniObject proxyInfo = QJniObject::callStaticObjectMethod<QtJniTypes::ProxyInfo>(
            networkClass, "getProxyInfo", QAndroidApplication::context());
    if (proxyInfo.isValid()) {
        QJniObject exclusionList =
                proxyInfo.callObjectMethod<QtJniTypes::JStringArray>("getExclusionList");
        bool exclude = false;
        if (exclusionList.isValid()) {
            jobjectArray listObject = exclusionList.object<jobjectArray>();
            QJniEnvironment env;
            QJniObject entry;
            const int size = env->GetArrayLength(listObject);
            QUrl host = QUrl(query.url().host());
            for (int i = 0; i < size; ++i) {
                entry = env->GetObjectArrayElement(listObject, i);
                if (host.matches(QUrl(entry.toString()), QUrl::RemoveScheme)) {
                    exclude = true;
                    break;
                }
            }
        }
        if (!exclude) {
            QJniObject hostName = proxyInfo.callObjectMethod<jstring>("getHost");
            const int port = proxyInfo.callMethod<jint>("getPort");
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, hostName.toString(), port);
            proxyList << proxy;
        }
    }
    if (proxyList.isEmpty())
        proxyList << QNetworkProxy::NoProxy;

    return proxyList;
}

QT_END_NAMESPACE

#endif
