// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtNetwork/qtnetworkglobal.h>

#include <QTest>
#include <QTestEventLoop>
#include <QScopeGuard>
#include <QRandomGenerator>
#include <QSignalSpy>

#include "http2srv.h"

#include <QtNetwork/private/http2protocol_p.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qhttp2configuration.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>

#if QT_CONFIG(ssl)
#include <QtNetwork/qsslsocket.h>
#endif

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>

#include <cstdlib>
#include <memory>
#include <string>

#include <QtTest/private/qemulationdetector_p.h>

Q_DECLARE_METATYPE(H2Type)
Q_DECLARE_METATYPE(QNetworkRequest::Attribute)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
    void defaultQnamHttp2Configuration();
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
    void http2DATAFrames();

    void moreActivitySignals_data();
    void moreActivitySignals();

    void contentEncoding_data();
    void contentEncoding();

    void authenticationRequired_data();
    void authenticationRequired();

    void h2cAllowedAttribute_data();
    void h2cAllowedAttribute();

    void redirect_data();
    void redirect();

    void trailingHEADERS();

    void duplicateRequestsWithAborts();

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
    bool POSTResponseHEADOnly = true;

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

bool clearTextHTTP2 = false;

using ServerPtr = QScopedPointer<Http2Server, ServerDeleter>;

H2Type defaultConnectionType()
{
    return clearTextHTTP2 ? H2Type::h2c : H2Type::h2Alpn;
}

} // unnamed namespace

tst_Http2::tst_Http2()
    : workerThread(new QThread)
{
#if QT_CONFIG(ssl)
    const auto features = QSslSocket::supportedFeatures();
    clearTextHTTP2 = !features.contains(QSsl::SupportedFeature::ServerSideAlpn);
#else
    clearTextHTTP2 = true;
#endif
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

void tst_Http2::defaultQnamHttp2Configuration()
{
    // The configuration we also implicitly use in QNAM.
    QCOMPARE(qt_defaultH2Configuration(), QNetworkRequest().http2Configuration());
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
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", "1");
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
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
    QFETCH(const QNetworkRequest::Attribute, h2Attribute);
    request.setAttribute(h2Attribute, QVariant(true));

    auto reply = manager->get(request);
#if QT_CONFIG(ssl)
    QSignalSpy encSpy(reply, &QNetworkReply::encrypted);
#endif // QT_CONFIG(ssl)

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

#if QT_CONFIG(ssl)
    if (connectionType == H2Type::h2Alpn || connectionType == H2Type::h2Direct)
        QCOMPARE(encSpy.size(), 1);
#endif // QT_CONFIG(ssl)
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

    if (QTestPrivate::isRunningArmOnX86())
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
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
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
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
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
        request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
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
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", "1");
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
        request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
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
        request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
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
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
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
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", "1");
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
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
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
    QCOMPARE(frameCounter.size(), 1);

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);
}

void tst_Http2::http2DATAFrames()
{
    using namespace Http2;

    {
        // 0. DATA frame with payload, no padding.

        FrameWriter writer(FrameType::DATA, FrameFlag::EMPTY, 1);
        writer.append('a');
        writer.append('b');
        writer.append('c');

        const Frame frame = writer.outboundFrame();
        const auto &buffer = frame.buffer;
        // Frame's header is 9 bytes + 3 bytes of payload
        // (+ 0 bytes of padding and no padding length):
        QCOMPARE(int(buffer.size()), 12);

        QVERIFY(!frame.padding());
        QCOMPARE(int(frame.payloadSize()), 3);
        QCOMPARE(int(frame.dataSize()), 3);
        QCOMPARE(frame.dataBegin() - buffer.data(), 9);
        QCOMPARE(char(*frame.dataBegin()), 'a');
    }

    {
        // 1. DATA with padding.

        const int padLength = 10;
        FrameWriter writer(FrameType::DATA, FrameFlag::END_STREAM | FrameFlag::PADDED, 1);
        writer.append(uchar(padLength)); // The length of padding is 1 byte long.
        writer.append('a');
        for (int i = 0; i < padLength; ++i)
            writer.append('b');

        const Frame frame = writer.outboundFrame();
        const auto &buffer = frame.buffer;
        // Frame's header is 9 bytes + 1 byte for padding length
        // + 1 byte of data + 10 bytes of padding:
        QCOMPARE(int(buffer.size()), 21);

        QCOMPARE(frame.padding(), padLength);
        QCOMPARE(int(frame.payloadSize()), 12); // Includes padding, its length + data.
        QCOMPARE(int(frame.dataSize()), 1);

        // Skipping 9 bytes long header and padding length:
        QCOMPARE(frame.dataBegin() - buffer.data(), 10);

        QCOMPARE(char(frame.dataBegin()[0]), 'a');
        QCOMPARE(char(frame.dataBegin()[1]), 'b');

        QVERIFY(frame.flags().testFlag(FrameFlag::END_STREAM));
        QVERIFY(frame.flags().testFlag(FrameFlag::PADDED));
    }
    {
        // 2. DATA with PADDED flag, but 0 as padding length.

        FrameWriter writer(FrameType::DATA, FrameFlag::END_STREAM | FrameFlag::PADDED, 1);

        writer.append(uchar(0)); // Number of padding bytes is 1 byte long.
        writer.append('a');

        const Frame frame = writer.outboundFrame();
        const auto &buffer = frame.buffer;

        // Frame's header is 9 bytes + 1 byte for padding length + 1 byte of data
        // + 0 bytes of padding:
        QCOMPARE(int(buffer.size()), 11);

        QCOMPARE(frame.padding(), 0);
        QCOMPARE(int(frame.payloadSize()), 2); // Includes padding (0 bytes), its length + data.
        QCOMPARE(int(frame.dataSize()), 1);

        // Skipping 9 bytes long header and padding length:
        QCOMPARE(frame.dataBegin() - buffer.data(), 10);

        QCOMPARE(char(*frame.dataBegin()), 'a');

        QVERIFY(frame.flags().testFlag(FrameFlag::END_STREAM));
        QVERIFY(frame.flags().testFlag(FrameFlag::PADDED));
    }
}

