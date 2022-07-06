/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtwasmtestlib.h>
#include <QtCore>
#include <QtNetwork>

const int socketWait = 1000;
const QString hostName = "localhost";
const int port = 1515;

class SockifySocketsTest: public QObject
{
    Q_OBJECT

private slots:
    void echo();
    void echoMultipleMessages();
    void echoMultipleSockets();
    void remoteClose();

#if QT_CONFIG(thread)
    void thread_echo();
    void thread_remoteClose();
    void thread_echoMultipleSockets();
#endif

#ifdef QT_HAVE_EMSCRIPTEN_ASYNCIFY
    void asyncify_echo();
    void asyncify_remoteClose();
#endif
};

class CompleteTestFunctionRefGuard {
public:
    CompleteTestFunctionRefGuard(CompleteTestFunctionRefGuard const&) = delete;
    CompleteTestFunctionRefGuard& operator=(CompleteTestFunctionRefGuard const&) = delete;

    static CompleteTestFunctionRefGuard *create() {
        return new CompleteTestFunctionRefGuard();
    }

    void ref() {
        QMutexLocker lock(&mutex);
        ++counter;
    }

    void deref() {
        bool itsTheFinalDeref = [this] {
            QMutexLocker lock(&mutex);
            return --counter == 0;
        }();

        if (itsTheFinalDeref) {
            delete this;
            QtWasmTest::completeTestFunction();
        }
    }
private:
    CompleteTestFunctionRefGuard() { };

    QMutex mutex;
    int counter = 0;
};

#if QT_CONFIG(thread)

class TestThread : public QThread
{
public:
    static QThread *create(std::function<void()> started, std::function<void()> finished)
    {
        TestThread *thread = new TestThread();
        connect(thread, &QThread::started, [started]() {
            started();
        });
        connect(thread, &QThread::finished, [thread, finished]() {
            finished();
            thread->deleteLater();
        });
        thread->start();
        return thread;
    }
};

#endif

void blockingEchoTest()
{
    QTcpSocket socket;
    socket.connectToHost(hostName, port);
    if (!socket.waitForConnected(socketWait))
        qFatal("socket connect error");

    QByteArray message = "Hello, echo server!";

    QByteArray command = "echo:" + message + ';';
    socket.write(command);
    socket.flush();

    socket.waitForReadyRead(socketWait);
    QByteArray expectedReply = message + ';';
    QByteArray reply = socket.readAll();
    if (reply != expectedReply)
        qFatal("echo_multiple received incorrect reply");
    socket.disconnectFromHost();
}

void blockingRemoteClose()
{
    QTcpSocket socket;

    qDebug() << "## connectToHost";
    socket.connectToHost(hostName, port);

    qDebug() << "## waitForConnected";
    socket.waitForConnected(socketWait);
    socket.write("close;");
    socket.flush();

    qDebug() << "## waitForBytesWritten";
    socket.waitForBytesWritten(socketWait);

    qDebug() << "## waitForReadyRead";
    socket.waitForReadyRead(200);

    qDebug() << "## waitForDisconnected";
    socket.waitForDisconnected(socketWait);
    qDebug() << "## done";
}

// Verify that sending one echo command and receiving the reply works
void SockifySocketsTest::echo()
{
    QTcpSocket *socket = new QTcpSocket();
    socket->connectToHost(hostName, port);

    QByteArray message = "Hello, echo server!";

    QObject::connect(socket, &QAbstractSocket::connected, [socket, message]() {
        QByteArray command = "echo:" + message + ';';
        socket->write(command);
        socket->flush();
    });

    QByteArray *reply = new QByteArray();
    QObject::connect(socket, &QIODevice::readyRead, [socket, reply, message]() {
        *reply += socket->readAll();
        if (reply->contains(';')) {
            bool match = (*reply == message + ';');
            socket->disconnectFromHost();
            socket->deleteLater();
            delete reply;
            QtWasmTest::completeTestFunction(match ? QtWasmTest::TestResult::Pass : QtWasmTest::TestResult::Fail, std::string());
        }
    });
}

