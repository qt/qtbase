/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#ifndef SUBMARINE_P_H
#define SUBMARINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

//Own
#include "animationmanager.h"
#include "submarine.h"
#include "qanimationstate.h"

//Qt
#include <QtCore/QPropertyAnimation>
#include <QtWidgets/QGraphicsScene>

//This state is describing when the boat is moving right
class MovementState : public QAnimationState
{
Q_OBJECT
public:
    explicit MovementState(SubMarine *submarine, QState *parent = 0) : QAnimationState(parent)
    {
        movementAnimation = new QPropertyAnimation(submarine, "pos");
        connect(movementAnimation,SIGNAL(valueChanged(const QVariant &)),this,SLOT(onAnimationMovementValueChanged(const QVariant &)));
        setAnimation(movementAnimation);
        AnimationManager::self()->registerAnimation(movementAnimation);
        this->submarine = submarine;
    }

protected slots:
    void onAnimationMovementValueChanged(const QVariant &)
    {
        if (qrand() % 200 + 1 == 3)
            submarine->launchTorpedo(qrand() % 3 + 1);
    }

protected:
    void onEntry(QEvent *e) override
    {
        if (submarine->currentDirection() == SubMarine::Left) {
            movementAnimation->setEndValue(QPointF(0,submarine->y()));
            movementAnimation->setDuration(submarine->x()/submarine->currentSpeed()*12);
        }
        else /*if (submarine->currentDirection() == SubMarine::Right)*/ {
            movementAnimation->setEndValue(QPointF(submarine->scene()->width()-submarine->size().width(),submarine->y()));
            movementAnimation->setDuration((submarine->scene()->width()-submarine->size().width()-submarine->x())/submarine->currentSpeed()*12);
        }
        QAnimationState::onEntry(e);
    }

private:
    SubMarine *submarine;
    QPropertyAnimation *movementAnimation;
};

//This state is describing when the boat is moving right
class ReturnState : public QAnimationState
{
public:
    explicit ReturnState(SubMarine *submarine, QState *parent = 0) : QAnimationState(parent)
    {
        returnAnimation = new QPropertyAnimation(submarine->rotation(), "angle");
        returnAnimation->setDuration(500);
        AnimationManager::self()->registerAnimation(returnAnimation);
        setAnimation(returnAnimation);
        this->submarine = submarine;
    }

protected:
    void onEntry(QEvent *e) override
    {
        returnAnimation->stop();
        returnAnimation->setEndValue(submarine->currentDirection() == SubMarine::Right ? 360. : 180.);
        QAnimationState::onEntry(e);
    }

    void onExit(QEvent *e) override
    {
        submarine->currentDirection() == SubMarine::Right ? submarine->setCurrentDirection(SubMarine::Left) : submarine->setCurrentDirection(SubMarine::Right);
        QAnimationState::onExit(e);
    }

private:
    SubMarine *submarine;
    QPropertyAnimation *returnAnimation;
};

#endif // SUBMARINE_P_H