void tst_Http2::moreActivitySignals_data()
{
    QTest::addColumn<QNetworkRequest::Attribute>("h2Attribute");
    QTest::addColumn<H2Type>("connectionType");

    QTest::addRow("h2c-upgrade")
            << QNetworkRequest::Http2AllowedAttribute << H2Type::h2c;
    QTest::addRow("h2c-direct")
            << QNetworkRequest::Http2DirectAttribute << H2Type::h2cDirect;

    if (!clearTextHTTP2)
        QTest::addRow("h2-ALPN")
                << QNetworkRequest::Http2AllowedAttribute << H2Type::h2Alpn;

#if QT_CONFIG(ssl)
    QTest::addRow("h2-direct")
            << QNetworkRequest::Http2DirectAttribute << H2Type::h2Direct;
#endif
}

void tst_Http2::moreActivitySignals()
{
    clearHTTP2State();

#if QT_CONFIG(securetransport)
    // Normally on macOS we use plain text only for SecureTransport
    // does not support ALPN on the server side. With 'direct encrytped'
    // we have to use TLS sockets (== private key) and thus suppress a
    // keychain UI asking for permission to use a private key.
    // Our CI has this, but somebody testing locally - will have a problem.
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", "1");
    auto envRollback = qScopeGuard([]() { qunsetenv("QT_SSL_USE_TEMPORARY_KEYCHAIN"); });
#endif

    serverPort = 0;
    QFETCH(H2Type, connectionType);
    ServerPtr srv(newServer(defaultServerSettings, connectionType));
    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);
    runEventLoop(100);
    QVERIFY(serverPort != 0);
    auto url = requestUrl(connectionType);
    url.setPath(QString("/stream1.html"));
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
    QFETCH(const QNetworkRequest::Attribute, h2Attribute);
    request.setAttribute(h2Attribute, QVariant(true));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
    QSharedPointer<QNetworkReply> reply(manager->get(request));
    nRequests = 1;
    connect(reply.data(), &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    QSignalSpy spy1(reply.data(), SIGNAL(socketStartedConnecting()));
    QSignalSpy spy2(reply.data(), SIGNAL(requestSent()));
    QSignalSpy spy3(reply.data(), SIGNAL(metaDataChanged()));
    // Since we're using self-signed certificates,
    // ignore SSL errors:
    reply->ignoreSslErrors();

    spy1.wait();
    spy2.wait();
    spy3.wait();

    runEventLoop();
    STOP_ON_FAILURE

    QVERIFY(nRequests == 0);
    QVERIFY(prefaceOK);
    QVERIFY(serverGotSettingsACK);

    QVERIFY(reply->error() == QNetworkReply::NoError);
    QVERIFY(reply->isFinished());
}

