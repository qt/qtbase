// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "xbeltree.h"

#include <QApplication>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include <QAction>
#include <QScreen>

using namespace Qt::StringLiterals;

//! [0]
MainWindow::MainWindow()
{
    xbelTree = new XbelTree;
    setCentralWidget(xbelTree);

    createMenus();

    statusBar()->showMessage(tr("Ready"));

    setWindowTitle(tr("DOM Bookmarks"));
    const QSize availableSize = screen()->availableGeometry().size();
    resize(availableSize.width() / 2, availableSize.height() / 3);
}
//! [0]

//! [1]
void MainWindow::open()
{
    QFileDialog fileDialog(this, tr("Open Bookmark File"), QDir::currentPath());
    fileDialog.setMimeTypeFilters({"application/x-xbel"_L1});
    if (fileDialog.exec() != QDialog::Accepted)
        return;

    const QString fileName = fileDialog.selectedFiles().constFirst();
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("DOM Bookmarks"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName),
                                  file.errorString()));
        return;
    }

    if (xbelTree->read(&file))
        statusBar()->showMessage(tr("File loaded"), 2000);
}
//! [1]

//! [2]
void MainWindow::saveAs()
{
    QFileDialog fileDialog(this, tr("Save Bookmark File"), QDir::currentPath());
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setDefaultSuffix("xbel"_L1);
    fileDialog.setMimeTypeFilters({"application/x-xbel"_L1});
    if (fileDialog.exec() != QDialog::Accepted)
        return;

    const QString fileName = fileDialog.selectedFiles().constFirst();
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("DOM Bookmarks"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName),
                                  file.errorString()));
        return;
    }

    if (xbelTree->write(&file))
        statusBar()->showMessage(tr("File saved"), 2000);
}
//! [2]

//! [3]
void MainWindow::about()
{
   QMessageBox::about(this, tr("About DOM Bookmarks"),
                      tr("The <b>DOM Bookmarks</b> example demonstrates how to "
                         "use Qt's DOM classes to read and write XML "
                         "documents."));
}
//! [3]

//! [4]
void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *openAct = fileMenu->addAction(tr("&Open..."), this, &MainWindow::open);
    openAct->setShortcuts(QKeySequence::Open);

    QAction *saveAsAct = fileMenu->addAction(tr("&Save As..."), this, &MainWindow::saveAs);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);

    QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);

    menuBar()->addSeparator();

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}
//! [4]
