// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QSignalSpy>

#include <QtConcurrent>
#include <private/qfutureinterface_p.h>

using namespace QtConcurrent;
using namespace std::chrono_literals;

#include <QTest>

//#define PRINT

class tst_QFutureWatcher: public QObject
{
    Q_OBJECT
private slots:
    void startFinish();
    void progressValueChanged();
    void canceled();
    void cancelAndFinish_data();
    void cancelAndFinish();
    void resultAt();
    void resultReadyAt();
    void futureSignals();
    void watchFinishedFuture();
    void watchCanceledFuture();
    void disconnectRunningFuture();
    void tooMuchProgress();
    void progressText();
    void sharedFutureInterface();
    void changeFuture();
    void cancelEvents();
#if QT_DEPRECATED_SINCE(6, 0)
    void pauseEvents();
    void pausedSuspendedOrder();
#endif
    void suspendEvents();
    void suspended();
    void suspendedEventsOrder();
    void throttling();
    void incrementalMapResults();
    void incrementalFilterResults();
    void qfutureSynchronizer();
    void warnRace();
    void matchFlags();
    void checkStateConsistency();
};

void sleeper()
{
    QTest::qSleep(100);
}

void tst_QFutureWatcher::startFinish()
{
    QFutureWatcher<void> futureWatcher;

    int startedCount = 0;
    int finishedCount = 0;
    QObject::connect(&futureWatcher, &QFutureWatcher<void>::started,
                     [&startedCount, &finishedCount](){
        ++startedCount;
        QCOMPARE(startedCount, 1);
        QCOMPARE(finishedCount, 0);
    });
    QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished,
                     [&startedCount, &finishedCount](){
        ++finishedCount;
        QCOMPARE(startedCount, 1);
        QCOMPARE(finishedCount, 1);
    });

    futureWatcher.setFuture(QtConcurrent::run(sleeper));
    futureWatcher.future().waitForFinished();

    // waitForFinished() may unblock before asynchronous
    // started() and finished() signals are delivered to the main thread.
    // prosessEvents() should empty the pending queue.
    qApp->processEvents();

    QCOMPARE(startedCount, 1);
    QCOMPARE(finishedCount, 1);
}

void mapSleeper(int &)
{
    QTest::qSleep(100);
}

static QSet<int> progressValues;
static QSet<QString> progressTexts;
static QMutex mutex;
class ProgressObject : public QObject
{
Q_OBJECT
public slots:
    void printProgress(int);
    void printText(const QString &text);
    void registerProgress(int);
    void registerText(const QString &text);
};

void ProgressObject::printProgress(int progress)
{
    qDebug() << "thread" << QThread::currentThread() << "reports progress" << progress;
}

void ProgressObject::printText(const QString &text)
{
    qDebug() << "thread" << QThread::currentThread() << "reports progress text" << text;
}

void ProgressObject::registerProgress(int progress)
{
    QTest::qSleep(1);
    progressValues.insert(progress);
}

void ProgressObject::registerText(const QString &text)
{
    QTest::qSleep(1);
    progressTexts.insert(text);
}


QList<int> createList(int listSize)
{
    QList<int> list;
    for (int i = 0; i < listSize; ++i) {
        list.append(i);
    }
    return list;
}

