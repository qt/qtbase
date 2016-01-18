/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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

//Own
#include "submarine.h"
#include "submarine_p.h"
#include "torpedo.h"
#include "pixmapitem.h"
#include "graphicsscene.h"
#include "animationmanager.h"
#include "qanimationstate.h"

#include <QtCore/QPropertyAnimation>
#include <QtCore/QStateMachine>
#include <QtCore/QFinalState>
#include <QtCore/QSequentialAnimationGroup>

static QAbstractAnimation *setupDestroyAnimation(SubMarine *sub)
{
    QSequentialAnimationGroup *group = new QSequentialAnimationGroup(sub);
    for (int i = 1; i <= 4; ++i) {
        PixmapItem *step = new PixmapItem(QString::fromLatin1("explosion/submarine/step%1").arg(i), GraphicsScene::Big, sub);
        step->setZValue(6);
        step->setOpacity(0);
        QPropertyAnimation *anim = new QPropertyAnimation(step, "opacity", group);
        anim->setDuration(100);
        anim->setEndValue(1);
    }
    AnimationManager::self()->registerAnimation(group);
    return group;
}


SubMarine::SubMarine(int type, const QString &name, int points) : PixmapItem(QString("submarine"), GraphicsScene::Big),
    subType(type), subName(name), subPoints(points), speed(0), direction(SubMarine::None)
{
    setZValue(5);
    setTransformOriginPoint(boundingRect().center());

    graphicsRotation = new QGraphicsRotation(this);
    graphicsRotation->setAxis(Qt::YAxis);
    graphicsRotation->setOrigin(QVector3D(size().width()/2, size().height()/2, 0));
    QList<QGraphicsTransform *> r;
    r.append(graphicsRotation);
    setTransformations(r);

    //We setup the state machine of the submarine
    QStateMachine *machine = new QStateMachine(this);

    //This state is when the boat is moving/rotating
    QState *moving = new QState(machine);

    //This state is when the boat is moving from left to right
    MovementState *movement = new MovementState(this, moving);

    //This state is when the boat is moving from left to right
    ReturnState *rotation = new ReturnState(this, moving);

    //This is the initial state of the moving root state
    moving->setInitialState(movement);

    movement->addTransition(this, SIGNAL(subMarineStateChanged()), moving);

    //This is the initial state of the machine
    machine->setInitialState(moving);

    //End
    QFinalState *final = new QFinalState(machine);

    //If the moving animation is finished we move to the return state
    movement->addTransition(movement, SIGNAL(animationFinished()), rotation);

    //If the return animation is finished we move to the moving state
    rotation->addTransition(rotation, SIGNAL(animationFinished()), movement);

    //This state play the destroyed animation
    QAnimationState *destroyedState = new QAnimationState(machine);
    destroyedState->setAnimation(setupDestroyAnimation(this));

    //Play a nice animation when the submarine is destroyed
    moving->addTransition(this, SIGNAL(subMarineDestroyed()), destroyedState);

    //Transition to final state when the destroyed animation is finished
    destroyedState->addTransition(destroyedState, SIGNAL(animationFinished()), final);

    //The machine has finished to be executed, then the submarine is dead
    connect(machine,SIGNAL(finished()),this, SIGNAL(subMarineExecutionFinished()));

    machine->start();
}

int SubMarine::points() const
{
    return subPoints;
}

void SubMarine::setCurrentDirection(SubMarine::Movement direction)
{
    if (this->direction == direction)
        return;
    if (direction == SubMarine::Right && this->direction == SubMarine::None) {
          graphicsRotation->setAngle(180);
    }
    this->direction = direction;
}

enum SubMarine::Movement SubMarine::currentDirection() const
{
    return direction;
}

void SubMarine::setCurrentSpeed(int speed)
{
    if (speed < 0 || speed > 3) {
        qWarning("SubMarine::setCurrentSpeed : The speed is invalid");
    }
    this->speed = speed;
    emit subMarineStateChanged();
}

int SubMarine::currentSpeed() const
{
    return speed;
}

void SubMarine::launchTorpedo(int speed)
{
    Torpedo * torp = new Torpedo();
    GraphicsScene *scene = static_cast<GraphicsScene *>(this->scene());
    scene->addItem(torp);
    torp->setPos(pos());
    torp->setCurrentSpeed(speed);
    torp->launch();
}

void SubMarine::destroy()
{
    emit subMarineDestroyed();
}

int SubMarine::type() const
{
    return Type;
}
