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
#include <string>

#if !defined(QT_NO_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10002000L && !defined(OPENSSL_NO_TLSEXT)
// HTTP/2 over TLS requires ALPN/NPN to negotiate the protocol version.
const bool clearTextHTTP2 = false;
#else
// No ALPN/NPN support to negotiate HTTP/2, we'll use cleartext 'h2c' with
// a protocol upgrade procedure.
const bool clearTextHTTP2 = true;
#endif

QT_BEGIN_NAMESPACE

class tst_Http2 : public QObject
{
    Q_OBJECT
public:
    tst_Http2();
    ~tst_Http2();
private slots:
    // Tests:
    void singleRequest();
    void multipleRequests();
    void flowControlClientSide();
    void flowControlServerSide();
    void pushPromise();
    void goaway_data();
    void goaway();
    void earlyResponse();

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
    Http2Server *newServer(const Http2::RawSettings &serverSettings,
                           const Http2::ProtocolParameters &clientSettings = {});
    // Send a get or post request, depending on a payload (empty or not).
    void sendRequest(int streamNumber,
                     QNetworkRequest::Priority priority = QNetworkRequest::NormalPriority,
                     const QByteArray &payload = QByteArray());
    QUrl requestUrl() const;

    quint16 serverPort = 0;
    QThread *workerThread = nullptr;
    QNetworkAccessManager manager;

    QEventLoop eventLoop;
    QTimer timer;

    int nRequests = 0;
    int nSentRequests = 0;

    int windowUpdates = 0;
    bool prefaceOK = false;
    bool serverGotSettingsACK = false;

    static const Http2::RawSettings defaultServerSettings;
};

const Http2::RawSettings tst_Http2::defaultServerSettings{{Http2::Settings::MAX_CONCURRENT_STREAMS_ID, 100}};

namespace {

// Our server lives/works on a different thread so we invoke its 'deleteLater'
// instead of simple 'delete'.
struct ServerDeleter
{
    static void cleanup(Http2Server *srv)
    {
        if (srv)
            QMetaObject::invokeMethod(srv, "deleteLater", Qt::QueuedConnection);
    }
};

using ServerPtr = QScopedPointer<Http2Server, ServerDeleter>;

} // unnamed namespace