void tst_QFutureWatcher::progressValueChanged()
{
#ifdef PRINT
    qDebug() << "main thread" << QThread::currentThread();
#endif

    progressValues.clear();
    const int listSize = 20;
    QList<int> list = createList(listSize);

    QFutureWatcher<void> futureWatcher;
    ProgressObject progressObject;
    QObject::connect(&futureWatcher, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
#ifdef PRINT
    QObject::connect(&futureWatcher, SIGNAL(progressValueChanged(int)), &progressObject, SLOT(printProgress(int)), Qt::DirectConnection );
#endif
    QObject::connect(&futureWatcher, SIGNAL(progressValueChanged(int)), &progressObject, SLOT(registerProgress(int)));

    futureWatcher.setFuture(QtConcurrent::map(list, mapSleeper));

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    futureWatcher.disconnect();
    QVERIFY(progressValues.contains(0));
    QVERIFY(progressValues.contains(listSize));
}

class CancelObject : public QObject
{
Q_OBJECT
public:
    bool wasCanceled;
    CancelObject() : wasCanceled(false) {}
public slots:
    void cancel();
};

void CancelObject::cancel()
{
#ifdef PRINT
    qDebug() << "thread" << QThread::currentThread() << "reports canceled";
#endif
    wasCanceled = true;
}

void tst_QFutureWatcher::canceled()
{
    const int listSize = 20;
    QList<int> list = createList(listSize);

    QFutureWatcher<void> futureWatcher;
    QFuture<void> future;
    CancelObject cancelObject;

    QObject::connect(&futureWatcher, SIGNAL(canceled()), &cancelObject, SLOT(cancel()));
    QObject::connect(&futureWatcher, SIGNAL(canceled()),
        &QTestEventLoop::instance(), SLOT(exitLoop()), Qt::QueuedConnection);

    future = QtConcurrent::map(list, mapSleeper);
    futureWatcher.setFuture(future);
    futureWatcher.cancel();
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QVERIFY(future.isCanceled());
    QVERIFY(cancelObject.wasCanceled);
    futureWatcher.disconnect();
    future.waitForFinished();
}

void tst_QFutureWatcher::cancelAndFinish_data()
{
    QTest::addColumn<bool>("isCanceled");
    QTest::addColumn<bool>("isFinished");

    QTest::addRow("running") << false << false;
    QTest::addRow("canceled") << true << false;
    QTest::addRow("finished") << false << true;
    QTest::addRow("canceledAndFinished") << true << true;
}

void tst_QFutureWatcher::cancelAndFinish()
{
    QFETCH(bool, isCanceled);
    QFETCH(bool, isFinished);

    QFutureInterface<void> fi;
    QFutureWatcher<void> futureWatcher;
    QSignalSpy finishedSpy(&futureWatcher, &QFutureWatcher<void>::finished);
    QSignalSpy canceledSpy(&futureWatcher, &QFutureWatcher<void>::canceled);
    futureWatcher.setFuture(fi.future());

    fi.reportStarted();

    if (isCanceled)
        fi.cancel();
    if (isFinished)
        fi.reportFinished();

    fi.cancelAndFinish();

    // The signals should be emitted only once
    QTRY_COMPARE(canceledSpy.size(), 1);
    QTRY_COMPARE(finishedSpy.size(), 1);
}

class IntTask : public RunFunctionTaskBase<int>
{
public:
    void runFunctor() override
    {
        promise.reportResult(10);
    }
};

void tst_QFutureWatcher::resultAt()
{
    QFutureWatcher<int> futureWatcher;
    futureWatcher.setFuture((new IntTask())->start());
    futureWatcher.waitForFinished();
    QCOMPARE(futureWatcher.result(), 10);
    QCOMPARE(futureWatcher.resultAt(0), 10);
}

void tst_QFutureWatcher::resultReadyAt()
{
    QFutureWatcher<int> futureWatcher;
    QSignalSpy resultSpy(&futureWatcher, &QFutureWatcher<int>::resultReadyAt);

    QFuture<int> future = (new IntTask())->start();
    futureWatcher.setFuture(future);

    QVERIFY(resultSpy.wait());

    // Setting the future again should give us another signal.
    // (this is to prevent the race where the task associated
    // with the future finishes before setFuture is called.)
    futureWatcher.setFuture(QFuture<int>());
    futureWatcher.setFuture(future);

    QVERIFY(resultSpy.wait());
}

class SignalSlotObject : public QObject
{
Q_OBJECT

signals:
    void cancel();

public slots:
    void started()
    {
        qDebug() << "started called";
    }

    void finished()
    {
        qDebug() << "finished called";
    }

    void canceled()
    {
        qDebug() << "canceled called";
    }

#ifdef PRINT
    void resultReadyAt(int index)
    {
        qDebug() << "result" << index << "ready";
    }
#else
    void resultReadyAt(int) { }
#endif
    void progressValueChanged(int progress)
    {
        qDebug() << "progress" << progress;
    }

    void progressRangeChanged(int min, int max)
    {
        qDebug() << "progress range" << min << max;
    }

};

void tst_QFutureWatcher::futureSignals()
{
    {
        QFutureInterface<int> a;
        QFutureWatcher<int> f;

        SignalSlotObject object;
#ifdef PRINT
        connect(&f, SIGNAL(finished()), &object, SLOT(finished()));
        connect(&f, SIGNAL(progressValueChanged(int)), &object, SLOT(progressValueChanged(int)));
#endif
        // must connect to resultReadyAt so that the watcher can detect the connection
        // (QSignalSpy does not trigger it.)
        connect(&f, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));
        a.reportStarted();

        QSignalSpy progressSpy(&f, &QFutureWatcher<void>::progressValueChanged);
        QSignalSpy finishedSpy(&f, &QFutureWatcher<void>::finished);
        QSignalSpy resultReadySpy(&f, &QFutureWatcher<void>::resultReadyAt);

        QVERIFY(progressSpy.isValid());
        QVERIFY(finishedSpy.isValid());
        QVERIFY(resultReadySpy.isValid());
        f.setFuture(a.future());

        const int progress = 1;
        a.setProgressValue(progress);
        QTRY_COMPARE(progressSpy.size(), 2);
        QCOMPARE(progressSpy.takeFirst().at(0).toInt(), 0);
        QCOMPARE(progressSpy.takeFirst().at(0).toInt(), 1);

        const int result = 10;
        a.reportResult(&result);
        QVERIFY(resultReadySpy.wait());
        QCOMPARE(resultReadySpy.size(), 1);
        a.reportFinished(&result);

        QTRY_COMPARE(resultReadySpy.size(), 2);
        QCOMPARE(resultReadySpy.takeFirst().at(0).toInt(), 0); // check the index
        QCOMPARE(resultReadySpy.takeFirst().at(0).toInt(), 1);

        QCOMPARE(finishedSpy.size(), 1);
    }
}

