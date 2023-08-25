// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "xbelreader.h"
#include "xbelwriter.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTreeWidget>

#include <QAction>
#if QT_CONFIG(clipboard)
#  include <QClipboard>
#endif
#include <QDesktopServices>
#include <QApplication>
#include <QScreen>

using namespace Qt::StringLiterals;

//! [0]
MainWindow::MainWindow() : treeWidget(new QTreeWidget)
{
    treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);
    treeWidget->setHeaderLabels(QStringList{tr("Title"), tr("Location")});
#if QT_CONFIG(clipboard) && QT_CONFIG(contextmenu)
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeWidget, &QWidget::customContextMenuRequested,
            this, &MainWindow::onCustomContextMenuRequested);
#endif
    setCentralWidget(treeWidget);

    createMenus();

    statusBar()->showMessage(tr("Ready"));

    setWindowTitle(tr("QXmlStream Bookmarks"));
    const QSize availableSize = screen()->availableGeometry().size();
    resize(availableSize.width() / 2, availableSize.height() / 3);
}
//! [0]

//! [1]
#if QT_CONFIG(clipboard) && QT_CONFIG(contextmenu)
void MainWindow::onCustomContextMenuRequested(const QPoint &pos)
{
    const QTreeWidgetItem *item = treeWidget->itemAt(pos);
    if (!item)
        return;
    const QString url = item->text(1);
    QMenu contextMenu;
    QAction *copyAction = contextMenu.addAction(tr("Copy Link to Clipboard"));
    QAction *openAction = contextMenu.addAction(tr("Open"));
    QAction *action = contextMenu.exec(treeWidget->viewport()->mapToGlobal(pos));
    if (action == copyAction)
        QGuiApplication::clipboard()->setText(url);
    else if (action == openAction)
        QDesktopServices::openUrl(QUrl(url));
}
#endif // QT_CONFIG(clipboard) && QT_CONFIG(contextmenu)
//! [1]

//! [2]
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
//! [2]

//! [3]
void MainWindow::open()
{
    QFileDialog fileDialog(this, tr("Open Bookmark File"), QDir::currentPath());
    fileDialog.setMimeTypeFilters({"application/x-xbel"_L1});
    if (fileDialog.exec() != QDialog::Accepted)
        return;

    treeWidget->clear();

    const QString fileName = fileDialog.selectedFiles().constFirst();
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("QXmlStream Bookmarks"),
                             tr("Cannot read file %1:\n%2.")
                                     .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    XbelReader reader(treeWidget);
    if (!reader.read(&file)) {
        QMessageBox::warning(
                this, tr("QXmlStream Bookmarks"),
                tr("Parse error in file %1:\n\n%2")
                        .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
    } else {
        statusBar()->showMessage(tr("File loaded"), 2000);
    }
}
//! [3]

//! [4]
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
        QMessageBox::warning(this, tr("QXmlStream Bookmarks"),
                             tr("Cannot write file %1:\n%2.")
                                     .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    XbelWriter writer(treeWidget);
    if (writer.writeFile(&file))
        statusBar()->showMessage(tr("File saved"), 2000);
}
//! [4]

//! [5]
void MainWindow::about()
{
    QMessageBox::about(this, tr("About QXmlStream Bookmarks"),
                       tr("The <b>QXmlStream Bookmarks</b> example demonstrates how to use Qt's "
                          "QXmlStream classes to read and write XML documents."));
}
//! [5]
