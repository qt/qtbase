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

#include <QtTest/QtTest>

#include "http2srv.h"

#include <QtNetwork/private/http2protocol_p.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qhttp2configuration.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>

#ifndef QT_NO_SSL
#ifndef QT_NO_OPENSSL
#include <QtNetwork/private/qsslsocket_openssl_symbols_p.h>
#endif // NO_OPENSSL
#endif // NO_SSL

#include <cstdlib>
#include <memory>
#include <string>

#include "emulationdetector.h"

#if (!defined(QT_NO_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10002000L && !defined(OPENSSL_NO_TLSEXT)) \
        || QT_CONFIG(schannel)
// HTTP/2 over TLS requires ALPN/NPN to negotiate the protocol version.
const bool clearTextHTTP2 = false;
#else
// No ALPN/NPN support to negotiate HTTP/2, we'll use cleartext 'h2c' with
// a protocol upgrade procedure.
const bool clearTextHTTP2 = true;
#endif

Q_DECLARE_METATYPE(H2Type)
Q_DECLARE_METATYPE(QNetworkRequest::Attribute)

QT_BEGIN_NAMESPACE

QHttp2Configuration qt_defaultH2Configuration()
{
    QHttp2Configuration config;
    config.setStreamReceiveWindowSize(Http2::qtDefaultStreamReceiveWindowSize);
    config.setSessionReceiveWindowSize(Http2::maxSessionReceiveWindowSize);
    config.setServerPushEnabled(false);
    return config;
}

RawSettings qt_H2ConfigurationToSettings(const QHttp2Configuration &config = qt_defaultH2Configuration())
{
    RawSettings settings;
    settings[Http2::Settings::ENABLE_PUSH_ID] = config.serverPushEnabled();
    settings[Http2::Settings::INITIAL_WINDOW_SIZE_ID] = config.streamReceiveWindowSize();
    if (config.maxFrameSize() != Http2::minPayloadLimit)
        settings[Http2::Settings::MAX_FRAME_SIZE_ID] = config.maxFrameSize();
    return settings;
}


class tst_Http2 : public QObject
{
    Q_OBJECT
public:
    tst_Http2();
    ~tst_Http2();
public slots:
    void init();
private slots:
    // Tests:
    void singleRequest_data();
    void singleRequest();
    void multipleRequests();
    void flowControlClientSide();
    void flowControlServerSide();
    void pushPromise();
    void goaway_data();
    void goaway();
    void earlyResponse();
    void connectToHost_data();
    void connectToHost();
    void maxFrameSize();

protected slots:
    // Slots to listen to our in-process server:
    void serverStarted(quint16 port);
    void clientPrefaceOK();
    void clientPrefaceError();
    void serverSettingsAcked();
    void invalidFrame();
    void invalidRequest(quint32 streamID);
    void decompressionFailed(quint32 streamID);
    void receivedRequest(quint32 streamID);
    void receivedData(quint32 streamID);
    void windowUpdated(quint32 streamID);
    void replyFinished();
    void replyFinishedWithError();

private:
    void clearHTTP2State();
    // Run event for 'ms' milliseconds.
    // The default value '5000' is enough for
    // small payload.
    void runEventLoop(int ms = 5000);
    void stopEventLoop();
    Http2Server *newServer(const RawSettings &serverSettings, H2Type connectionType,
                           const RawSettings &clientSettings = qt_H2ConfigurationToSettings());
    // Send a get or post request, depending on a payload (empty or not).
    void sendRequest(int streamNumber,
                     QNetworkRequest::Priority priority = QNetworkRequest::NormalPriority,
                     const QByteArray &payload = QByteArray(),
                     const QHttp2Configuration &clientConfiguration = qt_defaultH2Configuration());
    QUrl requestUrl(H2Type connnectionType) const;

    quint16 serverPort = 0;
    QThread *workerThread = nullptr;
    std::unique_ptr<QNetworkAccessManager> manager;

    QTestEventLoop eventLoop;

    int nRequests = 0;
    int nSentRequests = 0;

    int windowUpdates = 0;
    bool prefaceOK = false;
    bool serverGotSettingsACK = false;

    static const RawSettings defaultServerSettings;
};

#define STOP_ON_FAILURE \
    if (QTest::currentTestFailed()) \
        return;

