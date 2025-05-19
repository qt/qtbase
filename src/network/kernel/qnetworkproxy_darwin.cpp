// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:trusted-data

#include "qnetworkproxy.h"

#ifndef QT_NO_NETWORKPROXY

#include <CFNetwork/CFNetwork.h>
#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/qendian.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qsystemdetection.h>
#include "private/qcore_mac_p.h"

/*
 * MacOS X has a proxy configuration module in System Preferences (on
 * MacOS X 10.5, it's in Network, Advanced), where one can set the
 * proxy settings for:
 *
 * \list
 *   \li FTP proxy
 *   \li Web Proxy (HTTP)
 *   \li Secure Web Proxy (HTTPS)
 *   \li Streaming Proxy (RTSP)
 *   \li SOCKS Proxy
 *   \li Gopher Proxy
 *   \li URL for Automatic Proxy Configuration (PAC scripts)
 *   \li Bypass list (by default: *.local, 169.254/16)
 * \endlist
 *
 * The matching configuration can be obtained by calling CFNetworkCopySystemProxySettings()
 * (from <CFNetwork/CFProxySupport.h>). See
 * Apple's documentation:
 *
 * https://developer.apple.com/documentation/cfnetwork/1426754-cfnetworkcopysystemproxysettings?language=objc
 *
 */

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static bool isHostExcluded(CFDictionaryRef dict, const QString &host)
{
    Q_ASSERT(dict);

    if (host.isEmpty())
        return true;

#ifndef Q_OS_IOS
    // On iOS all those keys are not available, and worse so - entries
    // for HTTPS are not in the dictionary, but instead in some nested dictionary
    // with undocumented keys/object types.
    bool isSimple = !host.contains(u'.') && !host.contains(u':');
    CFNumberRef excludeSimples;
    if (isSimple &&
        (excludeSimples = (CFNumberRef)CFDictionaryGetValue(dict, kCFNetworkProxiesExcludeSimpleHostnames))) {
        int enabled;
        if (CFNumberGetValue(excludeSimples, kCFNumberIntType, &enabled) && enabled)
            return true;
    }

    QHostAddress ipAddress;
    bool isIpAddress = ipAddress.setAddress(host);

    // not a simple host name
    // does it match the list of exclusions?
    CFArrayRef exclusionList = (CFArrayRef)CFDictionaryGetValue(dict, kCFNetworkProxiesExceptionsList);
    if (!exclusionList)
        return false;

    CFIndex size = CFArrayGetCount(exclusionList);
    for (CFIndex i = 0; i < size; ++i) {
        CFStringRef cfentry = (CFStringRef)CFArrayGetValueAtIndex(exclusionList, i);
        QString entry = QString::fromCFString(cfentry);

        if (isIpAddress && ipAddress.isInSubnet(QHostAddress::parseSubnet(entry))) {
            return true;        // excluded
        } else {
            // do wildcard matching
            auto rx = QRegularExpression::fromWildcard(entry, Qt::CaseInsensitive);
            if (rx.match(host).hasMatch())
                return true;
        }
    }
#else
    Q_UNUSED(dict);
#endif // Q_OS_IOS
    // host was not excluded
    return false;
}

static QNetworkProxy proxyFromDictionary(CFDictionaryRef dict, QNetworkProxy::ProxyType type,
                                         CFStringRef enableKey, CFStringRef hostKey,
                                         CFStringRef portKey)
{
    CFNumberRef protoEnabled;
    CFNumberRef protoPort;
    CFStringRef protoHost;
    if (enableKey
        && (protoEnabled = (CFNumberRef)CFDictionaryGetValue(dict, enableKey))
        && (protoHost = (CFStringRef)CFDictionaryGetValue(dict, hostKey))
        && (protoPort = (CFNumberRef)CFDictionaryGetValue(dict, portKey))) {
        int enabled;
        if (CFNumberGetValue(protoEnabled, kCFNumberIntType, &enabled) && enabled) {
            QString host = QString::fromCFString(protoHost);

            int port;
            CFNumberGetValue(protoPort, kCFNumberIntType, &port);

            return QNetworkProxy(type, host, port);
        }
    }

    // proxy not enabled
    return QNetworkProxy();
}

