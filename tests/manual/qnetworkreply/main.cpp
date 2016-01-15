/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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
// This file contains benchmarks for QNetworkReply functions.

#include <QDebug>
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qhttpmultipart.h>
#include <QtNetwork/qauthenticator.h>
#include <QtCore/QJsonDocument>
#include "../../auto/network-settings.h"

#if defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL)
#include "private/qsslsocket_p.h"
#include <QtNetwork/private/qsslsocket_openssl_p.h>
#endif

#define BANDWIDTH_LIMIT_BYTES (1024*100)
#define TIME_ESTIMATION_SECONDS (97)

class tst_qnetworkreply : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void limiting_data();
    void limiting();
    void setSslConfiguration_data();
    void setSslConfiguration();
    void uploadToFacebook();
    void spdy_data();
    void spdy();
    void spdyMultipleRequestsPerHost();
    void proxyAuthentication_data();
    void proxyAuthentication();
    void authentication();
    void npnWithEmptyList(); // QTBUG-40714

protected slots:
    void spdyReplyFinished(); // only used by spdyMultipleRequestsPerHost test
    void authenticationRequiredSlot(QNetworkReply *, QAuthenticator *authenticator);

private:
    QHttpMultiPart *createFacebookMultiPart(const QByteArray &accessToken);
    QNetworkAccessManager m_manager;
};

QNetworkReply *reply;

class HttpReceiver : public QObject
{
    Q_OBJECT
    public slots:
    void finishedSlot() {
        quint64 bytesPerSec = (reply->header(QNetworkRequest::ContentLengthHeader).toLongLong()) / (stopwatch.elapsed() / 1000.0);
        qDebug() << "Finished HTTP(S) request with" << bytesPerSec << "bytes/sec";
        QVERIFY(bytesPerSec < BANDWIDTH_LIMIT_BYTES*1.05);
        QVERIFY(bytesPerSec > BANDWIDTH_LIMIT_BYTES*0.95);
        timer->stop();
        QTestEventLoop::instance().exitLoop();
    }
    void readyReadSlot() {
    }
    void timeoutSlot() {
        reply->read(BANDWIDTH_LIMIT_BYTES).size();
    }
    void startTimer() {
        stopwatch.start();
        timer = new QTimer(this);
        QObject::connect(timer, SIGNAL(timeout()), this, SLOT(timeoutSlot()));
        timer->start(1000);
    }
protected:
    QTimer *timer;
    QTime stopwatch;
};

void tst_qnetworkreply::initTestCase()
{
    qRegisterMetaType<QNetworkReply *>(); // for QSignalSpy
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
}

void tst_qnetworkreply::limiting_data()
{
    QTest::addColumn<QUrl>("url");

    QTest::newRow("HTTP") << QUrl("http://" + QtNetworkSettings::serverName() + "/mediumfile");
#ifndef QT_NO_SSL
    QTest::newRow("HTTP+SSL") << QUrl("https://" + QtNetworkSettings::serverName() + "/mediumfile");
#endif

}

void tst_qnetworkreply::limiting()
{
    HttpReceiver receiver;
    QNetworkAccessManager manager;

    QFETCH(QUrl, url);
    QNetworkRequest req (url);

    qDebug() << "Starting. This will take a while (around" << TIME_ESTIMATION_SECONDS << "sec).";
    qDebug() << "Please check the actual bandwidth usage with a network monitor, e.g. the KDE";
    qDebug() << "network plasma widget. It should be around" << BANDWIDTH_LIMIT_BYTES << "bytes/sec.";
    reply = manager.get(req);
    reply->ignoreSslErrors();
    reply->setReadBufferSize(BANDWIDTH_LIMIT_BYTES);
    QObject::connect(reply, SIGNAL(readyRead()), &receiver, SLOT(readyReadSlot()));
    QObject::connect(reply, SIGNAL(finished()), &receiver, SLOT(finishedSlot()));
    receiver.startTimer();

    // event loop
    QTestEventLoop::instance().enterLoop(TIME_ESTIMATION_SECONDS + 20);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void tst_qnetworkreply::setSslConfiguration_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("works");

    QTest::newRow("codereview.qt-project.org") << QUrl("https://codereview.qt-project.org") << true;
    QTest::newRow("test-server") << QUrl("https://" + QtNetworkSettings::serverName() + "/") << false;
}