void tst_QFutureWatcher::watchFinishedFuture()
{
    QFutureInterface<int> iface;
    iface.reportStarted();

    QFuture<int> f = iface.future();

    int value = 100;
    iface.reportFinished(&value);

    QFutureWatcher<int> watcher;

    SignalSlotObject object;
#ifdef PRINT
    connect(&watcher, SIGNAL(started()), &object, SLOT(started()));
    connect(&watcher, SIGNAL(canceled()), &object, SLOT(canceled()));
    connect(&watcher, SIGNAL(finished()), &object, SLOT(finished()));
    connect(&watcher, SIGNAL(progressValueChanged(int)), &object, SLOT(progressValueChanged(int)));
    connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &object, SLOT(progressRangeChanged(int,int)));
#endif
    connect(&watcher, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));

    QSignalSpy startedSpy(&watcher, &QFutureWatcher<int>::started);
    QSignalSpy finishedSpy(&watcher, &QFutureWatcher<int>::finished);
    QSignalSpy resultReadySpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
    QSignalSpy canceledSpy(&watcher, &QFutureWatcher<int>::canceled);

    QVERIFY(startedSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(resultReadySpy.isValid());
    QVERIFY(canceledSpy.isValid());

    watcher.setFuture(f);
    QVERIFY(finishedSpy.wait());

    QCOMPARE(startedSpy.size(), 1);
    QCOMPARE(finishedSpy.size(), 1);
    QCOMPARE(resultReadySpy.size(), 1);
    QCOMPARE(canceledSpy.size(), 0);
}

void tst_QFutureWatcher::watchCanceledFuture()
{
    QFuture<int> f;
    QFutureWatcher<int> watcher;

    SignalSlotObject object;
#ifdef PRINT
    connect(&watcher, SIGNAL(started()), &object, SLOT(started()));
    connect(&watcher, SIGNAL(canceled()), &object, SLOT(canceled()));
    connect(&watcher, SIGNAL(finished()), &object, SLOT(finished()));
    connect(&watcher, SIGNAL(progressValueChanged(int)), &object, SLOT(progressValueChanged(int)));
    connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &object, SLOT(progressRangeChanged(int,int)));
#endif
    connect(&watcher, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));

    QSignalSpy startedSpy(&watcher, &QFutureWatcher<int>::started);
    QSignalSpy finishedSpy(&watcher, &QFutureWatcher<int>::finished);
    QSignalSpy resultReadySpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
    QSignalSpy canceledSpy(&watcher, &QFutureWatcher<int>::canceled);

    QVERIFY(startedSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(resultReadySpy.isValid());
    QVERIFY(canceledSpy.isValid());

    watcher.setFuture(f);
    QVERIFY(finishedSpy.wait());

    QCOMPARE(startedSpy.size(), 1);
    QCOMPARE(finishedSpy.size(), 1);
    QCOMPARE(resultReadySpy.size(), 0);
    QCOMPARE(canceledSpy.size(), 1);
}

void tst_QFutureWatcher::disconnectRunningFuture()
{
    QFutureInterface<int> a;
    a.reportStarted();

    QFuture<int> f = a.future();
    QFutureWatcher<int> *watcher = new QFutureWatcher<int>();
    QSignalSpy finishedSpy(watcher, &QFutureWatcher<int>::finished);
    QSignalSpy resultReadySpy(watcher, &QFutureWatcher<int>::resultReadyAt);

    QVERIFY(finishedSpy.isValid());
    QVERIFY(resultReadySpy.isValid());
    watcher->setFuture(f);

    SignalSlotObject object;
    connect(watcher, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));

    const int result = 10;
    a.reportResult(&result);
    QVERIFY(resultReadySpy.wait());
    QCOMPARE(resultReadySpy.size(), 1);

    delete watcher;

    a.reportResult(&result);
    QTest::qWait(10);
    QCOMPARE(resultReadySpy.size(), 1);

    a.reportFinished(&result);
    QTest::qWait(10);
    QCOMPARE(finishedSpy.size(), 0);
}

