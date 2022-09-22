// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore>
#include <QTest>

#include <QtTest/private/qemulationdetector_p.h>

#include <optional>

enum { OneMinute = 60 * 1000,
       TwoMinutes = OneMinute * 2 };


struct Functor
{
    void operator()() const {};
};

class tst_QObjectRace: public QObject
{
    Q_OBJECT
public:
    tst_QObjectRace()
        : ThreadCount(QThread::idealThreadCount())
    {}

private slots:
    void moveToThreadRace();
    void destroyRace();
    void blockingQueuedDestroyRace();
    void disconnectRace();
    void disconnectRace2();

private:
    const int ThreadCount;
};

class RaceObject : public QObject
{
    Q_OBJECT
    QList<QThread *> threads;
    int count;

public:
    RaceObject()
        : count(0)
    { }

    void addThread(QThread *thread)
    { threads.append(thread); }

public slots:
    void theSlot()
    {
        enum { step = 35 };
        if ((++count % step) == 0) {
            QThread *nextThread = threads.at((count / step) % threads.size());
            moveToThread(nextThread);
        }
    }

    void destroSlot() {
        emit theSignal();
    }
signals:
    void theSignal();
};

class RaceThread : public QThread
{
    Q_OBJECT
    RaceObject *object;
    QElapsedTimer stopWatch;

public:
    RaceThread()
        : object(0)
    { }

    void setObject(RaceObject *o)
    {
        object = o;
        object->addThread(this);
    }

    void start() {
        stopWatch.start();
        QThread::start();
    }

    void run() override
    {
        QTimer zeroTimer;
        connect(&zeroTimer, SIGNAL(timeout()), object, SLOT(theSlot()));
        connect(&zeroTimer, SIGNAL(timeout()), this, SLOT(checkStopWatch()), Qt::DirectConnection);
        zeroTimer.start(0);
        (void) exec();
    }

signals:
    void theSignal();

private slots:
    void checkStopWatch()
    {
        if (stopWatch.elapsed() >= 5000)
            quit();

        QObject o;
        connect(&o, SIGNAL(destroyed()) , object, SLOT(destroSlot()));
        connect(object, SIGNAL(destroyed()) , &o, SLOT(deleteLater()));
    }
};

void tst_QObjectRace::moveToThreadRace()
{
    RaceObject *object = new RaceObject;

    QVarLengthArray<RaceThread *, 16> threads(ThreadCount);
    for (int i = 0; i < ThreadCount; ++i) {
        threads[i] = new RaceThread;
        threads[i]->setObject(object);
    }

    object->moveToThread(threads[0]);

    for (int i = 0; i < ThreadCount; ++i)
        threads[i]->start();

    while(!threads[0]->isFinished()) {
        QPointer<RaceObject> foo (object);
        QObject o;
        connect(&o, SIGNAL(destroyed()) , object, SLOT(destroSlot()));
        connect(object, SIGNAL(destroyed()) , &o, SLOT(deleteLater()));
        QTest::qWait(10);
    }
    // the other threads should finish pretty quickly now
    for (int i = 1; i < ThreadCount; ++i)
        QVERIFY(threads[i]->wait(300));

    for (int i = 0; i < ThreadCount; ++i)
        delete threads[i];
    delete object;
}


class MyObject : public QObject
{   Q_OBJECT
        bool ok;
    public:
        MyObject() : ok(true) {}
        ~MyObject() { Q_ASSERT(ok); ok = false; }
    public slots:
        void slot1() { Q_ASSERT(ok); }
        void slot2() { Q_ASSERT(ok); }
        void slot3() { Q_ASSERT(ok); }
        void slot4() { Q_ASSERT(ok); }
        void slot5() { Q_ASSERT(ok); }
        void slot6() { Q_ASSERT(ok); }
        void slot7() { Q_ASSERT(ok); }
    signals:
        void signal1();
        void signal2();
        void signal3();
        void signal4();
        void signal5();
        void signal6();
        void signal7();
};

namespace {
const char *_slots[] = { SLOT(slot1()) , SLOT(slot2()) , SLOT(slot3()),
                         SLOT(slot4()) , SLOT(slot5()) , SLOT(slot6()),
                         SLOT(slot7()) };

const char *_signals[] = { SIGNAL(signal1()), SIGNAL(signal2()), SIGNAL(signal3()),
                           SIGNAL(signal4()), SIGNAL(signal5()), SIGNAL(signal6()),
                           SIGNAL(signal7()) };

typedef void (MyObject::*PMFType)();
const PMFType _slotsPMF[] = { &MyObject::slot1, &MyObject::slot2, &MyObject::slot3,
                              &MyObject::slot4, &MyObject::slot5, &MyObject::slot6,
                              &MyObject::slot7 };

const PMFType _signalsPMF[] = { &MyObject::signal1, &MyObject::signal2, &MyObject::signal3,
                                &MyObject::signal4, &MyObject::signal5, &MyObject::signal6,
                                &MyObject::signal7 };

}

