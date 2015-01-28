/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtTest/QtTest>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QHttpPart>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QAuthenticator>
#if defined(QT_BUILD_INTERNAL) && !defined(QT_NO_OPENSSL)
#include <QtNetwork/private/qsslsocket_openssl_p.h>
#endif // QT_BUILD_INTERNAL && !QT_NO_OPENSSL

#include "../../../network-settings.h"

Q_DECLARE_METATYPE(QAuthenticator*)

class tst_Spdy: public QObject
{
    Q_OBJECT

public:
    tst_Spdy();
    ~tst_Spdy();

private Q_SLOTS:
    void initTestCase();
    void settingsAndNegotiation_data();
    void settingsAndNegotiation();
#ifndef QT_NO_NETWORKPROXY
    void download_data();
    void download();
#endif // !QT_NO_NETWORKPROXY
    void headerFields();
#ifndef QT_NO_NETWORKPROXY
    void upload_data();
    void upload();
    void errors_data();
    void errors();
#endif // !QT_NO_NETWORKPROXY
    void multipleRequests_data();
    void multipleRequests();

private:
    QNetworkAccessManager m_manager;
    int m_multipleRequestsCount;
    int m_multipleRepliesFinishedCount;
    const QString m_rfc3252FilePath;

protected Q_SLOTS:
    void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *authenticator);
    void multipleRequestsFinishedSlot();
};

tst_Spdy::tst_Spdy()
    : m_rfc3252FilePath(QFINDTESTDATA("../qnetworkreply/rfc3252.txt"))
{
#if defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL) && OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)
    qRegisterMetaType<QNetworkReply *>(); // for QSignalSpy
    qRegisterMetaType<QAuthenticator *>();

    connect(&m_manager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
            this, SLOT(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)));
#else
    QSKIP("Qt built withouth OpenSSL, or the OpenSSL version is too old");
#endif // defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL) ...
}

tst_Spdy::~tst_Spdy()
{
}

void tst_Spdy::initTestCase()
{
    QVERIFY(!m_rfc3252FilePath.isEmpty());
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
}

void tst_Spdy::settingsAndNegotiation_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("setAttribute");
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<QByteArray>("expectedProtocol");
    QTest::addColumn<QByteArray>("expectedContent");

    QTest::newRow("default-settings") << QUrl("https://" + QtNetworkSettings::serverName()
                                              + "/qtest/cgi-bin/echo.cgi?1")
                                      << false << false << QByteArray()
                                      << QByteArray("1");

    QTest::newRow("http-url") << QUrl("http://" + QtNetworkSettings::serverName()
                                      + "/qtest/cgi-bin/echo.cgi?1")
                              << true << true << QByteArray()
                              << QByteArray("1");

    QTest::newRow("spdy-disabled") << QUrl("https://" + QtNetworkSettings::serverName()
                                           + "/qtest/cgi-bin/echo.cgi?1")
                                   << true << false << QByteArray()
                                   << QByteArray("1");

#ifndef QT_NO_OPENSSL
    QTest::newRow("spdy-enabled") << QUrl("https://" + QtNetworkSettings::serverName()
                                     + "/qtest/cgi-bin/echo.cgi?1")
                                  << true << true << QByteArray(QSslConfiguration::NextProtocolSpdy3_0)
                             << QByteArray("1");
#endif // QT_NO_OPENSSL
}

void tst_Spdy::settingsAndNegotiation()
{
    QFETCH(QUrl, url);
    QFETCH(bool, setAttribute);
    QFETCH(bool, enabled);

    QNetworkRequest request(url);

    if (setAttribute) {
        request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, QVariant(enabled));
    }

    QNetworkReply *reply = m_manager.get(request);
    reply->ignoreSslErrors();
    QSignalSpy metaDataChangedSpy(reply, SIGNAL(metaDataChanged()));
    QSignalSpy readyReadSpy(reply, SIGNAL(readyRead()));
    QSignalSpy finishedSpy(reply, SIGNAL(finished()));

    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QSignalSpy finishedManagerSpy(&m_manager, SIGNAL(finished(QNetworkReply*)));

    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QFETCH(QByteArray, expectedProtocol);

#ifndef QT_NO_OPENSSL
    bool expectedSpdyUsed = (expectedProtocol == QSslConfiguration::NextProtocolSpdy3_0);
    QCOMPARE(reply->attribute(QNetworkRequest::SpdyWasUsedAttribute).toBool(), expectedSpdyUsed);
