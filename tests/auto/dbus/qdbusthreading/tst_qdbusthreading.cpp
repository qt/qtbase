/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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
#include <QtTest>
#include <QtDBus>
#include <QtCore/QVarLengthArray>
#include <QtCore/QThread>
#include <QtCore/QObject>
#include <QtCore/QSemaphore>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QMap>

class Thread : public QThread
{
    Q_OBJECT
    static int counter;
public:
    Thread(bool automatic = true);
    void run();

    using QThread::exec;
};
int Thread::counter;

class tst_QDBusThreading : public QObject
{
    Q_OBJECT
    static tst_QDBusThreading *_self;
    QAtomicInt threadJoinCount;
    QSemaphore threadJoin;
public:
    QSemaphore sem1, sem2;
    volatile bool success;
    QEventLoop *loop;
    enum FunctionSpy {
        NoMethod = 0,
        Adaptor_method,
        Object_method
    } functionSpy;

    QThread *threadSpy;
    int signalSpy;

    tst_QDBusThreading();
    static inline tst_QDBusThreading *self() { return _self; }

    void joinThreads();
    bool waitForSignal(QObject *obj, const char *signal, int delay = 1);

public Q_SLOTS:
    void cleanup();
    void signalSpySlot() { ++signalSpy; }
    void threadStarted() { threadJoinCount.ref(); }
    void threadFinished() { threadJoin.release(); }

    void dyingThread_thread();
    void lastInstanceInOtherThread_thread();
    void concurrentCreation_thread();
    void disconnectAnothersConnection_thread();
    void accessMainsConnection_thread();
    void accessOthersConnection_thread();
    void registerObjectInOtherThread_thread();
    void registerAdaptorInOtherThread_thread();
    void callbackInMainThread_thread();
    void callbackInAuxThread_thread();
    void callbackInAnotherAuxThread_thread();

private Q_SLOTS:
    void initTestCase();
    void dyingThread();
    void lastInstanceInOtherThread();
    void concurrentCreation();
    void disconnectAnothersConnection();
    void accessMainsConnection();
    void accessOthersConnection();
    void registerObjectInOtherThread();
    void registerAdaptorInOtherThread();
    void callbackInMainThread();
    void callbackInAuxThread();
    void callbackInAnotherAuxThread();
};
tst_QDBusThreading *tst_QDBusThreading::_self;

class Adaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.Adaptor")
public:
    Adaptor(QObject *parent)
        : QDBusAbstractAdaptor(parent)
    {
    }

public Q_SLOTS:
    void method()
    {
        tst_QDBusThreading::self()->functionSpy = tst_QDBusThreading::Adaptor_method;
        tst_QDBusThreading::self()->threadSpy = QThread::currentThread();
        emit signal();
    }

Q_SIGNALS:
    void signal();
};

class Object : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.Object")
public:
    Object(bool useAdaptor)
    {
        if (useAdaptor)
            new Adaptor(this);
    }

    ~Object()
    {
        QMetaObject::invokeMethod(QThread::currentThread(), "quit", Qt::QueuedConnection);
    }

public Q_SLOTS:
    void method()
    {
        tst_QDBusThreading::self()->functionSpy = tst_QDBusThreading::Object_method;
        tst_QDBusThreading::self()->threadSpy = QThread::currentThread();
        emit signal();
        deleteLater();
    }

Q_SIGNALS:
    void signal();
};

#if 0
typedef void (*qdbusThreadDebugFunc)(int, int, QDBusConnectionPrivate *);
QDBUS_EXPORT void qdbusDefaultThreadDebug(int, int, QDBusConnectionPrivate *);
extern QDBUS_EXPORT qdbusThreadDebugFunc qdbusThreadDebug;

static void threadDebug(int action, int condition, QDBusConnectionPrivate *p)
{
    qdbusDefaultThreadDebug(action, condition, p);
}
#endif

Thread::Thread(bool automatic)
{
    setObjectName(QString::fromLatin1("Aux thread %1").arg(++counter));
    connect(this, SIGNAL(started()), tst_QDBusThreading::self(), SLOT(threadStarted()));
    connect(this, SIGNAL(finished()), tst_QDBusThreading::self(), SLOT(threadFinished()),
            Qt::DirectConnection);
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()), Qt::DirectConnection);
    if (automatic)
        start();
}

