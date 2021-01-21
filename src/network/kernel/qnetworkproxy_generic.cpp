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

#include <QtCore/QByteArray>
#include <QtCore/QUrl>

#ifndef QT_NO_NETWORKPROXY

/*
 * Construct a proxy from the environment variables http_proxy and no_proxy.
 * Or no system proxy. Just return a list with NoProxy.
 */

QT_BEGIN_NAMESPACE

static bool ignoreProxyFor(const QNetworkProxyQuery &query)
{
    const QByteArray noProxy = qgetenv("no_proxy").trimmed();
    if (noProxy.isEmpty())
        return false;

    const QList<QByteArray> noProxyTokens = noProxy.split(',');

    for (const QByteArray &rawToken : noProxyTokens) {
        QByteArray token = rawToken.trimmed();
        QString peerHostName = query.peerHostName();

        // Since we use suffix matching, "*" is our 'default' behaviour
        if (token.startsWith('*'))
            token = token.mid(1);

        // Harmonize trailing dot notation
        if (token.endsWith('.') && !peerHostName.endsWith('.'))
            token = token.left(token.length()-1);

        // We prepend a dot to both values, so that when we do a suffix match,
        // we don't match "donotmatch.com" with "match.com"
        if (!token.startsWith('.'))
            token.prepend('.');

        if (!peerHostName.startsWith('.'))
            peerHostName.prepend('.');

        if (peerHostName.endsWith(QLatin1String(token)))
            return true;
    }

    return false;
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxyList;

    if (ignoreProxyFor(query))
        return proxyList << QNetworkProxy::NoProxy;

    // No need to care about casing here, QUrl lowercases values already
    const QString queryProtocol = query.protocolTag();
    QByteArray proxy_env;

    if (queryProtocol == QLatin1String("http"))
        proxy_env = qgetenv("http_proxy");
    else if (queryProtocol == QLatin1String("https"))
        proxy_env = qgetenv("https_proxy");
    else if (queryProtocol == QLatin1String("ftp"))
        proxy_env = qgetenv("ftp_proxy");
    else
        proxy_env = qgetenv("all_proxy");

    // Fallback to http_proxy is no protocol specific proxy was found
    if (proxy_env.isEmpty())
        proxy_env = qgetenv("http_proxy");

    if (!proxy_env.isEmpty()) {
        QUrl url = QUrl(QString::fromLocal8Bit(proxy_env));
        const QString scheme = url.scheme();
        if (scheme == QLatin1String("socks5")) {
            QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, url.host(),
                    url.port() ? url.port() : 1080, url.userName(), url.password());
            proxyList << proxy;
        } else if (scheme == QLatin1String("socks5h")) {
            QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, url.host(),
                    url.port() ? url.port() : 1080, url.userName(), url.password());
            proxy.setCapabilities(QNetworkProxy::HostNameLookupCapability);
            proxyList << proxy;
        } else if ((scheme.isEmpty() || scheme == QLatin1String("http"))
                  && query.queryType() != QNetworkProxyQuery::UdpSocket
                  && query.queryType() != QNetworkProxyQuery::TcpServer) {
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, url.host(),
                    url.port() ? url.port() : 8080, url.userName(), url.password());
            proxyList << proxy;
        }
    }
    if (proxyList.isEmpty())
        proxyList << QNetworkProxy::NoProxy;

    return proxyList;
}

QT_END_NAMESPACE

#endif
