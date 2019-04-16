/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <QtNetwork/QtNetwork>
#include "../../../network-settings.h"

#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/qnetworkconfigmanager.h>
#include <QtNetwork/qnetworkconfiguration.h>
#include <QtNetwork/qnetworksession.h>
#endif

#include <algorithm>

#define TESTFILE QLatin1String("http://") + QtNetworkSettings::httpServerName() + QLatin1String("/qtest/cgi-bin/")

class tst_QAbstractNetworkCache : public QObject
{
    Q_OBJECT

public:
    tst_QAbstractNetworkCache();
    virtual ~tst_QAbstractNetworkCache();

private slots:
    void initTestCase();
    void expires_data();
    void expires();
    void expiresSynchronous_data();
    void expiresSynchronous();

    void lastModified_data();
    void lastModified();
    void lastModifiedSynchronous_data();
    void lastModifiedSynchronous();

    void etag_data();
    void etag();
    void etagSynchronous_data();
    void etagSynchronous();

    void cacheControl_data();
    void cacheControl();
    void cacheControlSynchronous_data();
    void cacheControlSynchronous();

    void deleteCache();

private:
    void runTest();
    void checkSynchronous();

#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkConfigurationManager *netConfMan;
    QNetworkConfiguration networkConfiguration;
    QScopedPointer<QNetworkSession> networkSession;
#endif
};

class NetworkDiskCache : public QNetworkDiskCache
{
    Q_OBJECT
public:
    NetworkDiskCache(QObject *parent = 0)
        : QNetworkDiskCache(parent)
        , tempDir(QDir::tempPath() + QLatin1String("/tst_qabstractnetworkcache.XXXXXX"))
        , gotData(false)
    {
        setCacheDirectory(tempDir.path());
        clear();
    }

    QIODevice *data(const QUrl &url)
    {
        gotData = true;
        return QNetworkDiskCache::data(url);
    }

    QTemporaryDir tempDir;
    bool gotData;
};


tst_QAbstractNetworkCache::tst_QAbstractNetworkCache()
{
    QCoreApplication::setOrganizationName(QLatin1String("QtProject"));
    QCoreApplication::setApplicationName(QLatin1String("autotest_qabstractnetworkcache"));
    QCoreApplication::setApplicationVersion(QLatin1String("1.0"));
}

tst_QAbstractNetworkCache::~tst_QAbstractNetworkCache()
{
}

static bool AlwaysTrue = true;
static bool AlwaysFalse = false;

Q_DECLARE_METATYPE(QNetworkRequest::CacheLoadControl)


void tst_QAbstractNetworkCache::initTestCase()
{
#if defined(QT_TEST_SERVER)
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::httpServerName(), 80));
#else
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
#endif

#ifndef QT_NO_BEARERMANAGEMENT
    netConfMan = new QNetworkConfigurationManager(this);
    networkConfiguration = netConfMan->defaultConfiguration();
    networkSession.reset(new QNetworkSession(networkConfiguration));
    if (!networkSession->isOpen()) {
        networkSession->open();
        QVERIFY(networkSession->waitForOpened(30000));
    }
#endif
}


void tst_QAbstractNetworkCache::expires_data()
{
    QTest::addColumn<QNetworkRequest::CacheLoadControl>("cacheLoadControl");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("fetchFromCache");

    QTest::newRow("304-0") << QNetworkRequest::AlwaysNetwork << "httpcachetest_expires304.cgi" << AlwaysFalse;
    QTest::newRow("304-1") << QNetworkRequest::PreferNetwork << "httpcachetest_expires304.cgi" << true;
    QTest::newRow("304-2") << QNetworkRequest::AlwaysCache << "httpcachetest_expires304.cgi" << AlwaysTrue;
    QTest::newRow("304-3") << QNetworkRequest::PreferCache << "httpcachetest_expires304.cgi" << true;

    QTest::newRow("500-0") << QNetworkRequest::AlwaysNetwork << "httpcachetest_expires500.cgi" << AlwaysFalse;
    QTest::newRow("500-1") << QNetworkRequest::PreferNetwork << "httpcachetest_expires500.cgi" << true;
    QTest::newRow("500-2") << QNetworkRequest::AlwaysCache << "httpcachetest_expires500.cgi" << AlwaysTrue;
    QTest::newRow("500-3") << QNetworkRequest::PreferCache << "httpcachetest_expires500.cgi" << true;

    QTest::newRow("200-0") << QNetworkRequest::AlwaysNetwork << "httpcachetest_expires200.cgi" << AlwaysFalse;
    QTest::newRow("200-1") << QNetworkRequest::PreferNetwork << "httpcachetest_expires200.cgi" << false;
    QTest::newRow("200-2") << QNetworkRequest::AlwaysCache << "httpcachetest_expires200.cgi" << AlwaysTrue;
    QTest::newRow("200-3") << QNetworkRequest::PreferCache << "httpcachetest_expires200.cgi" << false;
}

