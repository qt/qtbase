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
#include "bomb.h"
#include "submarine.h"
#include "pixmapitem.h"
#include "animationmanager.h"
#include "qanimationstate.h"

//Qt
#include <QtCore/QSequentialAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QStateMachine>
#include <QtCore/QFinalState>

Bomb::Bomb() : PixmapItem(QString("bomb"), GraphicsScene::Big)
{
    setZValue(2);
}

void Bomb::launch(Bomb::Direction direction)
{
    QSequentialAnimationGroup *launchAnimation = new QSequentialAnimationGroup;
    AnimationManager::self()->registerAnimation(launchAnimation);
    qreal delta = direction == Right ? 20 : - 20;
    QPropertyAnimation *anim = new QPropertyAnimation(this, "pos");
    anim->setEndValue(QPointF(x() + delta,y() - 20));
    anim->setDuration(150);
    launchAnimation->addAnimation(anim);
    anim = new QPropertyAnimation(this, "pos");
    anim->setEndValue(QPointF(x() + delta*2, y() ));
    anim->setDuration(150);
    launchAnimation->addAnimation(anim);
    anim = new QPropertyAnimation(this, "pos");
    anim->setEndValue(QPointF(x() + delta*2,scene()->height()));
    anim->setDuration(y()/2*60);
    launchAnimation->addAnimation(anim);
    connect(anim,SIGNAL(valueChanged(QVariant)),this,SLOT(onAnimationLaunchValueChanged(QVariant)));
    connect(this, SIGNAL(bombExploded()), launchAnimation, SLOT(stop()));
    //We setup the state machine of the bomb
    QStateMachine *machine = new QStateMachine(this);

    //This state is when the launch animation is playing
    QAnimationState *launched = new QAnimationState(machine);
    launched->setAnimation(launchAnimation);

    //End
    QFinalState *final = new QFinalState(machine);

    machine->setInitialState(launched);

    //### Add a nice animation when the bomb is destroyed
    launched->addTransition(this, SIGNAL(bombExploded()),final);

    //If the animation is finished, then we move to the final state
    launched->addTransition(launched, SIGNAL(animationFinished()), final);

    //The machine has finished to be executed, then the boat is dead
    connect(machine,SIGNAL(finished()),this, SIGNAL(bombExecutionFinished()));

    machine->start();

}

void Bomb::onAnimationLaunchValueChanged(const QVariant &)
{
    foreach (QGraphicsItem * item , collidingItems(Qt::IntersectsItemBoundingRect)) {
        if (item->type() == SubMarine::Type) {
            SubMarine *s = static_cast<SubMarine *>(item);
            destroy();
            s->destroy();
        }
    }
}

void Bomb::destroy()
{
    emit bombExploded();
}