const int maxProgress = 100000;
class ProgressEmitterTask : public RunFunctionTaskBase<void>
{
public:
    void runFunctor() override
    {
        promise.setProgressRange(0, maxProgress);
        for (int p = 0; p <= maxProgress; ++p)
            promise.setProgressValue(p);
    }
};

void tst_QFutureWatcher::tooMuchProgress()
{
    progressValues.clear();
    ProgressObject o;

    QFutureWatcher<void> f;
    QObject::connect(&f, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
#ifdef PRINT
    QObject::connect(&f, SIGNAL(progressValueChanged(int)), &o, SLOT(printProgress(int)));
#endif
    QObject::connect(&f, SIGNAL(progressValueChanged(int)), &o, SLOT(registerProgress(int)));
    f.setFuture((new ProgressEmitterTask())->start());

    // Android reports ca. 10k progressValueChanged per second
    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(progressValues.contains(maxProgress));
}

template <typename T>
class ProgressTextTask : public RunFunctionTaskBase<T>
{
public:
    void runFunctor() override
    {
        this->promise.setProgressValueAndText(1, QLatin1String("Foo 1"));

        while (this->promise.isProgressUpdateNeeded() == false)
            QTest::qSleep(1);
        this->promise.setProgressValueAndText(2, QLatin1String("Foo 2"));

        while (this->promise.isProgressUpdateNeeded() == false)
            QTest::qSleep(1);
        this->promise.setProgressValueAndText(3, QLatin1String("Foo 3"));

        while (this->promise.isProgressUpdateNeeded() == false)
            QTest::qSleep(1);
        this->promise.setProgressValueAndText(4, QLatin1String("Foo 4"));
    }
};

void tst_QFutureWatcher::progressText()
{
    {   // instantiate API for T=int and T=void.
        ProgressTextTask<int> a;
        ProgressTextTask<void> b;
    }
    {
        progressValues.clear();
        progressTexts.clear();
        QFuture<int> f = ((new ProgressTextTask<int>())->start());
        QFutureWatcher<int> watcher;
        ProgressObject o;
        QObject::connect(&watcher, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
#ifdef PRINT
        QObject::connect(&watcher, SIGNAL(progressValueChanged(int)), &o, SLOT(printProgress(int)));
        QObject::connect(&watcher, SIGNAL(progressTextChanged(QString)), &o, SLOT(printText(QString)));
#endif
        QObject::connect(&watcher, SIGNAL(progressValueChanged(int)), &o, SLOT(registerProgress(int)));
        QObject::connect(&watcher, SIGNAL(progressTextChanged(QString)), &o, SLOT(registerText(QString)));

        watcher.setFuture(f);
        QTestEventLoop::instance().enterLoop(5);
        QVERIFY(!QTestEventLoop::instance().timeout());

        QCOMPARE(f.progressText(), QLatin1String("Foo 4"));
        QCOMPARE(f.progressValue(), 4);
        QVERIFY(progressValues.contains(1));
        QVERIFY(progressValues.contains(2));
        QVERIFY(progressValues.contains(3));
        QVERIFY(progressValues.contains(4));
        QVERIFY(progressTexts.contains(QLatin1String("Foo 1")));
        QVERIFY(progressTexts.contains(QLatin1String("Foo 2")));
        QVERIFY(progressTexts.contains(QLatin1String("Foo 3")));
        QVERIFY(progressTexts.contains(QLatin1String("Foo 4")));
    }
}

template <typename T>
void callInterface(T &obj)
{
    obj.progressValue();
    obj.progressMinimum();
    obj.progressMaximum();
    obj.progressText();

    obj.isStarted();
    obj.isFinished();
    obj.isRunning();
    obj.isCanceled();
    obj.isSuspended();
    obj.isSuspending();

    obj.cancel();
    obj.suspend();
    obj.resume();
    obj.toggleSuspended();
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    obj.isPaused();
    obj.pause();
    obj.togglePaused();
QT_WARNING_POP
#endif
    obj.waitForFinished();

    const T& objConst = obj;
    objConst.progressValue();
    objConst.progressMinimum();
    objConst.progressMaximum();
    objConst.progressText();

    objConst.isStarted();
    objConst.isFinished();
    objConst.isRunning();
    objConst.isCanceled();
    objConst.isSuspending();
    objConst.isSuspended();
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    objConst.isPaused();
QT_WARNING_POP
#endif
}

template <typename T>
void callInterface(const T &obj)
{
    obj.result();
    obj.resultAt(0);
}


// QFutureWatcher and QFuture has a similar interface. Test
// that the functions we want ot have in both are actually
// there.
void tst_QFutureWatcher::sharedFutureInterface()
{
    QFutureInterface<int> iface;
    iface.reportStarted();

    QFuture<int> intFuture = iface.future();

    int value = 0;
    iface.reportFinished(&value);

    QFuture<void> voidFuture;
    QFutureWatcher<int> intWatcher;
    intWatcher.setFuture(intFuture);
    QFutureWatcher<void> voidWatcher;

    callInterface(intFuture);
    callInterface(voidFuture);
    callInterface(intWatcher);
    callInterface(voidWatcher);

    callInterface(intFuture);
    callInterface(intWatcher);
}

void tst_QFutureWatcher::changeFuture()
{
    QFutureInterface<int> iface;
    iface.reportStarted();

    QFuture<int> a = iface.future();

    int value = 0;
    iface.reportFinished(&value);

    QFuture<int> b;

    QFutureWatcher<int> watcher;

    SignalSlotObject object;
    connect(&watcher, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));
    QSignalSpy resultReadySpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
    QVERIFY(resultReadySpy.isValid());

    watcher.setFuture(a); // Watch 'a' which will generate a resultReady event.
    watcher.setFuture(b); // But oh no! we're switching to another future
    QTest::qWait(10);     // before the event gets delivered.

    QCOMPARE(resultReadySpy.size(), 0);

    watcher.setFuture(a);
    watcher.setFuture(b);
    watcher.setFuture(a); // setting it back gets us one event, not two.
    QVERIFY(resultReadySpy.wait());

    QCOMPARE(resultReadySpy.size(), 1);
}

// Test that events aren't delivered from canceled futures
void tst_QFutureWatcher::cancelEvents()
{
    QFutureInterface<int> iface;
    iface.reportStarted();

    QFuture<int> a = iface.future();

    int value = 0;
    iface.reportFinished(&value);

    QFutureWatcher<int> watcher;

    SignalSlotObject object;
    connect(&watcher, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));
    QSignalSpy finishedSpy(&watcher, &QFutureWatcher<int>::finished);
    QSignalSpy resultReadySpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
    QVERIFY(finishedSpy.isValid());
    QVERIFY(resultReadySpy.isValid());

    watcher.setFuture(a);
    watcher.cancel();

    QVERIFY(finishedSpy.wait());

    QCOMPARE(resultReadySpy.size(), 0);
}

