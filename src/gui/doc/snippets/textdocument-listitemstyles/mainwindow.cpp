// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "mainwindow.h"

#include <QMenu>
#include <QMenuBar>
#include <QTextEdit>

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    fileMenu->addAction(tr("E&xit"), QKeySequence(tr("Ctrl+Q", "File|Exit")),
                        this, SLOT(close()));

    QMenu *insertMenu = new QMenu(tr("&Insert"));

    insertMenu->addAction(tr("&List"), QKeySequence(tr("Ctrl+L", "Insert|List")),
                          this, SLOT(insertList()));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(insertMenu);

    editor = new QTextEdit(this);
    document = new QTextDocument(this);
    editor->setDocument(document);

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document List Item Styles"));
}

void MainWindow::insertList()
{
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();

    //! [add a styled, ordered list]
    QTextListFormat listFormat;

    listFormat.setStyle(QTextListFormat::ListDecimal);
    listFormat.setNumberPrefix("(");
    listFormat.setNumberSuffix(")");

    cursor.insertList(listFormat);
    //! [add a styled, ordered list]

    cursor.endEditBlock();
}