static QNetworkProxy proxyFromDictionary(CFDictionaryRef dict)
{
    QNetworkProxy::ProxyType proxyType = QNetworkProxy::DefaultProxy;
    QString hostName;
    quint16 port = 0;
    QString user;
    QString password;

    CFStringRef cfProxyType = (CFStringRef)CFDictionaryGetValue(dict, kCFProxyTypeKey);
    if (CFStringCompare(cfProxyType, kCFProxyTypeNone, 0) == kCFCompareEqualTo) {
        proxyType = QNetworkProxy::NoProxy;
    } else if (CFStringCompare(cfProxyType, kCFProxyTypeFTP, 0) == kCFCompareEqualTo) {
        proxyType = QNetworkProxy::FtpCachingProxy;
    } else if (CFStringCompare(cfProxyType, kCFProxyTypeHTTP, 0) == kCFCompareEqualTo) {
        proxyType = QNetworkProxy::HttpProxy;
    } else if (CFStringCompare(cfProxyType, kCFProxyTypeHTTPS, 0) == kCFCompareEqualTo) {
        proxyType = QNetworkProxy::HttpProxy;
    } else if (CFStringCompare(cfProxyType, kCFProxyTypeSOCKS, 0) == kCFCompareEqualTo) {
        proxyType = QNetworkProxy::Socks5Proxy;
    }

    hostName = QString::fromCFString((CFStringRef)CFDictionaryGetValue(dict, kCFProxyHostNameKey));
    user     = QString::fromCFString((CFStringRef)CFDictionaryGetValue(dict, kCFProxyUsernameKey));
    password = QString::fromCFString((CFStringRef)CFDictionaryGetValue(dict, kCFProxyPasswordKey));

    CFNumberRef portNumber = (CFNumberRef)CFDictionaryGetValue(dict, kCFProxyPortNumberKey);
    if (portNumber) {
        CFNumberGetValue(portNumber, kCFNumberSInt16Type, &port);
    }

    return QNetworkProxy(proxyType, hostName, port, user, password);
}

namespace {
struct PACInfo {
    QCFType<CFArrayRef> proxies;
    QCFType<CFErrorRef> error;
    bool done = false;
};

void proxyAutoConfigCallback(void *client, CFArrayRef proxylist, CFErrorRef error)
{
    Q_ASSERT(client);

    PACInfo *info = static_cast<PACInfo *>(client);
    info->done = true;

    if (error) {
        CFRetain(error);
        info->error = error;
    }
    if (proxylist) {
        CFRetain(proxylist);
        info->proxies = proxylist;
    }
}

QCFType<CFStringRef> stringByAddingPercentEscapes(CFStringRef originalPath)
{
    Q_ASSERT(originalPath);
    const auto qtPath = QString::fromCFString(originalPath);
    const auto escaped = QString::fromUtf8(QUrl(qtPath).toEncoded());
    return escaped.toCFString();
}

#ifdef Q_OS_IOS
QList<QNetworkProxy> proxiesForQueryUrl(CFDictionaryRef dict, const QUrl &url)
{
    Q_ASSERT(dict);

    const QCFType<CFURLRef> cfUrl = url.toCFURL();
    const QCFType<CFArrayRef> proxies = CFNetworkCopyProxiesForURL(cfUrl, dict);
    Q_ASSERT(proxies);

    QList<QNetworkProxy> result;
    const auto count = CFArrayGetCount(proxies);
    if (!count) // Could be no proper proxy or host excluded.
        return result;

    for (CFIndex i = 0; i < count; ++i) {
        const void *obj = CFArrayGetValueAtIndex(proxies, i);
        if (CFGetTypeID(obj) != CFDictionaryGetTypeID())
            continue;
        const QNetworkProxy proxy = proxyFromDictionary(static_cast<CFDictionaryRef>(obj));
        if (proxy.type() == QNetworkProxy::NoProxy || proxy.type() == QNetworkProxy::DefaultProxy)
            continue;
        result << proxy;
    }

    return result;
}
#endif // Q_OS_IOS
} // unnamed namespace.

