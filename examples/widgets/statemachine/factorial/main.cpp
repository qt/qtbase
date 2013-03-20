/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
class Factorial : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int x READ x WRITE setX)
    Q_PROPERTY(int fac READ fac WRITE setFac)
public:
    Factorial(QObject *parent = 0)
        : QObject(parent), m_x(-1), m_fac(1)
    {
    }

    int x() const
    {
        return m_x;
    }

    void setX(int x)
    {
        if (x == m_x)
            return;
        m_x = x;
        emit xChanged(x);
    }

    int fac() const
    {
        return m_fac;
    }

    void setFac(int fac)
    {
        m_fac = fac;
    }

Q_SIGNALS:
    void xChanged(int value);

private:
    int m_x;
    int m_fac;
};
//! [0]

//! [1]
class FactorialLoopTransition : public QSignalTransition
{
public:
    FactorialLoopTransition(Factorial *fact)
        : QSignalTransition(fact, SIGNAL(xChanged(int))), m_fact(fact)
    {}

    virtual bool eventTest(QEvent *e)
    {
        if (!QSignalTransition::eventTest(e))
            return false;
        QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
        return se->arguments().at(0).toInt() > 1;
    }

    virtual void onTransition(QEvent *e)
    {
        QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
        int x = se->arguments().at(0).toInt();
        int fac = m_fact->property("fac").toInt();
        m_fact->setProperty("fac",  x * fac);
        m_fact->setProperty("x",  x - 1);
    }

private:
    Factorial *m_fact;
};
//! [1]

//! [2]
class FactorialDoneTransition : public QSignalTransition
{
public:
    FactorialDoneTransition(Factorial *fact)
        : QSignalTransition(fact, SIGNAL(xChanged(int))), m_fact(fact)
    {}

    virtual bool eventTest(QEvent *e)
    {
        if (!QSignalTransition::eventTest(e))
            return false;
        QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
        return se->arguments().at(0).toInt() <= 1;
    }

    virtual void onTransition(QEvent *)
    {
        fprintf(stdout, "%d\n", m_fact->property("fac").toInt());
    }

private:
    Factorial *m_fact;
};
//! [2]

//! [3]
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Factorial factorial;
    QStateMachine machine;
//! [3]

//! [4]
    QState *compute = new QState(&machine);
    compute->assignProperty(&factorial, "fac", 1);
    compute->assignProperty(&factorial, "x", 6);
    compute->addTransition(new FactorialLoopTransition(&factorial));
//! [4]

//! [5]
    QFinalState *done = new QFinalState(&machine);
    FactorialDoneTransition *doneTransition = new FactorialDoneTransition(&factorial);
    doneTransition->setTargetState(done);
    compute->addTransition(doneTransition);
//! [5]

//! [6]
    machine.setInitialState(compute);
    QObject::connect(&machine, SIGNAL(finished()), &app, SLOT(quit()));
    machine.start();

    return app.exec();
}
//! [6]

#include "main.moc"