void tst_QAbstractNetworkCache::expires()
{
    runTest();
}

void tst_QAbstractNetworkCache::expiresSynchronous_data()
{
    expires_data();
}

void tst_QAbstractNetworkCache::expiresSynchronous()
{
    checkSynchronous();
}

void tst_QAbstractNetworkCache::lastModified_data()
{
    QTest::addColumn<QNetworkRequest::CacheLoadControl>("cacheLoadControl");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("fetchFromCache");

    QTest::newRow("304-0") << QNetworkRequest::AlwaysNetwork << "httpcachetest_lastModified304.cgi" << AlwaysFalse;
    QTest::newRow("304-1") << QNetworkRequest::PreferNetwork << "httpcachetest_lastModified304.cgi" << true;
    QTest::newRow("304-2") << QNetworkRequest::AlwaysCache << "httpcachetest_lastModified304.cgi" << AlwaysTrue;
    QTest::newRow("304-3") << QNetworkRequest::PreferCache << "httpcachetest_lastModified304.cgi" << true;

    QTest::newRow("200-0") << QNetworkRequest::AlwaysNetwork << "httpcachetest_lastModified200.cgi" << AlwaysFalse;
    QTest::newRow("200-1") << QNetworkRequest::PreferNetwork << "httpcachetest_lastModified200.cgi" << true;
    QTest::newRow("200-2") << QNetworkRequest::AlwaysCache << "httpcachetest_lastModified200.cgi" << AlwaysTrue;
    QTest::newRow("200-3") << QNetworkRequest::PreferCache << "httpcachetest_lastModified200.cgi" << true;
}

void tst_QAbstractNetworkCache::lastModified()
{
    runTest();
}

void tst_QAbstractNetworkCache::lastModifiedSynchronous_data()
{
    tst_QAbstractNetworkCache::lastModified_data();
}

void tst_QAbstractNetworkCache::lastModifiedSynchronous()
{
    checkSynchronous();
}

void tst_QAbstractNetworkCache::etag_data()
{
    QTest::addColumn<QNetworkRequest::CacheLoadControl>("cacheLoadControl");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("fetchFromCache");

    QTest::newRow("304-0") << QNetworkRequest::AlwaysNetwork << "httpcachetest_etag304.cgi" << AlwaysFalse;
    QTest::newRow("304-1") << QNetworkRequest::PreferNetwork << "httpcachetest_etag304.cgi" << true;
    QTest::newRow("304-2") << QNetworkRequest::AlwaysCache << "httpcachetest_etag304.cgi" << AlwaysTrue;
    QTest::newRow("304-3") << QNetworkRequest::PreferCache << "httpcachetest_etag304.cgi" << true;

    QTest::newRow("200-0") << QNetworkRequest::AlwaysNetwork << "httpcachetest_etag200.cgi" << AlwaysFalse;
    QTest::newRow("200-1") << QNetworkRequest::PreferNetwork << "httpcachetest_etag200.cgi" << false;
    QTest::newRow("200-2") << QNetworkRequest::AlwaysCache << "httpcachetest_etag200.cgi" << AlwaysTrue;
    QTest::newRow("200-3") << QNetworkRequest::PreferCache << "httpcachetest_etag200.cgi" << false;
}

void tst_QAbstractNetworkCache::etag()
{
    runTest();
}

void tst_QAbstractNetworkCache::etagSynchronous_data()
{
    tst_QAbstractNetworkCache::etag_data();
}

void tst_QAbstractNetworkCache::etagSynchronous()
{
    checkSynchronous();
}