void Thread::run()
{
    QVarLengthArray<char, 56> name;
    name.append(QTest::currentTestFunction(), qstrlen(QTest::currentTestFunction()));
    name.append("_thread", sizeof "_thread");
    QMetaObject::invokeMethod(tst_QDBusThreading::self(), name.constData(), Qt::DirectConnection);
}

static const char myConnectionName[] = "connection";

tst_QDBusThreading::tst_QDBusThreading()
    : loop(0), functionSpy(NoMethod), threadSpy(0)
{
    _self = this;
    QCoreApplication::instance()->thread()->setObjectName("Main thread");
}

void tst_QDBusThreading::joinThreads()
{
    threadJoin.acquire(threadJoinCount.load());
    threadJoinCount.store(0);
}

bool tst_QDBusThreading::waitForSignal(QObject *obj, const char *signal, int delay)
{
    QObject::connect(obj, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    QPointer<QObject> safe = obj;

    QTestEventLoop::instance().enterLoop(delay);
    if (!safe.isNull())
        QObject::disconnect(safe, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    return QTestEventLoop::instance().timeout();
}

void tst_QDBusThreading::cleanup()
{
    joinThreads();

    if (sem1.available())
        sem1.acquire(sem1.available());
    if (sem2.available())
        sem2.acquire(sem2.available());

    if (QDBusConnection(myConnectionName).isConnected())
        QDBusConnection::disconnectFromBus(myConnectionName);

    delete loop;
    loop = 0;

    QTest::qWait(500);
}

void tst_QDBusThreading::initTestCase()
{
}

void tst_QDBusThreading::dyingThread_thread()
{
    QDBusConnection::connectToBus(QDBusConnection::SessionBus, myConnectionName);
}

void tst_QDBusThreading::dyingThread()
{
    Thread *th = new Thread(false);
    QTestEventLoop::instance().connect(th, SIGNAL(destroyed(QObject*)), SLOT(exitLoop()));
    th->start();

    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QDBusConnection con(myConnectionName);
    QDBusConnection::disconnectFromBus(myConnectionName);

    QVERIFY(con.isConnected());
    QDBusReply<QStringList> reply = con.interface()->registeredServiceNames();
    QVERIFY(reply.isValid());
    QVERIFY(!reply.value().isEmpty());
    QVERIFY(reply.value().contains(con.baseService()));

    con.interface()->callWithCallback("ListNames", QVariantList(),
                                      &QTestEventLoop::instance(), SLOT(exitLoop()));

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void tst_QDBusThreading::lastInstanceInOtherThread_thread()
{
    QDBusConnection con(myConnectionName);
    QVERIFY(con.isConnected());

    QDBusConnection::disconnectFromBus(myConnectionName);

    // con is being destroyed in the wrong thread
}

void tst_QDBusThreading::lastInstanceInOtherThread()
{
    Thread *th = new Thread(false);
    // create the connection:
    QDBusConnection::connectToBus(QDBusConnection::SessionBus, myConnectionName);

    th->start();
    th->wait();
}

void tst_QDBusThreading::concurrentCreation_thread()
{
    sem1.acquire();
    QDBusConnection con = QDBusConnection::connectToBus(QDBusConnection::SessionBus,
                                                        myConnectionName);
    sem2.release();
}

void tst_QDBusThreading::concurrentCreation()
{
    Thread *th = new Thread;

    {
        sem1.release();
        QDBusConnection con = QDBusConnection::connectToBus(QDBusConnection::SessionBus,
                                                            myConnectionName);
        QVERIFY(con.isConnected());
        sem2.acquire();
    }
    waitForSignal(th, SIGNAL(finished()));
    QDBusConnection::disconnectFromBus(myConnectionName);

    QVERIFY(!QDBusConnection(myConnectionName).isConnected());
}

void tst_QDBusThreading::disconnectAnothersConnection_thread()
{
    QDBusConnection con = QDBusConnection::connectToBus(QDBusConnection::SessionBus,
                                                        myConnectionName);
    sem2.release();
}

void tst_QDBusThreading::disconnectAnothersConnection()
{
    new Thread;
    sem2.acquire();

    QVERIFY(QDBusConnection(myConnectionName).isConnected());
    QDBusConnection::disconnectFromBus(myConnectionName);
}

void tst_QDBusThreading::accessMainsConnection_thread()
{
    sem1.acquire();
    QDBusConnection con = QDBusConnection::sessionBus();
    con.interface()->registeredServiceNames();
    sem2.release();
}

void tst_QDBusThreading::accessMainsConnection()
{
    QVERIFY(QDBusConnection::sessionBus().isConnected());

    new Thread;
    sem1.release();
    sem2.acquire();
};

void tst_QDBusThreading::accessOthersConnection_thread()
{
    QDBusConnection::connectToBus(QDBusConnection::SessionBus, myConnectionName);
    sem2.release();

    // wait for main thread to be done
    sem1.acquire();
    QDBusConnection::disconnectFromBus(myConnectionName);
    sem2.release();
}

void tst_QDBusThreading::accessOthersConnection()
{
    new Thread;

    // wait for the connection to be created
    sem2.acquire();

    {
        QDBusConnection con(myConnectionName);
        QVERIFY(con.isConnected());
        QVERIFY(con.baseService() != QDBusConnection::sessionBus().baseService());

        QDBusReply<QStringList> reply = con.interface()->registeredServiceNames();
        if (!reply.isValid())
            qDebug() << reply.error().name() << reply.error().message();
        QVERIFY(reply.isValid());
        QVERIFY(!reply.value().isEmpty());
        QVERIFY(reply.value().contains(con.baseService()));
        QVERIFY(reply.value().contains(QDBusConnection::sessionBus().baseService()));
    }

    // tell it to destroy:
    sem1.release();
    sem2.acquire();

    QDBusConnection con(myConnectionName);
    QVERIFY(!con.isConnected());
}

void tst_QDBusThreading::registerObjectInOtherThread_thread()
{
    {
        Object *obj = new Object(false);
        QDBusConnection::sessionBus().registerObject("/", obj, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);

        sem2.release();
        static_cast<Thread *>(QThread::currentThread())->exec();
    }

    sem2.release();
}

void tst_QDBusThreading::registerObjectInOtherThread()
{
    QVERIFY(QDBusConnection::sessionBus().isConnected());
    QThread *th = new Thread;
    sem2.acquire();

    signalSpy = 0;

    QDBusInterface iface(QDBusConnection::sessionBus().baseService(), "/", "local.Object");
    QVERIFY(iface.isValid());

    connect(&iface, SIGNAL(signal()), SLOT(signalSpySlot()));

    QTest::qWait(100);
    QCOMPARE(signalSpy, 0);

    functionSpy = NoMethod;
    threadSpy = 0;
    QDBusReply<void> reply = iface.call("method");
    QVERIFY(reply.isValid());
    QCOMPARE(functionSpy, Object_method);
    QCOMPARE(threadSpy, th);

    QTest::qWait(100);
    QCOMPARE(signalSpy, 1);

    sem2.acquire();             // the object is gone
    functionSpy = NoMethod;
    threadSpy = 0;
    reply = iface.call("method");
    QVERIFY(!reply.isValid());
    QCOMPARE(functionSpy, NoMethod);
    QCOMPARE(threadSpy, (QThread*)0);
}

void tst_QDBusThreading::registerAdaptorInOtherThread_thread()
{
    {
        Object *obj = new Object(true);
        QDBusConnection::sessionBus().registerObject("/", obj, QDBusConnection::ExportAdaptors |
                                                     QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);

        sem2.release();
        static_cast<Thread *>(QThread::currentThread())->exec();
    }

    sem2.release();
}

void tst_QDBusThreading::registerAdaptorInOtherThread()
{
    QVERIFY(QDBusConnection::sessionBus().isConnected());
    QThread *th = new Thread;
    sem2.acquire();

    QDBusInterface object(QDBusConnection::sessionBus().baseService(), "/", "local.Object");
    QDBusInterface adaptor(QDBusConnection::sessionBus().baseService(), "/", "local.Adaptor");
    QVERIFY(object.isValid());
    QVERIFY(adaptor.isValid());

    signalSpy = 0;
    connect(&adaptor, SIGNAL(signal()), SLOT(signalSpySlot()));
    QCOMPARE(signalSpy, 0);

    functionSpy = NoMethod;
    threadSpy = 0;
    QDBusReply<void> reply = adaptor.call("method");
    QVERIFY(reply.isValid());
    QCOMPARE(functionSpy, Adaptor_method);
    QCOMPARE(threadSpy, th);

    QTest::qWait(100);
    QCOMPARE(signalSpy, 1);

    functionSpy = NoMethod;
    threadSpy = 0;
    reply = object.call("method");
    QVERIFY(reply.isValid());
    QCOMPARE(functionSpy, Object_method);
    QCOMPARE(threadSpy, th);

    QTest::qWait(100);
    QCOMPARE(signalSpy, 1);

    sem2.acquire();             // the object is gone
    functionSpy = NoMethod;
    threadSpy = 0;
    reply = adaptor.call("method");
    QVERIFY(!reply.isValid());
    QCOMPARE(functionSpy, NoMethod);
    QCOMPARE(threadSpy, (QThread*)0);
    reply = object.call("method");
    QVERIFY(!reply.isValid());
    QCOMPARE(functionSpy, NoMethod);
    QCOMPARE(threadSpy, (QThread*)0);
}

void tst_QDBusThreading::callbackInMainThread_thread()
{
    QDBusConnection::connectToBus(QDBusConnection::SessionBus, myConnectionName);
    sem2.release();

    static_cast<Thread *>(QThread::currentThread())->exec();
    QDBusConnection::disconnectFromBus(myConnectionName);
}

void tst_QDBusThreading::callbackInMainThread()
{
    Thread *th = new Thread;

    // wait for it to be connected
    sem2.acquire();

    QDBusConnection con(myConnectionName);
    con.interface()->callWithCallback("ListNames", QVariantList(),
                                      &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QMetaObject::invokeMethod(th, "quit");
    waitForSignal(th, SIGNAL(finished()));
}

void tst_QDBusThreading::callbackInAuxThread_thread()
{
    QDBusConnection con(QDBusConnection::sessionBus());
    QTestEventLoop ownLoop;
    con.interface()->callWithCallback("ListNames", QVariantList(),
                                      &ownLoop, SLOT(exitLoop()));
    ownLoop.enterLoop(10);
    loop->exit(ownLoop.timeout() ? 1 : 0);
}

void tst_QDBusThreading::callbackInAuxThread()
{
    QVERIFY(QDBusConnection::sessionBus().isConnected());

    loop = new QEventLoop;

    new Thread;
    QCOMPARE(loop->exec(), 0);
}

void tst_QDBusThreading::callbackInAnotherAuxThread_thread()
{
    sem1.acquire();
    if (!loop) {
        // first thread
        // create the connection and just wait
        QDBusConnection con = QDBusConnection::connectToBus(QDBusConnection::SessionBus, myConnectionName);
        loop = new QEventLoop;

        // tell the main thread we have created the loop and connection
        sem2.release();

        // wait for the main thread to connect its signal
        sem1.acquire();
        success = loop->exec() == 0;
        sem2.release();

        // clean up
        QDBusConnection::disconnectFromBus(myConnectionName);
    } else {
        // second thread
        // try waiting for a message
        QDBusConnection con(myConnectionName);
        QTestEventLoop ownLoop;
        con.interface()->callWithCallback("ListNames", QVariantList(),
                                          &ownLoop, SLOT(exitLoop()));
        ownLoop.enterLoop(1);
        loop->exit(ownLoop.timeout() ? 1 : 0);
    }
}

void tst_QDBusThreading::callbackInAnotherAuxThread()
{
    // create first thread
    success = false;
    new Thread;

    // wait for the event loop
    sem1.release();
    sem2.acquire();
    QVERIFY(loop);

    // create the second thread
    new Thread;
    sem1.release(2);

    // wait for loop thread to finish executing:
    sem2.acquire();

    QVERIFY(success);
}

// Next tests:
// - unexport an object at the moment the call is being delivered
// - delete an object at the moment the call is being delivered
// - keep a global-static QDBusConnection for a thread-created connection

QTEST_MAIN(tst_QDBusThreading)
#include "tst_qdbusthreading.moc"
