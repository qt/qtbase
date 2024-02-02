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
};

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

QTEST_MAIN(tst_QCoreApplication)

#include "tst_bench_qcoreapplication.moc"