void tst_QAbstractNetworkCache::cacheControl_data()
{
    QTest::addColumn<QNetworkRequest::CacheLoadControl>("cacheLoadControl");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("fetchFromCache");
    QTest::newRow("200-0") << QNetworkRequest::PreferNetwork << "httpcachetest_cachecontrol.cgi?max-age=-1" << true;
    QTest::newRow("200-1") << QNetworkRequest::PreferNetwork << "httpcachetest_cachecontrol-expire.cgi" << false;

    QTest::newRow("200-2") << QNetworkRequest::AlwaysNetwork << "httpcachetest_cachecontrol.cgi?no-cache" << AlwaysFalse;
    QTest::newRow("200-3") << QNetworkRequest::PreferNetwork << "httpcachetest_cachecontrol.cgi?no-cache" << true;
    QTest::newRow("200-4") << QNetworkRequest::AlwaysCache << "httpcachetest_cachecontrol.cgi?no-cache" << false;
    QTest::newRow("200-5") << QNetworkRequest::PreferCache << "httpcachetest_cachecontrol.cgi?no-cache" << true;

    QTest::newRow("200-6") << QNetworkRequest::AlwaysNetwork << "httpcachetest_cachecontrol.cgi?no-store" << AlwaysFalse;
    QTest::newRow("200-7") << QNetworkRequest::PreferNetwork << "httpcachetest_cachecontrol.cgi?no-store" << false;
    QTest::newRow("200-8") << QNetworkRequest::AlwaysCache << "httpcachetest_cachecontrol.cgi?no-store" << false;
    QTest::newRow("200-9") << QNetworkRequest::PreferCache << "httpcachetest_cachecontrol.cgi?no-store" << false;

    QTest::newRow("304-0") << QNetworkRequest::PreferNetwork << "httpcachetest_cachecontrol.cgi?max-age=1000" << true;

    QTest::newRow("304-1") << QNetworkRequest::AlwaysNetwork << "httpcachetest_cachecontrol.cgi?max-age=1000, must-revalidate" << AlwaysFalse;
    QTest::newRow("304-2") << QNetworkRequest::PreferNetwork << "httpcachetest_cachecontrol.cgi?max-age=1000, must-revalidate" << true;
    QTest::newRow("304-3") << QNetworkRequest::AlwaysCache << "httpcachetest_cachecontrol.cgi?max-age=1000, must-revalidate" << false;
    QTest::newRow("304-4") << QNetworkRequest::PreferCache << "httpcachetest_cachecontrol.cgi?max-age=1000, must-revalidate" << true;

    QTest::newRow("304-2b") << QNetworkRequest::PreferNetwork << "httpcachetest_cachecontrol200.cgi?private, max-age=1000" << true;
    QTest::newRow("304-4b") << QNetworkRequest::PreferCache << "httpcachetest_cachecontrol200.cgi?private, max-age=1000" << true;
}

void tst_QAbstractNetworkCache::cacheControl()
{
    runTest();
}

void tst_QAbstractNetworkCache::cacheControlSynchronous_data()
{
    tst_QAbstractNetworkCache::cacheControl_data();
}

void tst_QAbstractNetworkCache::cacheControlSynchronous()
{
    checkSynchronous();
}