const RawSettings tst_Http2::defaultServerSettings{{Http2::Settings::MAX_CONCURRENT_STREAMS_ID, 100}};

namespace {

// Our server lives/works on a different thread so we invoke its 'deleteLater'
// instead of simple 'delete'.
struct ServerDeleter
{
    static void cleanup(Http2Server *srv)
    {
        if (srv) {
            srv->stopSendingDATAFrames();
            QMetaObject::invokeMethod(srv, "deleteLater", Qt::QueuedConnection);
        }
    }
};

using ServerPtr = QScopedPointer<Http2Server, ServerDeleter>;

H2Type defaultConnectionType()
{
    return clearTextHTTP2 ? H2Type::h2c : H2Type::h2Alpn;
}

} // unnamed namespace

tst_Http2::tst_Http2()
    : workerThread(new QThread)
{
    workerThread->start();
}

tst_Http2::~tst_Http2()
{
    workerThread->quit();
    workerThread->wait(5000);

    if (workerThread->isFinished()) {
        delete workerThread;
    } else {
        connect(workerThread, &QThread::finished,
                workerThread, &QThread::deleteLater);
    }
}

void tst_Http2::init()
{
    manager.reset(new QNetworkAccessManager);
}

void tst_Http2::singleRequest_data()
{
    QTest::addColumn<QNetworkRequest::Attribute>("h2Attribute");
    QTest::addColumn<H2Type>("connectionType");

    // 'Clear text' that should always work, either via the protocol upgrade
    // or as direct.
    QTest::addRow("h2c-upgrade") << QNetworkRequest::Http2AllowedAttribute << H2Type::h2c;
    QTest::addRow("h2c-direct") << QNetworkRequest::Http2DirectAttribute << H2Type::h2cDirect;

    if (!clearTextHTTP2) {
        // Qt with TLS where TLS-backend supports ALPN.
        QTest::addRow("h2-ALPN") << QNetworkRequest::Http2AllowedAttribute << H2Type::h2Alpn;
    }

#if QT_CONFIG(ssl)
    QTest::addRow("h2-direct") << QNetworkRequest::Http2DirectAttribute << H2Type::h2Direct;
#endif
}

void tst_Http2::singleRequest()
{
    clearHTTP2State();

#if QT_CONFIG(securetransport)
    // Normally on macOS we use plain text only for SecureTransport
    // does not support ALPN on the server side. With 'direct encrytped'
    // we have to use TLS sockets (== private key) and thus suppress a
    // keychain UI asking for permission to use a private key.
    // Our CI has this, but somebody testing locally - will have a problem.
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", QByteArray("1"));
    auto envRollback = qScopeGuard([](){
        qunsetenv("QT_SSL_USE_TEMPORARY_KEYCHAIN");
    });
#endif

    serverPort = 0;
    nRequests = 1;

    QFETCH(const H2Type, connectionType);
    ServerPtr srv(newServer(defaultServerSettings, connectionType));

    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    auto url = requestUrl(connectionType);
    url.setPath("/index.html");

    QNetworkRequest request(url);
    QFETCH(const QNetworkRequest::Attribute, h2Attribute);
    request.setAttribute(h2Attribute, QVariant(true));

    auto reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    // Since we're using self-signed certificates,
    // ignore SSL errors:
    reply->ignoreSslErrors();

    runEventLoop();
    STOP_ON_FAILURE

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);

    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QVERIFY(reply->isFinished());
}

void tst_Http2::multipleRequests()
{
    clearHTTP2State();

    serverPort = 0;
    nRequests = 10;

    ServerPtr srv(newServer(defaultServerSettings, defaultConnectionType()));

    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);

    runEventLoop();
    QVERIFY(serverPort != 0);

    // Just to make the order a bit more interesting
    // we'll index this randomly:
    const QNetworkRequest::Priority priorities[] = {
        QNetworkRequest::HighPriority,
        QNetworkRequest::NormalPriority,
        QNetworkRequest::LowPriority
    };

    for (int i = 0; i < nRequests; ++i)
        sendRequest(i, priorities[QRandomGenerator::global()->bounded(3)]);

    runEventLoop();
    STOP_ON_FAILURE

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);
}

