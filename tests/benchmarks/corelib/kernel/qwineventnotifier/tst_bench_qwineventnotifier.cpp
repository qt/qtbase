// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qglobal.h>
#include <QtCore/qwineventnotifier.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qvector.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qt_windows.h>

class tst_QWinEventNotifier : public QObject
{
    Q_OBJECT

private slots:
    void waves_data();
    void waves();
};

class EventsFactory : public QObject
{
    Q_OBJECT

public:
    explicit EventsFactory(int waves, int notifiers, int iterations)
        : numberOfWaves(waves), numberOfNotifiers(notifiers),
          numberOfIterations(iterations)
    {
        events.resize(notifiers);
        for (int i = 0; i < notifiers; ++i) {
            events[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
            QVERIFY(events[i] != NULL);
            QWinEventNotifier *notifier = new QWinEventNotifier(events[i], this);
            Q_CHECK_PTR(notifier);

            connect(notifier, &QWinEventNotifier::activated, [i, this]() {
                ResetEvent(this->events[i]);
                if (--this->numberOfIterations == 0)
                    this->eventLoop.quit();
                else
                    SetEvent(this->events[(i + 1) % this->numberOfNotifiers]);
            });

            connect(this, &EventsFactory::stop, [notifier]() {
                notifier->setEnabled(false);
            });
        }
    }
    virtual ~EventsFactory()
    {
        for (auto event : events)
            CloseHandle(event);
    }

    void run()
    {
        Q_ASSERT(numberOfWaves != 0);

        int offset = 0;
        for (int i = 0; i < numberOfWaves; ++i) {
            SetEvent(events[offset]);
            offset += qMax(1, numberOfNotifiers / numberOfWaves);
            offset %= numberOfNotifiers;
        }
        eventLoop.exec();
    }

signals:
    void stop();

protected:
    QVector<HANDLE> events;
    QEventLoop eventLoop;
    int numberOfWaves;
    int numberOfNotifiers;
    int numberOfIterations;
};

void tst_QWinEventNotifier::waves_data()
{
    QTest::addColumn<int>("waves");
    QTest::addColumn<int>("notifiers");
    for (int waves : {1, 3, 10}) {
        for (int notifiers : {10, 100, 1000})
            QTest::addRow("waves: %d, notifiers: %d", waves, notifiers) << waves << notifiers;
    }
}

void tst_QWinEventNotifier::waves()
{
    QFETCH(int, waves);
    QFETCH(int, notifiers);

    const int iterations = 100000;

    EventsFactory factory(waves, notifiers, iterations);

    QElapsedTimer timer;
    timer.start();

    factory.run();

    qDebug("Elapsed time: %.1f s", timer.elapsed() / 1000.0);

    emit factory.stop();
}

QTEST_MAIN(tst_QWinEventNotifier)

#include "tst_bench_qwineventnotifier.moc"
