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
#include "graphicsscene.h"
#include "states.h"
#include "boat.h"
#include "submarine.h"
#include "torpedo.h"
#include "bomb.h"
#include "pixmapitem.h"
#include "animationmanager.h"
#include "qanimationstate.h"
#include "progressitem.h"
#include "textinformationitem.h"

//Qt
#include <QtCore/QPropertyAnimation>
#include <QtCore/QSequentialAnimationGroup>
#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QStateMachine>
#include <QtCore/QFinalState>
#include <QtCore/QPauseAnimation>
#include <QtWidgets/QAction>
#include <QtCore/QDir>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtCore/QXmlStreamReader>

GraphicsScene::GraphicsScene(int x, int y, int width, int height, Mode mode)
    : QGraphicsScene(x , y, width, height), mode(mode), boat(new Boat)
{
    PixmapItem *backgroundItem = new PixmapItem(QString("background"),mode);
    backgroundItem->setZValue(1);
    backgroundItem->setPos(0,0);
    addItem(backgroundItem);

    PixmapItem *surfaceItem = new PixmapItem(QString("surface"),mode);
    surfaceItem->setZValue(3);
    surfaceItem->setPos(0,sealLevel() - surfaceItem->boundingRect().height()/2);
    addItem(surfaceItem);

    //The item that display score and level
    progressItem = new ProgressItem(backgroundItem);

    textInformationItem = new TextInformationItem(backgroundItem);
    textInformationItem->hide();
    //We create the boat
    addItem(boat);
    boat->setPos(this->width()/2, sealLevel() - boat->size().height());
    boat->hide();

    //parse the xml that contain all data of the game
    QXmlStreamReader reader;
    QFile file(":data.xml");
    file.open(QIODevice::ReadOnly);
    reader.setDevice(&file);
    LevelDescription currentLevel;
    while (!reader.atEnd()) {
         reader.readNext();
         if (reader.tokenType() == QXmlStreamReader::StartElement) {
             if (reader.name() == "submarine") {
                 SubmarineDescription desc;
                 desc.name = reader.attributes().value("name").toString();
                 desc.points = reader.attributes().value("points").toString().toInt();
                 desc.type = reader.attributes().value("type").toString().toInt();
                 submarinesData.append(desc);
             } else if (reader.name() == "level") {
                 currentLevel.id = reader.attributes().value("id").toString().toInt();
                 currentLevel.name = reader.attributes().value("name").toString();
             } else if (reader.name() == "subinstance") {
                 currentLevel.submarines.append(qMakePair(reader.attributes().value("type").toString().toInt(), reader.attributes().value("nb").toString().toInt()));
             }
         } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            if (reader.name() == "level") {
                levelsData.insert(currentLevel.id, currentLevel);
                currentLevel.submarines.clear();
            }
         }
   }
}

qreal GraphicsScene::sealLevel() const
{
    return (mode == Big) ? 220 : 160;
}