void tst_qnetworkreply::setSslConfiguration()
{
#ifdef QT_NO_SSL
    QSKIP("SSL is not enabled.");
#else
    QFETCH(QUrl, url);
    QNetworkRequest request(url);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setProtocol(QSsl::TlsV1_0); // TLS 1.0 will be used anyway, just make sure we change the configuration
    request.setSslConfiguration(conf);
    QNetworkAccessManager manager;
    reply = manager.get(request);
    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());
#ifdef QT_BUILD_INTERNAL
    QFETCH(bool, works);
    bool rootCertLoadingAllowed = QSslSocketPrivate::rootCertOnDemandLoadingSupported();
#if defined(Q_OS_LINUX)
    QCOMPARE(rootCertLoadingAllowed, true);
#elif defined(Q_OS_MAC)
    QCOMPARE(rootCertLoadingAllowed, false);
#else
    Q_UNUSED(rootCertLoadingAllowed)
#endif // other platforms: undecided (Windows: depends on the version)
    if (works) {
        QCOMPARE(reply->error(), QNetworkReply::NoError);
    } else {
        QCOMPARE(reply->error(), QNetworkReply::SslHandshakeFailedError);
    }
#endif
#endif // QT_NO_SSL
}

QHttpMultiPart *tst_qnetworkreply::createFacebookMultiPart(const QByteArray &accessToken)
{
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart accessTokenPart;
    accessTokenPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                              QVariant("form-data; name=\"access_token\""));
    accessTokenPart.setBody(accessToken);
    multiPart->append(accessTokenPart);

    QHttpPart batchPart;
    batchPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"batch\""));
    batchPart.setBody("["
"   {"
"      \"attached_files\" : \"image1\","
"      \"body\" : \"message=&published=0\","
"      \"method\" : \"POST\","
"      \"relative_url\" : \"me/photos\""
"   },"
"   {"
"      \"attached_files\" : \"image2\","
"      \"body\" : \"message=&published=0\","
"      \"method\" : \"POST\","
"      \"relative_url\" : \"me/photos\""
"   },"
"   {"
"      \"attached_files\" : \"image3\","
"      \"body\" : \"message=&published=0\","
"      \"method\" : \"POST\","
"      \"relative_url\" : \"me/photos\""
"   }"
"]");
    multiPart->append(batchPart);

    QHttpPart imagePart1;
    imagePart1.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
    imagePart1.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image1\"; filename=\"image1.jpg\""));
    QFile *file1 = new QFile(QFINDTESTDATA("../../auto/network/access/qnetworkreply/image1.jpg"));
    file1->open(QIODevice::ReadOnly);
    imagePart1.setBodyDevice(file1);
    file1->setParent(multiPart);

    multiPart->append(imagePart1);

    QHttpPart imagePart2;
    imagePart2.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
    imagePart2.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image2\"; filename=\"image2.jpg\""));
    QFile *file2 = new QFile(QFINDTESTDATA("../../auto/network/access/qnetworkreply/image2.jpg"));
    file2->open(QIODevice::ReadOnly);
    imagePart2.setBodyDevice(file2);
    file2->setParent(multiPart);

    multiPart->append(imagePart2);

    QHttpPart imagePart3;
    imagePart3.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
    imagePart3.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image3\"; filename=\"image3.jpg\""));
    QFile *file3 = new QFile(QFINDTESTDATA("../../auto/network/access/qnetworkreply/image3.jpg"));
    file3->open(QIODevice::ReadOnly);
    imagePart3.setBodyDevice(file3);
    file3->setParent(multiPart);

    multiPart->append(imagePart3);
    return multiPart;
}

void tst_qnetworkreply::uploadToFacebook()
{
    QByteArray accessToken = qgetenv("QT_FACEBOOK_ACCESS_TOKEN");
    if (accessToken.isEmpty())
        QSKIP("This test requires the QT_FACEBOOK_ACCESS_TOKEN environment variable to be set. "
              "Do something like 'export QT_FACEBOOK_ACCESS_TOKEN=MyAccessToken'");

    QElapsedTimer timer;
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("https://graph.facebook.com/me/photos"));
    QHttpMultiPart *multiPart = createFacebookMultiPart(accessToken);
    timer.start();

    QNetworkReply *reply = manager.post(request, multiPart);
    multiPart->setParent(reply);
    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(120);
    qint64 elapsed = timer.elapsed();
    QVERIFY(!QTestEventLoop::instance().timeout());
    qDebug() << "reply finished after" << elapsed / 1000.0 << "seconds";
    QCOMPARE(reply->error(), QNetworkReply::NoError);

    QByteArray content = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(content);
    QVERIFY(jsonDocument.isArray());
    QJsonArray uploadStatuses = jsonDocument.array();
    QVERIFY(uploadStatuses.size() > 0);
    for (int a = 0; a < uploadStatuses.size(); a++) {
        QJsonValue currentUploadStatus = uploadStatuses.at(a);
        QVERIFY(currentUploadStatus.isObject());
        QJsonObject statusObject = currentUploadStatus.toObject();
        QJsonValue statusCode = statusObject.value(QLatin1String("code"));
        QCOMPARE(statusCode.toVariant().toInt(), 200); // 200 OK
    }
}

