/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <qcoreapplication.h>
#include <qcoreevent.h>
#include <qthread.h>

class QTestAliveEvent: public QEvent
{
public:

    enum { AliveEventType = QEvent::User + 422 };

    inline QTestAliveEvent(int aSequenceId)
        : QEvent(QEvent::Type(AliveEventType)), seqId(aSequenceId)
    {}
    inline int sequenceId() const { return seqId; }

private:
    int seqId;
};

class QTestAlivePinger: public QObject
{
public:
    QTestAlivePinger(QObject *receiver, QObject *parent = 0);
    bool event(QEvent *e);

protected:
    void timerEvent(QTimerEvent *event);

private:
    QObject *rec;
    int timerId;
    int currentSequenceId;
    int lastSequenceId;
};

QTestAlivePinger::QTestAlivePinger(QObject *receiver, QObject *parent)
    : QObject(parent), rec(receiver), currentSequenceId(0), lastSequenceId(0)
{
    Q_ASSERT(rec);
    timerId = startTimer(850);
}

bool QTestAlivePinger::event(QEvent *event)
{
    // pong received
    if (int(event->type()) == QTestAliveEvent::AliveEventType) {
        QTestAliveEvent *e = static_cast<QTestAliveEvent *>(event);
        //qDebug("PONG %d received", e->sequenceId());
        // if the events are not delivered in order, we don't care.
        if (e->sequenceId() > lastSequenceId)
            lastSequenceId = e->sequenceId();
        return true;
    }
    return QObject::event(event);
}

void QTestAlivePinger::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timerId)
        return;

    if (lastSequenceId < currentSequenceId - 2) {
        qWarning("TEST LAGS %d PINGS behind!", currentSequenceId - lastSequenceId);
    }
    ++currentSequenceId;
    //qDebug("PING %d", currentSequenceId);
    QCoreApplication::postEvent(rec, new QTestAliveEvent(currentSequenceId));
}

class QTestAlive: public QThread
{
public:
    QTestAlive(QObject *parent = 0);
    ~QTestAlive();
    void run();

    bool event(QEvent *e);

private:
    QTestAlivePinger *pinger;
};

QTestAlive::QTestAlive(QObject *parent)
    : QThread(parent), pinger(0)
{
}

QTestAlive::~QTestAlive()
{
    quit();
    while (isRunning());
}

bool QTestAlive::event(QEvent *e)
{
    if (int(e->type()) == QTestAliveEvent::AliveEventType && pinger) {
        // ping received, send back the pong
        //qDebug("PONG %d", static_cast<QTestAliveEvent *>(e)->sequenceId());
        QCoreApplication::postEvent(pinger,
                new QTestAliveEvent(static_cast<QTestAliveEvent *>(e)->sequenceId()));
        return true;
    }
    return QThread::event(e);
}

void QTestAlive::run()
{
    Q_ASSERT_X(QCoreApplication::instance(), "QTestAlive::run()",
               "Cannot start QTestAlive without a QCoreApplication instance.");

    QTestAlivePinger p(this);
    pinger = &p;
    exec();
    pinger = 0;
}


