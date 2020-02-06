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
    argc = args.size();
    argv = const_cast<char**>(&args[0]);

    TestEventDispatcher dispatcher;
    QCoreApplication app(argc, argv);
    tst_BenchlibEventCounter test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_benchlibeventcounter.moc"