#endif // QT_NO_OPENSSL

    QCOMPARE(metaDataChangedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 1);

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QCOMPARE(statusCode, 200);

    QByteArray content = reply->readAll();

    QFETCH(QByteArray, expectedContent);
    QCOMPARE(expectedContent, content);

#ifndef QT_NO_OPENSSL
    QSslConfiguration::NextProtocolNegotiationStatus expectedStatus =
            (expectedProtocol.isEmpty())
            ? QSslConfiguration::NextProtocolNegotiationNone
            : QSslConfiguration::NextProtocolNegotiationNegotiated;
    QCOMPARE(reply->sslConfiguration().nextProtocolNegotiationStatus(),
             expectedStatus);

    QCOMPARE(reply->sslConfiguration().nextNegotiatedProtocol(), expectedProtocol);
#endif // QT_NO_OPENSSL
}

void tst_Spdy::proxyAuthenticationRequired(const QNetworkProxy &/*proxy*/,
                                           QAuthenticator *authenticator)
{
    authenticator->setUser("qsockstest");
    authenticator->setPassword("password");
}

#ifndef QT_NO_NETWORKPROXY
void tst_Spdy::download_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QNetworkProxy>("proxy");

    QTest::newRow("mediumfile") << QUrl("https://" + QtNetworkSettings::serverName()
                                        + "/qtest/rfc3252.txt")
                                << m_rfc3252FilePath
                                << QNetworkProxy();

    QHostInfo hostInfo = QHostInfo::fromName(QtNetworkSettings::serverName());
    QString proxyserver = hostInfo.addresses().first().toString();

    QTest::newRow("mediumfile-http-proxy") << QUrl("https://" + QtNetworkSettings::serverName()
                                                   + "/qtest/rfc3252.txt")
                                           << m_rfc3252FilePath
                                           << QNetworkProxy(QNetworkProxy::HttpProxy, proxyserver, 3128);

    QTest::newRow("mediumfile-http-proxy-auth") << QUrl("https://" + QtNetworkSettings::serverName()
                                                        + "/qtest/rfc3252.txt")
                                                << m_rfc3252FilePath
                                                << QNetworkProxy(QNetworkProxy::HttpProxy,
                                                                 proxyserver, 3129);

    QTest::newRow("mediumfile-socks-proxy") << QUrl("https://" + QtNetworkSettings::serverName()
                                                    + "/qtest/rfc3252.txt")
                                            << m_rfc3252FilePath
                                            << QNetworkProxy(QNetworkProxy::Socks5Proxy, proxyserver, 1080);

    QTest::newRow("mediumfile-socks-proxy-auth") << QUrl("https://" + QtNetworkSettings::serverName()
                                                         + "/qtest/rfc3252.txt")
                                                 << m_rfc3252FilePath
                                                 << QNetworkProxy(QNetworkProxy::Socks5Proxy,
                                                                  proxyserver, 1081);

    QTest::newRow("bigfile") << QUrl("https://" + QtNetworkSettings::serverName()
                                      + "/qtest/bigfile")
                             << QFINDTESTDATA("../qnetworkreply/bigfile")
                             << QNetworkProxy();
}

void tst_Spdy::download()
{
    QFETCH(QUrl, url);
    QFETCH(QString, fileName);
    QFETCH(QNetworkProxy, proxy);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);

    if (proxy.type() != QNetworkProxy::DefaultProxy) {
        m_manager.setProxy(proxy);
    }
    QNetworkReply *reply = m_manager.get(request);
    reply->ignoreSslErrors();
    QSignalSpy metaDataChangedSpy(reply, SIGNAL(metaDataChanged()));
    QSignalSpy downloadProgressSpy(reply, SIGNAL(downloadProgress(qint64, qint64)));
    QSignalSpy readyReadSpy(reply, SIGNAL(readyRead()));
    QSignalSpy finishedSpy(reply, SIGNAL(finished()));

    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QSignalSpy finishedManagerSpy(&m_manager, SIGNAL(finished(QNetworkReply*)));
    QSignalSpy proxyAuthRequiredSpy(&m_manager, SIGNAL(
                                        proxyAuthenticationRequired(const QNetworkProxy &,
                                                                    QAuthenticator *)));

    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(finishedManagerSpy.count(), 1);
    QCOMPARE(metaDataChangedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(downloadProgressSpy.count() > 0);
    QVERIFY(readyReadSpy.count() > 0);

    QVERIFY(proxyAuthRequiredSpy.count() <= 1);

    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QCOMPARE(reply->attribute(QNetworkRequest::SpdyWasUsedAttribute).toBool(), true);
    QCOMPARE(reply->attribute(QNetworkRequest::ConnectionEncryptedAttribute).toBool(), true);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);

    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));

    qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    qint64 expectedContentLength = file.bytesAvailable();
    QCOMPARE(contentLength, expectedContentLength);

    QByteArray expectedContent = file.readAll();
    QByteArray content = reply->readAll();
    QCOMPARE(content, expectedContent);

    reply->deleteLater();
    m_manager.setProxy(QNetworkProxy()); // reset
}
#endif // !QT_NO_NETWORKPROXY

