/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//Own
#include "mainwindow.h"
#include "graphicsscene.h"

//Qt
#include <QGraphicsView>

#ifdef QT_NO_OPENGL
    #include <QtGui/QMenuBar>
    #include <QtGui/QLayout>
    #include <QtGui/QApplication>
#else
    #include <QtOpenGL/QtOpenGL>
#endif

MainWindow::MainWindow() : QMainWindow(0)
{
    QMenu *file = menuBar()->addMenu(tr("&File"));

    QAction *newAction = file->addAction(tr("New Game"));
    newAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    QAction *quitAction = file->addAction(tr("Quit"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));

    if (QApplication::arguments().contains("-fullscreen")) {
        scene = new GraphicsScene(0, 0, 750, 400, GraphicsScene::Small);
        setWindowState(Qt::WindowFullScreen);
    } else {
        scene = new GraphicsScene(0, 0, 880, 630);
        layout()->setSizeConstraint(QLayout::SetFixedSize);
    }

    view = new QGraphicsView(scene, this);
    view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    scene->setupScene(newAction, quitAction);
#ifndef QT_NO_OPENGL
    view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
#endif

    setCentralWidget(view);
}
