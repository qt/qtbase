// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "mainwindow.h"
#include "xbelreader.h"
#include "xbelwriter.h"

//! [0]
MainWindow::MainWindow()
{
    QStringList labels;
    labels << tr("Title") << tr("Location");

    treeWidget = new QTreeWidget;
    treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);
    treeWidget->setHeaderLabels(labels);
#if !defined(QT_NO_CONTEXTMENU) && !defined(QT_NO_CLIPBOARD)
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

#if !defined(QT_NO_CONTEXTMENU) && !defined(QT_NO_CLIPBOARD)
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
#endif // !QT_NO_CONTEXTMENU && !QT_NO_CLIPBOARD

//! [1]
void MainWindow::open()
{
    QString fileName =
            QFileDialog::getOpenFileName(this, tr("Open Bookmark File"),
                                         QDir::currentPath(),
                                         tr("XBEL Files (*.xbel *.xml)"));
    if (fileName.isEmpty())
        return;

    treeWidget->clear();


    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("QXmlStream Bookmarks"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName),
                                  file.errorString()));
        return;
    }

    XbelReader reader(treeWidget);
    if (!reader.read(&file)) {
        QMessageBox::warning(this, tr("QXmlStream Bookmarks"),
                             tr("Parse error in file %1:\n\n%2")
                             .arg(QDir::toNativeSeparators(fileName),
                                  reader.errorString()));
    } else {
        statusBar()->showMessage(tr("File loaded"), 2000);
    }

}
//! [1]

//! [2]
void MainWindow::saveAs()
{
    QString fileName =
            QFileDialog::getSaveFileName(this, tr("Save Bookmark File"),
                                         QDir::currentPath(),
                                         tr("XBEL Files (*.xbel *.xml)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("QXmlStream Bookmarks"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName),
                                  file.errorString()));
        return;
    }

    XbelWriter writer(treeWidget);
    if (writer.writeFile(&file))
        statusBar()->showMessage(tr("File saved"), 2000);
}
//! [2]

//! [3]
void MainWindow::about()
{
   QMessageBox::about(this, tr("About QXmlStream Bookmarks"),
            tr("The <b>QXmlStream Bookmarks</b> example demonstrates how to use Qt's "
               "QXmlStream classes to read and write XML documents."));
}
//! [3]

//! [5]
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
    helpMenu->addAction(tr("About &Qt"), qApp, &QCoreApplication::quit);
}
//! [5]