void tst_Spdy::headerFields()
{
    QUrl url(QUrl("https://" + QtNetworkSettings::serverName()));
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);

    QNetworkReply *reply = m_manager.get(request);
    reply->ignoreSslErrors();

    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(reply->rawHeader("Content-Type"), QByteArray("text/html"));
    QVERIFY(reply->rawHeader("Content-Length").toInt() > 0);
    QVERIFY(reply->rawHeader("server").contains("Apache"));

    QCOMPARE(reply->header(QNetworkRequest::ContentTypeHeader).toByteArray(), QByteArray("text/html"));
    QVERIFY(reply->header(QNetworkRequest::ContentLengthHeader).toLongLong() > 0);
    QVERIFY(reply->header(QNetworkRequest::LastModifiedHeader).toDateTime().isValid());
    QVERIFY(reply->header(QNetworkRequest::ServerHeader).toByteArray().contains("Apache"));
}

static inline QByteArray md5sum(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex().append('\n');
}

#ifndef QT_NO_NETWORKPROXY
void tst_Spdy::upload_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("uploadMethod");
    QTest::addColumn<QObject *>("uploadObject");
    QTest::addColumn<QByteArray>("md5sum");
    QTest::addColumn<QNetworkProxy>("proxy");


    // 1. test uploading of byte arrays

    QUrl md5Url("https://" + QtNetworkSettings::serverName() + "/qtest/cgi-bin/md5sum.cgi");

    QByteArray data;
    data = "";
    QObject *dummyObject = 0;
    QTest::newRow("empty") << md5Url << data << QByteArray("POST") << dummyObject
                           << md5sum(data) << QNetworkProxy();

    data = "This is a normal message.";
    QTest::newRow("generic") << md5Url << data << QByteArray("POST") << dummyObject
                             << md5sum(data) << QNetworkProxy();

    data = "This is a message to show that Qt rocks!\r\n\n";
    QTest::newRow("small") << md5Url << data << QByteArray("POST") << dummyObject
                           << md5sum(data) << QNetworkProxy();

    data = QByteArray("abcd\0\1\2\abcd",12);
    QTest::newRow("with-nul") << md5Url << data << QByteArray("POST") << dummyObject
                              << md5sum(data) << QNetworkProxy();

    data = QByteArray(4097, '\4');
    QTest::newRow("4k+1") << md5Url << data << QByteArray("POST") << dummyObject
                          << md5sum(data)<< QNetworkProxy();

    QHostInfo hostInfo = QHostInfo::fromName(QtNetworkSettings::serverName());
    QString proxyserver = hostInfo.addresses().first().toString();

    QTest::newRow("4k+1-with-http-proxy") << md5Url << data << QByteArray("POST") << dummyObject
                                          << md5sum(data)
                                          << QNetworkProxy(QNetworkProxy::HttpProxy, proxyserver, 3128);

    QTest::newRow("4k+1-with-http-proxy-auth") << md5Url << data << QByteArray("POST") << dummyObject
                                               << md5sum(data)
                                               << QNetworkProxy(QNetworkProxy::HttpProxy,
                                                                proxyserver, 3129);

    QTest::newRow("4k+1-with-socks-proxy") << md5Url << data << QByteArray("POST") << dummyObject
                                           << md5sum(data)
                                           << QNetworkProxy(QNetworkProxy::Socks5Proxy, proxyserver, 1080);

    QTest::newRow("4k+1-with-socks-proxy-auth") << md5Url << data << QByteArray("POST") << dummyObject
                                                << md5sum(data)
                                                << QNetworkProxy(QNetworkProxy::Socks5Proxy,
                                                                 proxyserver, 1081);

    data = QByteArray(128*1024+1, '\177');
    QTest::newRow("128k+1") << md5Url << data << QByteArray("POST") << dummyObject
                            << md5sum(data) << QNetworkProxy();

    data = QByteArray(128*1024+1, '\177');
    QTest::newRow("128k+1-put") << md5Url << data << QByteArray("PUT") << dummyObject
                                << md5sum(data) << QNetworkProxy();

    data = QByteArray(2*1024*1024+1, '\177');
    QTest::newRow("2MB+1") << md5Url << data << QByteArray("POST") << dummyObject
                           << md5sum(data) << QNetworkProxy();


    // 2. test uploading of files

    QFile *file = new QFile(m_rfc3252FilePath);
    file->open(QIODevice::ReadOnly);
    QTest::newRow("file-26K") << md5Url << QByteArray() << QByteArray("POST")
                              << static_cast<QObject *>(file)
                              << QByteArray("b3e32ac459b99d3f59318f3ac31e4bee\n") << QNetworkProxy();

    QFile *file2 = new QFile(QFINDTESTDATA("../qnetworkreply/image1.jpg"));
    file2->open(QIODevice::ReadOnly);
    QTest::newRow("file-1MB") << md5Url << QByteArray() << QByteArray("POST")
                              << static_cast<QObject *>(file2)
                              << QByteArray("87ef3bb319b004ba9e5e9c9fa713776e\n") << QNetworkProxy();


    // 3. test uploading of multipart

    QUrl multiPartUrl("https://" + QtNetworkSettings::serverName() + "/qtest/cgi-bin/multipart.cgi");

    QHttpPart imagePart31;
    imagePart31.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart31.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"testImage1\""));
    imagePart31.setRawHeader("Content-Location", "http://my.test.location.tld");
    imagePart31.setRawHeader("Content-ID", "my@id.tld");
    QFile *file31 = new QFile(QFINDTESTDATA("../qnetworkreply/image1.jpg"));
    file31->open(QIODevice::ReadOnly);
    imagePart31.setBodyDevice(file31);
    QHttpMultiPart *imageMultiPart3 = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    imageMultiPart3->append(imagePart31);
    file31->setParent(imageMultiPart3);
    QHttpPart imagePart32;
    imagePart32.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart32.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"testImage2\""));
    QFile *file32 = new QFile(QFINDTESTDATA("../qnetworkreply/image2.jpg"));
    file32->open(QIODevice::ReadOnly);
    imagePart32.setBodyDevice(file31); // check that resetting works
    imagePart32.setBodyDevice(file32);
    imageMultiPart3->append(imagePart32);
    file32->setParent(imageMultiPart3);
    QHttpPart imagePart33;
    imagePart33.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart33.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"testImage3\""));
    QFile *file33 = new QFile(QFINDTESTDATA("../qnetworkreply/image3.jpg"));
    file33->open(QIODevice::ReadOnly);
    imagePart33.setBodyDevice(file33);
    imageMultiPart3->append(imagePart33);
    file33->setParent(imageMultiPart3);
    QByteArray expectedData = "content type: multipart/form-data; boundary=\""
            + imageMultiPart3->boundary();
    expectedData.append("\"\nkey: testImage1, value: 87ef3bb319b004ba9e5e9c9fa713776e\n"
            "key: testImage2, value: 483761b893f7fb1bd2414344cd1f3dfb\n"
            "key: testImage3, value: ab0eb6fd4fcf8b4436254870b4513033\n");

    QTest::newRow("multipart-3images") << multiPartUrl << QByteArray() << QByteArray("POST")
                                       << static_cast<QObject *>(imageMultiPart3) << expectedData
                                       << QNetworkProxy();
}

