// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "mainwindow.h"

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    QAction *saveAction = fileMenu->addAction(tr("&Save..."));
    saveAction->setShortcut(tr("Ctrl+S"));

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcut(tr("Ctrl+Q"));

    menuBar()->addMenu(fileMenu);
    editor = new QTextEdit;

//! [rootframe]
    QTextDocument *editorDocument = editor->document();
    QTextFrame *root = editorDocument->rootFrame();
//! [rootframe]
    processFrame(root);

    QTextCursor cursor(editor->textCursor());
    cursor.movePosition(QTextCursor::Start);

    QTextFrame *mainFrame = cursor.currentFrame();

    QTextCharFormat plainCharFormat;
    QTextCharFormat boldCharFormat;
    boldCharFormat.setFontWeight(QFont::Bold);
/*  main frame
//! [0]
    QTextFrame *mainFrame = cursor.currentFrame();
    cursor.insertText(...);
//! [0]
*/
    cursor.insertText("Text documents are represented by the "
                      "QTextDocument class, rather than by QString objects. "
                      "Each QTextDocument object contains information about "
                      "the document's internal representation, its structure, "
                      "and keeps track of modifications to provide undo/redo "
                      "facilities. This approach allows features such as the "
                      "layout management to be delegated to specialized "
                      "classes, but also provides a focus for the framework.",
                      plainCharFormat);

//! [1]
    QTextFrameFormat frameFormat;
    frameFormat.setMargin(32);
    frameFormat.setPadding(8);
    frameFormat.setBorder(4);
//! [1]
    cursor.insertFrame(frameFormat);

/*  insert frame
//! [2]
    cursor.insertFrame(frameFormat);
    cursor.insertText(...);
//! [2]
*/
    cursor.insertText("Documents are either converted from external sources "
                      "or created from scratch using Qt. The creation process "
                      "can done by an editor widget, such as QTextEdit, or by "
                      "explicit calls to the Scribe API.", boldCharFormat);

    cursor = mainFrame->lastCursorPosition();
/*  last cursor
//! [3]
    cursor = mainFrame->lastCursorPosition();
    cursor.insertText(...);
//! [3]
*/
    cursor.insertText("There are two complementary ways to visualize the "
                      "contents of a document: as a linear buffer that is "
                      "used by editors to modify the contents, and as an "
                      "object hierarchy containing structural information "
                      "that is useful to layout engines. In the hierarchical "
                      "model, the objects generally correspond to visual "
                      "elements such as frames, tables, and lists. At a lower "
                      "level, these elements describe properties such as the "
                      "style of text used and its alignment. The linear "
                      "representation of the document is used for editing and "
                      "manipulation of the document's contents.",
                      plainCharFormat);


    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document Frames"));
}

void MainWindow::saveFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save document as:"), "", tr("XML (*.xml)"));

    if (!fileName.isEmpty()) {
        if (writeXml(fileName))
            setWindowTitle(fileName);
        else
            QMessageBox::warning(this, tr("Warning"),
                tr("Failed to save the document."), QMessageBox::Cancel,
                QMessageBox::NoButton);
    }
}

void MainWindow::processBlock(QTextBlock)
{
}

void MainWindow::processFrame(QTextFrame *frame)
{
//! [4]
    QTextFrame::iterator it;
    for (it = frame->begin(); !(it.atEnd()); ++it) {

        QTextFrame *childFrame = it.currentFrame();
        QTextBlock childBlock = it.currentBlock();

        if (childFrame)
            processFrame(childFrame);
        else if (childBlock.isValid())
            processBlock(childBlock);
    }
//! [4]
}