void tst_Http2::flowControlClientSide()
{
    // Create a server but impose limits:
    // 1. Small client receive windows so server's responses cause client
    //    streams to suspend and protocol handler has to send WINDOW_UPDATE
    //    frames.
    // 2. Few concurrent streams supported by the server, to test protocol
    //    handler in the client can suspend and then resume streams.
    using namespace Http2;

    clearHTTP2State();

    serverPort = 0;
    nRequests = 10;
    windowUpdates = 0;

    QHttp2Configuration params;
    // A small window size for a session, and even a smaller one per stream -
    // this will result in WINDOW_UPDATE frames both on connection stream and
    // per stream.
    params.setSessionReceiveWindowSize(Http2::defaultSessionWindowSize * 5);
    params.setStreamReceiveWindowSize(Http2::defaultSessionWindowSize);

    const RawSettings serverSettings = {{Settings::MAX_CONCURRENT_STREAMS_ID, quint32(3)}};
    ServerPtr srv(newServer(serverSettings, defaultConnectionType(), qt_H2ConfigurationToSettings(params)));

    const QByteArray respond(int(Http2::defaultSessionWindowSize * 10), 'x');
    srv->setResponseBody(respond);

    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);

    runEventLoop();
    QVERIFY(serverPort != 0);

    for (int i = 0; i < nRequests; ++i)
        sendRequest(i, QNetworkRequest::NormalPriority, {}, params);

    runEventLoop(120000);
    STOP_ON_FAILURE

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);
    QVERIFY(windowUpdates > 0);
}

void tst_Http2::flowControlServerSide()
{
    // Quite aggressive test:
    // low MAX_FRAME_SIZE forces a lot of small DATA frames,
    // payload size exceedes stream/session RECV window sizes
    // so that our implementation should deal with WINDOW_UPDATE
    // on a session/stream level correctly + resume/suspend streams
    // to let all replies finish without any error.
    using namespace Http2;

    if (EmulationDetector::isRunningArmOnX86())
        QSKIP("Test is too slow to run on emulator");

    clearHTTP2State();

    serverPort = 0;
    nRequests = 10;

    const RawSettings serverSettings = {{Settings::MAX_CONCURRENT_STREAMS_ID, 7}};

    ServerPtr srv(newServer(serverSettings, defaultConnectionType()));

    const QByteArray payload(int(Http2::defaultSessionWindowSize * 500), 'x');

    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);

    runEventLoop();
    QVERIFY(serverPort != 0);

    for (int i = 0; i < nRequests; ++i)
        sendRequest(i, QNetworkRequest::NormalPriority, payload);

    runEventLoop(120000);
    STOP_ON_FAILURE

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);
}

void tst_Http2::pushPromise()
{
    // We will first send some request, the server should reply and also emulate
    // PUSH_PROMISE sending us another response as promised.
    using namespace Http2;

    clearHTTP2State();

    serverPort = 0;
    nRequests = 1;

    QHttp2Configuration params;
    // Defaults are good, except ENABLE_PUSH:
    params.setServerPushEnabled(true);

    ServerPtr srv(newServer(defaultServerSettings, defaultConnectionType(), qt_H2ConfigurationToSettings(params)));
    srv->enablePushPromise(true, QByteArray("/script.js"));

    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    auto url = requestUrl(defaultConnectionType());
    url.setPath("/index.html");

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, QVariant(true));
    request.setHttp2Configuration(params);

    auto reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    // Since we're using self-signed certificates, ignore SSL errors:
    reply->ignoreSslErrors();

    runEventLoop();
    STOP_ON_FAILURE

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);

    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QVERIFY(reply->isFinished());

    // Now, the most interesting part!
    nSentRequests = 0;
    nRequests = 1;
    // Create an additional request (let's say, we parsed reply and realized we
    // need another resource):

    url.setPath("/script.js");
    QNetworkRequest promisedRequest(url);
    promisedRequest.setAttribute(QNetworkRequest::Http2AllowedAttribute, QVariant(true));
    reply = manager->get(promisedRequest);
    connect(reply, &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    reply->ignoreSslErrors();

    runEventLoop();

    // Let's check that NO request was actually made:
    QCOMPARE(nSentRequests, 0);
    // Decreased by replyFinished():
    QCOMPARE(nRequests, 0);
    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QVERIFY(reply->isFinished());
}