void tst_Spdy::upload()
{
    QFETCH(QUrl, url);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);

    QFETCH(QByteArray, data);
    QFETCH(QByteArray, uploadMethod);
    QFETCH(QObject *, uploadObject);
    QFETCH(QNetworkProxy, proxy);

    if (proxy.type() != QNetworkProxy::DefaultProxy) {
        m_manager.setProxy(proxy);
    }

    QNetworkReply *reply;
    QHttpMultiPart *multiPart = 0;

    if (uploadObject) {
        // upload via device
        if (QIODevice *device = qobject_cast<QIODevice *>(uploadObject)) {
            reply = m_manager.post(request, device);
        } else if ((multiPart = qobject_cast<QHttpMultiPart *>(uploadObject))) {
            reply = m_manager.post(request, multiPart);
        } else {
            QFAIL("got unknown upload device");
        }
    } else {
        // upload via byte array
        if (uploadMethod == "PUT") {
            reply = m_manager.put(request, data);
        } else {
            reply = m_manager.post(request, data);
        }
    }

    reply->ignoreSslErrors();
    QSignalSpy metaDataChangedSpy(reply, SIGNAL(metaDataChanged()));
    QSignalSpy uploadProgressSpy(reply, SIGNAL(uploadProgress(qint64, qint64)));
    QSignalSpy readyReadSpy(reply, SIGNAL(readyRead()));
    QSignalSpy finishedSpy(reply, SIGNAL(finished()));

    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QSignalSpy finishedManagerSpy(&m_manager, SIGNAL(finished(QNetworkReply*)));

    QTestEventLoop::instance().enterLoop(20);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(finishedManagerSpy.count(), 1);
    QCOMPARE(metaDataChangedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(uploadProgressSpy.count() > 0);
    QVERIFY(readyReadSpy.count() > 0);

    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QCOMPARE(reply->attribute(QNetworkRequest::SpdyWasUsedAttribute).toBool(), true);
    QCOMPARE(reply->attribute(QNetworkRequest::ConnectionEncryptedAttribute).toBool(), true);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);

    qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    if (!multiPart) // script to test multiparts does not return a content length
        QCOMPARE(contentLength, 33); // 33 bytes for md5 sums (including new line)

    QFETCH(QByteArray, md5sum);
    QByteArray content = reply->readAll();
    QCOMPARE(content, md5sum);

    reply->deleteLater();
    if (uploadObject)
        uploadObject->deleteLater();

    m_manager.setProxy(QNetworkProxy()); // reset
}

