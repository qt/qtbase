// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Dock Widgets");

    setupDockWindow();
    setupContents();
    setupMenus();

    textBrowser = new QTextBrowser(this);

    connect(headingList, &QListWidget::itemClicked,
            this, &MainWindow::updateText);

    updateText(headingList->item(0));
    headingList->setCurrentRow(0);
    setCentralWidget(textBrowser);
}

void MainWindow::setupContents()
{
    QFile titlesFile(":/Resources/titles.txt");
    titlesFile.open(QFile::ReadOnly);
    int chapter = 0;

    do {
        QString line = titlesFile.readLine().trimmed();
        QStringList parts = line.split(u'\t', Qt::SkipEmptyParts);
        if (parts.size() != 2)
            break;

        QString chapterTitle = parts[0];
        QString fileName = parts[1];

        QFile chapterFile(fileName);

        chapterFile.open(QFile::ReadOnly);
        QListWidgetItem *item = new QListWidgetItem(chapterTitle, headingList);
        item->setData(Qt::DisplayRole, chapterTitle);
        item->setData(Qt::UserRole, chapterFile.readAll());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        chapterFile.close();

        chapter++;
    } while (titlesFile.isOpen());

    titlesFile.close();
}

void MainWindow::setupDockWindow()
{
//! [0]
    contentsWindow = new QDockWidget(tr("Table of Contents"), this);
    contentsWindow->setAllowedAreas(Qt::LeftDockWidgetArea
                                  | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, contentsWindow);

    headingList = new QListWidget(contentsWindow);
    contentsWindow->setWidget(headingList);
//! [0]
}

void MainWindow::setupMenus()
{
    QAction *exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAct);
}

void MainWindow::updateText(QListWidgetItem *item)
{
    QString text = item->data(Qt::UserRole).toString();
    textBrowser->setHtml(text);
}
