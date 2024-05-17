// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef EVENTFILTER
#define EVENTFILTER

#include <QWidget>
#include <QList>
#include <QEvent>
#include <QBasicTimer>

class ExpectedEventList : public QObject
{
    QList<QEvent *> eventList;
    QBasicTimer timer;
    int debug;
    int eventCount;
    void timerEvent(QTimerEvent *);

public:
    ExpectedEventList(QObject *target);
    ~ExpectedEventList();
    void append(QEvent *e);
    bool waitForAllEvents(int timeoutPerEvent = 2000);
    bool eventFilter(QObject *obj, QEvent *event);

private:
    void compareMouseEvents(QEvent *event1, QEvent *event2);
    void compareKeyEvents(QEvent *event1, QEvent *event2);
};

#endif

