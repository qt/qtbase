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


#include <QtCore>
#include <QtTest/QtTest>


enum { OneMinute = 60 * 1000,
       TwoMinutes = OneMinute * 2 };


struct Functor
{
    void operator()() const {};
};

class tst_QObjectRace: public QObject
{
    Q_OBJECT
private slots:
    void moveToThreadRace();
    void destroyRace();
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
    QTime stopWatch;

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

    void run() {
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

    enum { ThreadCount = 6 };
    RaceThread *threads[ThreadCount];
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

    void run() {
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
    enum { ThreadCount = 10, ObjectCountPerThread = 2777,
           ObjectCount = ThreadCount * ObjectCountPerThread };

    MyObject *objects[ObjectCount];
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

    DestroyThread *threads[ThreadCount];
    for (int i = 0; i < ThreadCount; ++i) {
        threads[i] = new DestroyThread;
        threads[i]->setObjects(objects + i*ObjectCountPerThread, ObjectCountPerThread);
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


QTEST_MAIN(tst_QObjectRace)
#include "tst_qobjectrace.moc"