void tst_Spdy::errors_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QNetworkProxy>("proxy");
    QTest::addColumn<bool>("ignoreSslErrors");
    QTest::addColumn<int>("expectedReplyError");

    QTest::newRow("http-404") << QUrl("https://" + QtNetworkSettings::serverName() + "/non-existent-url")
                              << QNetworkProxy() << true << int(QNetworkReply::ContentNotFoundError);

    QTest::newRow("ssl-errors") << QUrl("https://" + QtNetworkSettings::serverName())
                                << QNetworkProxy() << false << int(QNetworkReply::SslHandshakeFailedError);

    QTest::newRow("host-not-found") << QUrl("https://this-host-does-not.exist")
                                    << QNetworkProxy()
                                    << true << int(QNetworkReply::HostNotFoundError);

    QTest::newRow("proxy-not-found") << QUrl("https://" + QtNetworkSettings::serverName())
                                     << QNetworkProxy(QNetworkProxy::HttpProxy,
                                                      "https://this-host-does-not.exist", 3128)
                                     << true << int(QNetworkReply::HostNotFoundError);

    QHostInfo hostInfo = QHostInfo::fromName(QtNetworkSettings::serverName());
    QString proxyserver = hostInfo.addresses().first().toString();

    QTest::newRow("proxy-unavailable") << QUrl("https://" + QtNetworkSettings::serverName())
                                       << QNetworkProxy(QNetworkProxy::HttpProxy, proxyserver, 10)
                                       << true << int(QNetworkReply::UnknownNetworkError);

    QTest::newRow("no-proxy-credentials") << QUrl("https://" + QtNetworkSettings::serverName())
                                          << QNetworkProxy(QNetworkProxy::HttpProxy, proxyserver, 3129)
                                          << true << int(QNetworkReply::ProxyAuthenticationRequiredError);
}

void tst_Spdy::errors()
{
    QFETCH(QUrl, url);
    QFETCH(QNetworkProxy, proxy);
    QFETCH(bool, ignoreSslErrors);
    QFETCH(int, expectedReplyError);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);

    disconnect(&m_manager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
               0, 0);
    if (proxy.type() != QNetworkProxy::DefaultProxy) {
        m_manager.setProxy(proxy);
    }
    QNetworkReply *reply = m_manager.get(request);
    if (ignoreSslErrors)
        reply->ignoreSslErrors();
    QSignalSpy finishedSpy(reply, SIGNAL(finished()));
    QSignalSpy errorSpy(reply, SIGNAL(error(QNetworkReply::NetworkError)));

    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);

    QCOMPARE(reply->error(), static_cast<QNetworkReply::NetworkError>(expectedReplyError));

    m_manager.setProxy(QNetworkProxy()); // reset
    m_manager.clearAccessCache(); // e.g. to get an SSL error we need a new connection
    connect(&m_manager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
            this, SLOT(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
            Qt::UniqueConnection); // reset
}
#endif // !QT_NO_NETWORKPROXY

