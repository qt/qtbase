// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetworkproxy.h"

#ifndef QT_NO_NETWORKPROXY

#include <qmutex.h>
#include <qstringlist.h>
#include <qregularexpression.h>
#include <qurl.h>
#include <qnetworkinterface.h>
#include <qdebug.h>
#include <qvarlengtharray.h>
#include <qhash.h>

#include <string.h>
#include <qt_windows.h>
#include <lmcons.h>
#include <winhttp.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static bool currentProcessIsService()
{
    wchar_t userName[UNLEN + 1] = L"";
    DWORD size = UNLEN;
    if (GetUserNameW(userName, &size)) {
        SID_NAME_USE type = SidTypeUser;
        DWORD sidSize = 0;
        DWORD domainSize = 0;
        // first call is to get the correct size
        bool bRet = LookupAccountNameW(NULL, userName, NULL, &sidSize, NULL, &domainSize, &type);
        if (bRet == FALSE && ERROR_INSUFFICIENT_BUFFER != GetLastError())
            return false;
        QVarLengthArray<BYTE, 68> buff(sidSize);
        QVarLengthArray<wchar_t, MAX_PATH> domainName(domainSize);
        // second call to LookupAccountNameW actually gets the SID
        // both the pointer to the buffer and the pointer to the domain name should not be NULL
        if (LookupAccountNameW(NULL, userName, buff.data(), &sidSize, domainName.data(), &domainSize, &type))
            return type != SidTypeUser; //returns true if the current user is not a user
    }
    return false;
}

static QStringList splitSpaceSemicolon(const QString &source)
{
    QStringList list;
    qsizetype start = 0;
    qsizetype end;
    while (true) {
        qsizetype space = source.indexOf(u' ', start);
        qsizetype semicolon = source.indexOf(u';', start);
        end = space;
        if (semicolon != -1 && (end == -1 || semicolon < end))
            end = semicolon;

        if (end == -1) {
            if (start != source.length())
                list.append(source.mid(start));
            return list;
        }
        if (start != end)
            list.append(source.mid(start, end - start));
        start = end + 1;
    }
    return list;
}

static bool isBypassed(const QString &host, const QStringList &bypassList)
{
    if (host.isEmpty())
        return false;

    bool isSimple = !host.contains(u'.') && !host.contains(u':');

    QHostAddress ipAddress;
    bool isIpAddress = ipAddress.setAddress(host);

    // always exclude loopback
    if (isIpAddress && ipAddress.isLoopback())
        return true;

    // does it match the list of exclusions?
    for (const QString &entry : bypassList) {
        if (entry == "<local>"_L1) {
            if (isSimple)
                return true;
            if (isIpAddress) {
                //exclude all local subnets
                const auto ifaces = QNetworkInterface::allInterfaces();
                for (const QNetworkInterface &iface : ifaces) {
                    const auto netaddrs = iface.addressEntries();
                    for (const QNetworkAddressEntry &netaddr : netaddrs) {
                        if (ipAddress.isInSubnet(netaddr.ip(), netaddr.prefixLength())) {
                            return true;
                        }
                    }
                }
            }
        }
        if (isIpAddress && ipAddress.isInSubnet(QHostAddress::parseSubnet(entry))) {
            return true;        // excluded
        } else {
            // do wildcard matching
            auto rx = QRegularExpression::fromWildcard(entry, Qt::CaseInsensitive);
            if (rx.match(host).hasMatch())
                return true;
        }
    }

    // host was not excluded
    return false;
}

static QList<QNetworkProxy> filterProxyListByCapabilities(const QList<QNetworkProxy> &proxyList, const QNetworkProxyQuery &query)
{
    QNetworkProxy::Capabilities requiredCaps;
    switch (query.queryType()) {
    case QNetworkProxyQuery::TcpSocket:
        requiredCaps = QNetworkProxy::TunnelingCapability;
        break;
    case QNetworkProxyQuery::UdpSocket:
        requiredCaps = QNetworkProxy::UdpTunnelingCapability;
        break;
    case QNetworkProxyQuery::SctpSocket:
        requiredCaps = QNetworkProxy::SctpTunnelingCapability;
        break;
    case QNetworkProxyQuery::TcpServer:
        requiredCaps = QNetworkProxy::ListeningCapability;
        break;
    case QNetworkProxyQuery::SctpServer:
        requiredCaps = QNetworkProxy::SctpListeningCapability;
        break;
    default:
        return proxyList;
        break;
    }
    QList<QNetworkProxy> result;
    for (const QNetworkProxy &proxy : proxyList) {
        if (proxy.capabilities() & requiredCaps)
            result.append(proxy);
    }
    return result;
}