#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
// Tests that events from paused futures are saved and
// delivered on resume.
void tst_QFutureWatcher::pauseEvents()
{
    {
        QFutureInterface<int> iface;
        iface.reportStarted();

        QFutureWatcher<int> watcher;

        SignalSlotObject object;
        connect(&watcher, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));
        QSignalSpy resultReadySpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
        QVERIFY(resultReadySpy.isValid());

        QSignalSpy pauseSpy(&watcher, &QFutureWatcher<int>::paused);
        QVERIFY(pauseSpy.isValid());

        watcher.setFuture(iface.future());
        watcher.pause();

        QTRY_COMPARE(pauseSpy.size(), 1);

        int value = 0;
        iface.reportFinished(&value);

        // A result is reported, although the watcher is paused.
        // The corresponding event should be also reported.
        QTRY_COMPARE(resultReadySpy.size(), 1);

        watcher.resume();
    }
    {
        QFutureInterface<int> iface;
        iface.reportStarted();

        QFuture<int> a = iface.future();

        QFutureWatcher<int> watcher;

        SignalSlotObject object;
        connect(&watcher, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));
        QSignalSpy resultReadySpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
        QVERIFY(resultReadySpy.isValid());

        watcher.setFuture(a);
        a.pause();

        int value = 0;
        iface.reportFinished(&value);

        QFuture<int> b;
        watcher.setFuture(b); // If we watch b instead, resuming a
        a.resume();           // should give us no results.

        QTest::qWait(10);
        QCOMPARE(resultReadySpy.size(), 0);
    }
}

