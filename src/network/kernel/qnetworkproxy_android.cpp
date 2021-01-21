/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qnetworkproxy.h"
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>

#ifndef QT_NO_NETWORKPROXY

QT_BEGIN_NAMESPACE

struct ProxyInfoObject
{
public:
    ProxyInfoObject();
    ~ProxyInfoObject();
};

Q_GLOBAL_STATIC(ProxyInfoObject, proxyInfoInstance)

static const char networkClass[] = "org/qtproject/qt5/android/network/QtNetwork";

ProxyInfoObject::ProxyInfoObject()
{
    QJNIObjectPrivate::callStaticMethod<void>(networkClass,
                                              "registerReceiver",
                                              "(Landroid/content/Context;)V",
                                              QtAndroidPrivate::context());
}

ProxyInfoObject::~ProxyInfoObject()
{
    QJNIObjectPrivate::callStaticMethod<void>(networkClass,
                                              "unregisterReceiver",
                                              "(Landroid/content/Context;)V",
                                              QtAndroidPrivate::context());
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxyList;
    if (!proxyInfoInstance)
        return proxyList;

    QJNIObjectPrivate proxyInfo = QJNIObjectPrivate::callStaticObjectMethod(networkClass,
                                      "getProxyInfo",
                                      "(Landroid/content/Context;)Landroid/net/ProxyInfo;",
                                      QtAndroidPrivate::context());
    if (proxyInfo.isValid()) {
        QJNIObjectPrivate exclusionList = proxyInfo.callObjectMethod("getExclusionList",
                                                                     "()[Ljava/lang/String;");
        bool exclude = false;
        if (exclusionList.isValid()) {
            jobjectArray listObject = static_cast<jobjectArray>(exclusionList.object());
            QJNIEnvironmentPrivate env;
            QJNIObjectPrivate entry;
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
            QJNIObjectPrivate hostName = proxyInfo.callObjectMethod<jstring>("getHost");
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
