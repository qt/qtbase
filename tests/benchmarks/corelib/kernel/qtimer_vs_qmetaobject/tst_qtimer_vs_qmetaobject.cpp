// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore>
#include <QTest>
#include <QThread>
#include <QSignalSpy>

#define INVOKE_COUNT 10000

class qtimer_vs_qmetaobject : public QObject
{
    Q_OBJECT
private slots:
    void bench();
    void bench_data();
    void benchBackgroundThread();
    void benchBackgroundThread_data() { bench_data(); }
};

class InvokeCounter : public QObject {
    Q_OBJECT
public:
    InvokeCounter() : count(0) { };
public slots:
    void invokeSlot() {
        count++;
        if (count == INVOKE_COUNT)
            emit allInvoked();
    }
signals:
    void allInvoked();
protected:
    int count;
};

void qtimer_vs_qmetaobject::bench()
{
    QFETCH(int, type);

    std::function<void(InvokeCounter*)> invoke;
    if (type == 0) {
        invoke = [](InvokeCounter* invokeCounter) {
            QTimer::singleShot(0, invokeCounter, SLOT(invokeSlot()));
        };
    } else if (type == 1) {
        invoke = [](InvokeCounter* invokeCounter) {
            QTimer::singleShot(0, invokeCounter, &InvokeCounter::invokeSlot);
        };
    } else if (type == 2) {
        invoke = [](InvokeCounter* invokeCounter) {
            QTimer::singleShot(0, invokeCounter, [invokeCounter]() {
                invokeCounter->invokeSlot();
            });
        };
    } else if (type == 3) {
        invoke = [](InvokeCounter* invokeCounter) {
            QTimer::singleShot(0, [invokeCounter]() {
                invokeCounter->invokeSlot();
            });
        };
    } else if (type == 4) {
        invoke = [](InvokeCounter* invokeCounter) {
            QMetaObject::invokeMethod(invokeCounter, "invokeSlot", Qt::QueuedConnection);
        };
    } else if (type == 5) {
        invoke = [](InvokeCounter* invokeCounter) {
            QMetaObject::invokeMethod(invokeCounter, &InvokeCounter::invokeSlot, Qt::QueuedConnection);
        };
    } else if (type == 6) {
        invoke = [](InvokeCounter* invokeCounter) {
            QMetaObject::invokeMethod(invokeCounter, [invokeCounter]() {
                invokeCounter->invokeSlot();
            }, Qt::QueuedConnection);
        };
    } else {
        QFAIL("unhandled data tag");
    }

    QBENCHMARK {
        InvokeCounter invokeCounter;
        QSignalSpy spy(&invokeCounter, &InvokeCounter::allInvoked);
        for(int i = 0; i < INVOKE_COUNT; ++i) {
            invoke(&invokeCounter);
        }
        QVERIFY(spy.wait(10000));
    }
}

void qtimer_vs_qmetaobject::bench_data()
{
    QTest::addColumn<int>("type");
    QTest::addRow("singleShot_slot") << 0;
    QTest::addRow("singleShot_pmf") << 1;
    QTest::addRow("singleShot_functor") << 2;
    QTest::addRow("singleShot_functor_noctx") << 3;
    QTest::addRow("invokeMethod_string") << 4;
    QTest::addRow("invokeMethod_pmf") << 5;
    QTest::addRow("invokeMethod_functor") << 6;
}

void qtimer_vs_qmetaobject::benchBackgroundThread()
{
    QScopedPointer<QThread> thread(QThread::create([this]() { bench(); }));
    thread->start();
    QVERIFY(thread->wait());
}

QTEST_MAIN(qtimer_vs_qmetaobject)

#include "tst_qtimer_vs_qmetaobject.moc"