void tst_QFutureWatcher::pausedSuspendedOrder()
{
    QFutureInterface<void> iface;
    iface.reportStarted();

    QFutureWatcher<void> watcher;

    QSignalSpy pausedSpy(&watcher, &QFutureWatcher<void>::paused);
    QVERIFY(pausedSpy.isValid());

    QSignalSpy suspendedSpy(&watcher, &QFutureWatcher<void>::suspended);
    QVERIFY(suspendedSpy.isValid());

    bool pausedBeforeSuspended = false;
    bool notSuspendedBeforePaused = false;
    connect(&watcher, &QFutureWatcher<void>::paused,
            [&] { notSuspendedBeforePaused = (suspendedSpy.size() == 0); });
    connect(&watcher, &QFutureWatcher<void>::suspended,
            [&] { pausedBeforeSuspended = (pausedSpy.size() == 1); });

    watcher.setFuture(iface.future());
    iface.reportSuspended();

    // Make sure reportPaused() is ignored if the state is not paused
    pausedSpy.wait(100);
    QCOMPARE(pausedSpy.size(), 0);
    QCOMPARE(suspendedSpy.size(), 0);

    iface.setPaused(true);
    iface.reportSuspended();

    QTRY_COMPARE(suspendedSpy.size(), 1);
    QCOMPARE(pausedSpy.size(), 1);
    QVERIFY(notSuspendedBeforePaused);
    QVERIFY(pausedBeforeSuspended);

    iface.reportFinished();
}
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)

// Tests that events from suspended futures are saved and
// delivered on resume.
void tst_QFutureWatcher::suspendEvents()
{
    {
        QFutureInterface<int> iface;
        iface.reportStarted();

        QFutureWatcher<int> watcher;

        SignalSlotObject object;
        connect(&watcher, &QFutureWatcher<int>::resultReadyAt, &object,
                &SignalSlotObject::resultReadyAt);
        QSignalSpy resultReadySpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
        QVERIFY(resultReadySpy.isValid());

        QSignalSpy suspendingSpy(&watcher, &QFutureWatcher<int>::suspending);
        QVERIFY(suspendingSpy.isValid());

        watcher.setFuture(iface.future());
        watcher.suspend();

        QTRY_COMPARE(suspendingSpy.size(), 1);

        int value = 0;
        iface.reportFinished(&value);

        // A result is reported, although the watcher is paused.
        // The corresponding event should be also reported.
        QTRY_COMPARE(resultReadySpy.size(), 1);

        watcher.resume();
    }
    {
        QFutureInterface<int> iface;
        iface.reportStarted();

        QFuture<int> a = iface.future();

        QFutureWatcher<int> watcher;

        SignalSlotObject object;
        connect(&watcher, &QFutureWatcher<int>::resultReadyAt, &object,
                &SignalSlotObject::resultReadyAt);
        QSignalSpy resultReadySpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
        QVERIFY(resultReadySpy.isValid());

        watcher.setFuture(a);
        a.suspend();

        int value = 0;
        iface.reportFinished(&value);

        QFuture<int> b;
        watcher.setFuture(b); // If we watch b instead, resuming a
        a.resume();           // should give us no results.

        QTest::qWait(10);
        QCOMPARE(resultReadySpy.size(), 0);
    }
}

void tst_QFutureWatcher::suspended()
{
    QFutureWatcher<int> watcher;
    QSignalSpy resultReadySpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QSignalSpy pausedSpy(&watcher, &QFutureWatcher<int>::paused);
QT_WARNING_POP
#endif
    QSignalSpy suspendingSpy(&watcher, &QFutureWatcher<int>::suspending);
    QSignalSpy suspendedSpy(&watcher, &QFutureWatcher<int>::suspended);
    QSignalSpy finishedSpy(&watcher, &QFutureWatcher<int>::finished);

    const int numValues = 25;
    std::vector<int> values(numValues, 0);
    std::atomic_int count = 0;

    QThreadPool pool;
    pool.setMaxThreadCount(3);

    QFuture<int> future = QtConcurrent::mapped(&pool, values, [&](int value) {
        ++count;
        // Sleep, to make sure not all threads will start at once.
        QThread::sleep(50ms);
        return value;
    });
    watcher.setFuture(future);

    // Allow some threads to start before suspending.
    QThread::sleep(200ms);

    watcher.suspend();
    watcher.suspend();
    QTRY_COMPARE(suspendedSpy.size(), 1); // suspended() should be emitted only once
    QCOMPARE(suspendingSpy.size(), 2); // suspending() is emitted as many times as requested
#if QT_DEPRECATED_SINCE(6, 0)
    QCOMPARE(pausedSpy.size(), 2); // paused() is emitted as many times as requested
#endif

    // Make sure QFutureWatcher::resultReadyAt() is emitted only for already started threads.
    const auto resultReadyAfterPaused = resultReadySpy.size();
    QCOMPARE(resultReadyAfterPaused, count);

    // Make sure no more results are reported before resuming.
    QThread::sleep(200ms);
    QCOMPARE(resultReadyAfterPaused, resultReadySpy.size());
    resultReadySpy.clear();

    watcher.resume();
    QTRY_COMPARE(finishedSpy.size(), 1);

    // Make sure that no more suspended() signals have been emitted.
    QCOMPARE(suspendedSpy.size(), 1);

    // Make sure the rest of results were reported after resume.
    QCOMPARE(resultReadySpy.size(), numValues - resultReadyAfterPaused);
}