void tst_Http2::contentEncoding_data()
{
    QTest::addColumn<QByteArray>("encoding");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<QNetworkRequest::Attribute>("h2Attribute");
    QTest::addColumn<H2Type>("connectionType");

    struct ContentEncodingData
    {
        ContentEncodingData(QByteArray &&ce, QByteArray &&body, QByteArray &&ex)
            : contentEncoding(ce), body(body), expected(ex)
        {
        }
        QByteArray contentEncoding;
        QByteArray body;
        QByteArray expected;
    };

    QList<ContentEncodingData> contentEncodingData;
    contentEncodingData.emplace_back(
            "gzip", QByteArray::fromBase64("H4sIAAAAAAAAA8tIzcnJVyjPL8pJAQCFEUoNCwAAAA=="),
            "hello world");
    contentEncodingData.emplace_back(
            "deflate", QByteArray::fromBase64("eJzLSM3JyVcozy/KSQEAGgsEXQ=="), "hello world");

#if QT_CONFIG(brotli)
    contentEncodingData.emplace_back("br", QByteArray::fromBase64("DwWAaGVsbG8gd29ybGQD"),
                                     "hello world");
#endif

#if QT_CONFIG(zstd)
    contentEncodingData.emplace_back(
            "zstd", QByteArray::fromBase64("KLUv/QRYWQAAaGVsbG8gd29ybGRoaR6y"), "hello world");
#endif

    // Loop through and add the data...
    for (const auto &data : contentEncodingData) {
        const char *name = data.contentEncoding.data();
        QTest::addRow("%s-h2c-upgrade", name)
                << data.contentEncoding << data.body << data.expected
                << QNetworkRequest::Http2AllowedAttribute << H2Type::h2c;
        QTest::addRow("%s-h2c-direct", name)
                << data.contentEncoding << data.body << data.expected
                << QNetworkRequest::Http2DirectAttribute << H2Type::h2cDirect;

        if (!clearTextHTTP2)
            QTest::addRow("%s-h2-ALPN", name)
                    << data.contentEncoding << data.body << data.expected
                    << QNetworkRequest::Http2AllowedAttribute << H2Type::h2Alpn;

#if QT_CONFIG(ssl)
        QTest::addRow("%s-h2-direct", name)
                << data.contentEncoding << data.body << data.expected
                << QNetworkRequest::Http2DirectAttribute << H2Type::h2Direct;
#endif
    }
}

