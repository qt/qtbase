/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qnetworkproxy.h>

class tst_QNetworkProxyFactory : public QObject {
    Q_OBJECT
private slots:
    void systemProxyForQuery() const;

private:
    QString formatProxyName(const QNetworkProxy & proxy) const;
};

QString tst_QNetworkProxyFactory::formatProxyName(const QNetworkProxy & proxy) const
{
    QString proxyName;
    if (!proxy.user().isNull())
        proxyName.append("%1:%2@").arg(proxy.user(), proxy.password());
    proxyName.append(QString("%1:%2").arg(proxy.hostName()).arg(proxy.port()));
    proxyName.append(QString(" (type=%1, capabilities=%2)").arg(proxy.type()).arg(proxy.capabilities()));

    return proxyName;
}

void tst_QNetworkProxyFactory::systemProxyForQuery() const
{
    QNetworkProxyQuery query(QUrl(QString("http://www.abc.com")), QNetworkProxyQuery::UrlRequest);
    QList<QNetworkProxy> systemProxyList = QNetworkProxyFactory::systemProxyForQuery(query);
    bool pass = true;
    QNetworkProxy proxy;

    QList<QNetworkProxy> nativeProxyList;
    nativeProxyList << QNetworkProxy(QNetworkProxy::HttpProxy, QString("http://test.proxy.com"), 8080) << QNetworkProxy::NoProxy;

    foreach (proxy, systemProxyList) {
        if (!nativeProxyList.contains(proxy)) {
            qWarning() << "System proxy not found in native proxy list: " <<
                  formatProxyName(proxy);
            pass = false;
        }
    }

    foreach (proxy, nativeProxyList) {
        if (!systemProxyList.contains(proxy)) {
            qWarning() << "Native proxy not found in system proxy list: " <<
                  formatProxyName(proxy);
            pass = false;
        }
    }

    if (!pass)
        QFAIL("One or more system proxy lookup failures occurred.");
}

QTEST_MAIN(tst_QNetworkProxyFactory)
#include "tst_qnetworkproxyfactory.moc"
