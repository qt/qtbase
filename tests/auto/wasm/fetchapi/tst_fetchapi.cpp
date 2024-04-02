// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopedPointer>

#include <memory>

namespace {

const QUrl URL = QUrl("http://localhost:6931/test_batch.html");

class BackgroundThread : public QThread
{
    Q_OBJECT
    void run() override
    {
        manager = std::make_unique<QNetworkAccessManager>();
        eventLoop = std::make_unique<QEventLoop>();
        reply = manager->get(request);
        QObject::connect(reply, &QNetworkReply::finished, this, &BackgroundThread::requestFinished);
        eventLoop->exec();
    }

    void requestFinished()
    {
        eventLoop->quit();
    }

public:
    QNetworkReply *reply{ nullptr };

private:
    std::unique_ptr<QNetworkAccessManager> manager;
    std::unique_ptr<QEventLoop> eventLoop;
    QNetworkRequest request{ URL };
};

} // namespace

class tst_FetchApi : public QObject
{
    Q_OBJECT
public:
    tst_FetchApi() = default;

private Q_SLOTS:
    void sendRequestOnMainThread();
    void sendRequestOnBackgroundThread();
};

void tst_FetchApi::sendRequestOnMainThread()
{
    QNetworkAccessManager manager;
    QNetworkRequest request(URL);
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QVERIFY(reply != nullptr);
    QVERIFY(reply->error() == QNetworkReply::NoError);
}

void tst_FetchApi::sendRequestOnBackgroundThread()
{
    QEventLoop mainEventLoop;
    BackgroundThread *backgroundThread = new BackgroundThread();
    connect(backgroundThread, &BackgroundThread::finished, &mainEventLoop, &QEventLoop::quit);
    backgroundThread->start();
    mainEventLoop.exec();

    QVERIFY(backgroundThread != nullptr);
    QVERIFY(backgroundThread->reply != nullptr);
    QVERIFY(backgroundThread->reply->error() == QNetworkReply::NoError);
}

QTEST_MAIN(tst_FetchApi);
#include "tst_fetchapi.moc"
