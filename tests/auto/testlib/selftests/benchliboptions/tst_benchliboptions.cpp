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


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

/* Custom event dispatcher to ensure we don't receive any spontaneous events */
class TestEventDispatcher : public QAbstractEventDispatcher
{
    Q_OBJECT

public:
    TestEventDispatcher(QObject* parent =0)
        : QAbstractEventDispatcher(parent)
    {}
    void flush() {}
    bool hasPendingEvents() { return false; }
    void interrupt() {}
    bool processEvents(QEventLoop::ProcessEventsFlags) { return false; }
    void registerSocketNotifier(QSocketNotifier*) {}
    void registerTimer(int,int,Qt::TimerType,QObject*) {}
    QList<TimerInfo> registeredTimers(QObject*) const { return QList<TimerInfo>(); }
    void unregisterSocketNotifier(QSocketNotifier*) {}
    bool unregisterTimer(int) { return false; }
    bool unregisterTimers(QObject*) { return false; }
    int remainingTime(int) { return 0; }
    void wakeUp() {}

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
        ret += QTest::qExec(&test, args.size(), const_cast<char**>(&args[0]));
    }

    /* Run with an exact number of iterations. */
    {
        auto extraArgs = args;
        extraArgs.push_back("-iterations");
        extraArgs.push_back("15");
        tst_BenchlibFifteenIterations test;
        ret += QTest::qExec(&test, extraArgs.size(), const_cast<char**>(&extraArgs[0]));
    }

    /*
        Run until getting a value of at least 100.
    */
    {
        auto extraArgs = args;
        extraArgs.push_back("-minimumvalue");
        extraArgs.push_back("100");
        tst_BenchlibOneHundredMinimum test;
        ret += QTest::qExec(&test, extraArgs.size(), const_cast<char**>(&extraArgs[0]));
    }

    return ret;
}

#include "tst_benchliboptions.moc"
