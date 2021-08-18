/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
