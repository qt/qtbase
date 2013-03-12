/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
#include "../../auto/network-settings.h"

#if defined(QT_BUILD_INTERNAL) && !defined(QT_NO_SSL)
#include "private/qsslsocket_p.h"
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
#if defined(Q_OS_LINUX) || defined (Q_OS_BLACKBERRY)
    QCOMPARE(rootCertLoadingAllowed, true);
#elif defined(Q_OS_MAC)
    QCOMPARE(rootCertLoadingAllowed, false);
#endif // other platforms: undecided (Windows: depends on the version)
    if (works) {
        QCOMPARE(reply->error(), QNetworkReply::NoError);
    } else {
        QCOMPARE(reply->error(), QNetworkReply::SslHandshakeFailedError);
    }
#endif
#endif // QT_NO_SSL
}

QTEST_MAIN(tst_qnetworkreply)

#include "main.moc"