QList<QNetworkProxy> macQueryInternal(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> result;

    // obtain a dictionary to the proxy settings:
    const QCFType<CFDictionaryRef> dict = CFNetworkCopySystemProxySettings();
    if (!dict) {
        qWarning("QNetworkProxyFactory::systemProxyForQuery: CFNetworkCopySystemProxySettings returned nullptr");
        return result;          // failed
    }

    if (isHostExcluded(dict, query.peerHostName()))
        return result;          // no proxy for this host

    // is there a PAC enabled? If so, use it first.
    CFNumberRef pacEnabled;
    if ((pacEnabled = (CFNumberRef)CFDictionaryGetValue(dict, kCFNetworkProxiesProxyAutoConfigEnable))) {
        int enabled;
        if (CFNumberGetValue(pacEnabled, kCFNumberIntType, &enabled) && enabled) {
            // PAC is enabled
            // kSCPropNetProxiesProxyAutoConfigURLString returns the URL string
            // as entered in the system proxy configuration dialog
            CFStringRef pacLocationSetting = (CFStringRef)CFDictionaryGetValue(dict, kCFNetworkProxiesProxyAutoConfigURLString);
            auto cfPacLocation = stringByAddingPercentEscapes(pacLocationSetting);
            QCFType<CFDataRef> pacData;
            QCFType<CFURLRef> pacUrl = CFURLCreateWithString(kCFAllocatorDefault, cfPacLocation, NULL);
            if (!pacUrl) {
                qWarning("Invalid PAC URL \"%s\"", qPrintable(QString::fromCFString(cfPacLocation)));
                return result;
            }

            QByteArray encodedURL = query.url().toEncoded(); // converted to UTF-8
            if (encodedURL.isEmpty()) {
                return result; // Invalid URL, abort
            }

            QCFType<CFURLRef> targetURL = CFURLCreateWithBytes(kCFAllocatorDefault, (UInt8*)encodedURL.data(), encodedURL.size(), kCFStringEncodingUTF8, NULL);
            if (!targetURL) {
                return result; // URL creation problem, abort
            }

            CFStreamClientContext pacCtx;
            pacCtx.version = 0;
            PACInfo pacInfo;
            pacCtx.info = &pacInfo;
            pacCtx.retain = NULL;
            pacCtx.release = NULL;
            pacCtx.copyDescription = NULL;

            static CFStringRef pacRunLoopMode = CFSTR("qtPACRunLoopMode");

            QCFType<CFRunLoopSourceRef> pacRunLoopSource = CFNetworkExecuteProxyAutoConfigurationURL(pacUrl, targetURL, &proxyAutoConfigCallback, &pacCtx);
            CFRunLoopAddSource(CFRunLoopGetCurrent(), pacRunLoopSource, pacRunLoopMode);
            while (!pacInfo.done)
                CFRunLoopRunInMode(pacRunLoopMode, 1000, /*returnAfterSourceHandled*/ true);

            if (!pacInfo.proxies) {
                QString pacLocation = QString::fromCFString(cfPacLocation);
                QCFType<CFStringRef> pacErrorDescription = CFErrorCopyDescription(pacInfo.error);
                qWarning("Execution of PAC script at \"%s\" failed: %s", qPrintable(pacLocation), qPrintable(QString::fromCFString(pacErrorDescription)));
                return result;
            }

            CFIndex size = CFArrayGetCount(pacInfo.proxies);
            for (CFIndex i = 0; i < size; ++i) {
                CFDictionaryRef proxy = (CFDictionaryRef)CFArrayGetValueAtIndex(pacInfo.proxies, i);
                result << proxyFromDictionary(proxy);
            }
            return result;
        }
    }

    // No PAC, decide which proxy we're looking for based on the query
    // try the protocol-specific proxy
    const QString protocol = query.protocolTag();
    QNetworkProxy protocolSpecificProxy;
    if (protocol.compare("http"_L1, Qt::CaseInsensitive) == 0) {
        protocolSpecificProxy =
            proxyFromDictionary(dict, QNetworkProxy::HttpProxy,
                                kCFNetworkProxiesHTTPEnable,
                                kCFNetworkProxiesHTTPProxy,
                                kCFNetworkProxiesHTTPPort);
    }


#ifdef Q_OS_IOS
    if (protocolSpecificProxy.type() != QNetworkProxy::DefaultProxy
        && protocolSpecificProxy.type() != QNetworkProxy::DefaultProxy) {
        // HTTP proxy is enabled (on iOS there is no separate HTTPS, though
        // 'dict' contains deeply buried entries which are the same as HTTP.
        result << protocolSpecificProxy;
    }

    // TODO: check query.queryType()? It's possible, the exclude list
    // did exclude it but above we added a proxy because HTTP proxy
    // is found. We'll deal with such a situation later, since now NMI.
    const auto proxiesForUrl = proxiesForQueryUrl(dict, query.url());
    for (const auto &proxy : proxiesForUrl) {
        if (!result.contains(proxy))
            result << proxy;
    }
#else
    bool isHttps = false;
    if (protocol.compare("ftp"_L1, Qt::CaseInsensitive) == 0) {
        protocolSpecificProxy =
            proxyFromDictionary(dict, QNetworkProxy::FtpCachingProxy,
                                kCFNetworkProxiesFTPEnable,
                                kCFNetworkProxiesFTPProxy,
                                kCFNetworkProxiesFTPPort);
    } else if (protocol.compare("https"_L1, Qt::CaseInsensitive) == 0) {
        isHttps = true;
        protocolSpecificProxy =
            proxyFromDictionary(dict, QNetworkProxy::HttpProxy,
                                kCFNetworkProxiesHTTPSEnable,
                                kCFNetworkProxiesHTTPSProxy,
                                kCFNetworkProxiesHTTPSPort);
    }

    if (protocolSpecificProxy.type() != QNetworkProxy::DefaultProxy)
        result << protocolSpecificProxy;

    // let's add SOCKSv5 if present too
    QNetworkProxy socks5 = proxyFromDictionary(dict, QNetworkProxy::Socks5Proxy,
                                               kCFNetworkProxiesSOCKSEnable,
                                               kCFNetworkProxiesSOCKSProxy,
                                               kCFNetworkProxiesSOCKSPort);
    if (socks5.type() != QNetworkProxy::DefaultProxy)
        result << socks5;

    // let's add the HTTPS proxy if present (and if we haven't added
    // yet)
    if (!isHttps) {
        QNetworkProxy https;
        https = proxyFromDictionary(dict, QNetworkProxy::HttpProxy,
                                    kCFNetworkProxiesHTTPSEnable,
                                    kCFNetworkProxiesHTTPSProxy,
                                    kCFNetworkProxiesHTTPSPort);


        if (https.type() != QNetworkProxy::DefaultProxy && https != protocolSpecificProxy)
            result << https;
    }
#endif // !Q_OS_IOS

    return result;
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> result = macQueryInternal(query);
    if (result.isEmpty())
        result << QNetworkProxy::NoProxy;

    return result;
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKPROXY
