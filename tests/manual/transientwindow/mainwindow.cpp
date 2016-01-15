/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
