/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "animation.h"
#include "node.h"
#include "lifecycle.h"
#include "stickman.h"
#include "graphicsview.h"
#include "rectbutton.h"

#include <QtCore>
#include <QtGui>

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(stickman);
    QApplication app(argc, argv);

    StickMan *stickMan = new StickMan;
    stickMan->setDrawSticks(false);

#if defined(Q_WS_S60) || defined(Q_WS_MAEMO_5) || defined(Q_WS_SIMULATOR)
    RectButton *buttonJump = new RectButton("Jump"); buttonJump->setPos(100, 125);
    RectButton *buttonDance = new RectButton("Dance"); buttonDance->setPos(100, 200);
    RectButton *buttonChill = new RectButton("Chill"); buttonChill->setPos(100, 275);
#else
    QGraphicsTextItem *textItem = new QGraphicsTextItem();
    textItem->setHtml("<font color=\"white\"><b>Stickman</b>"
        "<p>"
        "Tell the stickman what to do!"
        "</p>"
        "<p><i>"
        "<li>Press <font color=\"purple\">J</font> to make the stickman jump.</li>"
        "<li>Press <font color=\"purple\">D</font> to make the stickman dance.</li>"
        "<li>Press <font color=\"purple\">C</font> to make him chill out.</li>"
        "<li>When you are done, press <font color=\"purple\">Escape</font>.</li>"
        "</i></p>"
        "<p>If he is unlucky, the stickman will get struck by lightning, and never jump, dance or chill out again."
        "</p></font>");
    qreal w = textItem->boundingRect().width();
    QRectF stickManBoundingRect = stickMan->mapToScene(stickMan->boundingRect()).boundingRect();
    textItem->setPos(-w / 2.0, stickManBoundingRect.bottom() + 25.0);
#endif

    QGraphicsScene scene;
    scene.addItem(stickMan);

#if defined(Q_WS_S60) || defined(Q_WS_MAEMO_5) || defined(Q_WS_SIMULATOR)
    scene.addItem(buttonJump);
    scene.addItem(buttonDance);
    scene.addItem(buttonChill);
#else
    scene.addItem(textItem);
#endif
    scene.setBackgroundBrush(Qt::black);

    GraphicsView view;
    view.setRenderHints(QPainter::Antialiasing);
    view.setTransformationAnchor(QGraphicsView::NoAnchor);
    view.setScene(&scene);

    QRectF sceneRect = scene.sceneRect();
    // making enough room in the scene for stickman to jump and die
    view.resize(sceneRect.width() + 100, sceneRect.height() + 100);
    view.setSceneRect(sceneRect);

#if defined(Q_WS_S60) || defined(Q_WS_MAEMO_5) || defined(Q_WS_SIMULATOR)
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.showMaximized();
    view.fitInView(scene.sceneRect(), Qt::KeepAspectRatio);
#else
    view.show();
    view.setFocus();
#endif

    LifeCycle cycle(stickMan, &view);
    cycle.setDeathAnimation(":/animations/dead");

#if defined(Q_WS_S60) || defined(Q_WS_MAEMO_5) || defined(Q_WS_SIMULATOR)
    cycle.addActivity(":/animations/jumping", Qt::Key_J, buttonJump, SIGNAL(clicked()));
    cycle.addActivity(":/animations/dancing", Qt::Key_D, buttonDance, SIGNAL(clicked()));
    cycle.addActivity(":/animations/chilling", Qt::Key_C, buttonChill, SIGNAL(clicked()));
#else
    cycle.addActivity(":/animations/jumping", Qt::Key_J);
    cycle.addActivity(":/animations/dancing", Qt::Key_D);
    cycle.addActivity(":/animations/chilling", Qt::Key_C);
#endif

    cycle.start();


    return app.exec();
}