void GraphicsScene::setupScene(QAction *newAction, QAction *quitAction)
{
    static const int nLetters = 10;
    static struct {
        char const *pix;
        qreal initX, initY;
        qreal destX, destY;
    } logoData[nLetters] = {
        {"s",   -1000, -1000, 300, 150 },
        {"u",    -800, -1000, 350, 150 },
        {"b",    -600, -1000, 400, 120 },
        {"dash", -400, -1000, 460, 150 },
        {"a",    1000,  2000, 350, 250 },
        {"t",     800,  2000, 400, 250 },
        {"t2",    600,  2000, 430, 250 },
        {"a2",    400,  2000, 465, 250 },
        {"q",     200,  2000, 510, 250 },
        {"excl",    0,  2000, 570, 220 } };

    QSequentialAnimationGroup * lettersGroupMoving = new QSequentialAnimationGroup(this);
    QParallelAnimationGroup * lettersGroupFading = new QParallelAnimationGroup(this);

    for (int i = 0; i < nLetters; ++i) {
        PixmapItem *logo = new PixmapItem(QLatin1String(":/logo-") + logoData[i].pix, this);
        logo->setPos(logoData[i].initX, logoData[i].initY);
        logo->setZValue(i + 3);
        //creation of the animations for moving letters
        QPropertyAnimation *moveAnim = new QPropertyAnimation(logo, "pos", lettersGroupMoving);
        moveAnim->setEndValue(QPointF(logoData[i].destX, logoData[i].destY));
        moveAnim->setDuration(200);
        moveAnim->setEasingCurve(QEasingCurve::OutElastic);
        lettersGroupMoving->addPause(50);
        //creation of the animations for fading out the letters
        QPropertyAnimation *fadeAnim = new QPropertyAnimation(logo, "opacity", lettersGroupFading);
        fadeAnim->setDuration(800);
        fadeAnim->setEndValue(0);
        fadeAnim->setEasingCurve(QEasingCurve::OutQuad);
    }

    QStateMachine *machine = new QStateMachine(this);

    //This state is when the player is playing
    PlayState *gameState = new PlayState(this, machine);

    //Final state
    QFinalState *final = new QFinalState(machine);

    //Animation when the player enter in the game
    QAnimationState *lettersMovingState = new QAnimationState(machine);
    lettersMovingState->setAnimation(lettersGroupMoving);

    //Animation when the welcome screen disappear
    QAnimationState *lettersFadingState = new QAnimationState(machine);
    lettersFadingState->setAnimation(lettersGroupFading);

    //if new game then we fade out the welcome screen and start playing
    lettersMovingState->addTransition(newAction, SIGNAL(triggered()), lettersFadingState);
    lettersFadingState->addTransition(lettersFadingState, SIGNAL(animationFinished()), gameState);

    //New Game is triggered then player start playing
    gameState->addTransition(newAction, SIGNAL(triggered()), gameState);

    //Wanna quit, then connect to CTRL+Q
    gameState->addTransition(quitAction, SIGNAL(triggered()), final);
    lettersMovingState->addTransition(quitAction, SIGNAL(triggered()), final);

    //Welcome screen is the initial state
    machine->setInitialState(lettersMovingState);

    machine->start();

    //We reach the final state, then we quit
    connect(machine, SIGNAL(finished()), qApp, SLOT(quit()));
}

void GraphicsScene::addItem(Bomb *bomb)
{
    bombs.insert(bomb);
    connect(bomb,SIGNAL(bombExecutionFinished()),this, SLOT(onBombExecutionFinished()));
    QGraphicsScene::addItem(bomb);
}

void GraphicsScene::addItem(Torpedo *torpedo)
{
    torpedos.insert(torpedo);
    connect(torpedo,SIGNAL(torpedoExecutionFinished()),this, SLOT(onTorpedoExecutionFinished()));
    QGraphicsScene::addItem(torpedo);
}

void GraphicsScene::addItem(SubMarine *submarine)
{
    submarines.insert(submarine);
    connect(submarine,SIGNAL(subMarineExecutionFinished()),this, SLOT(onSubMarineExecutionFinished()));
    QGraphicsScene::addItem(submarine);
}

void GraphicsScene::addItem(QGraphicsItem *item)
{
    QGraphicsScene::addItem(item);
}

void GraphicsScene::onBombExecutionFinished()
{
    Bomb *bomb = qobject_cast<Bomb *>(sender());
    bombs.remove(bomb);
    bomb->deleteLater();
    if (boat)
        boat->setBombsLaunched(boat->bombsLaunched() - 1);
}

void GraphicsScene::onTorpedoExecutionFinished()
{
    Torpedo *torpedo = qobject_cast<Torpedo *>(sender());
    torpedos.remove(torpedo);
    torpedo->deleteLater();
}

void GraphicsScene::onSubMarineExecutionFinished()
{
    SubMarine *submarine = qobject_cast<SubMarine *>(sender());
    submarines.remove(submarine);
    if (submarines.count() == 0)
        emit allSubMarineDestroyed(submarine->points());
    else
        emit subMarineDestroyed(submarine->points());
    submarine->deleteLater();
}

void GraphicsScene::clearScene()
{
    foreach (SubMarine *sub, submarines) {
        sub->destroy();
        sub->deleteLater();
    }

    foreach (Torpedo *torpedo, torpedos) {
        torpedo->destroy();
        torpedo->deleteLater();
    }

    foreach (Bomb *bomb, bombs) {
        bomb->destroy();
        bomb->deleteLater();
    }

    submarines.clear();
    bombs.clear();
    torpedos.clear();

    AnimationManager::self()->unregisterAllAnimations();

    boat->stop();
    boat->hide();
    boat->setEnabled(true);
}
