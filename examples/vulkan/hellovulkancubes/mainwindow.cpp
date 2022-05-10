// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
