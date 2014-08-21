/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//Own
#include "torpedo.h"
#include "pixmapitem.h"
#include "boat.h"
#include "graphicsscene.h"
#include "animationmanager.h"
#include "qanimationstate.h"

#include <QtCore/QPropertyAnimation>
#include <QtCore/QStateMachine>
#include <QtCore/QFinalState>

Torpedo::Torpedo() : PixmapItem(QString::fromLatin1("torpedo"),GraphicsScene::Big),
    currentSpeed(0)
{
    setZValue(2);
}

void Torpedo::launch()
{
    QPropertyAnimation *launchAnimation = new QPropertyAnimation(this, "pos");
    AnimationManager::self()->registerAnimation(launchAnimation);
    launchAnimation->setEndValue(QPointF(x(),qobject_cast<GraphicsScene *>(scene())->sealLevel() - 15));
    launchAnimation->setEasingCurve(QEasingCurve::InQuad);
    launchAnimation->setDuration(y()/currentSpeed*10);
    connect(launchAnimation,SIGNAL(valueChanged(QVariant)),this,SLOT(onAnimationLaunchValueChanged(QVariant)));
    connect(this,SIGNAL(torpedoExploded()), launchAnimation, SLOT(stop()));

    //We setup the state machine of the torpedo
    QStateMachine *machine = new QStateMachine(this);

    //This state is when the launch animation is playing
    QAnimationState *launched = new QAnimationState(machine);
    launched->setAnimation(launchAnimation);

    //End
    QFinalState *final = new QFinalState(machine);

    machine->setInitialState(launched);

    //### Add a nice animation when the torpedo is destroyed
    launched->addTransition(this, SIGNAL(torpedoExploded()),final);

    //If the animation is finished, then we move to the final state
    launched->addTransition(launched, SIGNAL(animationFinished()), final);

    //The machine has finished to be executed, then the boat is dead
    connect(machine,SIGNAL(finished()),this, SIGNAL(torpedoExecutionFinished()));

    machine->start();
}

void Torpedo::setCurrentSpeed(int speed)
{
    if (speed < 0) {
        qWarning("Torpedo::setCurrentSpeed : The speed is invalid");
        return;
    }
    currentSpeed = speed;
}

void Torpedo::onAnimationLaunchValueChanged(const QVariant &)
{
    foreach (QGraphicsItem *item , collidingItems(Qt::IntersectsItemBoundingRect)) {
        if (Boat *b = qgraphicsitem_cast<Boat*>(item))
            b->destroy();
    }
}

void Torpedo::destroy()
{
    emit torpedoExploded();
}