void tst_Http2::goaway_data()
{
    // For now we test only basic things in two very simple scenarios:
    // - server sends GOAWAY immediately or
    // - server waits for some time (enough for ur to init several streams on a
    // client side); then suddenly it replies with GOAWAY, never processing any
    // request.
    if (clearTextHTTP2)
        QSKIP("This test requires TLS with ALPN to work");

    QTest::addColumn<int>("responseTimeoutMS");
    QTest::newRow("ImmediateGOAWAY") << 0;
    QTest::newRow("DelayedGOAWAY") << 1000;
}

void tst_Http2::goaway()
{
    using namespace Http2;

    QFETCH(const int, responseTimeoutMS);

    clearHTTP2State();

    serverPort = 0;
    nRequests = 3;

    ServerPtr srv(newServer(defaultServerSettings, defaultConnectionType()));
    srv->emulateGOAWAY(responseTimeoutMS);
    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    auto url = requestUrl(defaultConnectionType());
    // We have to store these replies, so that we can check errors later.
    std::vector<QNetworkReply *> replies(nRequests);
    for (int i = 0; i < nRequests; ++i) {
        url.setPath(QString("/%1").arg(i));
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, QVariant(true));
        replies[i] = manager->get(request);
        QCOMPARE(replies[i]->error(), QNetworkReply::NoError);
        connect(replies[i], &QNetworkReply::errorOccurred, this, &tst_Http2::replyFinishedWithError);
        // Since we're using self-signed certificates, ignore SSL errors:
        replies[i]->ignoreSslErrors();
    }

    runEventLoop(5000 + responseTimeoutMS);
    STOP_ON_FAILURE

    // No request processed, no 'replyFinished' slot calls:
    QCOMPARE(nRequests, 0);
    // Our server did not bother to send anything except a single GOAWAY frame:
    QVERIFY(!prefaceOK);
    QVERIFY(!serverGotSettingsACK);
}

void tst_Http2::earlyResponse()
{
    // In this test we'd like to verify client side can handle HEADERS frame while
    // its stream is in 'open' state. To achieve this, we send a POST request
    // with some payload, so that the client is first sending HEADERS and then
    // DATA frames without END_STREAM flag set yet (thus the stream is in Stream::open
    // state). Upon receiving the client's HEADERS frame our server ('redirector')
    // immediately (without trying to read any DATA frames) responds with status
    // code 308. The client should properly handle this.

    clearHTTP2State();

    serverPort = 0;
    nRequests = 1;

    ServerPtr targetServer(newServer(defaultServerSettings, defaultConnectionType()));

    QMetaObject::invokeMethod(targetServer.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    const quint16 targetPort = serverPort;
    serverPort = 0;

    ServerPtr redirector(newServer(defaultServerSettings, defaultConnectionType()));
    redirector->redirectOpenStream(targetPort);

    QMetaObject::invokeMethod(redirector.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort);
    sendRequest(1, QNetworkRequest::NormalPriority, {1000000, Qt::Uninitialized});

    runEventLoop();
    STOP_ON_FAILURE

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);
}

void tst_Http2::connectToHost_data()
{
    // The attribute to set on a new request:
    QTest::addColumn<QNetworkRequest::Attribute>("requestAttribute");
    // The corresponding (to the attribute above) connection type the
    // server will use:
    QTest::addColumn<H2Type>("connectionType");

#if QT_CONFIG(ssl)
    QTest::addRow("encrypted-h2-direct") << QNetworkRequest::Http2DirectAttribute << H2Type::h2Direct;
    if (!clearTextHTTP2)
        QTest::addRow("encrypted-h2-ALPN") << QNetworkRequest::Http2AllowedAttribute << H2Type::h2Alpn;
#endif // QT_CONFIG(ssl)
    // This works for all configurations, tests 'preconnect-http' scheme:
    // h2 with protocol upgrade is not working for now (the logic is a bit
    // complicated there ...).
    QTest::addRow("h2-direct") << QNetworkRequest::Http2DirectAttribute << H2Type::h2cDirect;
}

