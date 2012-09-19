/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "softkeys.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    central = new QWidget(this);
    central->setContextMenuPolicy(Qt::NoContextMenu); // explicitly forbid usage of context menu so actions item is not shown menu
    setCentralWidget(central);

    // Create text editor and set softkeys to it
    textEditor= new QTextEdit(tr("Navigate in UI to see context sensitive softkeys in action"), this);
    QAction* clear = new QAction(tr("Clear"), this);
    clear->setSoftKeyRole(QAction::NegativeSoftKey);

    textEditor->addAction(clear);

    ok = new QAction(tr("Ok"), this);
    ok->setSoftKeyRole(QAction::PositiveSoftKey);
    connect(ok, SIGNAL(triggered()), this, SLOT(okPressed()));

    cancel = new QAction(tr("Cancel"), this);
    cancel->setSoftKeyRole(QAction::NegativeSoftKey);
    connect(cancel, SIGNAL(triggered()), this, SLOT(cancelPressed()));

    infoLabel = new QLabel(tr(""), this);
    infoLabel->setContextMenuPolicy(Qt::NoContextMenu);

    toggleButton = new QPushButton(tr("Custom"), this);
    toggleButton->setContextMenuPolicy(Qt::NoContextMenu);
    toggleButton->setCheckable(true);

    modeButton = new QPushButton(tr("Loop SK window type"), this);
    modeButton->setContextMenuPolicy(Qt::NoContextMenu);

    modeLabel = new QLabel(tr("Normal maximized"), this);
    modeLabel->setContextMenuPolicy(Qt::NoContextMenu);

    pushButton = new QPushButton(tr("File Dialog"), this);
    pushButton->setContextMenuPolicy(Qt::NoContextMenu);

    QComboBox* comboBox = new QComboBox(this);
    comboBox->setContextMenuPolicy(Qt::NoContextMenu);
    comboBox->insertItems(0, QStringList()
     << QApplication::translate("MainWindow", "Selection1", 0, QApplication::UnicodeUTF8)
     << QApplication::translate("MainWindow", "Selection2", 0, QApplication::UnicodeUTF8)
     << QApplication::translate("MainWindow", "Selection3", 0, QApplication::UnicodeUTF8)
    );

    layout = new QGridLayout;
    layout->addWidget(textEditor, 0, 0, 1, 2);
    layout->addWidget(infoLabel, 1, 0, 1, 2);
    layout->addWidget(toggleButton, 2, 0);
    layout->addWidget(pushButton, 2, 1);
    layout->addWidget(comboBox, 3, 0, 1, 2);
    layout->addWidget(modeButton, 4, 0, 1, 2);
    layout->addWidget(modeLabel, 5, 0, 1, 2);
    central->setLayout(layout);

    fileMenu = menuBar()->addMenu(tr("&File"));
    exit = new QAction(tr("&Exit"), this);
    fileMenu->addAction(exit);

    connect(clear, SIGNAL(triggered()), this, SLOT(clearTextEditor()));
    connect(pushButton, SIGNAL(clicked()), this, SLOT(openDialog()));
    connect(exit, SIGNAL(triggered()), this, SLOT(exitApplication()));
    connect(toggleButton, SIGNAL(clicked()), this, SLOT(setCustomSoftKeys()));
    connect(modeButton, SIGNAL(clicked()), this, SLOT(setMode()));
    pushButton->setFocus();
}

MainWindow::~MainWindow()
{
}

void MainWindow::clearTextEditor()
{
    textEditor->setText(tr(""));
}

void MainWindow::openDialog()
{
    QFileDialog::getOpenFileName(this);
}

void MainWindow::addSoftKeys()
{
    addAction(ok);
    addAction(cancel);
}

void MainWindow::setCustomSoftKeys()
{
    if (toggleButton->isChecked()) {
        infoLabel->setText(tr("Custom softkeys set"));
        addSoftKeys();
        }
    else {
        infoLabel->setText(tr("Custom softkeys removed"));
        removeAction(ok);
        removeAction(cancel);
    }
}

void MainWindow::setMode()
{
    if(isMaximized()) {
        showFullScreen();
        modeLabel->setText(tr("Normal Fullscreen"));
    } else {
        Qt::WindowFlags flags = windowFlags();
        if(flags & Qt::WindowSoftkeysRespondHint) {
            flags |= Qt::WindowSoftkeysVisibleHint;
            flags &= ~Qt::WindowSoftkeysRespondHint;
            setWindowFlags(flags); // Hides visible window
            showFullScreen();
            modeLabel->setText(tr("Fullscreen with softkeys"));
        } else if(flags & Qt::WindowSoftkeysVisibleHint) {
            flags &= ~Qt::WindowSoftkeysVisibleHint;
            flags &= ~Qt::WindowSoftkeysRespondHint;
            setWindowFlags(flags); // Hides visible window
            showMaximized();
            modeLabel->setText(tr("Normal Maximized"));
        } else {
            flags &= ~Qt::WindowSoftkeysVisibleHint;
            flags |= Qt::WindowSoftkeysRespondHint;
            setWindowFlags(flags); // Hides visible window
            showFullScreen();
            modeLabel->setText(tr("Fullscreen with SK respond"));
        }
    }
}

void MainWindow::exitApplication()
{
    qApp->exit();
}

void MainWindow::okPressed()
{
    infoLabel->setText(tr("OK pressed"));
}

void MainWindow::cancelPressed()
{
    infoLabel->setText(tr("Cancel pressed"));
}


