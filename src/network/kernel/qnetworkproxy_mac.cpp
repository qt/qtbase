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

#include <CFNetwork/CFNetwork.h>
#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>

#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/qendian.h>
#include <QtCore/qstringlist.h>
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
 * The matching configuration can be obtained by calling SCDynamicStoreCopyProxies
 * (from <SystemConfiguration/SCDynamicStoreCopySpecific.h>). See
 * Apple's documentation:
 *
 * http://developer.apple.com/DOCUMENTATION/Networking/Reference/SysConfig/SCDynamicStoreCopySpecific/CompositePage.html#//apple_ref/c/func/SCDynamicStoreCopyProxies
 *
 */

QT_BEGIN_NAMESPACE

static bool isHostExcluded(CFDictionaryRef dict, const QString &host)
{
    if (host.isEmpty())
        return true;

    bool isSimple = !host.contains(QLatin1Char('.')) && !host.contains(QLatin1Char(':'));
    CFNumberRef excludeSimples;
    if (isSimple &&
        (excludeSimples = (CFNumberRef)CFDictionaryGetValue(dict, kSCPropNetProxiesExcludeSimpleHostnames))) {
        int enabled;
        if (CFNumberGetValue(excludeSimples, kCFNumberIntType, &enabled) && enabled)
            return true;
    }

    QHostAddress ipAddress;
    bool isIpAddress = ipAddress.setAddress(host);

    // not a simple host name
    // does it match the list of exclusions?
    CFArrayRef exclusionList = (CFArrayRef)CFDictionaryGetValue(dict, kSCPropNetProxiesExceptionsList);
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
            QRegExp rx(entry, Qt::CaseInsensitive, QRegExp::Wildcard);
            if (rx.exactMatch(host))
                return true;
        }
    }

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
} // anon namespace

QList<QNetworkProxy> macQueryInternal(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> result;

    // obtain a dictionary to the proxy settings:
    CFDictionaryRef dict = SCDynamicStoreCopyProxies(NULL);
    if (!dict) {
        qWarning("QNetworkProxyFactory::systemProxyForQuery: SCDynamicStoreCopyProxies returned NULL");
        return result;          // failed
    }

    if (isHostExcluded(dict, query.peerHostName())) {
        CFRelease(dict);
        return result;          // no proxy for this host
    }

    // is there a PAC enabled? If so, use it first.
    CFNumberRef pacEnabled;
    if ((pacEnabled = (CFNumberRef)CFDictionaryGetValue(dict, kSCPropNetProxiesProxyAutoConfigEnable))) {
        int enabled;
        if (CFNumberGetValue(pacEnabled, kCFNumberIntType, &enabled) && enabled) {
            // PAC is enabled
            // kSCPropNetProxiesProxyAutoConfigURLString returns the URL string
            // as entered in the system proxy configuration dialog
            CFStringRef pacLocationSetting = (CFStringRef)CFDictionaryGetValue(dict, kSCPropNetProxiesProxyAutoConfigURLString);
            QCFType<CFStringRef> cfPacLocation = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault, pacLocationSetting, NULL, NULL,
                kCFStringEncodingUTF8);

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

    // no PAC, decide which proxy we're looking for based on the query
    bool isHttps = false;
    QString protocol = query.protocolTag().toLower();

    // try the protocol-specific proxy
    QNetworkProxy protocolSpecificProxy;
    if (protocol == QLatin1String("ftp")) {
        protocolSpecificProxy =
            proxyFromDictionary(dict, QNetworkProxy::FtpCachingProxy,
                                kSCPropNetProxiesFTPEnable,
                                kSCPropNetProxiesFTPProxy,
                                kSCPropNetProxiesFTPPort);
    } else if (protocol == QLatin1String("http")) {
        protocolSpecificProxy =
            proxyFromDictionary(dict, QNetworkProxy::HttpProxy,
                                kSCPropNetProxiesHTTPEnable,
                                kSCPropNetProxiesHTTPProxy,
                                kSCPropNetProxiesHTTPPort);
    } else if (protocol == QLatin1String("https")) {
        isHttps = true;
        protocolSpecificProxy =
            proxyFromDictionary(dict, QNetworkProxy::HttpProxy,
                                kSCPropNetProxiesHTTPSEnable,
                                kSCPropNetProxiesHTTPSProxy,
                                kSCPropNetProxiesHTTPSPort);
    }
    if (protocolSpecificProxy.type() != QNetworkProxy::DefaultProxy)
        result << protocolSpecificProxy;

    // let's add SOCKSv5 if present too
    QNetworkProxy socks5 = proxyFromDictionary(dict, QNetworkProxy::Socks5Proxy,
                                               kSCPropNetProxiesSOCKSEnable,
                                               kSCPropNetProxiesSOCKSProxy,
                                               kSCPropNetProxiesSOCKSPort);
    if (socks5.type() != QNetworkProxy::DefaultProxy)
        result << socks5;

    // let's add the HTTPS proxy if present (and if we haven't added
    // yet)
    if (!isHttps) {
        QNetworkProxy https = proxyFromDictionary(dict, QNetworkProxy::HttpProxy,
                                                  kSCPropNetProxiesHTTPSEnable,
                                                  kSCPropNetProxiesHTTPSProxy,
                                                  kSCPropNetProxiesHTTPSPort);
        if (https.type() != QNetworkProxy::DefaultProxy && https != protocolSpecificProxy)
            result << https;
    }

    CFRelease(dict);
    return result;
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> result = macQueryInternal(query);
    if (result.isEmpty())
        result << QNetworkProxy::NoProxy;

    return result;
}

#endif

QT_END_NAMESPACE