void tst_QFutureWatcher::suspendedEventsOrder()
{
    QFutureInterface<void> iface;
    iface.reportStarted();

    QFutureWatcher<void> watcher;

    QSignalSpy suspendingSpy(&watcher, &QFutureWatcher<void>::suspending);
    QVERIFY(suspendingSpy.isValid());

    QSignalSpy suspendedSpy(&watcher, &QFutureWatcher<void>::suspended);
    QVERIFY(suspendedSpy.isValid());

    bool suspendingBeforeSuspended = false;
    bool notSuspendedBeforeSuspending = false;
    connect(&watcher, &QFutureWatcher<void>::suspending,
            [&] { notSuspendedBeforeSuspending = (suspendedSpy.size() == 0); });
    connect(&watcher, &QFutureWatcher<void>::suspended,
            [&] { suspendingBeforeSuspended = (suspendingSpy.size() == 1); });

    watcher.setFuture(iface.future());
    iface.reportSuspended();

    // Make sure reportPaused() is ignored if the state is not paused
    suspendingSpy.wait(100);
    QCOMPARE(suspendingSpy.size(), 0);
    QCOMPARE(suspendedSpy.size(), 0);

    iface.setSuspended(true);
    iface.reportSuspended();

    QTRY_COMPARE(suspendedSpy.size(), 1);
    QCOMPARE(suspendingSpy.size(), 1);
    QVERIFY(notSuspendedBeforeSuspending);
    QVERIFY(suspendingBeforeSuspended);

    iface.reportFinished();
}

/*
    Verify that throttling kicks in if you report a lot of results,
    and that it clears when the result events are processed.
*/
void tst_QFutureWatcher::throttling()
{
    QFutureInterface<int> iface;
    iface.reportStarted();
    QFuture<int> future = iface.future();
    QFutureWatcher<int> watcher;
    QSignalSpy resultSpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
    watcher.setFuture(future);

    QVERIFY(!iface.isThrottled());

    const int resultCount = 1000;
    for (int i = 0; i < resultCount; ++i) {
        int result = 0;
        iface.reportResult(result);
    }

    QVERIFY(iface.isThrottled());

    QTRY_COMPARE(resultSpy.size(), resultCount); // Process the results

    QVERIFY(!iface.isThrottled());

    iface.reportFinished();
}

int mapper(const int &i)
{
    return i;
}

class ResultReadyTester : public QObject
{
Q_OBJECT
public:
    ResultReadyTester(QFutureWatcher<int> *watcher)
    :m_watcher(watcher), filter(false), ok(true), count(0)
    {

    }
public slots:
    void resultReadyAt(int index)
    {
        ++count;
        if (m_watcher->future().isResultReadyAt(index) == false)
            ok = false;
        if (!filter && m_watcher->future().resultAt(index) != index)
            ok = false;
        if (filter && m_watcher->future().resultAt(index) != index * 2 + 1)
            ok = false;
    }
public:
    QFutureWatcher<int> *m_watcher;
    bool filter;
    bool ok;
    int count;
};

void tst_QFutureWatcher::incrementalMapResults()
{
    QFutureWatcher<int> watcher;

    SignalSlotObject object;
#ifdef PRINT
    connect(&watcher, SIGNAL(finished()), &object, SLOT(finished()));
    connect(&watcher, SIGNAL(progressValueChanged(int)), &object, SLOT(progressValueChanged(int)));
    connect(&watcher, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));
#endif

    QObject::connect(&watcher, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    ResultReadyTester resultReadyTester(&watcher);
    connect(&watcher, SIGNAL(resultReadyAt(int)), &resultReadyTester, SLOT(resultReadyAt(int)));

    const int count = 10000;
    QList<int> ints;
    for (int i = 0; i < count; ++i)
        ints << i;

    QFuture<int> future = QtConcurrent::mapped(ints, mapper);
    watcher.setFuture(future);

    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(resultReadyTester.count, count);
    QVERIFY(resultReadyTester.ok);
    QVERIFY(watcher.isFinished());
    future.waitForFinished();
}

bool filterer(int i)
{
    return (i % 2);
}

