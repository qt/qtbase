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
#include "states.h"
#include "graphicsscene.h"
#include "boat.h"
#include "submarine.h"
#include "torpedo.h"
#include "animationmanager.h"
#include "progressitem.h"
#include "textinformationitem.h"

//Qt
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QGraphicsView>
#include <QtCore/QStateMachine>
#include <QtWidgets/QKeyEventTransition>
#include <QtCore/QFinalState>

PlayState::PlayState(GraphicsScene *scene, QState *parent)
    : QState(parent),
    scene(scene),
    machine(0),
    currentLevel(0),
    score(0)
{
}

PlayState::~PlayState()
{
    delete machine;
}

void PlayState::onEntry(QEvent *)
{
    //We are now playing?
    if (machine) {
        machine->stop();
        //we hide the information
        scene->textInformationItem->hide();
        scene->clearScene();
        currentLevel = 0;
        score = 0;
        delete machine;
    }

    machine = new QStateMachine;

    //This state is when player is playing
    LevelState *levelState = new LevelState(scene, this, machine);

    //This state is when the player is actually playing but the game is not paused
    QState *playingState = new QState(levelState);
    levelState->setInitialState(playingState);

    //This state is when the game is paused
    PauseState *pauseState = new PauseState(scene, levelState);

    //We have one view, it receive the key press event
    QKeyEventTransition *pressPplay = new QKeyEventTransition(scene->views().at(0), QEvent::KeyPress, Qt::Key_P);
    pressPplay->setTargetState(pauseState);
    QKeyEventTransition *pressPpause = new QKeyEventTransition(scene->views().at(0), QEvent::KeyPress, Qt::Key_P);
    pressPpause->setTargetState(playingState);

    //Pause "P" is triggered, the player pause the game
    playingState->addTransition(pressPplay);

    //To get back playing when the game has been paused
    pauseState->addTransition(pressPpause);

    //This state is when player have lost
    LostState *lostState = new LostState(scene, this, machine);

    //This state is when player have won
    WinState *winState = new WinState(scene, this, machine);

    //The boat has been destroyed then the game is finished
    levelState->addTransition(scene->boat, SIGNAL(boatExecutionFinished()),lostState);

    //This transition check if we won or not
    WinTransition *winTransition = new WinTransition(scene, this, winState);

    //The boat has been destroyed then the game is finished
    levelState->addTransition(winTransition);

    //This state is an animation when the score changed
    UpdateScoreState *scoreState = new UpdateScoreState(levelState);

    //This transition update the score when a submarine die
    UpdateScoreTransition *scoreTransition = new UpdateScoreTransition(scene, this, levelState);
    scoreTransition->setTargetState(scoreState);

    //The boat has been destroyed then the game is finished
    playingState->addTransition(scoreTransition);

    //We go back to play state
    scoreState->addTransition(playingState);

    //We start playing!!!
    machine->setInitialState(levelState);

    //Final state
    QFinalState *final = new QFinalState(machine);

    //This transition is triggered when the player press space after completing a level
    CustomSpaceTransition *spaceTransition = new CustomSpaceTransition(scene->views().at(0), this, QEvent::KeyPress, Qt::Key_Space);
    spaceTransition->setTargetState(levelState);
    winState->addTransition(spaceTransition);

    //We lost we should reach the final state
    lostState->addTransition(lostState, SIGNAL(finished()), final);

    machine->start();
}

LevelState::LevelState(GraphicsScene *scene, PlayState *game, QState *parent) : QState(parent), scene(scene), game(game)
{
}
void LevelState::onEntry(QEvent *)
{
    initializeLevel();
}

void LevelState::initializeLevel()
{
    //we re-init the boat
    scene->boat->setPos(scene->width()/2, scene->sealLevel() - scene->boat->size().height());
    scene->boat->setCurrentSpeed(0);
    scene->boat->setCurrentDirection(Boat::None);
    scene->boat->setBombsLaunched(0);
    scene->boat->show();
    scene->setFocusItem(scene->boat, Qt::OtherFocusReason);
    scene->boat->run();

    scene->progressItem->setScore(game->score);
    scene->progressItem->setLevel(game->currentLevel + 1);

    GraphicsScene::LevelDescription currentLevelDescription = scene->levelsData.value(game->currentLevel);

    for (int i = 0; i < currentLevelDescription.submarines.size(); ++i ) {

        QPair<int,int> subContent = currentLevelDescription.submarines.at(i);
        GraphicsScene::SubmarineDescription submarineDesc = scene->submarinesData.at(subContent.first);

        for (int j = 0; j < subContent.second; ++j ) {
            SubMarine *sub = new SubMarine(submarineDesc.type, submarineDesc.name, submarineDesc.points);
            scene->addItem(sub);
            int random = (qrand() % 15 + 1);
            qreal x = random == 13 || random == 5 ? 0 : scene->width() - sub->size().width();
            qreal y = scene->height() -(qrand() % 150 + 1) - sub->size().height();
            sub->setPos(x,y);
            sub->setCurrentDirection(x == 0 ? SubMarine::Right : SubMarine::Left);
            sub->setCurrentSpeed(qrand() % 3 + 1);
        }
    }
}

