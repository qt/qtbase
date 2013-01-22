/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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

    explicit inline QTestAliveEvent(int aSequenceId)
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
    if (!rec)
        qFatal("Null receiver object passed to QTestAlivePinger::QTestAlivePinger()");
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
    if (!QCoreApplication::instance())
        qFatal("QTestAlive::run(): Cannot start QTestAlive without a QCoreApplication instance.");

    QTestAlivePinger p(this);
    pinger = &p;
    exec();
    pinger = 0;
}


