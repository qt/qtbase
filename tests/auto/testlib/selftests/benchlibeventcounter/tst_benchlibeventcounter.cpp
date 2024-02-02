// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QTest>
#if defined(Q_OS_WIN32)
#include <QWinEventNotifier>
#endif
#include <QAbstractEventDispatcher>

/* Custom event dispatcher to ensure we don't receive any spontaneous events */
class TestEventDispatcher : public QAbstractEventDispatcher
{
    Q_OBJECT

public:
    TestEventDispatcher(QObject* parent =0)
        : QAbstractEventDispatcher(parent)
    {}
    void interrupt() override {}
    bool processEvents(QEventLoop::ProcessEventsFlags) override { return false; }
    void registerSocketNotifier(QSocketNotifier*) override {}
    void registerTimer(int,qint64,Qt::TimerType,QObject*) override {}
    QList<TimerInfo> registeredTimers(QObject*) const override { return QList<TimerInfo>(); }
    void unregisterSocketNotifier(QSocketNotifier*) override {}
    bool unregisterTimer(int) override { return false; }
    bool unregisterTimers(QObject*) override { return false; }
    int remainingTime(int) override { return 0; }
    void wakeUp() override {}

#ifdef Q_OS_WIN
    bool registerEventNotifier(QWinEventNotifier *) { return false; }
    void unregisterEventNotifier(QWinEventNotifier *) { }
#endif
};

class tst_BenchlibEventCounter: public QObject
{
    Q_OBJECT

private slots:
    void events();
    void events_data();
};

void tst_BenchlibEventCounter::events()
{
    QFETCH(int, eventCount);

    QAbstractEventDispatcher* ed = QAbstractEventDispatcher::instance();
    QBENCHMARK {
        for (int i = 0; i < eventCount; ++i) {
            ed->filterNativeEvent("", 0, 0);
        }
    }
}

void tst_BenchlibEventCounter::events_data()
{
    QTest::addColumn<int>("eventCount");

    QTest::newRow("0")      << 0;
    QTest::newRow("1")      << 1;
    QTest::newRow("10")     << 10;
    QTest::newRow("100")    << 100;
    QTest::newRow("500")    << 500;
    QTest::newRow("5000")   << 5000;
    QTest::newRow("100000") << 100000;
}

int main(int argc, char** argv)
{
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-eventcounter");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);

    TestEventDispatcher dispatcher;
    QCoreApplication app(argc, argv);
    tst_BenchlibEventCounter test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_benchlibeventcounter.moc"
