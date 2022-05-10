// Copyright (C) 2011 Robin Burchell <robin+qt@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtCore>
#include <qtest.h>
#include <qcoreapplication.h>

class tst_QCoreApplication : public QObject
{
Q_OBJECT
private slots:
    void event_posting_benchmark_data();
    void event_posting_benchmark();
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

QTEST_MAIN(tst_QCoreApplication)

#include "tst_bench_qcoreapplication.moc"
