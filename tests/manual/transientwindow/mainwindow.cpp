// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_showButton("Toggle visible", this), m_window(0)
{
    connect(&m_showButton, SIGNAL(clicked()), this, SLOT(toggleVisible()));
    setWindowTitle(QString::fromLatin1("Main Window"));
    m_showButton.setVisible(true);
    setMinimumSize(300, 200);
}

MainWindow::~MainWindow()
{
}

void MainWindow::toggleVisible()
{
    if (!m_window) {
        m_window = new QWindow();
        m_window->setTransientParent(windowHandle());
        m_window->setMinimumSize(QSize(200, 100));
        m_window->setTitle("Transient Window");
        m_window->setFlags(Qt::Dialog);
    }
    m_window->setVisible(!m_window->isVisible());
}
