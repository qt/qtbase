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

    QMenu *showMenu = new QMenu(tr("&Show"));

    QAction *showTableAction = showMenu->addAction(tr("&Table"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(showMenu);

    editor = new QTextEdit();

//! [0] //! [1]
    QTextCursor cursor(editor->textCursor());
//! [0]
    cursor.movePosition(QTextCursor::Start);
//! [1]

    int rows = 11;
    int columns = 4;

//! [2]
    QTextTableFormat tableFormat;
    tableFormat.setBackground(QColor("#e0e0e0"));
    QList<QTextLength> constraints;
    constraints << QTextLength(QTextLength::PercentageLength, 16);
    constraints << QTextLength(QTextLength::PercentageLength, 28);
    constraints << QTextLength(QTextLength::PercentageLength, 28);
    constraints << QTextLength(QTextLength::PercentageLength, 28);
    tableFormat.setColumnWidthConstraints(constraints);
//! [3]
    QTextTable *table = cursor.insertTable(rows, columns, tableFormat);
//! [2] //! [3]

    int column;
    int row;
    QTextTableCell cell;
    QTextCursor cellCursor;

    QTextCharFormat charFormat;
    charFormat.setForeground(Qt::black);

//! [4]
    cell = table->cellAt(0, 0);
    cellCursor = cell.firstCursorPosition();
    cellCursor.insertText(tr("Week"), charFormat);
//! [4]

//! [5]
    for (column = 1; column < columns; ++column) {
        cell = table->cellAt(0, column);
        cellCursor = cell.firstCursorPosition();
        cellCursor.insertText(tr("Team %1").arg(column), charFormat);
    }

    for (row = 1; row < rows; ++row) {
        cell = table->cellAt(row, 0);
        cellCursor = cell.firstCursorPosition();
        cellCursor.insertText(tr("%1").arg(row), charFormat);

        for (column = 1; column < columns; ++column) {
            if ((row-1) % 3 == column-1) {
//! [5] //! [6]
                cell = table->cellAt(row, column);
                QTextCursor cellCursor = cell.firstCursorPosition();
                cellCursor.insertText(tr("On duty"), charFormat);
            }
//! [6] //! [7]
        }
//! [7] //! [8]
    }
//! [8]

    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);
    connect(showTableAction, &QAction::triggered, this, &MainWindow::showTable);

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document Tables"));
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

void MainWindow::showTable()
{
    QTextCursor cursor = editor->textCursor();
    QTextTable *table = cursor.currentTable();

    if (!table)
        return;

    QTableWidget *tableWidget = new QTableWidget(table->rows(), table->columns());

//! [9]
    for (int row = 0; row < table->rows(); ++row) {
        for (int column = 0; column < table->columns(); ++column) {
            QTextTableCell tableCell = table->cellAt(row, column);
//! [9]
            QTextFrame::iterator it;
            QString text;
            for (it = tableCell.begin(); !(it.atEnd()); ++it) {
                QTextBlock childBlock = it.currentBlock();
                if (childBlock.isValid())
                    text += childBlock.text();
            }
            QTableWidgetItem *newItem = new QTableWidgetItem(text);
            tableWidget->setItem(row, column, newItem);
            /*
//! [10]
            processTableCell(tableCell);
//! [10]
            */
//! [11]
        }
//! [11] //! [12]
    }
//! [12]

    tableWidget->setWindowTitle(tr("Table Contents"));
    tableWidget->show();
}

void MainWindow::processFrame(QTextFrame *)
{
}

void MainWindow::processBlock(QTextBlock)
{
}

void MainWindow::processTable(QTextTable *table)
{
    QTextFrame *frame = qobject_cast<QTextFrame *>(table);
//! [13]
    QTextFrame::iterator it;
    for (it = frame->begin(); !(it.atEnd()); ++it) {

        QTextFrame *childFrame = it.currentFrame();
        QTextBlock childBlock = it.currentBlock();

        if (childFrame) {
            QTextTable *childTable = qobject_cast<QTextTable*>(childFrame);

            if (childTable)
                processTable(childTable);
            else
                processFrame(childFrame);

        } else if (childBlock.isValid()) {
            processBlock(childBlock);
        }
    }
//! [13]
}