void tst_Spdy::multipleRequests_data()
{
    QTest::addColumn<QList<QUrl> >("urls");

    QString baseUrl = "https://" + QtNetworkSettings::serverName() + "/qtest/cgi-bin/echo.cgi?";
    QList<QUrl> urls;
    for (int a = 1; a <= 50; ++a)
        urls.append(QUrl(baseUrl + QLatin1String(QByteArray::number(a))));

    QTest::newRow("one-request") << urls.mid(0, 1);
    QTest::newRow("two-requests") << urls.mid(0, 2);
    QTest::newRow("ten-requests") << urls.mid(0, 10);
    QTest::newRow("twenty-requests") << urls.mid(0, 20);
    QTest::newRow("fifty-requests") << urls;
}

void tst_Spdy::multipleRequestsFinishedSlot()
{
    m_multipleRepliesFinishedCount++;
    if (m_multipleRepliesFinishedCount == m_multipleRequestsCount)
        QTestEventLoop::instance().exitLoop();
}

void tst_Spdy::multipleRequests()
{
    QFETCH(QList<QUrl>, urls);
    m_multipleRequestsCount = urls.count();
    m_multipleRepliesFinishedCount = 0;

    QList<QNetworkReply *> replies;
    QList<QSignalSpy *> metaDataChangedSpies;
    QList<QSignalSpy *> readyReadSpies;
    QList<QSignalSpy *> finishedSpies;

    foreach (const QUrl &url, urls) {
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
        QNetworkReply *reply = m_manager.get(request);
        replies.append(reply);
        reply->ignoreSslErrors();
        QObject::connect(reply, SIGNAL(finished()), this, SLOT(multipleRequestsFinishedSlot()));
        QSignalSpy *metaDataChangedSpy = new QSignalSpy(reply, SIGNAL(metaDataChanged()));
        metaDataChangedSpies << metaDataChangedSpy;
        QSignalSpy *readyReadSpy = new QSignalSpy(reply, SIGNAL(readyRead()));
        readyReadSpies << readyReadSpy;
        QSignalSpy *finishedSpy = new QSignalSpy(reply, SIGNAL(finished()));
        finishedSpies << finishedSpy;
    }

    QSignalSpy finishedManagerSpy(&m_manager, SIGNAL(finished(QNetworkReply*)));

    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(finishedManagerSpy.count(), m_multipleRequestsCount);

    for (int a = 0; a < replies.count(); ++a) {

#ifndef QT_NO_OPENSSL
        QCOMPARE(replies.at(a)->sslConfiguration().nextProtocolNegotiationStatus(),
                 QSslConfiguration::NextProtocolNegotiationNegotiated);
        QCOMPARE(replies.at(a)->sslConfiguration().nextNegotiatedProtocol(),
                 QByteArray(QSslConfiguration::NextProtocolSpdy3_0));
#endif // QT_NO_OPENSSL

        QCOMPARE(replies.at(a)->error(), QNetworkReply::NoError);
        QCOMPARE(replies.at(a)->attribute(QNetworkRequest::SpdyWasUsedAttribute).toBool(), true);
        QCOMPARE(replies.at(a)->attribute(QNetworkRequest::ConnectionEncryptedAttribute).toBool(), true);
        QCOMPARE(replies.at(a)->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);

        // using the echo script, a request to "echo.cgi?1" will return a body of "1"
        QByteArray expectedContent = replies.at(a)->url().query().toUtf8();
        QByteArray content = replies.at(a)->readAll();
        QCOMPARE(expectedContent, content);

        QCOMPARE(metaDataChangedSpies.at(a)->count(), 1);
        metaDataChangedSpies.at(a)->deleteLater();

        QCOMPARE(finishedSpies.at(a)->count(), 1);
        finishedSpies.at(a)->deleteLater();

        QVERIFY(readyReadSpies.at(a)->count() > 0);
        readyReadSpies.at(a)->deleteLater();

        replies.at(a)->deleteLater();
    }
}

QTEST_MAIN(tst_Spdy)

#include "tst_spdy.moc"