void tst_QFutureWatcher::incrementalFilterResults()
{
    QFutureWatcher<int> watcher;

    SignalSlotObject object;
#ifdef PRINT
    connect(&watcher, SIGNAL(finished()), &object, SLOT(finished()));
    connect(&watcher, SIGNAL(progressValueChanged(int)), &object, SLOT(progressValueChanged(int)));
    connect(&watcher, SIGNAL(resultReadyAt(int)), &object, SLOT(resultReadyAt(int)));
#endif

    QObject::connect(&watcher, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));


    ResultReadyTester resultReadyTester(&watcher);
    resultReadyTester.filter = true;
    connect(&watcher, SIGNAL(resultReadyAt(int)), &resultReadyTester, SLOT(resultReadyAt(int)));

    const int count = 10000;
    QList<int> ints;
    for (int i = 0; i < count; ++i)
        ints << i;

    QFuture<int> future = QtConcurrent::filtered(ints, filterer);
    watcher.setFuture(future);

    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(resultReadyTester.count, count / 2);
    QVERIFY(resultReadyTester.ok);
    QVERIFY(watcher.isFinished());
    future.waitForFinished();
}

void tst_QFutureWatcher::qfutureSynchronizer()
{
    int taskCount = 1000;
    QElapsedTimer t;
    t.start();

    {
        QFutureSynchronizer<void> sync;

        sync.setCancelOnWait(true);
        for (int i = 0; i < taskCount; ++i) {
            sync.addFuture(run(sleeper));
        }
    }

    // Test that we're not running each task.
    QVERIFY(t.elapsed() < taskCount * 10);
}

class DummyObject : public QObject {
    Q_OBJECT
public slots:
    void dummySlot() {}
public:
    static void function(QMutex *m)
    {
        QMutexLocker lock(m);
    }
};

void tst_QFutureWatcher::warnRace()
{
#ifndef Q_OS_DARWIN // I don't know why it is not working on mac
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtWarningMsg, "QFutureWatcher::connect: connecting after calling setFuture() is likely to produce race");
#endif
#endif
    QFutureWatcher<void> watcher;
    DummyObject object;
    QMutex mutex;
    mutex.lock();

    QFuture<void> future = QtConcurrent::run(DummyObject::function, &mutex);
    watcher.setFuture(future);
    QTRY_VERIFY(future.isStarted());
    connect(&watcher, SIGNAL(finished()), &object, SLOT(dummySlot()));
    mutex.unlock();
    future.waitForFinished();
}

void tst_QFutureWatcher::matchFlags()
{
    /* Regression test: expect a default watcher to be in the same state as a
     * default future. */
    QFutureWatcher<int> watcher;
    QFuture<int> future;
    QCOMPARE(watcher.isStarted(), future.isStarted());
    QCOMPARE(watcher.isCanceled(), future.isCanceled());
    QCOMPARE(watcher.isFinished(), future.isFinished());
}

void tst_QFutureWatcher::checkStateConsistency()
{
#define CHECK_FAIL(state)                                                                          \
    do {                                                                                           \
        if (QTest::currentTestFailed())                                                            \
            QFAIL("checkState() failed, QFutureWatcher has inconistent state after " state "!");   \
    } while (false)

    QFutureWatcher<void> futureWatcher;

    auto checkState = [&futureWatcher] {
        QCOMPARE(futureWatcher.isStarted(), futureWatcher.future().isStarted());
        QCOMPARE(futureWatcher.isRunning(), futureWatcher.future().isRunning());
        QCOMPARE(futureWatcher.isCanceled(), futureWatcher.future().isCanceled());
        QCOMPARE(futureWatcher.isSuspended(), futureWatcher.future().isSuspended());
        QCOMPARE(futureWatcher.isSuspending(), futureWatcher.future().isSuspending());
        QCOMPARE(futureWatcher.isFinished(), futureWatcher.future().isFinished());
    };

    checkState();
    CHECK_FAIL("default-constructing");

    QFutureInterface<void> fi;
    futureWatcher.setFuture(fi.future());
    checkState();
    CHECK_FAIL("setting future");

    fi.reportStarted();
    checkState();
    CHECK_FAIL("starting");

    fi.future().suspend();
    checkState();
    CHECK_FAIL("suspending");

    fi.reportSuspended();
    checkState();
    CHECK_FAIL("suspended");

    fi.reportCanceled();
    checkState();
    CHECK_FAIL("canceling");

    fi.reportFinished();
    checkState();
    CHECK_FAIL("finishing");

#undef CHECK_FAIL
}

QTEST_MAIN(tst_QFutureWatcher)
#include "tst_qfuturewatcher.moc"
