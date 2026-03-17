// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"

#include <QAction>
#include <QFileDialog>
#include <QMenuBar>
#include <QStatusBar>

#include <QIcon>
#include <QKeySequence>

#include <QDir>
#include <QItemSelection>
#include <QModelIndexList>

//! [0]
MainWindow::MainWindow()
    : QMainWindow(),
      addressWidget(new AddressWidget)
{
    setCentralWidget(addressWidget);
    createMenus();
    setWindowTitle(tr("Address Book"));
}
//! [0]

//! [1a]
void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *openAct = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen),
                                tr("&Open..."), this);
    openAct->setShortcut(QKeySequence(QKeySequence::Open));
    fileMenu->addAction(openAct);
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);
//! [1a]

    auto *saveAct = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentSave),
                                tr("&Save"), this);
    saveAct->setShortcut(QKeySequence(QKeySequence::Save));
    fileMenu->addAction(saveAct);
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);

    fileMenu->addSeparator();

    auto *exitAct = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ApplicationExit),
                                tr("E&xit"), this);
    exitAct->setShortcut(QKeySequence(QKeySequence::Quit));
    fileMenu->addAction(exitAct);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    QMenu *toolMenu = menuBar()->addMenu(tr("&Tools"));

    auto *addAct = new QAction(tr("&Add Entry..."), this);
    addAct->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_A));
    toolMenu->addAction(addAct);
    connect(addAct, &QAction::triggered,
            addressWidget, &AddressWidget::showAddEntryDialog);

//! [1b]
    editAct = new QAction(tr("&Edit Entry..."), this);
    editAct->setEnabled(false);
    toolMenu->addAction(editAct);
    connect(editAct, &QAction::triggered, addressWidget, &AddressWidget::editEntry);

    toolMenu->addSeparator();

    removeAct = new QAction(tr("&Remove Entry"), this);
    removeAct->setEnabled(false);
    toolMenu->addAction(removeAct);
    connect(removeAct, &QAction::triggered, addressWidget, &AddressWidget::removeEntry);

    connect(addressWidget, &AddressWidget::selectionChanged,
            this, &MainWindow::updateActions);
}
//! [1b]

//! [2]
void MainWindow::openFile()
{
    if (addressWidget->readFromFile())
        statusBar()->showMessage(tr("Read %1").arg(QDir::toNativeSeparators(AddressWidget::fileName())));
}
//! [2]

//! [3]
void MainWindow::saveFile()
{
    if (addressWidget->writeToFile())
        statusBar()->showMessage(tr("Wrote %1").arg(QDir::toNativeSeparators(AddressWidget::fileName())));
}
//! [3]

//! [4]
void MainWindow::updateActions(const QItemSelection &selection)
{
    const QModelIndexList indexes = selection.indexes();

    removeAct->setEnabled(!indexes.isEmpty());
    editAct->setEnabled(!indexes.isEmpty());
}
//! [4]
