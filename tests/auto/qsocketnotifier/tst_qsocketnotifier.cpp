/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <QtCore/QCoreApplication>
#include <QtCore/QSocketNotifier>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#ifdef Q_OS_SYMBIAN
#include <private/qsymbiansocketengine_p.h>
#define NATIVESOCKETENGINE QSymbianSocketEngine
#else
#include <private/qnativesocketengine_p.h>
#define NATIVESOCKETENGINE QNativeSocketEngine
#endif

class tst_QSocketNotifier : public QObject
{
    Q_OBJECT
public:
    tst_QSocketNotifier();
    ~tst_QSocketNotifier();

private slots:
    void unexpectedDisconnection();
    void mixingWithTimers();
};

tst_QSocketNotifier::tst_QSocketNotifier()
{ }

tst_QSocketNotifier::~tst_QSocketNotifier()
{
}

class UnexpectedDisconnectTester : public QObject
{
    Q_OBJECT
public:
    NATIVESOCKETENGINE *readEnd1, *readEnd2;
    int sequence;

    UnexpectedDisconnectTester(NATIVESOCKETENGINE *s1, NATIVESOCKETENGINE *s2)
        : readEnd1(s1), readEnd2(s2), sequence(0)
    {
        QSocketNotifier *notifier1 =
            new QSocketNotifier(readEnd1->socketDescriptor(), QSocketNotifier::Read, this);
        connect(notifier1, SIGNAL(activated(int)), SLOT(handleActivated()));
        QSocketNotifier *notifier2 =
            new QSocketNotifier(readEnd2->socketDescriptor(), QSocketNotifier::Read, this);
        connect(notifier2, SIGNAL(activated(int)), SLOT(handleActivated()));
    }

public slots:
    void handleActivated()
    {
        char data1[1], data2[1];
        ++sequence;
        if (sequence == 1) {
            // read from both ends
            (void) readEnd1->read(data1, sizeof(data1));
            (void) readEnd2->read(data2, sizeof(data2));
            emit finished();
        } else if (sequence == 2) {
            // we should never get here
            QCOMPARE(readEnd2->read(data2, sizeof(data2)), qint64(-2));
            QVERIFY(readEnd2->isValid());
        }
    }

signals:
    void finished();
};

void tst_QSocketNotifier::unexpectedDisconnection()
{
    /*
      Given two sockets and two QSocketNotifiers registered on each
      their socket. If both sockets receive data, and the first slot
      invoked by one of the socket notifiers empties both sockets, the
      other notifier will also emit activated(). This results in
      unexpected disconnection in QAbstractSocket.

      The use case is that somebody calls one of the
      waitFor... functions in a QSocketNotifier activated slot, and
      the waitFor... functions do local selects that can empty both
      stdin and stderr while waiting for fex bytes to be written.
    */

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    NATIVESOCKETENGINE readEnd1;
    readEnd1.initialize(QAbstractSocket::TcpSocket);
    bool b = readEnd1.connectToHost(server.serverAddress(), server.serverPort());
    QVERIFY(readEnd1.waitForWrite());
//    while (!b && readEnd1.state() != QAbstractSocket::ConnectedState)
//        b = readEnd1.connectToHost(server.serverAddress(), server.serverPort());
    QVERIFY(readEnd1.state() == QAbstractSocket::ConnectedState);
    QVERIFY(server.waitForNewConnection());
    QTcpSocket *writeEnd1 = server.nextPendingConnection();
    QVERIFY(writeEnd1 != 0);

    NATIVESOCKETENGINE readEnd2;
    readEnd2.initialize(QAbstractSocket::TcpSocket);
    b = readEnd2.connectToHost(server.serverAddress(), server.serverPort());
    QVERIFY(readEnd2.waitForWrite());
//    while (!b)
//        b = readEnd2.connectToHost(server.serverAddress(), server.serverPort());
    QVERIFY(readEnd2.state() == QAbstractSocket::ConnectedState);
    QVERIFY(server.waitForNewConnection());
    QTcpSocket *writeEnd2 = server.nextPendingConnection();
    QVERIFY(writeEnd2 != 0);

    writeEnd1->write("1", 1);
    writeEnd2->write("2", 1);

    writeEnd1->waitForBytesWritten();
    writeEnd2->waitForBytesWritten();

    writeEnd1->flush();
    writeEnd2->flush();

    UnexpectedDisconnectTester tester(&readEnd1, &readEnd2);

    do {
        // we have to wait until sequence value changes
        // as any event can make us jump out processing
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }  while(tester.sequence <= 0);

    QVERIFY(readEnd1.state() == QAbstractSocket::ConnectedState);
    QVERIFY(readEnd2.state() == QAbstractSocket::ConnectedState);

    QCOMPARE(tester.sequence, 2);

    readEnd1.close();
    readEnd2.close();
    writeEnd1->close();
    writeEnd2->close();
    server.close();
}

class MixingWithTimersHelper : public QObject
{
    Q_OBJECT

public:
    MixingWithTimersHelper(QTimer *timer, QTcpServer *server);

    bool timerActivated;
    bool socketActivated;

private slots:
    void timerFired();
    void socketFired();
};

MixingWithTimersHelper::MixingWithTimersHelper(QTimer *timer, QTcpServer *server)
{
    timerActivated = false;
    socketActivated = false;

    connect(timer, SIGNAL(timeout()), SLOT(timerFired()));
    connect(server, SIGNAL(newConnection()), SLOT(socketFired()));
}

void MixingWithTimersHelper::timerFired()
{
    timerActivated = true;
}

void MixingWithTimersHelper::socketFired()
{
    socketActivated = true;
}

void tst_QSocketNotifier::mixingWithTimers()
{
    QTimer timer;
    timer.setInterval(0);
    timer.start();

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    MixingWithTimersHelper helper(&timer, &server);

    QCoreApplication::processEvents();

    QCOMPARE(helper.timerActivated, true);
    QCOMPARE(helper.socketActivated, false);

    helper.timerActivated = false;
    helper.socketActivated = false;

    QTcpSocket socket;
    socket.connectToHost(server.serverAddress(), server.serverPort());

    QCoreApplication::processEvents();

    QCOMPARE(helper.timerActivated, true);
    QCOMPARE(helper.socketActivated, true);
}

QTEST_MAIN(tst_QSocketNotifier)
#include <tst_qsocketnotifier.moc>