void tst_qnetworkreply::spdy_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<bool>("setAttribute");
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<QByteArray>("expectedProtocol");

    QList<QString> hosts = QList<QString>()
            << QStringLiteral("www.google.com") // sends SPDY and 30x redirect
            << QStringLiteral("www.google.de") // sends SPDY and 200 OK
            << QStringLiteral("mail.google.com") // sends SPDY and 200 OK
            << QStringLiteral("www.youtube.com") // sends SPDY and 200 OK
            << QStringLiteral("www.dropbox.com") // no SPDY, but NPN which selects HTTP
            << QStringLiteral("www.facebook.com") // sends SPDY and 200 OK
            << QStringLiteral("graph.facebook.com") // sends SPDY and 200 OK
            << QStringLiteral("www.twitter.com") // sends SPDY and 30x redirect
            << QStringLiteral("twitter.com") // sends SPDY and 200 OK
            << QStringLiteral("api.twitter.com"); // sends SPDY and 200 OK

    foreach (const QString &host, hosts) {
        QByteArray tag = host.toLocal8Bit();
        tag.append("-not-used");
        QTest::newRow(tag)
                << QStringLiteral("https://") + host
                << false
                << false
                << QByteArray();

        tag = host.toLocal8Bit();
        tag.append("-disabled");
        QTest::newRow(tag)
                << QStringLiteral("https://") + host
                << true
                << false
                << QByteArray();

        if (host != QStringLiteral("api.twitter.com")) { // they don't offer an API over HTTP
            tag = host.toLocal8Bit();
            tag.append("-no-https-url");
            QTest::newRow(tag)
                    << QStringLiteral("http://") + host
                    << true
                    << true
                    << QByteArray();
        }

#ifndef QT_NO_OPENSSL
        tag = host.toLocal8Bit();
        tag.append("-enabled");
        QTest::newRow(tag)
                << QStringLiteral("https://") + host
                << true
                << true
                << (host == QStringLiteral("www.dropbox.com")
                    ? QByteArray(QSslConfiguration::NextProtocolHttp1_1)
                    : QByteArray(QSslConfiguration::NextProtocolSpdy3_0));
#endif // QT_NO_OPENSSL
    }
}

void tst_qnetworkreply::spdy()
{
#if defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL) && OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)

    m_manager.clearAccessCache();

    QFETCH(QString, host);
    QUrl url(host);
    QNetworkRequest request(url);

    QFETCH(bool, setAttribute);
    QFETCH(bool, enabled);
    if (setAttribute) {
        request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, QVariant(enabled));
    }

    QNetworkReply *reply = m_manager.get(request);
    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QSignalSpy metaDataChangedSpy(reply, SIGNAL(metaDataChanged()));
    QSignalSpy readyReadSpy(reply, SIGNAL(readyRead()));
    QSignalSpy finishedSpy(reply, SIGNAL(finished()));
    QSignalSpy finishedManagerSpy(&m_manager, SIGNAL(finished(QNetworkReply*)));

    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QFETCH(QByteArray, expectedProtocol);

    bool expectedSpdyUsed = (expectedProtocol == QSslConfiguration::NextProtocolSpdy3_0);
    QCOMPARE(reply->attribute(QNetworkRequest::SpdyWasUsedAttribute).toBool(), expectedSpdyUsed);

    QCOMPARE(metaDataChangedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedManagerSpy.count(), 1);

    QUrl redirectUrl = reply->header(QNetworkRequest::LocationHeader).toUrl();
    QByteArray content = reply->readAll();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QVERIFY(statusCode >= 200 && statusCode < 500);
    if (statusCode == 200 || statusCode >= 400) {
        QVERIFY(readyReadSpy.count() > 0);
        QVERIFY(!content.isEmpty());
    } else if (statusCode >= 300 && statusCode < 400) {
        QVERIFY(!redirectUrl.isEmpty());
    }

    QSslConfiguration::NextProtocolNegotiationStatus expectedStatus =
            expectedProtocol.isNull() ? QSslConfiguration::NextProtocolNegotiationNone
            : QSslConfiguration::NextProtocolNegotiationNegotiated;
    QCOMPARE(reply->sslConfiguration().nextProtocolNegotiationStatus(),
             expectedStatus);

    QCOMPARE(reply->sslConfiguration().nextNegotiatedProtocol(), expectedProtocol);
