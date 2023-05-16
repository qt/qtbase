// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "chiptester.h"
#include "chip.h"

#include <QtGui>
#include <QScrollBar>
#ifndef QT_NO_OPENGL
#include <QtOpenGL>
#endif

ChipTester::ChipTester(QWidget *parent)
    : QGraphicsView(parent),
      npaints(0)
{
    resize(400, 300);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(0);
    setTransformationAnchor(NoAnchor);

    populateScene();
    setScene(scene);

    setWindowTitle(tr("Chip Example"));
}

void ChipTester::setAntialias(bool enabled)
{
    setRenderHint(QPainter::Antialiasing, enabled);
}

void ChipTester::setOperation(Operation operation)
{
    this->operation = operation;
}

void ChipTester::runBenchmark()
{
    npaints = 0;
    timerId = startTimer(0);
    eventLoop.exec();
    killTimer(timerId);
}

void ChipTester::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);
    if (++npaints == 50)
        eventLoop.quit();
}

void ChipTester::timerEvent(QTimerEvent *)
{
    switch (operation) {
    case Rotate360:
        rotate(1);
        break;
    case ZoomInOut: {
        qreal s = 0.05 + (npaints / 20.0);
        setTransform(QTransform().scale(s, s));
        break;
    }
    case Translate: {
        int offset = horizontalScrollBar()->minimum()
            + (npaints % (horizontalScrollBar()->maximum() - horizontalScrollBar()->minimum()));
        horizontalScrollBar()->setValue(offset);
        break;
    }
    }
}

void ChipTester::populateScene()
{
    scene = new QGraphicsScene;

    QImage image(":/qt4logo.png");

    // Populate scene
    int xx = 0;
    for (int i = -1100; i < 1100; i += 110) {
        ++xx;
        int yy = 0;
        for (int j = -700; j < 700; j += 70) {
            ++yy;
            qreal x = (i + 1100) / 2200.0;
            qreal y = (j + 700) / 1400.0;

            QColor color(image.pixel(int(image.width() * x), int(image.height() * y)));
            QGraphicsItem *item = new Chip(color, xx, yy);
            item->setPos(QPointF(i, j));
            scene->addItem(item);
        }
    }
}
