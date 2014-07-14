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
#include <QtWidgets>

//![15]
class StateSwitchEvent: public QEvent
{
public:
    StateSwitchEvent()
        : QEvent(Type(StateSwitchType))
    {
    }

    explicit StateSwitchEvent(int rand)
        : QEvent(Type(StateSwitchType)),
          m_rand(rand)
    {
    }

    enum { StateSwitchType = QEvent::User + 256 };

    int rand() const { return m_rand; }

private:
    int m_rand;
};
//![15]

//![16]
class QGraphicsRectWidget : public QGraphicsWidget
{
public:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *,
               QWidget *) Q_DECL_OVERRIDE
    {
        painter->fillRect(rect(), Qt::blue);
    }
};
//![16]

class StateSwitchTransition: public QAbstractTransition
{
public:
    StateSwitchTransition(int rand)
        : QAbstractTransition(),
          m_rand(rand)
    {
    }

protected:
//![14]
    virtual bool eventTest(QEvent *event) Q_DECL_OVERRIDE
    {
        return (event->type() == QEvent::Type(StateSwitchEvent::StateSwitchType))
            && (static_cast<StateSwitchEvent *>(event)->rand() == m_rand);
    }
//![14]

    virtual void onTransition(QEvent *) Q_DECL_OVERRIDE {}

private:
    int m_rand;
};

//![10]
class StateSwitcher : public QState
{
    Q_OBJECT
public:
    StateSwitcher(QStateMachine *machine)
        : QState(machine), m_stateCount(0), m_lastIndex(0)
    { }
//![10]

//![11]
    virtual void onEntry(QEvent *) Q_DECL_OVERRIDE
    {
        int n;
        while ((n = (qrand() % m_stateCount + 1)) == m_lastIndex)
        { }
        m_lastIndex = n;
        machine()->postEvent(new StateSwitchEvent(n));
    }
    virtual void onExit(QEvent *) Q_DECL_OVERRIDE {}
//![11]

//![12]
    void addState(QState *state, QAbstractAnimation *animation) {
        StateSwitchTransition *trans = new StateSwitchTransition(++m_stateCount);
        trans->setTargetState(state);
        addTransition(trans);
        trans->addAnimation(animation);
    }
//![12]

private:
    int m_stateCount;
    int m_lastIndex;
};

//![13]
QState *createGeometryState(QObject *w1, const QRect &rect1,
                            QObject *w2, const QRect &rect2,
                            QObject *w3, const QRect &rect3,
                            QObject *w4, const QRect &rect4,
                            QState *parent)
{
    QState *result = new QState(parent);
    result->assignProperty(w1, "geometry", rect1);
    result->assignProperty(w2, "geometry", rect2);
    result->assignProperty(w3, "geometry", rect3);
    result->assignProperty(w4, "geometry", rect4);

    return result;
}
//![13]


class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(QGraphicsScene *scene, QWidget *parent = NULL) : QGraphicsView(scene, parent)
    {
    }

protected:
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE
    {
        fitInView(scene()->sceneRect());
        QGraphicsView::resizeEvent(event);
    }
};