class DestroyThread : public QThread
{
    Q_OBJECT
    MyObject **objects;
    int number;

public:
    void setObjects(MyObject **o, int n)
    {
        objects = o;
        number = n;
        for(int i = 0; i < number; i++)
            objects[i]->moveToThread(this);
    }

    void run() override
    {
        for (int i = number-1; i >= 0; --i) {
            /* Do some more connection and disconnection between object in this thread that have not been destroyed yet */

            const int nAlive = i+1;
            connect   (objects[((i+1)*31) % nAlive], _signals[(12*i)%7], objects[((i+2)*37) % nAlive],  _slots[(15*i+2)%7] );
            disconnect(objects[((i+1)*31) % nAlive], _signals[(12*i)%7], objects[((i+2)*37) % nAlive],  _slots[(15*i+2)%7] );

            connect   (objects[((i+4)*41) % nAlive], _signalsPMF[(18*i)%7], objects[((i+5)*43) % nAlive],  _slotsPMF[(19*i+2)%7] );
            disconnect(objects[((i+4)*41) % nAlive], _signalsPMF[(18*i)%7], objects[((i+5)*43) % nAlive],  _slotsPMF[(19*i+2)%7] );

            QMetaObject::Connection c = connect(objects[((i+5)*43) % nAlive], _signalsPMF[(9*i+1)%7], Functor());

            for (int f = 0; f < 7; ++f)
                emit (objects[i]->*_signalsPMF[f])();

            disconnect(c);

            disconnect(objects[i], _signalsPMF[(10*i+5)%7], 0, 0);
            disconnect(objects[i], _signals[(11*i+6)%7], 0, 0);

            disconnect(objects[i], 0, objects[(i*17+6) % nAlive], 0);
            if (i%4 == 1) {
                disconnect(objects[i], 0, 0, 0);
            }

            delete objects[i];
        }

        //run the possible queued slots
        qApp->processEvents();
    }
};

#define EXTRA_THREAD_WAIT 3000
#define MAIN_THREAD_WAIT TwoMinutes

void tst_QObjectRace::destroyRace()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Test is too slow to run on emulator");

    constexpr int ObjectCountPerThread = 2777;
    const int ObjectCount = ThreadCount * ObjectCountPerThread;

    QVarLengthArray<MyObject *, ObjectCountPerThread * 10> objects(ObjectCount);
    for (int i = 0; i < ObjectCount; ++i)
        objects[i] = new MyObject;


    for (int i = 0; i < ObjectCount * 17; ++i) {
        connect(objects[(i*13) % ObjectCount], _signals[(2*i)%7],
                objects[((i+2)*17) % ObjectCount],  _slots[(3*i+2)%7] );
        connect(objects[((i+6)*23) % ObjectCount], _signals[(5*i+4)%7],
                objects[((i+8)*41) % ObjectCount],  _slots[(i+6)%7] );

        connect(objects[(i*67) % ObjectCount], _signalsPMF[(2*i)%7],
                objects[((i+1)*71) % ObjectCount],  _slotsPMF[(3*i+2)%7] );
        connect(objects[((i+3)*73) % ObjectCount], _signalsPMF[(5*i+4)%7],
                objects[((i+5)*79) % ObjectCount],  Functor() );
    }

    QVarLengthArray<DestroyThread *, 16> threads(ThreadCount);
    for (int i = 0; i < ThreadCount; ++i) {
        threads[i] = new DestroyThread;
        threads[i]->setObjects(objects.data() + i*ObjectCountPerThread, ObjectCountPerThread);
    }

    for (int i = 0; i < ThreadCount; ++i)
        threads[i]->start();

    QVERIFY(threads[0]->wait(MAIN_THREAD_WAIT));
    // the other threads should finish pretty quickly now
    for (int i = 1; i < ThreadCount; ++i)
        QVERIFY(threads[i]->wait(EXTRA_THREAD_WAIT));

    for (int i = 0; i < ThreadCount; ++i)
        delete threads[i];
}

