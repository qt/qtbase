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

#include "flippablepad.h"
#include "padnavigator.h"
#include "splashitem.h"

#include <QEventTransition>
#include <QGraphicsProxyWidget>
#include <QGraphicsRotation>
#include <QHistoryState>
#include <QKeyEventTransition>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QStateMachine>

#ifndef QT_NO_OPENGL
#include <QOpenGLWidget>
#endif

//! [0]
PadNavigator::PadNavigator(const QSize &size, QWidget *parent)
    : QGraphicsView(parent)
{
//! [0]
//! [1]
    // Splash item
    SplashItem *splash = new SplashItem;
    splash->setZValue(1);
//! [1]

//! [2]
    // Pad item
    FlippablePad *pad = new FlippablePad(size);
    QGraphicsRotation *flipRotation = new QGraphicsRotation(pad);
    QGraphicsRotation *xRotation = new QGraphicsRotation(pad);
    QGraphicsRotation *yRotation = new QGraphicsRotation(pad);
    flipRotation->setAxis(Qt::YAxis);
    xRotation->setAxis(Qt::YAxis);
    yRotation->setAxis(Qt::XAxis);
    pad->setTransformations(QList<QGraphicsTransform *>()
                            << flipRotation
                            << xRotation << yRotation);
//! [2]

//! [3]
    // Back (proxy widget) item
    QGraphicsProxyWidget *backItem = new QGraphicsProxyWidget(pad);
    QWidget *widget = new QWidget;
    form.setupUi(widget);
    form.hostName->setFocus();
    backItem->setWidget(widget);
    backItem->setVisible(false);
    backItem->setFocus();
    backItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    const QRectF r = backItem->rect();
    backItem->setTransform(QTransform()
                           .rotate(180, Qt::YAxis)
                           .translate(-r.width()/2, -r.height()/2));
//! [3]

//! [4]
    // Selection item
    RoundRectItem *selectionItem = new RoundRectItem(QRectF(-60, -60, 120, 120), Qt::gray, pad);
    selectionItem->setZValue(0.5);
//! [4]

//! [5]
    // Splash animations
    QPropertyAnimation *smoothSplashMove = new QPropertyAnimation(splash, "y");
    QPropertyAnimation *smoothSplashOpacity = new QPropertyAnimation(splash, "opacity");
    smoothSplashMove->setEasingCurve(QEasingCurve::InQuad);
    smoothSplashMove->setDuration(250);
    smoothSplashOpacity->setDuration(250);
//! [5]

//! [6]
    // Selection animation
    QPropertyAnimation *smoothXSelection = new QPropertyAnimation(selectionItem, "x");
    QPropertyAnimation *smoothYSelection = new QPropertyAnimation(selectionItem, "y");
    QPropertyAnimation *smoothXRotation = new QPropertyAnimation(xRotation, "angle");
    QPropertyAnimation *smoothYRotation = new QPropertyAnimation(yRotation, "angle");
    smoothXSelection->setDuration(125);
    smoothYSelection->setDuration(125);
    smoothXRotation->setDuration(125);
    smoothYRotation->setDuration(125);
    smoothXSelection->setEasingCurve(QEasingCurve::InOutQuad);
    smoothYSelection->setEasingCurve(QEasingCurve::InOutQuad);
    smoothXRotation->setEasingCurve(QEasingCurve::InOutQuad);
    smoothYRotation->setEasingCurve(QEasingCurve::InOutQuad);
//! [6]

//! [7]
    // Flip animation setup
    QPropertyAnimation *smoothFlipRotation = new QPropertyAnimation(flipRotation, "angle");
    QPropertyAnimation *smoothFlipScale = new QPropertyAnimation(pad, "scale");
    QParallelAnimationGroup *flipAnimation = new QParallelAnimationGroup(this);
    smoothFlipScale->setDuration(500);
    smoothFlipRotation->setDuration(500);
    smoothFlipScale->setEasingCurve(QEasingCurve::InOutQuad);
    smoothFlipRotation->setEasingCurve(QEasingCurve::InOutQuad);
    smoothFlipScale->setKeyValueAt(0, qvariant_cast<qreal>(1.0));
    smoothFlipScale->setKeyValueAt(0.5, qvariant_cast<qreal>(0.7));
    smoothFlipScale->setKeyValueAt(1, qvariant_cast<qreal>(1.0));
    flipAnimation->addAnimation(smoothFlipRotation);
    flipAnimation->addAnimation(smoothFlipScale);
//! [7]

//! [8]
    // Flip animation delayed property assignment
    QSequentialAnimationGroup *setVariablesSequence = new QSequentialAnimationGroup;
    QPropertyAnimation *setFillAnimation = new QPropertyAnimation(pad, "fill");
    QPropertyAnimation *setBackItemVisibleAnimation = new QPropertyAnimation(backItem, "visible");
    QPropertyAnimation *setSelectionItemVisibleAnimation = new QPropertyAnimation(selectionItem, "visible");
    setFillAnimation->setDuration(0);
    setBackItemVisibleAnimation->setDuration(0);
    setSelectionItemVisibleAnimation->setDuration(0);
    setVariablesSequence->addPause(250);
    setVariablesSequence->addAnimation(setBackItemVisibleAnimation);
    setVariablesSequence->addAnimation(setSelectionItemVisibleAnimation);
    setVariablesSequence->addAnimation(setFillAnimation);
    flipAnimation->addAnimation(setVariablesSequence);
//! [8]

//! [9]
    // Build the state machine
    QStateMachine *stateMachine = new QStateMachine(this);
    QState *splashState = new QState(stateMachine);
    QState *frontState = new QState(stateMachine);
    QHistoryState *historyState = new QHistoryState(frontState);
    QState *backState = new QState(stateMachine);
//! [9]
//! [10]
    frontState->assignProperty(pad, "fill", false);
    frontState->assignProperty(splash, "opacity", 0.0);
    frontState->assignProperty(backItem, "visible", false);
    frontState->assignProperty(flipRotation, "angle", qvariant_cast<qreal>(0.0));
    frontState->assignProperty(selectionItem, "visible", true);
    backState->assignProperty(pad, "fill", true);
    backState->assignProperty(backItem, "visible", true);
    backState->assignProperty(xRotation, "angle", qvariant_cast<qreal>(0.0));
    backState->assignProperty(yRotation, "angle", qvariant_cast<qreal>(0.0));
    backState->assignProperty(flipRotation, "angle", qvariant_cast<qreal>(180.0));
    backState->assignProperty(selectionItem, "visible", false);
    stateMachine->addDefaultAnimation(smoothXRotation);
    stateMachine->addDefaultAnimation(smoothYRotation);
    stateMachine->addDefaultAnimation(smoothXSelection);
    stateMachine->addDefaultAnimation(smoothYSelection);
    stateMachine->setInitialState(splashState);
//! [10]

//! [11]
    // Transitions
    QEventTransition *anyKeyTransition = new QEventTransition(this, QEvent::KeyPress, splashState);
    anyKeyTransition->setTargetState(frontState);
    anyKeyTransition->addAnimation(smoothSplashMove);
    anyKeyTransition->addAnimation(smoothSplashOpacity);
//! [11]

//! [12]
    QKeyEventTransition *enterTransition = new QKeyEventTransition(this, QEvent::KeyPress,
                                                                   Qt::Key_Enter, backState);
    QKeyEventTransition *returnTransition = new QKeyEventTransition(this, QEvent::KeyPress,
                                                                    Qt::Key_Return, backState);
    QKeyEventTransition *backEnterTransition = new QKeyEventTransition(this, QEvent::KeyPress,
                                                                       Qt::Key_Enter, frontState);
    QKeyEventTransition *backReturnTransition = new QKeyEventTransition(this, QEvent::KeyPress,
                                                                        Qt::Key_Return, frontState);
    enterTransition->setTargetState(historyState);
    returnTransition->setTargetState(historyState);
    backEnterTransition->setTargetState(backState);
    backReturnTransition->setTargetState(backState);
    enterTransition->addAnimation(flipAnimation);
    returnTransition->addAnimation(flipAnimation);
    backEnterTransition->addAnimation(flipAnimation);
    backReturnTransition->addAnimation(flipAnimation);
//! [12]

//! [13]
    // Create substates for each icon; store in temporary grid.
    int columns = size.width();
    int rows = size.height();
    QVector< QVector< QState * > > stateGrid;
    stateGrid.resize(rows);
    for (int y = 0; y < rows; ++y) {
        stateGrid[y].resize(columns);
        for (int x = 0; x < columns; ++x)
            stateGrid[y][x] = new QState(frontState);
    }
    frontState->setInitialState(stateGrid[0][0]);
    selectionItem->setPos(pad->iconAt(0, 0)->pos());
//! [13]

//! [14]
    // Enable key navigation using state transitions
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < columns; ++x) {
            QState *state = stateGrid[y][x];
            QKeyEventTransition *rightTransition = new QKeyEventTransition(this, QEvent::KeyPress,
                                                                           Qt::Key_Right, state);
            QKeyEventTransition *leftTransition = new QKeyEventTransition(this, QEvent::KeyPress,
                                                                          Qt::Key_Left, state);
            QKeyEventTransition *downTransition = new QKeyEventTransition(this, QEvent::KeyPress,
                                                                          Qt::Key_Down, state);
            QKeyEventTransition *upTransition = new QKeyEventTransition(this, QEvent::KeyPress,
                                                                        Qt::Key_Up, state);
            rightTransition->setTargetState(stateGrid[y][(x + 1) % columns]);
            leftTransition->setTargetState(stateGrid[y][((x - 1) + columns) % columns]);
            downTransition->setTargetState(stateGrid[(y + 1) % rows][x]);
            upTransition->setTargetState(stateGrid[((y - 1) + rows) % rows][x]);
//! [14]
//! [15]
            RoundRectItem *icon = pad->iconAt(x, y);
            state->assignProperty(xRotation, "angle", -icon->x() / 6.0);
            state->assignProperty(yRotation, "angle", icon->y() / 6.0);
            state->assignProperty(selectionItem, "x", icon->x());
            state->assignProperty(selectionItem, "y", icon->y());
            frontState->assignProperty(icon, "visible", true);
            backState->assignProperty(icon, "visible", false);

            QPropertyAnimation *setIconVisibleAnimation = new QPropertyAnimation(icon, "visible");
            setIconVisibleAnimation->setDuration(0);
            setVariablesSequence->addAnimation(setIconVisibleAnimation);
        }
    }
//! [15]

//! [16]
    // Scene
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setBackgroundBrush(QPixmap(":/images/blue_angle_swirl.jpg"));
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->addItem(pad);
    scene->setSceneRect(scene->itemsBoundingRect());
    setScene(scene);
//! [16]

//! [17]
    // Adjust splash item to scene contents
    const QRectF sbr = splash->boundingRect();
    splash->setPos(-sbr.width() / 2, scene->sceneRect().top() - 2);
    frontState->assignProperty(splash, "y", splash->y() - 100.0);
    scene->addItem(splash);
//! [17]

//! [18]
    // View
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMinimumSize(50, 50);
    setViewportUpdateMode(FullViewportUpdate);
    setCacheMode(CacheBackground);
    setRenderHints(QPainter::Antialiasing
                   | QPainter::SmoothPixmapTransform
                   | QPainter::TextAntialiasing);
#ifndef QT_NO_OPENGL
    setViewport(new QOpenGLWidget);
#endif

    stateMachine->start();
//! [18]
}

//! [19]
void PadNavigator::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}
//! [19]