/** Pause State */
PauseState::PauseState(GraphicsScene *scene, QState *parent) : QState(parent),scene(scene)
{
}
void PauseState::onEntry(QEvent *)
{
    AnimationManager::self()->pauseAll();
    scene->boat->setEnabled(false);
}
void PauseState::onExit(QEvent *)
{
    AnimationManager::self()->resumeAll();
    scene->boat->setEnabled(true);
    scene->boat->setFocus();
}

/** Lost State */
LostState::LostState(GraphicsScene *scene, PlayState *game, QState *parent) : QState(parent), scene(scene), game(game)
{
}

void LostState::onEntry(QEvent *)
{
    //The message to display
    QString message = QString("You lose on level %1. Your score is %2.").arg(game->currentLevel+1).arg(game->score);

    //We set the level back to 0
    game->currentLevel = 0;

    //We set the score back to 0
    game->score = 0;

    //We clear the scene
    scene->clearScene();

    //We inform the player
    scene->textInformationItem->setMessage(message);
    scene->textInformationItem->show();
}

void LostState::onExit(QEvent *)
{
    //we hide the information
    scene->textInformationItem->hide();
}

/** Win State */
WinState::WinState(GraphicsScene *scene, PlayState *game, QState *parent) : QState(parent), scene(scene), game(game)
{
}

void WinState::onEntry(QEvent *)
{
    //We clear the scene
    scene->clearScene();

    QString message;
    if (scene->levelsData.size() - 1 != game->currentLevel) {
        message = QString("You win the level %1. Your score is %2.\nPress Space to continue.").arg(game->currentLevel+1).arg(game->score);
        //We increment the level number
        game->currentLevel++;
    } else {
        message = QString("You finish the game on level %1. Your score is %2.").arg(game->currentLevel+1).arg(game->score);
        //We set the level back to 0
        game->currentLevel = 0;
        //We set the score back to 0
        game->score = 0;
    }

    //We inform the player
    scene->textInformationItem->setMessage(message);
    scene->textInformationItem->show();
}

void WinState::onExit(QEvent *)
{
    //we hide the information
    scene->textInformationItem->hide();
}

/** UpdateScore State */
UpdateScoreState::UpdateScoreState(QState *parent) : QState(parent)
{
}

/** Win transition */
UpdateScoreTransition::UpdateScoreTransition(GraphicsScene *scene, PlayState *game, QAbstractState *target)
    : QSignalTransition(scene,SIGNAL(subMarineDestroyed(int))),
    game(game), scene(scene)
{
    setTargetState(target);
}

bool UpdateScoreTransition::eventTest(QEvent *event)
{
    if (!QSignalTransition::eventTest(event))
        return false;
    QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(event);
    game->score += se->arguments().at(0).toInt();
    scene->progressItem->setScore(game->score);
    return true;
}

/** Win transition */
WinTransition::WinTransition(GraphicsScene *scene, PlayState *game, QAbstractState *target)
    : QSignalTransition(scene,SIGNAL(allSubMarineDestroyed(int))),
    game(game), scene(scene)
{
    setTargetState(target);
}

bool WinTransition::eventTest(QEvent *event)
{
    if (!QSignalTransition::eventTest(event))
        return false;
    QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(event);
    game->score += se->arguments().at(0).toInt();
    scene->progressItem->setScore(game->score);
    return true;
}

/** Space transition */
CustomSpaceTransition::CustomSpaceTransition(QWidget *widget, PlayState *game, QEvent::Type type, int key)
    :   QKeyEventTransition(widget, type, key),
        game(game)
{
}

bool CustomSpaceTransition::eventTest(QEvent *event)
{
    if (!QKeyEventTransition::eventTest(event))
        return false;
    return (game->currentLevel != 0);
}
