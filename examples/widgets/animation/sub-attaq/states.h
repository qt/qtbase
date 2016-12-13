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
    explicit PlayState(GraphicsScene *scene, QState *parent = 0);
    ~PlayState();

 protected:
    void onEntry(QEvent *) override;

private :
    GraphicsScene *scene;
    QStateMachine *machine;
    int currentLevel;
    int score;

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
    void onEntry(QEvent *) override;
private :
    void initializeLevel();
    GraphicsScene *scene;
    PlayState *game;
};

class PauseState : public QState
{
public:
    explicit PauseState(GraphicsScene *scene, QState *parent = 0);

protected:
    void onEntry(QEvent *) override;
    void onExit(QEvent *) override;
private :
    GraphicsScene *scene;
};

class LostState : public QState
{
public:
    LostState(GraphicsScene *scene, PlayState *game, QState *parent = 0);

protected:
    void onEntry(QEvent *) override;
    void onExit(QEvent *) override;
private :
    GraphicsScene *scene;
    PlayState *game;
};

class WinState : public QState
{
public:
    WinState(GraphicsScene *scene, PlayState *game, QState *parent = 0);

protected:
    void onEntry(QEvent *) override;
    void onExit(QEvent *) override;
private :
    GraphicsScene *scene;
    PlayState *game;
};

class UpdateScoreState : public QState
{
public:
    UpdateScoreState(QState *parent);
private:
};

//These transtion is used to update the score
class UpdateScoreTransition : public QSignalTransition
{
public:
    UpdateScoreTransition(GraphicsScene *scene, PlayState *game, QAbstractState *target);
protected:
    bool eventTest(QEvent *event) override;
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
    bool eventTest(QEvent *event) override;
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
    bool eventTest(QEvent *event) override;
private:
    PlayState *game;
};

#endif // STATES_H
