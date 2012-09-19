/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <stdio.h>

//! [0]
class PingEvent : public QEvent
{
public:
    PingEvent() : QEvent(QEvent::Type(QEvent::User+2))
        {}
};

class PongEvent : public QEvent
{
public:
    PongEvent() : QEvent(QEvent::Type(QEvent::User+3))
        {}
};
//! [0]

//! [1]
class Pinger : public QState
{
public:
    Pinger(QState *parent)
        : QState(parent) {}

protected:
    virtual void onEntry(QEvent *)
    {
        machine()->postEvent(new PingEvent());
        fprintf(stdout, "ping?\n");
    }
};
//! [1]

//! [3]
class PongTransition : public QAbstractTransition
{
public:
    PongTransition() {}

protected:
    virtual bool eventTest(QEvent *e) {
        return (e->type() == QEvent::User+3);
    }
    virtual void onTransition(QEvent *)
    {
        machine()->postDelayedEvent(new PingEvent(), 500);
        fprintf(stdout, "ping?\n");
    }
};
//! [3]

//! [2]
class PingTransition : public QAbstractTransition
{
public:
    PingTransition() {}

protected:
    virtual bool eventTest(QEvent *e) {
        return (e->type() == QEvent::User+2);
    }
    virtual void onTransition(QEvent *)
    {
        machine()->postDelayedEvent(new PongEvent(), 500);
        fprintf(stdout, "pong!\n");
    }
};
//! [2]

//! [4]
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QStateMachine machine;
    QState *group = new QState(QState::ParallelStates);
    group->setObjectName("group");
//! [4]

//! [5]
    Pinger *pinger = new Pinger(group);
    pinger->setObjectName("pinger");
    pinger->addTransition(new PongTransition());

    QState *ponger = new QState(group);
    ponger->setObjectName("ponger");
    ponger->addTransition(new PingTransition());
//! [5]

//! [6]
    machine.addState(group);
    machine.setInitialState(group);
    machine.start();

    return app.exec();
}
//! [6]
