/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QVariant>
#include <QStringList>
#include <QDebug>
#include <QWriteLocker>
#include <QNetworkProxyFactory>
#include <QNetworkProxy>
#include <gconf/gconf-value.h>
#include <gconf/gconf-client.h>
#include "proxyconf.h"

#define CONF_PROXY "/system/proxy"
#define HTTP_PROXY "/system/http_proxy"


namespace Maemo {

static QString convertKey(const char *key)
{
    return QString::fromUtf8(key);
}

static QVariant convertValue(GConfValue *src)
{
    if (!src) {
        return QVariant();
    } else {
        switch (src->type) {
        case GCONF_VALUE_INVALID:
            return QVariant(QVariant::Invalid);
        case GCONF_VALUE_BOOL:
            return QVariant((bool)gconf_value_get_bool(src));
        case GCONF_VALUE_INT:
            return QVariant(gconf_value_get_int(src));
        case GCONF_VALUE_FLOAT:
            return QVariant(gconf_value_get_float(src));
        case GCONF_VALUE_STRING:
            return QVariant(QString::fromUtf8(gconf_value_get_string(src)));
        case GCONF_VALUE_LIST:
            switch (gconf_value_get_list_type(src)) {
            case GCONF_VALUE_STRING:
                {
                    QStringList result;
                    for (GSList *elts = gconf_value_get_list(src); elts; elts = elts->next)
                        result.append(QString::fromUtf8(gconf_value_get_string((GConfValue *)elts->data)));
                    return QVariant(result);
                }
            default:
                {
                    QList<QVariant> result;
                    for (GSList *elts = gconf_value_get_list(src); elts; elts = elts->next)
                        result.append(convertValue((GConfValue *)elts->data));
                    return QVariant(result);
                }
            }
        case GCONF_VALUE_SCHEMA:
        default:
            return QVariant();
        }
    }
}


/* Fast version of GConfItem, allows reading subtree at a time */
class GConfItemFast {
public:
  GConfItemFast(const QString &k) : key(k) {}
  QHash<QString,QVariant> getEntries() const;

private:
  QString key;
};

#define withClient(c) for (GConfClient *c = gconf_client_get_default(); c; c=0)


QHash<QString,QVariant> GConfItemFast::getEntries() const
{
    QHash<QString,QVariant> children;

    withClient(client) {
        QByteArray k = key.toUtf8();
        GSList *entries = gconf_client_all_entries(client, k.data(), NULL);
        for (GSList *e = entries; e; e = e->next) {
	    char *key_name = strrchr(((GConfEntry *)e->data)->key, '/');
	    if (!key_name)
	        key_name = ((GConfEntry *)e->data)->key;
	    else
	        key_name++;
	    QString key(convertKey(key_name));
	    QVariant value = convertValue(((GConfEntry *)e->data)->value);
	    gconf_entry_unref((GConfEntry *)e->data);
	    //qDebug()<<"key="<<key<<"value="<<value;
	    children.insert(key, value);
        }
        g_slist_free (entries);
    }

    return children;
}



class NetworkProxyFactory : QNetworkProxyFactory
{
    ProxyConf proxy_conf;
    bool proxy_data_read;

public:
    NetworkProxyFactory() : proxy_data_read(false) {  }
    QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query = QNetworkProxyQuery());
};


QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery &query)
{
    if (proxy_data_read == false) {
        proxy_data_read = true;
        proxy_conf.readProxyData();
    }

    QList<QNetworkProxy> result = proxy_conf.flush(query);
    if (result.isEmpty())
        result << QNetworkProxy::NoProxy;

    return result;
}


class ProxyConfPrivate {
private:
    // proxy values from gconf
    QString mode;
    bool use_http_host;
    QString autoconfig_url;
    QString http_proxy;
    quint16 http_port;
    QList<QVariant> ignore_hosts;
    QString secure_host;
    quint16 secure_port;
    QString ftp_host;
    quint16 ftp_port;
    QString socks_host;
    quint16 socks_port;
    QString rtsp_host;
    quint16 rtsp_port;

    bool isHostExcluded(const QString &host);

public:
    QString prefix;
    QString http_prefix;

    void readProxyData();
    QList<QNetworkProxy> flush(const QNetworkProxyQuery &query);
};


static QHash<QString,QVariant> getValues(const QString& prefix)
{
    GConfItemFast item(prefix);
    return item.getEntries();
}

static QHash<QString,QVariant> getHttpValues(const QString& prefix)
{
    GConfItemFast item(prefix);
    return item.getEntries();
}