static QList<QNetworkProxy> removeDuplicateProxies(const QList<QNetworkProxy> &proxyList)
{
    QList<QNetworkProxy> result;
    for (const QNetworkProxy &proxy : proxyList) {
         bool append = true;
         for (int i=0; i < result.count(); i++) {
             if (proxy.hostName() == result.at(i).hostName()
                 && proxy.port() == result.at(i).port()) {
                     append = false;
                     // HttpProxy trumps FtpCachingProxy or HttpCachingProxy on the same host/port
                     if (proxy.type() == QNetworkProxy::HttpProxy)
                         result[i] = proxy;
             }
         }
         if (append)
             result.append(proxy);
     }
     return result;
}

static QList<QNetworkProxy> parseServerList(const QNetworkProxyQuery &query, const QStringList &proxyList)
{
    // Reference documentation from Microsoft:
    // http://msdn.microsoft.com/en-us/library/aa383912(VS.85).aspx
    //
    // According to the website, the proxy server list is
    // one or more of the space- or semicolon-separated strings in the format:
    //   ([<scheme>=][<scheme>"://"]<server>[":"<port>])
    // The first scheme relates to the protocol tag
    // The second scheme, if present, overrides the proxy type

    QList<QNetworkProxy> result;
    QHash<QString, QNetworkProxy> taggedProxies;
    const QString requiredTag = query.protocolTag();
    // windows tags are only for clients
    bool checkTags = !requiredTag.isEmpty()
            && query.queryType() != QNetworkProxyQuery::TcpServer
            && query.queryType() != QNetworkProxyQuery::SctpServer;
    for (const QString &entry : proxyList) {
        qsizetype server = 0;

        QNetworkProxy::ProxyType proxyType = QNetworkProxy::HttpProxy;
        quint16 port = 8080;

        qsizetype pos = entry.indexOf(u'=');
        QStringView scheme;
        QStringView protocolTag;
        if (pos != -1) {
            scheme = protocolTag = QStringView{entry}.left(pos);
            server = pos + 1;
        }
        pos = entry.indexOf("://"_L1, server);
        if (pos != -1) {
            scheme = QStringView{entry}.mid(server, pos - server);
            server = pos + 3;
        }

        if (!scheme.isEmpty()) {
            if (scheme == "http"_L1 || scheme == "https"_L1) {
                // no-op
                // defaults are above
            } else if (scheme == "socks"_L1 || scheme == "socks5"_L1) {
                proxyType = QNetworkProxy::Socks5Proxy;
                port = 1080;
            } else if (scheme == "ftp"_L1) {
                proxyType = QNetworkProxy::FtpCachingProxy;
                port = 2121;
            } else {
                // unknown proxy type
                continue;
            }
        }

        pos = entry.indexOf(u':', server);
        if (pos != -1) {
            bool ok;
            uint value = QStringView{entry}.mid(pos + 1).toUInt(&ok);
            if (!ok || value > 65535)
                continue;       // invalid port number

            port = value;
        } else {
            pos = entry.length();
        }

        result << QNetworkProxy(proxyType, entry.mid(server, pos - server), port);
        if (!protocolTag.isEmpty())
            taggedProxies.insert(protocolTag.toString(), result.constLast());
    }

    if (checkTags && taggedProxies.contains(requiredTag)) {
        if (query.queryType() == QNetworkProxyQuery::UrlRequest) {
            result.clear();
            result.append(taggedProxies.value(requiredTag));
            return result;
        } else {
            result.prepend(taggedProxies.value(requiredTag));
        }
    }
    if (!checkTags || requiredTag != "http"_L1) {
        // if there are different http proxies for http and https, prefer the https one (more likely to be capable of CONNECT)
        QNetworkProxy httpProxy = taggedProxies.value("http"_L1);
        QNetworkProxy httpsProxy = taggedProxies.value("http"_L1);
        if (httpProxy != httpsProxy && httpProxy.type() == QNetworkProxy::HttpProxy && httpsProxy.type() == QNetworkProxy::HttpProxy) {
            for (int i = 0; i < result.count(); i++) {
                if (httpProxy == result.at(i))
                    result[i].setType(QNetworkProxy::HttpCachingProxy);
            }
        }
    }
    result = filterProxyListByCapabilities(result, query);
    return removeDuplicateProxies(result);
}

