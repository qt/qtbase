/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

#include "mainwindow.h"

#include <QApplication>
#include <QMenuBar>
#include <QGroupBox>
#include <QGridLayout>
#include <QSlider>
#include <QLabel>
#include <QTimer>

#include "glwidget.h"

MainWindow::MainWindow()
{
    GLWidget *glwidget = new GLWidget();
    QLabel *label = new QLabel(this);
    QTimer *timer = new QTimer(this);
    QSlider *slider = new QSlider(this);
    slider->setOrientation(Qt::Horizontal);

    slider->setRange(0, 100);
    slider->setSliderPosition(50);
    timer->setInterval(10);
    label->setText("A QGlWidget with OpenGl ES");
    label->setAlignment(Qt::AlignHCenter);

    QGroupBox * groupBox = new QGroupBox(this);
    setCentralWidget(groupBox);
    groupBox->setTitle("OpenGL ES Example");

    QGridLayout *layout = new QGridLayout(groupBox);

    layout->addWidget(glwidget,1,0,8,1);
    layout->addWidget(label,9,0,1,1);
    layout->addWidget(slider, 11,0,1,1);

    groupBox->setLayout(layout);

    QMenu *fileMenu = new QMenu("File");
    QMenu *helpMenu = new QMenu("Help");
    QMenu *showMenu = new QMenu("Show");
    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(showMenu);
    menuBar()->addMenu(helpMenu);
    QAction *exit = new QAction("Exit", fileMenu);
    QAction *aboutQt = new QAction("AboutQt", helpMenu);
    QAction *showLogo = new QAction("Show 3D Logo", showMenu);
    QAction *showTexture = new QAction("Show 2D Texture", showMenu);
    QAction *showBubbles = new QAction("Show bubbles", showMenu);
    showBubbles->setCheckable(true);
    showBubbles->setChecked(true);
    fileMenu->addAction(exit);
    helpMenu->addAction(aboutQt);
    showMenu->addAction(showLogo);
    showMenu->addAction(showTexture);
    showMenu->addAction(showBubbles);

    QObject::connect(timer, SIGNAL(timeout()), glwidget, SLOT(updateGL()));
    QObject::connect(exit, SIGNAL(triggered(bool)), this, SLOT(close()));
    QObject::connect(aboutQt, SIGNAL(triggered(bool)), qApp, SLOT(aboutQt()));

    QObject::connect(showLogo, SIGNAL(triggered(bool)), glwidget, SLOT(setLogo()));
    QObject::connect(showTexture, SIGNAL(triggered(bool)), glwidget, SLOT(setTexture()));
    QObject::connect(showBubbles, SIGNAL(triggered(bool)), glwidget, SLOT(showBubbles(bool)));
    QObject::connect(slider, SIGNAL(valueChanged(int)), glwidget, SLOT(setScaling(int)));
    timer->start();
}