#define GET(var, type)				\
  do {						\
    QVariant v = values.value(#var);		\
    if (v.isValid())				\
      var = v.to##type ();			\
  } while(0)

#define GET_HTTP(var, name, type)		\
  do {						\
    QVariant v = httpValues.value(#name);	\
    if (v.isValid())				\
      var = v.to##type ();			\
  } while(0)


void ProxyConfPrivate::readProxyData()
{
    QHash<QString,QVariant> values = getValues(prefix);
    QHash<QString,QVariant> httpValues = getHttpValues(http_prefix);

    //qDebug()<<"values="<<values;

    /* Read the proxy settings from /system/proxy* */
    GET_HTTP(http_proxy, host, String);
    GET_HTTP(http_port, port, Int);
    GET_HTTP(ignore_hosts, ignore_hosts, List);

    GET(mode, String);
    GET(autoconfig_url, String);
    GET(secure_host, String);
    GET(secure_port, Int);
    GET(ftp_host, String);
    GET(ftp_port, Int);
    GET(socks_host, String);
    GET(socks_port, Int);
    GET(rtsp_host, String);
    GET(rtsp_port, Int);

    if (http_proxy.isEmpty())
        use_http_host = false;
    else
        use_http_host = true;
}


bool ProxyConfPrivate::isHostExcluded(const QString &host)
{
    if (host.isEmpty())
        return true;

    if (ignore_hosts.isEmpty())
        return false;

    QHostAddress ipAddress;
    bool isIpAddress = ipAddress.setAddress(host);

    foreach (QVariant h, ignore_hosts) {
        QString entry = h.toString();
        if (isIpAddress && ipAddress.isInSubnet(QHostAddress::parseSubnet(entry))) {
            return true;  // excluded
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


QList<QNetworkProxy> ProxyConfPrivate::flush(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> result;

#if 0
    qDebug()<<"http_proxy" << http_proxy;
    qDebug()<<"http_port" << http_port;
    qDebug()<<"ignore_hosts" << ignore_hosts;
    qDebug()<<"use_http_host" << use_http_host;
    qDebug()<<"mode" << mode;
    qDebug()<<"autoconfig_url" << autoconfig_url;
    qDebug()<<"secure_host" << secure_host;
    qDebug()<<"secure_port" << secure_port;
    qDebug()<<"ftp_host" << ftp_host;
    qDebug()<<"ftp_port" << ftp_port;
    qDebug()<<"socks_host" << socks_host;
    qDebug()<<"socks_port" << socks_port;
    qDebug()<<"rtsp_host" << rtsp_host;
    qDebug()<<"rtsp_port" << rtsp_port;
#endif

    if (isHostExcluded(query.peerHostName()))
        return result;          // no proxy for this host

    if (mode == QLatin1String("AUTO")) {
        // TODO: pac currently not supported, fix me
        return result;
    }

    if (mode == QLatin1String("MANUAL")) {
        bool isHttps = false;
	QString protocol = query.protocolTag().toLower();

	// try the protocol-specific proxy
	QNetworkProxy protocolSpecificProxy;

	if (protocol == QLatin1String("ftp")) {
	    if (!ftp_host.isEmpty()) {
	        protocolSpecificProxy.setType(QNetworkProxy::FtpCachingProxy);
		protocolSpecificProxy.setHostName(ftp_host);
		protocolSpecificProxy.setPort(ftp_port);
	    }
	} else if (protocol == QLatin1String("http")) {
	    if (!http_proxy.isEmpty()) {
	        protocolSpecificProxy.setType(QNetworkProxy::HttpProxy);
		protocolSpecificProxy.setHostName(http_proxy);
		protocolSpecificProxy.setPort(http_port);
	    }
	} else if (protocol == QLatin1String("https")) {
	    isHttps = true;
	    if (!secure_host.isEmpty()) {
	        protocolSpecificProxy.setType(QNetworkProxy::HttpProxy);
		protocolSpecificProxy.setHostName(secure_host);
		protocolSpecificProxy.setPort(secure_port);
	    }
	}

	if (protocolSpecificProxy.type() != QNetworkProxy::DefaultProxy)
	    result << protocolSpecificProxy;


        if (!socks_host.isEmpty()) {
	    QNetworkProxy proxy;
	    proxy.setType(QNetworkProxy::Socks5Proxy);
	    proxy.setHostName(socks_host);
	    proxy.setPort(socks_port);
	    result << proxy;
	}


	// Add the HTTPS proxy if present (and if we haven't added yet)
	if (!isHttps) {
	    QNetworkProxy https;
	    if (!secure_host.isEmpty()) {
	        https.setType(QNetworkProxy::HttpProxy);
		https.setHostName(secure_host);
		https.setPort(secure_port);
	    }

	    if (https.type() != QNetworkProxy::DefaultProxy &&
		https != protocolSpecificProxy)
	        result << https;
	}
    }

    return result;
}


ProxyConf::ProxyConf()
    : d_ptr(new ProxyConfPrivate)
{
    g_type_init();
    d_ptr->prefix = CONF_PROXY;
    d_ptr->http_prefix = HTTP_PROXY;
}

ProxyConf::~ProxyConf()
{
    delete d_ptr;
}

void ProxyConf::readProxyData()
{
    d_ptr->readProxyData();
}

QList<QNetworkProxy> ProxyConf::flush(const QNetworkProxyQuery &query)
{
    return d_ptr->flush(query);
}


static int refcount = 0;
static QReadWriteLock lock;

void ProxyConf::update()
{
    QWriteLocker locker(&lock);
    NetworkProxyFactory *factory = new NetworkProxyFactory();
    QNetworkProxyFactory::setApplicationProxyFactory((QNetworkProxyFactory*)factory);
    refcount++;
}


void ProxyConf::clear(void)
{
    QWriteLocker locker(&lock);
    refcount--;
    if (refcount == 0)
        QNetworkProxyFactory::setApplicationProxyFactory(NULL);

    if (refcount<0)
        refcount = 0;
}


} // namespace Maemo