void tst_Http2::connectToHost()
{
    // QNetworkAccessManager::connectToHostEncrypted() and connectToHost()
    // creates a special request with 'preconnect-https' or 'preconnect-http'
    // schemes. At the level of the protocol handler we are supposed to report
    // these requests as finished and wait for the real requests. This test will
    // connect to a server with the first reply 'finished' signal meaning we
    // indeed connected. At this point we check that a client preface was not
    // sent yet, and no response received. Then we send the second (the real)
    // request and do our usual checks. Since our server closes its listening
    // socket on the first incoming connection (would not accept a new one),
    // the successful completion of the second requests also means we were able
    // to find a cached connection and re-use it.

    QFETCH(const QNetworkRequest::Attribute, requestAttribute);
    QFETCH(const H2Type, connectionType);

    clearHTTP2State();

    serverPort = 0;
    nRequests = 2;

    ServerPtr targetServer(newServer(defaultServerSettings, connectionType));

#if QT_CONFIG(ssl)
    Q_ASSERT(!clearTextHTTP2 || connectionType != H2Type::h2Alpn);

#if QT_CONFIG(securetransport)
    // Normally on macOS we use plain text only for SecureTransport
    // does not support ALPN on the server side. With 'direct encrytped'
    // we have to use TLS sockets (== private key) and thus suppress a
    // keychain UI asking for permission to use a private key.
    // Our CI has this, but somebody testing locally - will have a problem.
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", QByteArray("1"));
    auto envRollback = qScopeGuard([](){
        qunsetenv("QT_SSL_USE_TEMPORARY_KEYCHAIN");
    });
#endif // QT_CONFIG(securetransport)

#else
    Q_ASSERT(connectionType == H2Type::h2c || connectionType == H2Type::h2cDirect);
    Q_ASSERT(targetServer->isClearText());
#endif // QT_CONFIG(ssl)

    QMetaObject::invokeMethod(targetServer.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    auto url = requestUrl(connectionType);
    url.setPath("/index.html");

    QNetworkReply *reply = nullptr;
    // Here some mess with how we create this first reply:
#if QT_CONFIG(ssl)
    if (!targetServer->isClearText()) {
        // Let's emulate what QNetworkAccessManager::connectToHostEncrypted() does.
        // Alas, we cannot use it directly, since it does not return the reply and
        // also does not know the difference between H2 with ALPN or direct.
        auto copyUrl = url;
        copyUrl.setScheme(QLatin1String("preconnect-https"));
        QNetworkRequest request(copyUrl);
        request.setAttribute(requestAttribute, true);
        reply = manager->get(request);
        // Since we're using self-signed certificates, ignore SSL errors:
        reply->ignoreSslErrors();
    } else
#endif  // QT_CONFIG(ssl)
    {
        // Emulating what QNetworkAccessManager::connectToHost() does with
        // additional information that it cannot provide (the attribute).
        auto copyUrl = url;
        copyUrl.setScheme(QLatin1String("preconnect-http"));
        QNetworkRequest request(copyUrl);
        request.setAttribute(requestAttribute, true);
        reply = manager->get(request);
    }

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        --nRequests;
        eventLoop.exitLoop();
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        QVERIFY(reply->isFinished());
        // Nothing received back:
        QVERIFY(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isNull());
        QCOMPARE(reply->readAll().size(), 0);
    });

    runEventLoop();
    STOP_ON_FAILURE

    QCOMPARE(nRequests, 1);

    QNetworkRequest request(url);
    request.setAttribute(requestAttribute, QVariant(true));
    reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    // Note, unlike the first request, when the connection is ecnrytped, we
    // do not ignore TLS errors on this reply - we should re-use existing
    // connection, there TLS errors were already ignored.

    runEventLoop();
    STOP_ON_FAILURE

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);

    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QVERIFY(reply->isFinished());
}