namespace {
class QRegistryWatcher {
    Q_DISABLE_COPY_MOVE(QRegistryWatcher)
public:
    QRegistryWatcher() = default;

    void addLocation(HKEY hive, const QString& path)
    {
        HKEY openedKey;
        if (RegOpenKeyEx(hive, reinterpret_cast<const wchar_t*>(path.utf16()), 0, KEY_READ, &openedKey) != ERROR_SUCCESS)
            return;

        const DWORD filter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES |
                REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_SECURITY;

        // Watch the registry key for a change of value.
        HANDLE handle = CreateEvent(NULL, true, false, NULL);
        if (RegNotifyChangeKeyValue(openedKey, true, filter, handle, true) != ERROR_SUCCESS) {
            CloseHandle(handle);
            return;
        }
        m_watchEvents.append(handle);
        m_registryHandles.append(openedKey);
    }

    bool hasChanged() const {
        return !isEmpty() &&
               WaitForMultipleObjects(m_watchEvents.size(), m_watchEvents.data(), false, 0) < WAIT_OBJECT_0 + m_watchEvents.size();
    }

    bool isEmpty() const {
        return m_watchEvents.isEmpty();
    }

    void clear() {
        for (HANDLE event : std::as_const(m_watchEvents))
            CloseHandle(event);
        for (HKEY key : std::as_const(m_registryHandles))
            RegCloseKey(key);

        m_watchEvents.clear();
        m_registryHandles.clear();
    }

    ~QRegistryWatcher() {
        clear();
    }

private:
    QList<HANDLE> m_watchEvents;
    QList<HKEY> m_registryHandles;
};
} // namespace

class QWindowsSystemProxy
{
    Q_DISABLE_COPY_MOVE(QWindowsSystemProxy)
public:
    QWindowsSystemProxy();
    ~QWindowsSystemProxy();
    void init();
    void reset();

    QMutex mutex;

    HINTERNET hHttpSession;
    WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions;

    QString autoConfigUrl;
    QStringList proxyServerList;
    QStringList proxyBypass;
    QList<QNetworkProxy> defaultResult;
    QRegistryWatcher proxySettingsWatcher;
    bool initialized;
    bool functional;
    bool isAutoConfig;
};

Q_GLOBAL_STATIC(QWindowsSystemProxy, systemProxy)

QWindowsSystemProxy::QWindowsSystemProxy()
    : hHttpSession(0), initialized(false), functional(false), isAutoConfig(false)
{
    defaultResult << QNetworkProxy::NoProxy;
}

QWindowsSystemProxy::~QWindowsSystemProxy()
{
    if (hHttpSession)
        WinHttpCloseHandle(hHttpSession);
}

void QWindowsSystemProxy::reset()
{
    autoConfigUrl.clear();
    proxyServerList.clear();
    proxyBypass.clear();
    defaultResult.clear();
    defaultResult << QNetworkProxy::NoProxy;
    functional = false;
    isAutoConfig = false;
}

