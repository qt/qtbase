/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    QList<QUrl> rawProxies = libProxyWrapper()->getProxies(queryUrl);

    bool haveDirectConnection = false;
    foreach (const QUrl& url, rawProxies) {
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