void tst_Http2::maxFrameSize()
{
#if !QT_CONFIG(ssl)
    QSKIP("TLS support is needed for this test");
#endif // QT_CONFIG(ssl)

    // Here we test we send 'MAX_FRAME_SIZE' setting in our
    // 'SETTINGS'. If done properly, our server will not chunk
    // the payload into several DATA frames.

#if QT_CONFIG(securetransport)
    // Normally on macOS we use plain text only for SecureTransport
    // does not support ALPN on the server side. With 'direct encrytped'
    // we have to use TLS sockets (== private key) and thus suppress a
    // keychain UI asking for permission to use a private key.
    // Our CI has this, but somebody testing locally - will have a problem.
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", QByteArray("1"));
    auto envRollback = qScopeGuard([](){
        qunsetenv("QT_SSL_USE_TEMPORARY_KEYCHAIN");
    });
#endif // QT_CONFIG(securetransport)

    auto connectionType = H2Type::h2Alpn;
    auto attribute = QNetworkRequest::Http2AllowedAttribute;
    if (clearTextHTTP2) {
        connectionType = H2Type::h2Direct;
        attribute = QNetworkRequest::Http2DirectAttribute;
    }

    auto h2Config = qt_defaultH2Configuration();
    h2Config.setMaxFrameSize(Http2::minPayloadLimit * 3);

    serverPort = 0;
    nRequests = 1;

    ServerPtr srv(newServer(defaultServerSettings, connectionType,
                            qt_H2ConfigurationToSettings(h2Config)));
    srv->setResponseBody(QByteArray(Http2::minPayloadLimit * 2, 'q'));
    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();
    QVERIFY(serverPort != 0);

    const QSignalSpy frameCounter(srv.data(), &Http2Server::sendingData);
    auto url = requestUrl(connectionType);
    url.setPath(QString("/stream1.html"));

    QNetworkRequest request(url);
    request.setAttribute(attribute, QVariant(true));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
    request.setHttp2Configuration(h2Config);

    QNetworkReply *reply = manager->get(request);
    reply->ignoreSslErrors();
    connect(reply, &QNetworkReply::finished, this, &tst_Http2::replyFinished);

    runEventLoop();
    STOP_ON_FAILURE

    // Normally, with a 16kb limit, our server would split such
    // a response into 3 'DATA' frames (16kb + 16kb + 0|END_STREAM).
    QCOMPARE(frameCounter.count(), 1);

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);
}

void tst_Http2::serverStarted(quint16 port)
{
    serverPort = port;
    stopEventLoop();
}

void tst_Http2::clearHTTP2State()
{
    windowUpdates = 0;
    prefaceOK = false;
    serverGotSettingsACK = false;
}

void tst_Http2::runEventLoop(int ms)
{
    eventLoop.enterLoopMSecs(ms);
}

void tst_Http2::stopEventLoop()
{
    eventLoop.exitLoop();
}

Http2Server *tst_Http2::newServer(const RawSettings &serverSettings, H2Type connectionType,
                                  const RawSettings &clientSettings)
{
    using namespace Http2;
    auto srv = new Http2Server(connectionType, serverSettings, clientSettings);

    using Srv = Http2Server;
    using Cl = tst_Http2;

    connect(srv, &Srv::serverStarted, this, &Cl::serverStarted);
    connect(srv, &Srv::clientPrefaceOK, this, &Cl::clientPrefaceOK);
    connect(srv, &Srv::clientPrefaceError, this, &Cl::clientPrefaceError);
    connect(srv, &Srv::serverSettingsAcked, this, &Cl::serverSettingsAcked);
    connect(srv, &Srv::invalidFrame, this, &Cl::invalidFrame);
    connect(srv, &Srv::invalidRequest, this, &Cl::invalidRequest);
    connect(srv, &Srv::receivedRequest, this, &Cl::receivedRequest);
    connect(srv, &Srv::receivedData, this, &Cl::receivedData);
    connect(srv, &Srv::windowUpdate, this, &Cl::windowUpdated);

    srv->moveToThread(workerThread);

    return srv;
}

void tst_Http2::sendRequest(int streamNumber,
                            QNetworkRequest::Priority priority,
                            const QByteArray &payload,
                            const QHttp2Configuration &h2Config)
{
    auto url = requestUrl(defaultConnectionType());
    url.setPath(QString("/stream%1.html").arg(streamNumber));

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, QVariant(true));
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, QVariant(true));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
    request.setPriority(priority);
    request.setHttp2Configuration(h2Config);

    QNetworkReply *reply = nullptr;
    if (payload.size())
        reply = manager->post(request, payload);
    else
        reply = manager->get(request);

    reply->ignoreSslErrors();
    connect(reply, &QNetworkReply::finished, this, &tst_Http2::replyFinished);
}