void QWindowsSystemProxy::init()
{
    bool proxySettingsChanged = false;
    proxySettingsChanged = proxySettingsWatcher.hasChanged();

    if (initialized && !proxySettingsChanged)
        return;
    initialized = true;

    reset();

    proxySettingsWatcher.clear(); // needs reset to trigger a new detection
    proxySettingsWatcher.addLocation(HKEY_CURRENT_USER,  QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"));
    proxySettingsWatcher.addLocation(HKEY_LOCAL_MACHINE, QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"));
    proxySettingsWatcher.addLocation(HKEY_LOCAL_MACHINE, QStringLiteral("Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"));

    // Try to obtain the Internet Explorer configuration.
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ieProxyConfig;
    const bool hasIEConfig = WinHttpGetIEProxyConfigForCurrentUser(&ieProxyConfig);
    if (hasIEConfig) {
        if (ieProxyConfig.lpszAutoConfigUrl) {
            autoConfigUrl = QString::fromWCharArray(ieProxyConfig.lpszAutoConfigUrl);
            GlobalFree(ieProxyConfig.lpszAutoConfigUrl);
        }
        if (ieProxyConfig.lpszProxy) {
            // http://msdn.microsoft.com/en-us/library/aa384250%28VS.85%29.aspx speaks only about a "proxy URL",
            // not multiple URLs. However we tested this and it can return multiple URLs. So we use splitSpaceSemicolon
            // on it.
            proxyServerList = splitSpaceSemicolon(QString::fromWCharArray(ieProxyConfig.lpszProxy));
            GlobalFree(ieProxyConfig.lpszProxy);
        }
        if (ieProxyConfig.lpszProxyBypass) {
            proxyBypass = splitSpaceSemicolon(QString::fromWCharArray(ieProxyConfig.lpszProxyBypass));
            GlobalFree(ieProxyConfig.lpszProxyBypass);
        }
    }

    if (!hasIEConfig ||
        (currentProcessIsService() && proxyServerList.isEmpty() && proxyBypass.isEmpty())) {
        // no user configuration
        // attempt to get the default configuration instead
        // that config will serve as default if WPAD fails
        WINHTTP_PROXY_INFO proxyInfo;
        if (WinHttpGetDefaultProxyConfiguration(&proxyInfo) &&
            proxyInfo.dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY) {
            // we got information from the registry
            // overwrite the IE configuration, if any

            proxyBypass = splitSpaceSemicolon(QString::fromWCharArray(proxyInfo.lpszProxyBypass));
            proxyServerList = splitSpaceSemicolon(QString::fromWCharArray(proxyInfo.lpszProxy));
        }

        if (proxyInfo.lpszProxy)
            GlobalFree(proxyInfo.lpszProxy);
        if (proxyInfo.lpszProxyBypass)
            GlobalFree(proxyInfo.lpszProxyBypass);
    }

    hHttpSession = NULL;
    if (ieProxyConfig.fAutoDetect || !autoConfigUrl.isEmpty()) {
        // open the handle and obtain the options
        hHttpSession = WinHttpOpen(L"Qt System Proxy access/1.0",
                                   WINHTTP_ACCESS_TYPE_NO_PROXY,
                                   WINHTTP_NO_PROXY_NAME,
                                   WINHTTP_NO_PROXY_BYPASS,
                                   0);
        if (!hHttpSession)
            return;

        isAutoConfig = true;
        memset(&autoProxyOptions, 0, sizeof autoProxyOptions);
        autoProxyOptions.fAutoLogonIfChallenged = false;
        //Although it is possible to specify dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT | WINHTTP_AUTOPROXY_CONFIG_URL
        //this has poor performance (WPAD is attempted for every url, taking 2.5 seconds per interface,
        //before the configured pac file is used)
        if (ieProxyConfig.fAutoDetect) {
            autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
            autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP |
                                                 WINHTTP_AUTO_DETECT_TYPE_DNS_A;
        } else {
            autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
            autoProxyOptions.lpszAutoConfigUrl = reinterpret_cast<LPCWSTR>(autoConfigUrl.utf16());
        }
    }

    functional = isAutoConfig || !proxyServerList.isEmpty();
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QWindowsSystemProxy *sp = systemProxy();
    if (!sp)
        return QList<QNetworkProxy>() << QNetworkProxy();

    QMutexLocker locker(&sp->mutex);
    sp->init();
    if (!sp->functional)
        return sp->defaultResult;

    if (sp->isAutoConfig) {
        WINHTTP_PROXY_INFO proxyInfo;

        // try to get the proxy config for the URL
        QUrl url = query.url();
        // url could be empty, e.g. from QNetworkProxy::applicationProxy(), that's fine,
        // we'll still ask for the proxy.
        // But for a file url, we know we don't need one.
        if (url.scheme() == "file"_L1 || url.scheme() == "qrc"_L1)
            return sp->defaultResult;
        if (query.queryType() != QNetworkProxyQuery::UrlRequest) {
            // change the scheme to https, maybe it'll work
            url.setScheme("https"_L1);
        }

        QString urlQueryString = url.toString();
        if (urlQueryString.size() > 2083) {
            // calls to WinHttpGetProxyForUrl with urls longer than 2083 characters
            // fail with error code ERROR_INVALID_PARAMETER(87), so we truncate it
            qWarning("Proxy query URL too long for windows API, try with truncated URL");
            urlQueryString = url.toString().left(2083);
        }

        bool getProxySucceeded = WinHttpGetProxyForUrl(sp->hHttpSession,
                                                reinterpret_cast<LPCWSTR>(urlQueryString.utf16()),
                                                &sp->autoProxyOptions,
                                                &proxyInfo);
        DWORD getProxyError = GetLastError();

        if (!getProxySucceeded
            && (ERROR_WINHTTP_AUTODETECTION_FAILED == getProxyError)) {
            // WPAD failed
            if (sp->autoConfigUrl.isEmpty()) {
                //No config file could be retrieved on the network.
                //Don't search for it next time again.
                sp->isAutoConfig = false;
            } else {
                //pac file URL is specified as well, try using that
                sp->autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
                sp->autoProxyOptions.lpszAutoConfigUrl =
                    reinterpret_cast<LPCWSTR>(sp->autoConfigUrl.utf16());
                getProxySucceeded = WinHttpGetProxyForUrl(sp->hHttpSession,
                                                reinterpret_cast<LPCWSTR>(urlQueryString.utf16()),
                                                &sp->autoProxyOptions,
                                                &proxyInfo);
                getProxyError = GetLastError();
            }
        }

        if (!getProxySucceeded
            && (ERROR_WINHTTP_LOGIN_FAILURE == getProxyError)) {
            // We first tried without AutoLogon, because this might prevent caching the result.
            // But now we've to enable it (http://msdn.microsoft.com/en-us/library/aa383153%28v=VS.85%29.aspx)
            sp->autoProxyOptions.fAutoLogonIfChallenged = TRUE;
            getProxySucceeded = WinHttpGetProxyForUrl(sp->hHttpSession,
                                                reinterpret_cast<LPCWSTR>(urlQueryString.utf16()),
                                                &sp->autoProxyOptions,
                                                &proxyInfo);
            getProxyError = GetLastError();
        }

        if (!getProxySucceeded
            && (ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT == getProxyError)) {
            // PAC file url is not connectable, or server returned error (e.g. http 404)
            //Don't search for it next time again.
            sp->isAutoConfig = false;
        }

        if (getProxySucceeded) {
            // yes, we got a config for this URL
            QString proxyBypass = QString::fromWCharArray(proxyInfo.lpszProxyBypass);
            QStringList proxyServerList = splitSpaceSemicolon(QString::fromWCharArray(proxyInfo.lpszProxy));
            if (proxyInfo.lpszProxy)
                GlobalFree(proxyInfo.lpszProxy);
            if (proxyInfo.lpszProxyBypass)
                GlobalFree(proxyInfo.lpszProxyBypass);

            if (proxyInfo.dwAccessType == WINHTTP_ACCESS_TYPE_NO_PROXY)
                return sp->defaultResult; //i.e. the PAC file result was "DIRECT"
            if (isBypassed(query.peerHostName(), splitSpaceSemicolon(proxyBypass)))
                return sp->defaultResult;
            return parseServerList(query, proxyServerList);
        }

        // GetProxyForUrl failed, fall back to static configuration
    }

    // static configuration
    if (isBypassed(query.peerHostName(), sp->proxyBypass))
        return sp->defaultResult;

    QList<QNetworkProxy> result = parseServerList(query, sp->proxyServerList);
    // In some cases, this was empty. See SF task 00062670
    if (result.isEmpty())
        return sp->defaultResult;

    return result;
}

QT_END_NAMESPACE

#endif