class BlockingQueuedDestroyRaceObject : public QObject
{
    Q_OBJECT

public:
    enum class Behavior { Normal, Crash };
    explicit BlockingQueuedDestroyRaceObject(Behavior b = Behavior::Normal)
        : m_behavior(b) {}

signals:
    bool aSignal();

public slots:
    bool aSlot()
    {
        switch (m_behavior) {
        case Behavior::Normal:
            return true;
        case Behavior::Crash:
            qFatal("Race detected in a blocking queued connection");
            break;
        }

        Q_UNREACHABLE_RETURN(false);
    }

private:
    Behavior m_behavior;
};

void tst_QObjectRace::blockingQueuedDestroyRace()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires QThread::create");
#else
    enum { MinIterations = 100, MinTime = 3000, WaitTime = 25 };

    BlockingQueuedDestroyRaceObject sender;

    QDeadlineTimer timer(MinTime);
    int iteration = 0;

    while (iteration++ < MinIterations || !timer.hasExpired()) {
        // Manually allocate some storage, and create a receiver in there
        std::optional<BlockingQueuedDestroyRaceObject> receiver;

        receiver.emplace(BlockingQueuedDestroyRaceObject::Behavior::Normal);

        // Connect it to the sender via BlockingQueuedConnection
        QVERIFY(connect(&sender, &BlockingQueuedDestroyRaceObject::aSignal,
                        &*receiver, &BlockingQueuedDestroyRaceObject::aSlot,
                        Qt::BlockingQueuedConnection));

        const auto emitUntilDestroyed = [&sender] {
            // Hack: as long as the receiver is alive and the connection
            // established, the signal will return true (from the slot).
            // When the receiver gets destroyed, the signal is disconnected
            // and therefore the emission returns false.
            while (emit sender.aSignal())
                ;
        };

        std::unique_ptr<QThread> thread(QThread::create(emitUntilDestroyed));
        thread->start();

        QTest::qWait(WaitTime);

        // Destroy the receiver, and immediately allocate a new one at
        // the same address. In case of a race, this might cause:
        // - the metacall event to be posted to a destroyed object;
        // - the metacall event to be posted to the wrong object.
        // In both cases we hope to catch the race by crashing.
        receiver.reset();
        receiver.emplace(BlockingQueuedDestroyRaceObject::Behavior::Crash);

        // Flush events
        QTest::qWait(0);

        thread->wait();
    }
#endif
}

static QAtomicInteger<unsigned> countedStructObjectsCount;
struct CountedFunctor
{
    CountedFunctor() : destroyed(false) { countedStructObjectsCount.fetchAndAddRelaxed(1); }
    CountedFunctor(const CountedFunctor &) : destroyed(false) { countedStructObjectsCount.fetchAndAddRelaxed(1); }
    CountedFunctor &operator=(const CountedFunctor &) { return *this; }
    ~CountedFunctor() { destroyed = true; countedStructObjectsCount.fetchAndAddRelaxed(-1);}
    void operator()() const {QCOMPARE(destroyed, false);}

private:
    bool destroyed;
};

class DisconnectRaceSenderObject : public QObject
{
    Q_OBJECT
signals:
    void theSignal();
};

class DisconnectRaceThread : public QThread
{
    Q_OBJECT

    DisconnectRaceSenderObject *sender;
    bool emitSignal;
public:
    DisconnectRaceThread(DisconnectRaceSenderObject *s, bool emitIt)
        : QThread(), sender(s), emitSignal(emitIt)
    {
    }

    void run() override
    {
        while (!isInterruptionRequested()) {
            QMetaObject::Connection conn = connect(sender, &DisconnectRaceSenderObject::theSignal,
                                                   sender, CountedFunctor(), Qt::BlockingQueuedConnection);
            if (emitSignal)
                emit sender->theSignal();
            disconnect(conn);
            yieldCurrentThread();
        }
    }
};

class DeleteReceiverRaceSenderThread : public QThread
{
    Q_OBJECT

    DisconnectRaceSenderObject *sender;
public:
    DeleteReceiverRaceSenderThread(DisconnectRaceSenderObject *s)
        : QThread(), sender(s)
    {
    }

    void run() override
    {
        while (!isInterruptionRequested()) {
            emit sender->theSignal();
            yieldCurrentThread();
        }
    }
};

class DeleteReceiverRaceReceiver : public QObject
{
    Q_OBJECT