QUrl tst_Http2::requestUrl(H2Type connectionType) const
{
#if !QT_CONFIG(ssl)
    Q_ASSERT(connectionType != H2Type::h2Alpn && connectionType != H2Type::h2Direct);
#endif
    static auto url = QUrl(QLatin1String(clearTextHTTP2 ? "http://127.0.0.1" : "https://127.0.0.1"));
    url.setPort(serverPort);
    // Clear text may mean no-TLS-at-all or crappy-TLS-without-ALPN.
    switch (connectionType) {
    case H2Type::h2Alpn:
    case H2Type::h2Direct:
        url.setScheme(QStringLiteral("https"));
        break;
    case H2Type::h2c:
    case H2Type::h2cDirect:
        url.setScheme(QStringLiteral("http"));
        break;
    }

    return url;
}

void tst_Http2::clientPrefaceOK()
{
    prefaceOK = true;
}

void tst_Http2::clientPrefaceError()
{
    prefaceOK = false;
}

void tst_Http2::serverSettingsAcked()
{
    serverGotSettingsACK = true;
    if (!nRequests)
        stopEventLoop();
}

void tst_Http2::invalidFrame()
{
}

void tst_Http2::invalidRequest(quint32 streamID)
{
    Q_UNUSED(streamID)
}

void tst_Http2::decompressionFailed(quint32 streamID)
{
    Q_UNUSED(streamID)
}

void tst_Http2::receivedRequest(quint32 streamID)
{
    ++nSentRequests;
    qDebug() << "   server got a request on stream" << streamID;
    Http2Server *srv = qobject_cast<Http2Server *>(sender());
    Q_ASSERT(srv);
    QMetaObject::invokeMethod(srv, "sendResponse", Qt::QueuedConnection,
                              Q_ARG(quint32, streamID),
                              Q_ARG(bool, false /*non-empty body*/));
}

void tst_Http2::receivedData(quint32 streamID)
{
    qDebug() << "   server got a 'POST' request on stream" << streamID;
    Http2Server *srv = qobject_cast<Http2Server *>(sender());
    Q_ASSERT(srv);
    QMetaObject::invokeMethod(srv, "sendResponse", Qt::QueuedConnection,
                              Q_ARG(quint32, streamID),
                              Q_ARG(bool, true /*HEADERS only*/));
}

void tst_Http2::windowUpdated(quint32 streamID)
{
    Q_UNUSED(streamID)

    ++windowUpdates;
}

void tst_Http2::replyFinished()
{
    QVERIFY(nRequests);

    if (const auto reply = qobject_cast<QNetworkReply *>(sender())) {
        if (reply->error() != QNetworkReply::NoError)
            stopEventLoop();

        QCOMPARE(reply->error(), QNetworkReply::NoError);

        const QVariant http2Used(reply->attribute(QNetworkRequest::Http2WasUsedAttribute));
        if (!http2Used.isValid() || !http2Used.toBool())
            stopEventLoop();

        QVERIFY(http2Used.isValid());
        QVERIFY(http2Used.toBool());

        const QVariant spdyUsed(reply->attribute(QNetworkRequest::SpdyWasUsedAttribute));
        if (!spdyUsed.isValid() || spdyUsed.toBool())
            stopEventLoop();

        QVERIFY(spdyUsed.isValid());
        QVERIFY(!spdyUsed.toBool());

        const QVariant code(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute));
        if (!code.isValid() || !code.canConvert<int>() || code.value<int>() != 200)
            stopEventLoop();

        QVERIFY(code.isValid());
        QVERIFY(code.canConvert<int>());
        QCOMPARE(code.value<int>(), 200);
    }

    --nRequests;
    if (!nRequests && serverGotSettingsACK)
        stopEventLoop();
}

void tst_Http2::replyFinishedWithError()
{
    QVERIFY(nRequests);

    if (const auto reply = qobject_cast<QNetworkReply *>(sender())) {
        // For now this is a 'generic' code, it just verifies some error was
        // reported without testing its type.
        if (reply->error() == QNetworkReply::NoError)
            stopEventLoop();
        QVERIFY(reply->error() != QNetworkReply::NoError);
    }

    --nRequests;
    if (!nRequests)
        stopEventLoop();
}

QT_END_NAMESPACE

QTEST_MAIN(tst_Http2)

#include "tst_http2.moc"