#else
    QSKIP("Qt built withouth OpenSSL, or the OpenSSL version is too old");
#endif // defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL) ...
}

void tst_qnetworkreply::spdyReplyFinished()
{
    static int finishedCount = 0;
    finishedCount++;

    if (finishedCount == 12)
        QTestEventLoop::instance().exitLoop();
}

void tst_qnetworkreply::spdyMultipleRequestsPerHost()
{
#if defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL) && OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)

    QList<QNetworkRequest> requests;
    requests
            << QNetworkRequest(QUrl("https://www.facebook.com"))
            << QNetworkRequest(QUrl("https://www.facebook.com/images/fb_icon_325x325.png"))

            << QNetworkRequest(QUrl("https://www.google.de"))
            << QNetworkRequest(QUrl("https://www.google.de/preferences?hl=de"))
            << QNetworkRequest(QUrl("https://www.google.de/intl/de/policies/?fg=1"))
            << QNetworkRequest(QUrl("https://www.google.de/intl/de/about.html?fg=1"))
            << QNetworkRequest(QUrl("https://www.google.de/services/?fg=1"))
            << QNetworkRequest(QUrl("https://www.google.de/intl/de/ads/?fg=1"))

            << QNetworkRequest(QUrl("https://i1.ytimg.com/li/tnHdj3df7iM/default.jpg"))
            << QNetworkRequest(QUrl("https://i1.ytimg.com/li/7Dr1BKwqctY/default.jpg"))
            << QNetworkRequest(QUrl("https://i1.ytimg.com/li/hfZhJdhTqX8/default.jpg"))
            << QNetworkRequest(QUrl("https://i1.ytimg.com/vi/14Nprh8163I/hqdefault.jpg"))
               ;
    QList<QNetworkReply *> replies;
    QList<QSignalSpy *> metaDataChangedSpies;
    QList<QSignalSpy *> readyReadSpies;
    QList<QSignalSpy *> finishedSpies;

    QSignalSpy finishedManagerSpy(&m_manager, SIGNAL(finished(QNetworkReply*)));

    foreach (QNetworkRequest request, requests) {
        request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
        QNetworkReply *reply = m_manager.get(request);
        QObject::connect(reply, SIGNAL(finished()), this, SLOT(spdyReplyFinished()));
        replies << reply;
        QSignalSpy *metaDataChangedSpy = new QSignalSpy(reply, SIGNAL(metaDataChanged()));
        metaDataChangedSpies << metaDataChangedSpy;
        QSignalSpy *readyReadSpy = new QSignalSpy(reply, SIGNAL(readyRead()));
        readyReadSpies << readyReadSpy;
        QSignalSpy *finishedSpy = new QSignalSpy(reply, SIGNAL(finished()));
        finishedSpies << finishedSpy;
    }

    QCOMPARE(requests.count(), replies.count());

    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(finishedManagerSpy.count(), requests.count());

    for (int a = 0; a < replies.count(); ++a) {

        QCOMPARE(replies.at(a)->sslConfiguration().nextProtocolNegotiationStatus(),
                 QSslConfiguration::NextProtocolNegotiationNegotiated);
        QCOMPARE(replies.at(a)->sslConfiguration().nextNegotiatedProtocol(),
                 QByteArray(QSslConfiguration::NextProtocolSpdy3_0));

        QCOMPARE(replies.at(a)->error(), QNetworkReply::NoError);
        QCOMPARE(replies.at(a)->attribute(QNetworkRequest::SpdyWasUsedAttribute).toBool(), true);
        QCOMPARE(replies.at(a)->attribute(QNetworkRequest::ConnectionEncryptedAttribute).toBool(), true);
        QCOMPARE(replies.at(a)->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);

        QByteArray content = replies.at(a)->readAll();
        QVERIFY(content.count() > 0);

        QCOMPARE(metaDataChangedSpies.at(a)->count(), 1);
        metaDataChangedSpies.at(a)->deleteLater();

        QCOMPARE(finishedSpies.at(a)->count(), 1);
        finishedSpies.at(a)->deleteLater();

        QVERIFY(readyReadSpies.at(a)->count() > 0);
        readyReadSpies.at(a)->deleteLater();

        replies.at(a)->deleteLater();
    }
#else
    QSKIP("Qt built withouth OpenSSL, or the OpenSSL version is too old");
#endif // defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL) ...
}

