/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    int ret = 0;

    TestEventDispatcher dispatcher;
    QCoreApplication app(argc, argv);

    /* Run with no special arguments. */
    {
        tst_BenchlibOptions test;
        ret += QTest::qExec(&test, argc, argv);
    }

    /* Run with an exact number of iterations. */
    {
        QVector<char const*> args;
        for (int i = 0; i < argc; ++i) args << argv[i];
        args << "-iterations";
        args << "15";
        tst_BenchlibFifteenIterations test;
        ret += QTest::qExec(&test, args.count(), const_cast<char**>(args.data()));
    }

    /*
        Run until getting a value of at least 100.
    */
    {
        QVector<char const*> args;
        for (int i = 0; i < argc; ++i) args << argv[i];
        args << "-minimumvalue";
        args << "100";
        tst_BenchlibOneHundredMinimum test;
        ret += QTest::qExec(&test, args.count(), const_cast<char**>(args.data()));
    }

    return ret;
}

#include "tst_benchliboptions.moc"
