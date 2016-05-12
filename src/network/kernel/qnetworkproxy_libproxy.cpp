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

#include "qnetworkproxy.h"

#ifndef QT_NO_NETWORKPROXY

#include <QtCore/QByteArray>
#include <QtCore/QUrl>

#include <proxy.h>

QT_BEGIN_NAMESPACE

class QLibProxyWrapper
{
public:
    QLibProxyWrapper()
        : factory(px_proxy_factory_new())
    {
        if (!factory)
            qWarning("libproxy initialization failed.");
    }

    ~QLibProxyWrapper()
    {
        px_proxy_factory_free(factory);
    }

    QList<QUrl> getProxies(const QUrl &url);

private:
    pxProxyFactory *factory;
};

Q_GLOBAL_STATIC(QLibProxyWrapper, libProxyWrapper);

/*
    Gets the list of proxies from libproxy, converted to QUrl list.
    Thread safe, according to libproxy documentation.
*/
QList<QUrl> QLibProxyWrapper::getProxies(const QUrl &url)
{
    QList<QUrl> ret;

    if (factory) {
        char **proxies = px_proxy_factory_get_proxies(factory, url.toEncoded());
        if (proxies) {
            for (int i = 0; proxies[i]; i++) {
                ret.append(QUrl::fromEncoded(proxies[i]));
                free(proxies[i]);
            }
            free(proxies);
        }
    }

    return ret;
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxyList;

    QUrl queryUrl;
    QNetworkProxy::Capabilities requiredCapabilities(0);
    switch (query.queryType()) {
    //URL requests are directly supported by libproxy
    case QNetworkProxyQuery::UrlRequest:
        queryUrl = query.url();
        break;
    // fake URLs to get libproxy to tell us the SOCKS proxy
    case QNetworkProxyQuery::TcpSocket:
        queryUrl.setScheme(QStringLiteral("tcp"));
        queryUrl.setHost(query.peerHostName());
        queryUrl.setPort(query.peerPort());
        requiredCapabilities |= QNetworkProxy::TunnelingCapability;
        break;
    case QNetworkProxyQuery::UdpSocket:
        queryUrl.setScheme(QStringLiteral("udp"));
        queryUrl.setHost(query.peerHostName());
        queryUrl.setPort(query.peerPort());
        requiredCapabilities |= QNetworkProxy::UdpTunnelingCapability;
        break;
    default:
        proxyList.append(QNetworkProxy(QNetworkProxy::NoProxy));
        return proxyList;
    }

    const QList<QUrl> rawProxies = libProxyWrapper()->getProxies(queryUrl);

    bool haveDirectConnection = false;
    for (const QUrl& url : rawProxies) {
        QNetworkProxy::ProxyType type;
        const QString scheme = url.scheme();
        if (scheme == QLatin1String("http")) {
            type = QNetworkProxy::HttpProxy;
        } else if (scheme == QLatin1String("socks")
              || scheme == QLatin1String("socks5")) {
            type = QNetworkProxy::Socks5Proxy;
        } else if (scheme == QLatin1String("ftp")) {
            type = QNetworkProxy::FtpCachingProxy;
        } else if (scheme == QLatin1String("direct")) {
            type = QNetworkProxy::NoProxy;
            haveDirectConnection = true;
        } else {
            continue; //unsupported proxy type e.g. socks4
        }

        QNetworkProxy proxy(type,
            url.host(QUrl::EncodeUnicode),
            url.port(0),
            url.userName(QUrl::FullyDecoded),
            url.password(QUrl::FullyDecoded));

        if ((proxy.capabilities() & requiredCapabilities) == requiredCapabilities)
            proxyList.append(proxy);
    }

    // fallback is direct connection
    if (proxyList.isEmpty() || !haveDirectConnection)
        proxyList.append(QNetworkProxy(QNetworkProxy::NoProxy));

    return proxyList;
}

QT_END_NAMESPACE

#endif