void tst_qnetworkreply::proxyAuthentication_data()
{
    QTest::addColumn<QUrl>("url");

    QTest::newRow("http://www.google.com") << QUrl("http://www.google.com");
    QTest::newRow("https://www.google.com") << QUrl("https://www.google.com");
}

void tst_qnetworkreply::proxyAuthentication()
{
    QFETCH(QUrl, url);
    QNetworkRequest request(url);
    QNetworkAccessManager manager;

    QByteArray proxyHostName = qgetenv("QT_PROXY_HOST");
    QByteArray proxyPort = qgetenv("QT_PROXY_PORT");
    QByteArray proxyUser = qgetenv("QT_PROXY_USER");
    QByteArray proxyPassword = qgetenv("QT_PROXY_PASSWORD");
    if (proxyHostName.isEmpty() || proxyPort.isEmpty() || proxyUser.isEmpty()
            || proxyPassword.isEmpty())
        QSKIP("This test requires the QT_PROXY_* environment variables to be set. "
              "Do something like:\n"
              "export QT_PROXY_HOST=myNTLMHost\n"
              "export QT_PROXY_PORT=8080\n"
              "export QT_PROXY_USER='myDomain\\myUser'\n"
              "export QT_PROXY_PASSWORD=myPassword\n");

    QNetworkProxy proxy(QNetworkProxy::HttpProxy);
    proxy.setHostName(proxyHostName);
    proxy.setPort(proxyPort.toInt());
    proxy.setUser(proxyUser);
    proxy.setPassword(proxyPassword);

    manager.setProxy(proxy);

    reply = manager.get(request);
    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(reply->error(), QNetworkReply::NoError);
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QVERIFY(statusCode >= 200 && statusCode < 400);
}

void tst_qnetworkreply::authenticationRequiredSlot(QNetworkReply *,
                                                   QAuthenticator *authenticator)
{
    QString authUser = QString::fromLocal8Bit(qgetenv("QT_AUTH_USER"));
    QString authPassword = QString::fromLocal8Bit(qgetenv("QT_AUTH_PASSWORD"));
    authenticator->setUser(authUser);
    authenticator->setPassword(authPassword);
}

void tst_qnetworkreply::authentication()
{
    QByteArray authUrl = qgetenv("QT_AUTH_URL");
    if (authUrl.isEmpty())
        QSKIP("This test requires the QT_AUTH_* environment variables to be set. "
              "Do something like:\n"
              "export QT_AUTH_URL='http://myUrl.com/myPath'\n"
              "export QT_AUTH_USER='myDomain\\myUser'\n"
              "export QT_AUTH_PASSWORD=myPassword\n");

    QUrl url(QString::fromLocal8Bit(authUrl));
    QNetworkRequest request(url);
    QNetworkAccessManager manager;
    QObject::connect(&manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
                     this, SLOT(authenticationRequiredSlot(QNetworkReply*,QAuthenticator*)));
    reply = manager.get(request);
    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY2(reply->error() == QNetworkReply::NoError, reply->errorString().toLocal8Bit());
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QVERIFY(statusCode >= 200 && statusCode < 400);
}

void tst_qnetworkreply::npnWithEmptyList() // QTBUG-40714
{
#if defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL) && OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)

    // The server does not send a list of Next Protocols, so we test
    // that we continue anyhow and it works

    m_manager.clearAccessCache();

    QUrl url(QStringLiteral("https://www.ossifrage.net/"));
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, QVariant(true));
    QNetworkReply *reply = m_manager.get(request);
    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QVERIFY(statusCode == 200);

    QCOMPARE(reply->sslConfiguration().nextProtocolNegotiationStatus(),
             QSslConfiguration::NextProtocolNegotiationUnsupported);
#else
    QSKIP("Qt built withouth OpenSSL, or the OpenSSL version is too old");
#endif // defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL) ...
}

QTEST_MAIN(tst_qnetworkreply)

#include "main.moc"