    DisconnectRaceSenderObject *sender;
    QObject *receiver;
    QTimer *timer;
public:
    DeleteReceiverRaceReceiver(DisconnectRaceSenderObject *s)
        : QObject(), sender(s), receiver(0)
    {
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &DeleteReceiverRaceReceiver::onTimeout);
        timer->start(1);
    }
    ~DeleteReceiverRaceReceiver()
    {
        delete receiver;
    }

    void onTimeout()
    {
        if (receiver)
            delete receiver;
        receiver = new QObject;
        connect(sender, &DisconnectRaceSenderObject::theSignal, receiver, CountedFunctor(), Qt::BlockingQueuedConnection);
    }
};

class DeleteReceiverRaceReceiverThread : public QThread
{
    Q_OBJECT

    DisconnectRaceSenderObject *sender;
public:
    DeleteReceiverRaceReceiverThread(DisconnectRaceSenderObject *s)
        : QThread(), sender(s)
    {
    }

    void run() override
    {
        QScopedPointer<DeleteReceiverRaceReceiver> receiver(new DeleteReceiverRaceReceiver(sender));
        exec();
    }
};

void tst_QObjectRace::disconnectRace()
{
    enum { TimeLimit = 3000 };

    QCOMPARE(countedStructObjectsCount.loadRelaxed(), 0u);

    {
        QScopedPointer<DisconnectRaceSenderObject> sender(new DisconnectRaceSenderObject());
        QScopedPointer<QThread> senderThread(new QThread());
        senderThread->start();
        sender->moveToThread(senderThread.data());

        QVarLengthArray<DisconnectRaceThread *, 16> threads(ThreadCount);
        for (int i = 0; i < ThreadCount; ++i) {
            threads[i] = new DisconnectRaceThread(sender.data(), !(i % 10));
            threads[i]->start();
        }

        QTest::qWait(TimeLimit);

        for (int i = 0; i < ThreadCount; ++i) {
            threads[i]->requestInterruption();
            QVERIFY(threads[i]->wait());
            delete threads[i];
        }

        senderThread->quit();
        QVERIFY(senderThread->wait());
    }

    QCOMPARE(countedStructObjectsCount.loadRelaxed(), 0u);

    {
        QScopedPointer<DisconnectRaceSenderObject> sender(new DisconnectRaceSenderObject());
        QScopedPointer<DeleteReceiverRaceSenderThread> senderThread(new DeleteReceiverRaceSenderThread(sender.data()));
        senderThread->start();
        sender->moveToThread(senderThread.data());

        QVarLengthArray<DeleteReceiverRaceReceiverThread *, 16> threads(ThreadCount);
        for (int i = 0; i < ThreadCount; ++i) {
            threads[i] = new DeleteReceiverRaceReceiverThread(sender.data());
            threads[i]->start();
        }

        QTest::qWait(TimeLimit);

        senderThread->requestInterruption();
        QVERIFY(senderThread->wait());

        for (int i = 0; i < ThreadCount; ++i) {
            threads[i]->quit();
            QVERIFY(threads[i]->wait());
            delete threads[i];
        }
    }

    QCOMPARE(countedStructObjectsCount.loadRelaxed(), 0u);
}

void tst_QObjectRace::disconnectRace2()
{
    enum { IterationCount = 100, ConnectionCount = 100, YieldCount = 100 };

    QAtomicPointer<MyObject> ptr;
    QSemaphore createSemaphore(0);
    QSemaphore proceedSemaphore(0);

    std::unique_ptr<QThread> t1(QThread::create([&]() {
        for (int i = 0; i < IterationCount; ++i) {
            MyObject sender;
            ptr.storeRelease(&sender);
            createSemaphore.release();
            proceedSemaphore.acquire();
            ptr.storeRelaxed(nullptr);
            for (int i = 0; i < YieldCount; ++i)
                QThread::yieldCurrentThread();
        }
    }));
    t1->start();


    std::unique_ptr<QThread> t2(QThread::create([&]() {
        auto connections = std::make_unique<QMetaObject::Connection[]>(ConnectionCount);
        for (int i = 0; i < IterationCount; ++i) {
            MyObject receiver;
            MyObject *sender = nullptr;

            createSemaphore.acquire();

            while (!(sender = ptr.loadAcquire()))
                ;

            for (int i = 0; i < ConnectionCount; ++i)
                connections[i] = QObject::connect(sender, &MyObject::signal1, &receiver, &MyObject::slot1);

            proceedSemaphore.release();

            for (int i = 0; i < ConnectionCount; ++i)
                QObject::disconnect(connections[i]);
        }
    }));
    t2->start();

    t1->wait();
    t2->wait();
}

QTEST_MAIN(tst_QObjectRace)
#include "tst_qobjectrace.moc"