int main(int argc, char **argv)
{
    QApplication app(argc, argv);

//![1]
    QGraphicsRectWidget *button1 = new QGraphicsRectWidget;
    QGraphicsRectWidget *button2 = new QGraphicsRectWidget;
    QGraphicsRectWidget *button3 = new QGraphicsRectWidget;
    QGraphicsRectWidget *button4 = new QGraphicsRectWidget;
    button2->setZValue(1);
    button3->setZValue(2);
    button4->setZValue(3);
    QGraphicsScene scene(0, 0, 300, 300);
    scene.setBackgroundBrush(Qt::black);
    scene.addItem(button1);
    scene.addItem(button2);
    scene.addItem(button3);
    scene.addItem(button4);
//![1]
    GraphicsView window(&scene);
    window.setFrameStyle(0);
    window.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    window.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    window.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//![2]
    QStateMachine machine;

    QState *group = new QState();
    group->setObjectName("group");
    QTimer timer;
    timer.setInterval(1250);
    timer.setSingleShot(true);
    QObject::connect(group, SIGNAL(entered()), &timer, SLOT(start()));
//![2]

//![3]
    QState *state1;
    QState *state2;
    QState *state3;
    QState *state4;
    QState *state5;
    QState *state6;
    QState *state7;

    state1 = createGeometryState(button1, QRect(100, 0, 50, 50),
                                 button2, QRect(150, 0, 50, 50),
                                 button3, QRect(200, 0, 50, 50),
                                 button4, QRect(250, 0, 50, 50),
                                 group);
//![3]
    state2 = createGeometryState(button1, QRect(250, 100, 50, 50),
                                 button2, QRect(250, 150, 50, 50),
                                 button3, QRect(250, 200, 50, 50),
                                 button4, QRect(250, 250, 50, 50),
                                 group);
    state3 = createGeometryState(button1, QRect(150, 250, 50, 50),
                                 button2, QRect(100, 250, 50, 50),
                                 button3, QRect(50, 250, 50, 50),
                                 button4, QRect(0, 250, 50, 50),
                                 group);
    state4 = createGeometryState(button1, QRect(0, 150, 50, 50),
                                 button2, QRect(0, 100, 50, 50),
                                 button3, QRect(0, 50, 50, 50),
                                 button4, QRect(0, 0, 50, 50),
                                 group);
    state5 = createGeometryState(button1, QRect(100, 100, 50, 50),
                                 button2, QRect(150, 100, 50, 50),
                                 button3, QRect(100, 150, 50, 50),
                                 button4, QRect(150, 150, 50, 50),
                                 group);
    state6 = createGeometryState(button1, QRect(50, 50, 50, 50),
                                 button2, QRect(200, 50, 50, 50),
                                 button3, QRect(50, 200, 50, 50),
                                 button4, QRect(200, 200, 50, 50),
                                 group);
//![4]
    state7 = createGeometryState(button1, QRect(0, 0, 50, 50),
                                 button2, QRect(250, 0, 50, 50),
                                 button3, QRect(0, 250, 50, 50),
                                 button4, QRect(250, 250, 50, 50),
                                 group);
    group->setInitialState(state1);
//![4]

//![5]
    QParallelAnimationGroup animationGroup;
    QSequentialAnimationGroup *subGroup;

    QPropertyAnimation *anim = new QPropertyAnimation(button4, "geometry");
    anim->setDuration(1000);
    anim->setEasingCurve(QEasingCurve::OutElastic);
    animationGroup.addAnimation(anim);
//![5]

//![6]
    subGroup = new QSequentialAnimationGroup(&animationGroup);
    subGroup->addPause(100);
    anim = new QPropertyAnimation(button3, "geometry");
    anim->setDuration(1000);
    anim->setEasingCurve(QEasingCurve::OutElastic);
    subGroup->addAnimation(anim);
//![6]

    subGroup = new QSequentialAnimationGroup(&animationGroup);
    subGroup->addPause(150);
    anim = new QPropertyAnimation(button2, "geometry");
    anim->setDuration(1000);
    anim->setEasingCurve(QEasingCurve::OutElastic);
    subGroup->addAnimation(anim);

    subGroup = new QSequentialAnimationGroup(&animationGroup);
    subGroup->addPause(200);
    anim = new QPropertyAnimation(button1, "geometry");
    anim->setDuration(1000);
    anim->setEasingCurve(QEasingCurve::OutElastic);
    subGroup->addAnimation(anim);

//![7]
    StateSwitcher *stateSwitcher = new StateSwitcher(&machine);
    stateSwitcher->setObjectName("stateSwitcher");
    group->addTransition(&timer, SIGNAL(timeout()), stateSwitcher);
    stateSwitcher->addState(state1, &animationGroup);
    stateSwitcher->addState(state2, &animationGroup);
//![7]
    stateSwitcher->addState(state3, &animationGroup);
    stateSwitcher->addState(state4, &animationGroup);
    stateSwitcher->addState(state5, &animationGroup);
    stateSwitcher->addState(state6, &animationGroup);
//![8]
    stateSwitcher->addState(state7, &animationGroup);
//![8]

//![9]
    machine.addState(group);
    machine.setInitialState(group);
    machine.start();
//![9]

    window.resize(300, 300);
    window.show();

    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

    return app.exec();
}

#include "main.moc"