void tst_Http2::contentEncoding()
{
    clearHTTP2State();

#if QT_CONFIG(securetransport)
    // Normally on macOS we use plain text only for SecureTransport
    // does not support ALPN on the server side. With 'direct encrytped'
    // we have to use TLS sockets (== private key) and thus suppress a
    // keychain UI asking for permission to use a private key.
    // Our CI has this, but somebody testing locally - will have a problem.
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", "1");
    auto envRollback = qScopeGuard([]() { qunsetenv("QT_SSL_USE_TEMPORARY_KEYCHAIN"); });
#endif

    QFETCH(H2Type, connectionType);

    ServerPtr targetServer(newServer(defaultServerSettings, connectionType));
    QFETCH(QByteArray, body);
    targetServer->setResponseBody(body);
    QFETCH(QByteArray, encoding);
    targetServer->setContentEncoding(encoding);

    QMetaObject::invokeMethod(targetServer.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    nRequests = 1;

    auto url = requestUrl(connectionType);
    url.setPath("/index.html");

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
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
    QTEST(reply->readAll(), "expected");
}

void tst_Http2::authenticationRequired_data()
{
    QTest::addColumn<bool>("success");
    QTest::addColumn<bool>("responseHEADOnly");

    QTest::addRow("failed-auth") << false << true;
    QTest::addRow("successful-auth") << true << true;
    // Include a DATA frame in the response from the remote server. An example would be receiving a
    // JSON response on a request along with the 401 error.
    QTest::addRow("failed-auth-with-response") << false << false;
    QTest::addRow("successful-auth-with-response") << true << false;
}

void tst_Http2::authenticationRequired()
{
    clearHTTP2State();
    serverPort = 0;
    QFETCH(const bool, responseHEADOnly);
    POSTResponseHEADOnly = responseHEADOnly;

    QFETCH(const bool, success);

    ServerPtr targetServer(newServer(defaultServerSettings, defaultConnectionType()));
    targetServer->setResponseBody("Hello");
    targetServer->setAuthenticationHeader("Basic realm=\"Shadow\"");

    QMetaObject::invokeMethod(targetServer.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    nRequests = 1;

    auto url = requestUrl(defaultConnectionType());
    url.setPath("/index.html");
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);

    QByteArray expectedBody = "Hello, World!";
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QScopedPointer<QNetworkReply> reply;
    reply.reset(manager->post(request, expectedBody));

    bool authenticationRequested = false;
    connect(manager.get(), &QNetworkAccessManager::authenticationRequired, reply.get(),
            [&](QNetworkReply *, QAuthenticator *auth) {
                authenticationRequested = true;
                if (success) {
                    auth->setUser("admin");
                    auth->setPassword("admin");
                }
            });

    QByteArray receivedBody;
    connect(targetServer.get(), &Http2Server::receivedDATAFrame, reply.get(),
            [&receivedBody](quint32 streamID, const QByteArray &body) {
                if (streamID == 3) // The expected body is on the retry, so streamID == 3
                    receivedBody += body;
            });

    if (success)
        connect(reply.get(), &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    else
        connect(reply.get(), &QNetworkReply::errorOccurred, this, &tst_Http2::replyFinishedWithError);
    // Since we're using self-signed certificates,
    // ignore SSL errors:
    reply->ignoreSslErrors();

    runEventLoop();
    STOP_ON_FAILURE

    if (!success)
        QCOMPARE(reply->error(), QNetworkReply::AuthenticationRequiredError);
    // else: no error (is checked in tst_Http2::replyFinished)

    QVERIFY(authenticationRequested);

    const auto isAuthenticated = [](QByteArray bv) {
        return bv == "Basic YWRtaW46YWRtaW4="; // admin:admin
    };
    // Get the "authorization" header out from the server and make sure it's as expected:
    auto reqAuthHeader = targetServer->requestAuthorizationHeader();
    QCOMPARE(isAuthenticated(reqAuthHeader), success);
    if (success)
        QCOMPARE(receivedBody, expectedBody);
    // In the `!success` case we need to wait for the server to emit this or it might cause issues
    // in the next test running after this. In the `success` case we anyway expect it to have been
    // received.
    QTRY_VERIFY(serverGotSettingsACK);
}

void tst_Http2::h2cAllowedAttribute_data()
{
    QTest::addColumn<bool>("h2cAllowed");
    QTest::addColumn<bool>("useAttribute"); // true: use attribute, false: use environment variable
    QTest::addColumn<bool>("success");

    QTest::addRow("h2c-not-allowed") << false << false << false;
    // Use the attribute to enable/disable the H2C:
    QTest::addRow("attribute") << true << true << true;
    // Use the QT_NETWORK_H2C_ALLOWED environment variable to enable/disable the H2C:
    QTest::addRow("environment-variable") << true << false << true;
}

void tst_Http2::h2cAllowedAttribute()
{
    QFETCH(const bool, h2cAllowed);
    QFETCH(const bool, useAttribute);
    QFETCH(const bool, success);

    clearHTTP2State();
    serverPort = 0;

    ServerPtr targetServer(newServer(defaultServerSettings, H2Type::h2c));
    targetServer->setResponseBody("Hello");

    QMetaObject::invokeMethod(targetServer.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    nRequests = 1;

    auto url = requestUrl(H2Type::h2c);
    url.setPath("/index.html");
    QNetworkRequest request(url);
    if (h2cAllowed) {
        if (useAttribute)
            request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
        else
            qputenv("QT_NETWORK_H2C_ALLOWED", "1");
    }
    auto envCleanup = qScopeGuard([]() { qunsetenv("QT_NETWORK_H2C_ALLOWED"); });

    QScopedPointer<QNetworkReply> reply;
    reply.reset(manager->get(request));

    if (success)
        connect(reply.get(), &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    else
        connect(reply.get(), &QNetworkReply::errorOccurred, this, &tst_Http2::replyFinishedWithError);

    // Since we're using self-signed certificates,
    // ignore SSL errors:
    reply->ignoreSslErrors();

    runEventLoop();
    STOP_ON_FAILURE

    if (!success) {
        QCOMPARE(reply->error(), QNetworkReply::ConnectionRefusedError);
    } else {
        QCOMPARE(reply->readAll(), QByteArray("Hello"));
        QTRY_VERIFY(serverGotSettingsACK);
    }
}

void tst_Http2::redirect_data()
{
    QTest::addColumn<int>("maxRedirects");
    QTest::addColumn<int>("redirectCount");
    QTest::addColumn<bool>("success");

    QTest::addRow("1-redirects-none-allowed-failure") << 0 << 1 << false;
    QTest::addRow("1-redirects-success") << 1 << 1 << true;
    QTest::addRow("2-redirects-1-allowed-failure") << 1 << 2 << false;
}

void tst_Http2::redirect()
{
    QFETCH(const int, maxRedirects);
    QFETCH(const int, redirectCount);
    QFETCH(const bool, success);
    const QByteArray redirectUrl = "/b.html"_ba;

    clearHTTP2State();
    serverPort = 0;

    ServerPtr targetServer(newServer(defaultServerSettings, defaultConnectionType()));
    targetServer->setRedirect(redirectUrl, redirectCount);

    QMetaObject::invokeMethod(targetServer.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    nRequests = 1;

    auto originalUrl = requestUrl(defaultConnectionType());
    auto url = originalUrl;
    url.setPath("/index.html");
    QNetworkRequest request(url);
    request.setMaximumRedirectsAllowed(maxRedirects);
    // H2C might be used on macOS where SecureTransport doesn't support server-side ALPN
    qputenv("QT_NETWORK_H2C_ALLOWED", "1");
    auto envCleanup = qScopeGuard([]() { qunsetenv("QT_NETWORK_H2C_ALLOWED"); });

    QScopedPointer<QNetworkReply> reply;
    reply.reset(manager->get(request));

    if (success) {
        connect(reply.get(), &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    } else {
        connect(reply.get(), &QNetworkReply::errorOccurred, this,
                &tst_Http2::replyFinishedWithError);
    }

    // Since we're using self-signed certificates,
    // ignore SSL errors:
    reply->ignoreSslErrors();

    runEventLoop();
    STOP_ON_FAILURE

    QCOMPARE(nRequests, 0);
    if (success) {
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        QCOMPARE(reply->url().toString(),
                 originalUrl.resolved(QString::fromLatin1(redirectUrl)).toString());
    } else if (maxRedirects < redirectCount) {
        QCOMPARE(reply->error(), QNetworkReply::TooManyRedirectsError);
    }
    QTRY_VERIFY(serverGotSettingsACK);
}

void tst_Http2::trailingHEADERS()
{
    clearHTTP2State();
    serverPort = 0;

    ServerPtr targetServer(newServer(defaultServerSettings, defaultConnectionType()));
    targetServer->setSendTrailingHEADERS(true);

    QMetaObject::invokeMethod(targetServer.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    nRequests = 1;

    const auto url = requestUrl(defaultConnectionType());
    QNetworkRequest request(url);
    // H2C might be used on macOS where SecureTransport doesn't support server-side ALPN
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);

    std::unique_ptr<QNetworkReply> reply{ manager->get(request) };
    connect(reply.get(), &QNetworkReply::finished, this, &tst_Http2::replyFinished);

    // Since we're using self-signed certificates, ignore SSL errors:
    reply->ignoreSslErrors();

    runEventLoop();
    STOP_ON_FAILURE

    QCOMPARE(nRequests, 0);

    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QTRY_VERIFY(serverGotSettingsACK);
}

void tst_Http2::duplicateRequestsWithAborts()
{
    clearHTTP2State();
    serverPort = 0;

    ServerPtr targetServer(newServer(defaultServerSettings, defaultConnectionType()));

    QMetaObject::invokeMethod(targetServer.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    constexpr int ExpectedSuccessfulRequests = 1;
    nRequests = ExpectedSuccessfulRequests;

    const auto url = requestUrl(defaultConnectionType());
    QNetworkRequest request(url);
    // H2C might be used on macOS where SecureTransport doesn't support server-side ALPN
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);

    qint32 finishedCount = 0;
    auto connectToSlots = [this, &finishedCount](QNetworkReply *reply){
        const auto onFinished = [&finishedCount, reply, this]() {
            ++finishedCount;
            if (reply->error() == QNetworkReply::NoError)
                replyFinished();
        };
        connect(reply, &QNetworkReply::finished, reply, onFinished);
    };

    std::vector<QNetworkReply *> replies;
    for (qint32 i = 0; i < 3; ++i) {
        auto &reply = replies.emplace_back(manager->get(request));
        connectToSlots(reply);
        if (i < 2) // Delete and abort all-but-one:
            reply->deleteLater();
        // Since we're using self-signed certificates, ignore SSL errors:
        reply->ignoreSslErrors();
    }

    runEventLoop();
    STOP_ON_FAILURE

    QCOMPARE(nRequests, 0);
    QCOMPARE(finishedCount, ExpectedSuccessfulRequests);
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
    POSTResponseHEADOnly = true;
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
    request.setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, QVariant(true));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
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
    Q_UNUSED(streamID);
}

void tst_Http2::decompressionFailed(quint32 streamID)
{
    Q_UNUSED(streamID);
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
                              Q_ARG(bool, POSTResponseHEADOnly /*true = HEADERS only*/));
}

void tst_Http2::windowUpdated(quint32 streamID)
{
    Q_UNUSED(streamID);

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
