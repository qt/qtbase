// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QTest>
#if defined(Q_OS_WIN32)
#include <QWinEventNotifier>
#endif
#include <QAbstractEventDispatcher>

/* Custom event dispatcher to ensure we don't receive any spontaneous events */
class TestEventDispatcher : public QAbstractEventDispatcherV2
{
    Q_OBJECT

public:
    void interrupt() override {}
    bool processEvents(QEventLoop::ProcessEventsFlags) override { return false; }
    void registerSocketNotifier(QSocketNotifier*) override {}
    void registerTimer(Qt::TimerId,Duration,Qt::TimerType,QObject*) override {}
    QList<TimerInfoV2> timersForObject(QObject*) const override { return {}; }
    void unregisterSocketNotifier(QSocketNotifier*) override {}
    bool unregisterTimer(Qt::TimerId) override { return false; }
    bool unregisterTimers(QObject*) override { return false; }
    Duration remainingTime(Qt::TimerId) const override { return {}; }
    void wakeUp() override {}

#ifdef Q_OS_WIN
    bool registerEventNotifier(QWinEventNotifier *) { return false; }
    void unregisterEventNotifier(QWinEventNotifier *) { }
#endif
};

class tst_BenchlibOptions: public QObject
{
    Q_OBJECT

private slots:
    void threeEvents();
};

class tst_BenchlibFifteenIterations : public tst_BenchlibOptions
{ Q_OBJECT };
class tst_BenchlibOneHundredMinimum : public tst_BenchlibOptions
{ Q_OBJECT };

void tst_BenchlibOptions::threeEvents()
{
    QAbstractEventDispatcher* ed = QAbstractEventDispatcher::instance();
    QBENCHMARK {
        ed->filterNativeEvent("", 0, 0);
        ed->filterNativeEvent("", 0, 0);
        ed->filterNativeEvent("", 0, 0);
    }
}

int main(int argc, char** argv)
{
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-eventcounter");

    int ret = 0;

    TestEventDispatcher dispatcher;
    QCoreApplication app(argc, argv);

    /* Run with no special arguments. */
    {
        tst_BenchlibOptions test;
        ret += QTest::qExec(&test, int(args.size()), const_cast<char**>(&args[0]));
    }

    /* Run with an exact number of iterations. */
    {
        auto extraArgs = args;
        extraArgs.push_back("-iterations");
        extraArgs.push_back("15");
        tst_BenchlibFifteenIterations test;
        ret += QTest::qExec(&test, int(extraArgs.size()), const_cast<char**>(&extraArgs[0]));
    }

    /*
        Run until getting a value of at least 100.
    */
    {
        auto extraArgs = args;
        extraArgs.push_back("-minimumvalue");
        extraArgs.push_back("100");
        tst_BenchlibOneHundredMinimum test;
        ret += QTest::qExec(&test, int(extraArgs.size()), const_cast<char**>(&extraArgs[0]));
    }

    return ret;
}

#include "tst_benchliboptions.moc"