tst_Http2::tst_Http2()
    : workerThread(new QThread)
{
    workerThread->start();

    timer.setInterval(10000);
    timer.setSingleShot(true);

    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
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

void tst_Http2::singleRequest()
{
    clearHTTP2State();

    serverPort = 0;
    nRequests = 1;

    ServerPtr srv(newServer(defaultServerSettings));

    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    auto url = requestUrl();
    url.setPath("/index.html");

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, QVariant(true));

    auto reply = manager.get(request);
    connect(reply, &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    // Since we're using self-signed certificates,
    // ignore SSL errors:
    reply->ignoreSslErrors();

    runEventLoop();

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

    ServerPtr srv(newServer(defaultServerSettings));

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

    Http2::ProtocolParameters params;
    // A small window size for a session, and even a smaller one per stream -
    // this will result in WINDOW_UPDATE frames both on connection stream and
    // per stream.
    params.maxSessionReceiveWindowSize = Http2::defaultSessionWindowSize * 5;
    params.settingsFrameData[Settings::INITIAL_WINDOW_SIZE_ID] = Http2::defaultSessionWindowSize;
    // Inform our manager about non-default settings:
    manager.setProperty(Http2::http2ParametersPropertyName, QVariant::fromValue(params));

    const Http2::RawSettings serverSettings = {{Settings::MAX_CONCURRENT_STREAMS_ID, quint32(3)}};
    ServerPtr srv(newServer(serverSettings, params));


    const QByteArray respond(int(Http2::defaultSessionWindowSize * 10), 'x');
    srv->setResponseBody(respond);

    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);

    runEventLoop();
    QVERIFY(serverPort != 0);

    for (int i = 0; i < nRequests; ++i)
        sendRequest(i);

    runEventLoop(120000);

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

    clearHTTP2State();

    serverPort = 0;
    nRequests = 30;

    const Http2::RawSettings serverSettings = {{Settings::MAX_CONCURRENT_STREAMS_ID, 7}};

    ServerPtr srv(newServer(serverSettings));

    const QByteArray payload(int(Http2::defaultSessionWindowSize * 500), 'x');

    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);

    runEventLoop();
    QVERIFY(serverPort != 0);

    for (int i = 0; i < nRequests; ++i)
        sendRequest(i, QNetworkRequest::NormalPriority, payload);

    runEventLoop(120000);

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

    Http2::ProtocolParameters params;
    // Defaults are good, except ENABLE_PUSH:
    params.settingsFrameData[Settings::ENABLE_PUSH_ID] = 1;
    manager.setProperty(Http2::http2ParametersPropertyName, QVariant::fromValue(params));

    ServerPtr srv(newServer(defaultServerSettings, params));
    srv->enablePushPromise(true, QByteArray("/script.js"));

    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    auto url = requestUrl();
    url.setPath("/index.html");

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, QVariant(true));

    auto reply = manager.get(request);
    connect(reply, &QNetworkReply::finished, this, &tst_Http2::replyFinished);
    // Since we're using self-signed certificates, ignore SSL errors:
    reply->ignoreSslErrors();

    runEventLoop();

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
    promisedRequest.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, QVariant(true));
    reply = manager.get(promisedRequest);
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

    ServerPtr srv(newServer(defaultServerSettings));
    srv->emulateGOAWAY(responseTimeoutMS);
    QMetaObject::invokeMethod(srv.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    auto url = requestUrl();
    // We have to store these replies, so that we can check errors later.
    std::vector<QNetworkReply *> replies(nRequests);
    for (int i = 0; i < nRequests; ++i) {
        url.setPath(QString("/%1").arg(i));
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, QVariant(true));
        replies[i] = manager.get(request);
        QCOMPARE(replies[i]->error(), QNetworkReply::NoError);
        void (QNetworkReply::*errorSignal)(QNetworkReply::NetworkError) =
            &QNetworkReply::error;
        connect(replies[i], errorSignal, this, &tst_Http2::replyFinishedWithError);
        // Since we're using self-signed certificates, ignore SSL errors:
        replies[i]->ignoreSslErrors();
    }

    runEventLoop(5000 + responseTimeoutMS);

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

    ServerPtr targetServer(newServer(defaultServerSettings));

    QMetaObject::invokeMethod(targetServer.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort != 0);

    const quint16 targetPort = serverPort;
    serverPort = 0;

    ServerPtr redirector(newServer(defaultServerSettings));
    redirector->redirectOpenStream(targetPort);

    QMetaObject::invokeMethod(redirector.data(), "startServer", Qt::QueuedConnection);
    runEventLoop();

    QVERIFY(serverPort);
    sendRequest(1, QNetworkRequest::NormalPriority, {10000000, Qt::Uninitialized});

    runEventLoop();

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
    manager.setProperty(Http2::http2ParametersPropertyName, QVariant());
}

void tst_Http2::runEventLoop(int ms)
{
    timer.setInterval(ms);
    timer.start();
    eventLoop.exec();
}

void tst_Http2::stopEventLoop()
{
    timer.stop();
    eventLoop.quit();
}

Http2Server *tst_Http2::newServer(const Http2::RawSettings &serverSettings,
                                  const Http2::ProtocolParameters &clientSettings)
{
    using namespace Http2;
    auto srv = new Http2Server(clearTextHTTP2, serverSettings,
                               clientSettings.settingsFrameData);

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
                            const QByteArray &payload)
{
    auto url = requestUrl();
    url.setPath(QString("/stream%1.html").arg(streamNumber));

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, QVariant(true));
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, QVariant(true));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
    request.setPriority(priority);

    QNetworkReply *reply = nullptr;
    if (payload.size())
        reply = manager.post(request, payload);
    else
        reply = manager.get(request);

    reply->ignoreSslErrors();
    connect(reply, &QNetworkReply::finished, this, &tst_Http2::replyFinished);
}

QUrl tst_Http2::requestUrl() const
{
    static auto url = QUrl(QLatin1String(clearTextHTTP2 ? "http://127.0.0.1" : "https://127.0.0.1"));
    url.setPort(serverPort);

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
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        const QVariant http2Used(reply->attribute(QNetworkRequest::HTTP2WasUsedAttribute));
        QVERIFY(http2Used.isValid());
        QVERIFY(http2Used.toBool());
        const QVariant spdyUsed(reply->attribute(QNetworkRequest::SpdyWasUsedAttribute));
        QVERIFY(spdyUsed.isValid());
        QVERIFY(!spdyUsed.toBool());
        const QVariant code(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute));
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
        QVERIFY(reply->error() != QNetworkReply::NoError);
    }

    --nRequests;
    if (!nRequests)
        stopEventLoop();
}

QT_END_NAMESPACE

QTEST_MAIN(tst_Http2)

#include "tst_http2.moc"
