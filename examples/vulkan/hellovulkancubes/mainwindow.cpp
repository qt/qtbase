/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "mainwindow.h"
#include "vulkanwindow.h"
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QLCDNumber>
#include <QCheckBox>
#include <QGridLayout>

MainWindow::MainWindow(VulkanWindow *vulkanWindow)
{
    QWidget *wrapper = QWidget::createWindowContainer(vulkanWindow);
    wrapper->setFocusPolicy(Qt::StrongFocus);
    wrapper->setFocus();

    infoLabel = new QLabel;
    infoLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setText(tr("This example demonstrates instanced drawing\nof a mesh loaded from a file.\n"
                          "Uses a Phong material with a single light.\n"
                          "Also demonstrates dynamic uniform buffers\nand a bit of threading with QtConcurrent.\n"
                          "Uses 4x MSAA when available.\n"
                          "Comes with an FPS camera.\n"
                          "Hit [Shift+]WASD to walk and strafe.\nPress and move mouse to look around.\n"
                          "Click Add New to increase the number of instances."));

    meshSwitch = new QCheckBox(tr("&Use Qt logo"));
    meshSwitch->setFocusPolicy(Qt::NoFocus); // do not interfere with vulkanWindow's keyboard input

    counterLcd = new QLCDNumber(5);
    counterLcd->setSegmentStyle(QLCDNumber::Filled);
    counterLcd->display(m_count);

    newButton = new QPushButton(tr("&Add new"));
    newButton->setFocusPolicy(Qt::NoFocus);
    quitButton = new QPushButton(tr("&Quit"));
    quitButton->setFocusPolicy(Qt::NoFocus);
    pauseButton = new QPushButton(tr("&Pause"));
    pauseButton->setFocusPolicy(Qt::NoFocus);

    connect(quitButton, &QPushButton::clicked, qApp, &QCoreApplication::quit);
    connect(newButton, &QPushButton::clicked, vulkanWindow, [=] {
        vulkanWindow->addNew();
        m_count = vulkanWindow->instanceCount();
        counterLcd->display(m_count);
    });
    connect(pauseButton, &QPushButton::clicked, vulkanWindow, &VulkanWindow::togglePaused);
    connect(meshSwitch, &QCheckBox::clicked, vulkanWindow, &VulkanWindow::meshSwitched);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(infoLabel, 0, 2);
    layout->addWidget(meshSwitch, 1, 2);
    layout->addWidget(createLabel(tr("INSTANCES")), 2, 2);
    layout->addWidget(counterLcd, 3, 2);
    layout->addWidget(newButton, 4, 2);
    layout->addWidget(pauseButton, 5, 2);
    layout->addWidget(quitButton, 6, 2);
    layout->addWidget(wrapper, 0, 0, 7, 2);
    setLayout(layout);
}

QLabel *MainWindow::createLabel(const QString &text)
{
    QLabel *lbl = new QLabel(text);
    lbl->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    return lbl;
}
