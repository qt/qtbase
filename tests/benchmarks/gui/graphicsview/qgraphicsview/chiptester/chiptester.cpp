/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "chiptester.h"
#include "chip.h"

#include <QtGui>
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

    setWindowTitle(tr("Chip Demo"));
}

void ChipTester::setAntialias(bool enabled)
{
    setRenderHint(QPainter::Antialiasing, enabled);
}

void ChipTester::setOpenGL(bool enabled)
{
#ifndef QT_NO_OPENGL
    setViewport(enabled ? new QGLWidget(QGLFormat(QGL::SampleBuffers)) : 0);
#endif
}

void ChipTester::setOperation(Operation operation)
{
    this->operation = operation;
}

void ChipTester::runBenchmark()
{
    npaints = 0;
    timerId = startTimer(0);
    stopWatch.start();
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
    int nitems = 0;
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

            ++nitems;
        }
    }
}
