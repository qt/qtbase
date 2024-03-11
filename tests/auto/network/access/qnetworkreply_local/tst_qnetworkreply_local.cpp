// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qtnetworkglobal.h>

#include <QtTest/qtest.h>

#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkaccessmanager.h>

#include "minihttpserver.h"

using namespace Qt::StringLiterals;

/*
    The tests here are meant to be self-contained, using servers in the same
    process if needed. This enables externals to more easily run the tests too.
*/
class tst_QNetworkReply_local : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase_data();

    void get();
    void post();
};

void tst_QNetworkReply_local::initTestCase_data()
{
    QTest::addColumn<QString>("scheme");

    QTest::newRow("http") << "http";
#if QT_CONFIG(localserver)
    QTest::newRow("unix") << "unix+http";
    QTest::newRow("local") << "local+http"; // equivalent to unix, but test that it works
#endif
}

static std::unique_ptr<MiniHttpServerV2> getServerForCurrentScheme()
{
    auto server = std::make_unique<MiniHttpServerV2>();
    QFETCH_GLOBAL(QString, scheme);
    if (scheme.startsWith("unix"_L1) || scheme.startsWith("local"_L1)) {
#if QT_CONFIG(localserver)
        QLocalServer *localServer = new QLocalServer(server.get());
        localServer->listen(u"qt_networkreply_test_"_s
                            % QLatin1StringView(QTest::currentTestFunction())
                            % QString::number(QCoreApplication::applicationPid()));
        server->bind(localServer);
#endif
    } else if (scheme == "http") {
        QTcpServer *tcpServer = new QTcpServer(server.get());
        tcpServer->listen(QHostAddress::LocalHost, 0);
        server->bind(tcpServer);
    }
    return server;
}

static QUrl getUrlForCurrentScheme(MiniHttpServerV2 *server)
{
    QFETCH_GLOBAL(QString, scheme);
    const QString address = server->addressForScheme(scheme);
    const QString urlString = QLatin1StringView("%1://%2").arg(scheme, address);
    return { urlString };
}

void tst_QNetworkReply_local::get()
{
    std::unique_ptr<MiniHttpServerV2> server = getServerForCurrentScheme();
    const QUrl url = getUrlForCurrentScheme(server.get());

    QNetworkAccessManager manager;
    std::unique_ptr<QNetworkReply> reply(manager.get(QNetworkRequest(url)));

    const bool res = QTest::qWaitFor([reply = reply.get()] { return reply->isFinished(); });
    QVERIFY(res);

    QCOMPARE(reply->readAll(), QByteArray("Hello World!"));
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
}

void tst_QNetworkReply_local::post()
{
    std::unique_ptr<MiniHttpServerV2> server = getServerForCurrentScheme();
    const QUrl url = getUrlForCurrentScheme(server.get());

    QNetworkAccessManager manager;
    const QByteArray payload = "Hello from the other side!"_ba;
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    std::unique_ptr<QNetworkReply> reply(manager.post(req, payload));

    const bool res = QTest::qWaitFor([reply = reply.get()] { return reply->isFinished(); });
    QVERIFY(res);

    QCOMPARE(reply->readAll(), QByteArray("Hello World!"));
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);

    auto states = server->peerStates();
    QCOMPARE(states.size(), 1);

    const auto &firstRequest = states.at(0);

    QVERIFY(firstRequest.checkedContentLength);
    QCOMPARE(firstRequest.contentLength, payload.size());
    QCOMPARE_GT(firstRequest.receivedData.size(), payload.size() + 4);
    QCOMPARE(firstRequest.receivedData.last(payload.size() + 4), "\r\n\r\n" + payload);
}

QTEST_MAIN(tst_QNetworkReply_local)

#include "tst_qnetworkreply_local.moc"
#include "moc_minihttpserver.cpp"
