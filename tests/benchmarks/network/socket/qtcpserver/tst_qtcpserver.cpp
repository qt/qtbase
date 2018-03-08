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
#include <qglobal.h>
#include <qcoreapplication.h>
#include <qtcpsocket.h>
#include <qtcpserver.h>
#include <qhostaddress.h>
#include <qstringlist.h>
#include <qplatformdefs.h>
#include <qhostinfo.h>

#include <QNetworkProxy>

#include "../../../../auto/network-settings.h"

class tst_QTcpServer : public QObject
{
    Q_OBJECT

public:
    tst_QTcpServer();
    virtual ~tst_QTcpServer();


public slots:
    void initTestCase_data();
    void initTestCase();
    void init();
    void cleanup();
private slots:
    void ipv4LoopbackPerformanceTest();
    void ipv6LoopbackPerformanceTest();
    void ipv4PerformanceTest();
};

tst_QTcpServer::tst_QTcpServer()
{
}

tst_QTcpServer::~tst_QTcpServer()
{
}

void tst_QTcpServer::initTestCase_data()
{
    QTest::addColumn<bool>("setProxy");
    QTest::addColumn<int>("proxyType");

    QTest::newRow("WithoutProxy") << false << 0;
#ifndef QT_NO_NETWORKPROXY
    QTest::newRow("WithSocks5Proxy") << true << int(QNetworkProxy::Socks5Proxy);
#endif
}

void tst_QTcpServer::initTestCase()
{
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
}

void tst_QTcpServer::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080));
        }
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }
}

void tst_QTcpServer::cleanup()
{
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
#endif
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::ipv4LoopbackPerformanceTest()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    QVERIFY(server.isListening());

    QTcpSocket clientA;
    clientA.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(clientA.waitForConnected(5000));
    QVERIFY(clientA.state() == QAbstractSocket::ConnectedState);

    QVERIFY(server.waitForNewConnection());
    QTcpSocket *clientB = server.nextPendingConnection();
    QVERIFY(clientB);

    QByteArray buffer(16384, '@');
    QTime stopWatch;
    stopWatch.start();
    qlonglong totalWritten = 0;
    while (stopWatch.elapsed() < 5000) {
        QVERIFY(clientA.write(buffer.data(), buffer.size()) > 0);
        clientA.flush();
        totalWritten += buffer.size();
        while (clientB->waitForReadyRead(100)) {
            if (clientB->bytesAvailable() == 16384)
                break;
        }
        clientB->read(buffer.data(), buffer.size());
        clientB->write(buffer.data(), buffer.size());
        clientB->flush();
        totalWritten += buffer.size();
        while (clientA.waitForReadyRead(100)) {
            if (clientA.bytesAvailable() == 16384)
                break;
        }
        clientA.read(buffer.data(), buffer.size());
    }

    qDebug("\t\t%s: %.1fMB/%.1fs: %.1fMB/s",
           server.serverAddress().toString().toLatin1().constData(),
           totalWritten / (1024.0 * 1024.0),
           stopWatch.elapsed() / 1000.0,
           (totalWritten / (stopWatch.elapsed() / 1000.0)) / (1024 * 1024));

    delete clientB;
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::ipv6LoopbackPerformanceTest()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHostIPv6, 0)) {
        QVERIFY(server.serverError() == QAbstractSocket::UnsupportedSocketOperationError);
    } else {
        QTcpSocket clientA;
        clientA.connectToHost(server.serverAddress(), server.serverPort());
        QVERIFY(clientA.waitForConnected(5000));

        QVERIFY(server.waitForNewConnection(5000));
        QTcpSocket *clientB = server.nextPendingConnection();
        QVERIFY(clientB);

        QByteArray buffer(16384, '@');
        QTime stopWatch;
        stopWatch.start();
        qlonglong totalWritten = 0;
        while (stopWatch.elapsed() < 5000) {
            clientA.write(buffer.data(), buffer.size());
            clientA.flush();
            totalWritten += buffer.size();
            while (clientB->waitForReadyRead(100)) {
                if (clientB->bytesAvailable() == 16384)
                    break;
            }
            clientB->read(buffer.data(), buffer.size());
            clientB->write(buffer.data(), buffer.size());
            clientB->flush();
            totalWritten += buffer.size();
            while (clientA.waitForReadyRead(100)) {
                if (clientA.bytesAvailable() == 16384)
                   break;
            }
            clientA.read(buffer.data(), buffer.size());
        }

        qDebug("\t\t%s: %.1fMB/%.1fs: %.1fMB/s",
               server.serverAddress().toString().toLatin1().constData(),
               totalWritten / (1024.0 * 1024.0),
               stopWatch.elapsed() / 1000.0,
               (totalWritten / (stopWatch.elapsed() / 1000.0)) / (1024 * 1024));
        delete clientB;
    }
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::ipv4PerformanceTest()
{
    QTcpSocket probeSocket;
    probeSocket.connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(probeSocket.waitForConnected(5000));

    QTcpServer server;
    QVERIFY(server.listen(probeSocket.localAddress(), 0));

    QTcpSocket clientA;
    clientA.connectToHost(server.serverAddress(), server.serverPort());
    QVERIFY(clientA.waitForConnected(5000));

    QVERIFY(server.waitForNewConnection(5000));
    QTcpSocket *clientB = server.nextPendingConnection();
    QVERIFY(clientB);

    QByteArray buffer(16384, '@');
    QTime stopWatch;
    stopWatch.start();
    qlonglong totalWritten = 0;
    while (stopWatch.elapsed() < 5000) {
        qlonglong writtenA = clientA.write(buffer.data(), buffer.size());
        clientA.flush();
        totalWritten += buffer.size();
        while (clientB->waitForReadyRead(100)) {
            if (clientB->bytesAvailable() == writtenA)
                break;
        }
        clientB->read(buffer.data(), buffer.size());
        qlonglong writtenB = clientB->write(buffer.data(), buffer.size());
        clientB->flush();
        totalWritten += buffer.size();
        while (clientA.waitForReadyRead(100)) {
            if (clientA.bytesAvailable() == writtenB)
               break;
        }
        clientA.read(buffer.data(), buffer.size());
    }

    qDebug("\t\t%s: %.1fMB/%.1fs: %.1fMB/s",
           probeSocket.localAddress().toString().toLatin1().constData(),
           totalWritten / (1024.0 * 1024.0),
           stopWatch.elapsed() / 1000.0,
           (totalWritten / (stopWatch.elapsed() / 1000.0)) / (1024 * 1024));

    delete clientB;
}

QTEST_MAIN(tst_QTcpServer)
#include "tst_qtcpserver.moc"