void tst_QAbstractNetworkCache::runTest()
{
    QFETCH(QNetworkRequest::CacheLoadControl, cacheLoadControl);
    QFETCH(QString, url);
    QFETCH(bool, fetchFromCache);

    QNetworkAccessManager manager;
    NetworkDiskCache *diskCache = new NetworkDiskCache(&manager);
    QVERIFY2(diskCache->tempDir.isValid(), qPrintable(diskCache->tempDir.errorString()));
    manager.setCache(diskCache);
    QCOMPARE(diskCache->gotData, false);

    QUrl realUrl = url.contains("://") ? url : TESTFILE + url;
    QNetworkRequest request(realUrl);

    // prime the cache
    QNetworkReply *reply = manager.get(request);
    QSignalSpy downloaded1(reply, SIGNAL(finished()));
    QTRY_COMPARE(downloaded1.count(), 1);
    QCOMPARE(diskCache->gotData, false);
    QByteArray goodData = reply->readAll();

    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, cacheLoadControl);

    // should be in the cache now
    QNetworkReply *reply2 = manager.get(request);
    QSignalSpy downloaded2(reply2, SIGNAL(finished()));
    QTRY_COMPARE(downloaded2.count(), 1);

    QByteArray secondData = reply2->readAll();
    if (!fetchFromCache && cacheLoadControl == QNetworkRequest::AlwaysCache) {
        QCOMPARE(reply2->error(), QNetworkReply::ContentNotFoundError);
        QCOMPARE(secondData, QByteArray());
    } else {
        QCOMPARE(reply2->error(), QNetworkReply::NoError);
        QCOMPARE(QString(secondData), QString(goodData));
        QCOMPARE(secondData, goodData);
        QCOMPARE(reply2->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    }

    if (fetchFromCache) {
        QList<QByteArray> rawHeaderList = reply->rawHeaderList();
        QList<QByteArray> rawHeaderList2 = reply2->rawHeaderList();
        std::sort(rawHeaderList.begin(), rawHeaderList.end());
        std::sort(rawHeaderList2.begin(), rawHeaderList2.end());
    }
    QCOMPARE(diskCache->gotData, fetchFromCache);
}

void tst_QAbstractNetworkCache::checkSynchronous()
{
    QSKIP("not working yet, see QTBUG-15221");

    QFETCH(QNetworkRequest::CacheLoadControl, cacheLoadControl);
    QFETCH(QString, url);
    QFETCH(bool, fetchFromCache);

    QNetworkAccessManager manager;
    NetworkDiskCache *diskCache = new NetworkDiskCache(&manager);
    QVERIFY2(diskCache->tempDir.isValid(), qPrintable(diskCache->tempDir.errorString()));
    manager.setCache(diskCache);
    QCOMPARE(diskCache->gotData, false);

    QUrl realUrl = url.contains("://") ? url : TESTFILE + url;
    QNetworkRequest request(realUrl);

    request.setAttribute(
            QNetworkRequest::SynchronousRequestAttribute,
            true);

    // prime the cache
    QNetworkReply *reply = manager.get(request);
    QVERIFY(reply->isFinished()); // synchronous
    QCOMPARE(diskCache->gotData, false);
    QByteArray goodData = reply->readAll();

    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, cacheLoadControl);

    // should be in the cache now
    QNetworkReply *reply2 = manager.get(request);
    QVERIFY(reply2->isFinished()); // synchronous

    QByteArray secondData = reply2->readAll();
    if (!fetchFromCache && cacheLoadControl == QNetworkRequest::AlwaysCache) {
        QCOMPARE(reply2->error(), QNetworkReply::ContentNotFoundError);
        QCOMPARE(secondData, QByteArray());
    } else {
        if (reply2->error() != QNetworkReply::NoError)
            qDebug() << reply2->errorString();
        QCOMPARE(reply2->error(), QNetworkReply::NoError);
        QCOMPARE(QString(secondData), QString(goodData));
        QCOMPARE(secondData, goodData);
        QCOMPARE(reply2->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    }

    if (fetchFromCache) {
        QList<QByteArray> rawHeaderList = reply->rawHeaderList();
        QList<QByteArray> rawHeaderList2 = reply2->rawHeaderList();
        std::sort(rawHeaderList.begin(), rawHeaderList.end());
        std::sort(rawHeaderList2.begin(), rawHeaderList2.end());
    }
    QCOMPARE(diskCache->gotData, fetchFromCache);
}

void tst_QAbstractNetworkCache::deleteCache()
{
    QNetworkAccessManager manager;
    NetworkDiskCache *diskCache = new NetworkDiskCache(&manager);
    QVERIFY2(diskCache->tempDir.isValid(), qPrintable(diskCache->tempDir.errorString()));
    manager.setCache(diskCache);

    QString url = "httpcachetest_cachecontrol.cgi?max-age=1000";
    QNetworkRequest request(QUrl(TESTFILE + url));
    QNetworkReply *reply = manager.get(request);
    QSignalSpy downloaded1(reply, SIGNAL(finished()));
    manager.setCache(0);
    QTRY_COMPARE(downloaded1.count(), 1);
}


QTEST_MAIN(tst_QAbstractNetworkCache)
#include "tst_qabstractnetworkcache.moc"