void SockifySocketsTest::echoMultipleMessages()
{
    const int count = 20;

    QTcpSocket *socket = new QTcpSocket();
    socket->connectToHost(hostName, port);
    QByteArray message = "Hello, echo server!";

    QObject::connect(socket, &QAbstractSocket::connected, [socket, message]() {
        QByteArray command = "echo:" + message + ';';
        for (int i = 0; i < count; ++i) {
            quint64 written = socket->write(command);
            if (written != quint64(command.size()))
                qFatal("Unable to write to socket");
        }
        socket->flush();
    });

    QByteArray expectedReply;
    for (int i = 0; i < count; ++i)
        expectedReply += (message + ';');
    QByteArray *receivedReply = new QByteArray;
    QObject::connect(socket, &QIODevice::readyRead, [socket, receivedReply, expectedReply]() {
        QByteArray reply = socket->readAll();
        *receivedReply += reply;

        if (*receivedReply == expectedReply) {
            socket->disconnectFromHost();
            socket->deleteLater();
            delete receivedReply;
            QtWasmTest::completeTestFunction();
        }
    });
}

void SockifySocketsTest::echoMultipleSockets()
{
    const int connections = 5;
    auto guard = CompleteTestFunctionRefGuard::create();

    QByteArray message = "Hello, echo server!";

    for (int i = 0; i < connections; ++i) {
        guard->ref();

        QTcpSocket *socket = new QTcpSocket();
        socket->connectToHost(hostName, port);

        QObject::connect(socket, &QAbstractSocket::connected, [socket, message]() {
            QByteArray command = "echo:" + message + ';';
            socket->write(command);
            socket->flush();
        });

        QObject::connect(socket, &QIODevice::readyRead, [guard, socket, message]() {
            QByteArray reply = socket->readAll();
            socket->disconnectFromHost();
            socket->deleteLater();
            if (reply != (message + ';'))
                qFatal("echo_multiple received incorrect reply");
            guard->deref();
        });
    }
}

void SockifySocketsTest::remoteClose()
{
    QTcpSocket *socket = new QTcpSocket();
    socket->connectToHost(hostName, port);
    QObject::connect(socket, &QAbstractSocket::connected, [socket]() {
        socket->write("close;");
        socket->flush();
    });
    QObject::connect(socket, &QAbstractSocket::disconnected, [socket]() {
        qDebug() << "disconnected";
        socket->deleteLater();
        QtWasmTest::completeTestFunction();
    });
}

#if QT_CONFIG(thread)

void SockifySocketsTest::thread_echo()
{
    auto started = []() {
        blockingEchoTest();
        QThread::currentThread()->quit();
    };

    auto finished = [](){
        QtWasmTest::completeTestFunction();
    };

    TestThread::create(started, finished);
}

void SockifySocketsTest::thread_echoMultipleSockets()
{
    const int connections = 2; // TODO: test more threads
    auto guard = CompleteTestFunctionRefGuard::create();
    guard->ref();

    for (int i = 0; i < connections; ++i) {
        guard->ref();
        auto started = [](){
            blockingEchoTest();
            QThread::currentThread()->quit();
        };

        auto finished = [guard](){
            guard->deref();
        };

        TestThread::create(started, finished);
    }

    guard->deref();
}

void SockifySocketsTest::thread_remoteClose()
{
    auto started = [](){
        blockingRemoteClose();
        QThread::currentThread()->quit();
    };

    auto finished = [](){
        QtWasmTest::completeTestFunction();
    };

    TestThread::create(started, finished);
}

#endif

#ifdef QT_HAVE_EMSCRIPTEN_ASYNCIFY

// Post an event to the main thread and asyncify wait for it
void SockifySocketsTest::asyncify_echo()
{
    blockingEchoTest();
    QtWasmTest::completeTestFunction();
}

void SockifySocketsTest::asyncify_remoteClose()
{
    blockingRemoteClose();
    QtWasmTest::completeTestFunction();
}

#endif

int main(int argc, char **argv)
{
    auto testObject = std::make_shared<SockifySocketsTest>();
    QtWasmTest::initTestCase<QCoreApplication>(argc, argv, testObject);
    return 0;
}

#include "main.moc"
