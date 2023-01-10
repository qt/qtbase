// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "mainwindow.h"

#include <QtWidgets>

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    fileMenu->addAction(tr("&Open..."), QKeySequence(tr("Ctrl+O", "File|Open")),
                        this, &MainWindow::openFile);

    QAction *quitAction = fileMenu->addAction(tr("E&xit"), this, &MainWindow::close);
    quitAction->setShortcut(tr("Ctrl+Q"));

    QMenu *editMenu = new QMenu(tr("&Edit"));

    cutAction = editMenu->addAction(tr("Cu&t"), this, &MainWindow::cutSelection);
    cutAction->setShortcut(tr("Ctrl+X"));
    cutAction->setEnabled(false);

    copyAction = editMenu->addAction(tr("&Copy"), this, &MainWindow::copySelection);
    copyAction->setShortcut(tr("Ctrl+C"));
    copyAction->setEnabled(false);

    pasteAction = editMenu->addAction(tr("&Paste"), this, &MainWindow::pasteSelection);
    pasteAction->setShortcut(tr("Ctrl+V"));
    pasteAction->setEnabled(false);

    QMenu *selectMenu = new QMenu(tr("&Select"));
    selectMenu->addAction(tr("&Word"), this, &MainWindow::selectWord);
    selectMenu->addAction(tr("&Line"), this, &MainWindow::selectLine);
    selectMenu->addAction(tr("&Block"), this, &MainWindow::selectBlock);
    selectMenu->addAction(tr("&Frame"), this, &MainWindow::selectFrame);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(editMenu);
    menuBar()->addMenu(selectMenu);

    editor = new QTextEdit(this);
    document = new QTextDocument(this);
    editor->setDocument(document);

    connect(editor, &QTextEdit::selectionChanged,
            this, &MainWindow::updateMenus);

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document Writer"));
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open file"), currentFile, "HTML files (*.html);;Text files (*.txt)");

    if (!fileName.isEmpty()) {
        QFileInfo info(fileName);
        if (info.completeSuffix() == "html") {
            QFile file(fileName);

            if (file.open(QFile::ReadOnly)) {
                editor->setHtml(QString(file.readAll()));
                file.close();
                currentFile = fileName;
            }
        } else if (info.completeSuffix() == "txt") {
            QFile file(fileName);

            if (file.open(QFile::ReadOnly)) {
                editor->setPlainText(file.readAll());
                file.close();
                currentFile = fileName;
            }
        }
    }
}

void MainWindow::cutSelection()
{
    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection()) {
        selection = cursor.selection();
        cursor.removeSelectedText();
    }
}

void MainWindow::copySelection()
{
    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection()) {
        selection = cursor.selection();
        cursor.clearSelection();
    }
}

void MainWindow::pasteSelection()
{
    QTextCursor cursor = editor->textCursor();
    cursor.insertFragment(selection);
}

void MainWindow::selectWord()
{
    QTextCursor cursor = editor->textCursor();

//! [0]
    cursor.beginEditBlock();
//! [1]
    cursor.movePosition(QTextCursor::StartOfWord);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
//! [1]
    cursor.endEditBlock();
//! [0]

    editor->setTextCursor(cursor);
}

void MainWindow::selectLine()
{
    QTextCursor cursor = editor->textCursor();

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    cursor.endEditBlock();

    editor->setTextCursor(cursor);
}

void MainWindow::selectBlock()
{
    QTextCursor cursor = editor->textCursor();

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.endEditBlock();

    editor->setTextCursor(cursor);
}

void MainWindow::selectFrame()
{
    QTextCursor cursor = editor->textCursor();
    QTextFrame *frame = cursor.currentFrame();

    cursor.beginEditBlock();
    cursor.setPosition(frame->firstPosition());
    cursor.setPosition(frame->lastPosition(), QTextCursor::KeepAnchor);
    cursor.endEditBlock();

    editor->setTextCursor(cursor);
}

void MainWindow::updateMenus()
{
    QTextCursor cursor = editor->textCursor();
    cutAction->setEnabled(cursor.hasSelection());
    copyAction->setEnabled(cursor.hasSelection());

    pasteAction->setEnabled(!selection.isEmpty());
}
