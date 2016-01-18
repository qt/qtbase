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

#ifndef BOAT_P_H
#define BOAT_P_H

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
#include "bomb.h"
#include "graphicsscene.h"

// Qt
#include <QtWidgets/QKeyEventTransition>

static const int MAX_BOMB = 5;


//These transtion test if we have to stop the boat (i.e current speed is 1)
class KeyStopTransition : public QKeyEventTransition
{
public:
    KeyStopTransition(Boat *b, QEvent::Type t, int k)
    : QKeyEventTransition(b, t, k), boat(b)
    {
    }
protected:
    virtual bool eventTest(QEvent *event) Q_DECL_OVERRIDE
    {
        if (!QKeyEventTransition::eventTest(event))
            return false;
        return (boat->currentSpeed() == 1);
    }
private:
    Boat * boat;
};

//These transtion test if we have to move the boat (i.e current speed was 0 or another value)
 class KeyMoveTransition : public QKeyEventTransition
{
public:
    KeyMoveTransition(Boat *b, QEvent::Type t, int k)
    : QKeyEventTransition(b, t, k), boat(b), key(k)
    {
    }
protected:
    virtual bool eventTest(QEvent *event) Q_DECL_OVERRIDE
    {
        if (!QKeyEventTransition::eventTest(event))
            return false;
        return (boat->currentSpeed() >= 0);
    }
    void onTransition(QEvent *) Q_DECL_OVERRIDE
    {
        //We decrease the speed if needed
        if (key == Qt::Key_Left && boat->currentDirection() == Boat::Right)
            boat->setCurrentSpeed(boat->currentSpeed() - 1);
        else if (key == Qt::Key_Right && boat->currentDirection() == Boat::Left)
            boat->setCurrentSpeed(boat->currentSpeed() - 1);
        else if (boat->currentSpeed() < 3)
            boat->setCurrentSpeed(boat->currentSpeed() + 1);
        boat->updateBoatMovement();
    }
private:
    Boat * boat;
    int key;
};

//This transition trigger the bombs launch
 class KeyLaunchTransition : public QKeyEventTransition
{
public:
    KeyLaunchTransition(Boat *boat, QEvent::Type type, int key)
    : QKeyEventTransition(boat, type, key), boat(boat)
    {
    }
protected:
    virtual bool eventTest(QEvent *event) Q_DECL_OVERRIDE
    {
        if (!QKeyEventTransition::eventTest(event))
            return false;
        //We have enough bomb?
        return (boat->bombsLaunched() < MAX_BOMB);
    }
private:
    Boat * boat;
};

//This state is describing when the boat is moving right
class MoveStateRight : public QState
{
public:
    explicit MoveStateRight(Boat *boat,QState *parent = 0) : QState(parent), boat(boat)
    {
    }
protected:
    void onEntry(QEvent *) Q_DECL_OVERRIDE
    {
        boat->setCurrentDirection(Boat::Right);
        boat->updateBoatMovement();
    }
private:
    Boat * boat;
};

 //This state is describing when the boat is moving left
class MoveStateLeft : public QState
{
public:
    explicit MoveStateLeft(Boat *boat,QState *parent = 0) : QState(parent), boat(boat)
    {
    }
protected:
    void onEntry(QEvent *) Q_DECL_OVERRIDE
    {
        boat->setCurrentDirection(Boat::Left);
        boat->updateBoatMovement();
    }
private:
    Boat * boat;
};

//This state is describing when the boat is in a stand by position
class StopState : public QState
{
public:
    explicit StopState(Boat *boat,QState *parent = 0) : QState(parent), boat(boat)
    {
    }
protected:
    void onEntry(QEvent *) Q_DECL_OVERRIDE
    {
        boat->setCurrentSpeed(0);
        boat->setCurrentDirection(Boat::None);
        boat->updateBoatMovement();
    }
private:
    Boat * boat;
};

//This state is describing the launch of the torpedo on the right
class LaunchStateRight : public QState
{
public:
    explicit LaunchStateRight(Boat *boat,QState *parent = 0) : QState(parent), boat(boat)
    {
    }
protected:
    void onEntry(QEvent *) Q_DECL_OVERRIDE
    {
        Bomb *b = new Bomb();
        b->setPos(boat->x()+boat->size().width(),boat->y());
        GraphicsScene *scene = static_cast<GraphicsScene *>(boat->scene());
        scene->addItem(b);
        b->launch(Bomb::Right);
        boat->setBombsLaunched(boat->bombsLaunched() + 1);
    }
private:
    Boat * boat;
};

//This state is describing the launch of the torpedo on the left
class LaunchStateLeft : public QState
{
public:
    explicit LaunchStateLeft(Boat *boat,QState *parent = 0) : QState(parent), boat(boat)
    {
    }
protected:
    void onEntry(QEvent *) Q_DECL_OVERRIDE
    {
        Bomb *b = new Bomb();
        b->setPos(boat->x() - b->size().width(), boat->y());
        GraphicsScene *scene = static_cast<GraphicsScene *>(boat->scene());
        scene->addItem(b);
        b->launch(Bomb::Left);
        boat->setBombsLaunched(boat->bombsLaunched() + 1);
    }
private:
    Boat * boat;
};

#endif // BOAT_P_H
