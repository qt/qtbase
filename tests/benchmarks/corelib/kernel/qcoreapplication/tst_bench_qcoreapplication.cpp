// Copyright (C) 2011 Robin Burchell <robin+qt@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtCore>
#include <qtest.h>
#include <qcoreapplication.h>

class tst_QCoreApplication : public QObject
{
Q_OBJECT
private slots:
    void event_posting_benchmark_data();
    void event_posting_benchmark();

    void event_posting_multiple_objects_benchmark_data();
    void event_posting_multiple_objects_benchmark();

    void compress_timer_events_during_delivery_data();
    void compress_timer_events_during_delivery();
};

namespace {
// Posts a timer event to target whenever it receives a QEvent::User,
// i.e. while sendPostedEvents() is busy delivering the queue.
class TimerEventPoster : public QObject
{
public:
    explicit TimerEventPoster(QObject *target, int timerId) : target(target), timerId(timerId) {}

    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::User) {
            QCoreApplication::postEvent(target, new QTimerEvent(timerId));
            return true;
        }
        return QObject::event(e);
    }

private:
    QObject *target;
    int timerId;
};
} // unnamed namespace

void tst_QCoreApplication::event_posting_benchmark_data()
{
    QTest::addColumn<int>("size");
    QTest::newRow("50 events") << 50;
    QTest::newRow("100 events") << 100;
    QTest::newRow("200 events") << 200;
    QTest::newRow("1000 events") << 1000;
    QTest::newRow("10000 events") << 10000;
    QTest::newRow("100000 events") << 100000;
    QTest::newRow("1000000 events") << 1000000;
}

void tst_QCoreApplication::event_posting_benchmark()
{
    QFETCH(int, size);

    int type = QEvent::registerEventType();
    QCoreApplication *app = QCoreApplication::instance();

    // benchmark posting & sending events
    QBENCHMARK {
        for (int i = 0; i < size; ++i)
            QCoreApplication::postEvent(app, new QEvent(QEvent::Type(type)));
        QCoreApplication::sendPostedEvents();
    }
}

void tst_QCoreApplication::event_posting_multiple_objects_benchmark_data()
{
    event_posting_benchmark_data();
}

void tst_QCoreApplication::event_posting_multiple_objects_benchmark()
{
    QFETCH(int, size);

    QObject objects[15]; // The size of the array has not been chosen through any meaningful means

    QRandomGenerator gen;

    // benchmark posting & sending events
    QBENCHMARK {
        for (int i = 0; i < size; ++i) {
            QCoreApplication::postEvent(&objects[gen.bounded(0, int(std::size(objects)))],
                                        new QTimerEvent(i % 10));
        }
        QCoreApplication::sendPostedEvents();
    }
}

void tst_QCoreApplication::compress_timer_events_during_delivery_data()
{
    QTest::addColumn<int>("size");
    QTest::newRow("100 events") << 100;
    QTest::newRow("1000 events") << 1000;
    QTest::newRow("10000 events") << 10000;
}

void tst_QCoreApplication::compress_timer_events_during_delivery()
{
    QFETCH(int, size);

    constexpr int timerId = 42;
    QObject target;
    TimerEventPoster poster(&target, timerId);

    QBENCHMARK {
        for (int i = 0; i < size; ++i)
            QCoreApplication::postEvent(&poster, new QEvent(QEvent::User));
        // pending timer event at the end of the queue: every timer event
        // posted while delivering the events above gets compressed into it
        QCoreApplication::postEvent(&target, new QTimerEvent(timerId), Qt::LowEventPriority);
        QCoreApplication::sendPostedEvents();
    }
}

QTEST_MAIN(tst_QCoreApplication)

#include "tst_bench_qcoreapplication.moc"
