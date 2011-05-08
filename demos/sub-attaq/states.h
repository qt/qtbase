/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef STATES_H
#define STATES_H

//Qt
#include <QtCore/QState>
#include <QtCore/QSignalTransition>
#include <QtCore/QPropertyAnimation>
#include <QtWidgets/QKeyEventTransition>
#include <QtCore/QSet>

class GraphicsScene;
class Boat;
class SubMarine;
QT_BEGIN_NAMESPACE
class QStateMachine;
QT_END_NAMESPACE

class PlayState : public QState
{
public:
    PlayState(GraphicsScene *scene, QState *parent = 0);
    ~PlayState();

 protected:
    void onEntry(QEvent *);

private :
    GraphicsScene *scene;
    QStateMachine *machine;
    int currentLevel;
    int score;
    QState *parallelChild;

    friend class UpdateScoreState;
    friend class UpdateScoreTransition;
    friend class WinTransition;
    friend class CustomSpaceTransition;
    friend class WinState;
    friend class LostState;
    friend class LevelState;
};

class LevelState : public QState
{
public:
    LevelState(GraphicsScene *scene, PlayState *game, QState *parent = 0);
protected:
    void onEntry(QEvent *);
private :
    void initializeLevel();
    GraphicsScene *scene;
    PlayState *game;
};

class PauseState : public QState
{
public:
    PauseState(GraphicsScene *scene, QState *parent = 0);

protected:
    void onEntry(QEvent *);
    void onExit(QEvent *);
private :
    GraphicsScene *scene;
    Boat *boat;
};

class LostState : public QState
{
public:
    LostState(GraphicsScene *scene, PlayState *game, QState *parent = 0);

protected:
    void onEntry(QEvent *);
    void onExit(QEvent *);
private :
    GraphicsScene *scene;
    PlayState *game;
};

class WinState : public QState
{
public:
    WinState(GraphicsScene *scene, PlayState *game, QState *parent = 0);

protected:
    void onEntry(QEvent *);
    void onExit(QEvent *);
private :
    GraphicsScene *scene;
    PlayState *game;
};

class UpdateScoreState : public QState
{
public:
    UpdateScoreState(PlayState *game, QState *parent);
private:
    QPropertyAnimation *scoreAnimation;
    PlayState *game;
};

//These transtion is used to update the score
class UpdateScoreTransition : public QSignalTransition
{
public:
    UpdateScoreTransition(GraphicsScene *scene, PlayState *game, QAbstractState *target);
protected:
    virtual bool eventTest(QEvent *event);
private:
    PlayState * game;
    GraphicsScene *scene;
};

//These transtion test if we have won the game
class WinTransition : public QSignalTransition
{
public:
    WinTransition(GraphicsScene *scene, PlayState *game, QAbstractState *target);
protected:
    virtual bool eventTest(QEvent *event);
private:
    PlayState * game;
    GraphicsScene *scene;
};

//These transtion is true if one level has been completed and the player want to continue
 class CustomSpaceTransition : public QKeyEventTransition
{
public:
    CustomSpaceTransition(QWidget *widget, PlayState *game, QEvent::Type type, int key);
protected:
    virtual bool eventTest(QEvent *event);
private:
    PlayState *game;
    int key;
};

#endif // STATES_H
