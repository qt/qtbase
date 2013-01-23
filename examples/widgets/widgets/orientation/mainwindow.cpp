/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
#include "ui_landscape.h"
#include "ui_portrait.h"

#include <QDesktopWidget>
#include <QResizeEvent>

//! [0]
MainWindow::MainWindow(QWidget *parent) :
        QWidget(parent),
        landscapeWidget(0),
        portraitWidget(0)
{
    landscapeWidget = new QWidget(this);
    landscape.setupUi(landscapeWidget);

    portraitWidget = new QWidget(this);
    portrait.setupUi(portraitWidget);
//! [0]

//! [1]
    connect(portrait.exitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(landscape.exitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(landscape.buttonGroup, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(onRadioButtonClicked(QAbstractButton*)));

    landscape.radioAButton->setChecked(true);
    onRadioButtonClicked(landscape.radioAButton);
//! [1]

//! [2]
}
//! [2]

//! [3]
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QSize size = event->size();
    bool isLandscape = size.width() > size.height();

    if (!isLandscape)
        size.transpose();

    landscapeWidget->setFixedSize(size);
    size.transpose();
    portraitWidget->setFixedSize(size);

    landscapeWidget->setVisible(isLandscape);
    portraitWidget->setVisible(!isLandscape);
}
//! [3]

//! [4]
void MainWindow::onRadioButtonClicked(QAbstractButton *button)
{
    QString styleTemplate = "background-image: url(:/image_%1.png);"
                            "background-repeat: no-repeat;"
                            "background-position: center center";

    QString imageStyle("");
    if (button == landscape.radioAButton)
        imageStyle = styleTemplate.arg("a");
    else if (button == landscape.radioBButton)
        imageStyle = styleTemplate.arg("b");
    else if (button == landscape.radioCButton)
        imageStyle = styleTemplate.arg("c");

    portrait.choiceWidget->setStyleSheet(imageStyle);
    landscape.choiceWidget->setStyleSheet(imageStyle);
}
//! [4]

