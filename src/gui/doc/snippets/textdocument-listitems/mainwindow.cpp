// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "mainwindow.h"

#include <QMenu>
#include <QMenuBar>
#include <QTextEdit>
#include <QTextList>
#include <QTreeWidget>

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    fileMenu->addAction(tr("E&xit"), QKeySequence(tr("Ctrl+Q", "File|Exit")),
                        this, &QWidget::close);

    QMenu *actionsMenu = new QMenu(tr("&Actions"));
    actionsMenu->addAction(tr("&Highlight List Items"),
                        this, &MainWindow::highlightListItems);
    actionsMenu->addAction(tr("&Show Current List"), this, &MainWindow::showList);

    QMenu *insertMenu = new QMenu(tr("&Insert"));

    insertMenu->addAction(tr("&List"), QKeySequence(tr("Ctrl+L", "Insert|List")),
                          this, &MainWindow::insertList);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(insertMenu);
    menuBar()->addMenu(actionsMenu);

    editor = new QTextEdit(this);
    document = new QTextDocument(this);
    editor->setDocument(document);

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document List Items"));
}

void MainWindow::highlightListItems()
{
    QTextCursor cursor = editor->textCursor();
    QTextList *list = cursor.currentList();

    if (!list)
        return;

    cursor.beginEditBlock();
//! [0]
    for (int index = 0; index < list->count(); ++index) {
        QTextBlock listItem = list->item(index);
//! [0]
        QTextBlockFormat newBlockFormat = listItem.blockFormat();
        newBlockFormat.setBackground(Qt::lightGray);
        QTextCursor itemCursor = cursor;
        itemCursor.setPosition(listItem.position());
        //itemCursor.movePosition(QTextCursor::StartOfBlock);
        itemCursor.movePosition(QTextCursor::EndOfBlock,
                                QTextCursor::KeepAnchor);
        itemCursor.setBlockFormat(newBlockFormat);
        /*
//! [1]
        processListItem(listItem);
//! [1]
        */
//! [2]
    }
//! [2]
    cursor.endEditBlock();
}

void MainWindow::showList()
{
    QTextCursor cursor = editor->textCursor();
    QTextFrame *frame = cursor.currentFrame();

    if (!frame)
        return;

    QTreeWidget *treeWidget = new QTreeWidget;
    treeWidget->setColumnCount(1);
    QStringList headerLabels;
    headerLabels << tr("Lists");
    treeWidget->setHeaderLabels(headerLabels);

    QTreeWidgetItem *parentItem = nullptr;
    QTreeWidgetItem *item;
    QTreeWidgetItem *lastItem = nullptr;
    parentItems.clear();
    previousItems.clear();

//! [3]
    QTextFrame::iterator it;
    for (it = frame->begin(); !(it.atEnd()); ++it) {

        QTextBlock block = it.currentBlock();

        if (block.isValid()) {

            QTextList *list = block.textList();

            if (list) {
                int index = list->itemNumber(block);
//! [3]
                if (index == 0) {
                    parentItems.append(parentItem);
                    previousItems.append(lastItem);
                    listStructures.append(list);
                    parentItem = lastItem;
                    lastItem = 0;

                    if (parentItem != 0)
                        item = new QTreeWidgetItem(parentItem, lastItem);
                    else
                        item = new QTreeWidgetItem(treeWidget, lastItem);

                } else {

                    while (parentItem != 0 && listStructures.last() != list) {
                        listStructures.pop_back();
                        parentItem = parentItems.takeLast();
                        lastItem = previousItems.takeLast();
                    }
                    if (parentItem != 0)
                        item = new QTreeWidgetItem(parentItem, lastItem);
                    else
                        item = new QTreeWidgetItem(treeWidget, lastItem);
                }
                item->setText(0, block.text());
                lastItem = item;
                /*
//! [4]
                processListItem(list, index);
//! [4]
                */
//! [5]
            }
//! [5] //! [6]
        }
//! [6] //! [7]
    }
//! [7]

    treeWidget->setWindowTitle(tr("List Contents"));
    treeWidget->show();
}

void MainWindow::insertList()
{
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();

    QTextList *list = cursor.currentList();
    QTextListFormat listFormat;
    if (list)
        listFormat = list->format();

    listFormat.setStyle(QTextListFormat::ListDisc);
    listFormat.setIndent(listFormat.indent() + 1);
    cursor.insertList(listFormat);

    cursor.endEditBlock();
}
