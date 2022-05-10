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

    QMenu *insertMenu = new QMenu(tr("&Insert"));

    QAction *calendarAction = insertMenu->addAction(tr("&Calendar"));
    calendarAction->setShortcut(tr("Ctrl+I"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(insertMenu);

//! [0]
    editor = new QTextEdit(this);
//! [0]

    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);
    connect(calendarAction, &QAction::triggered, this, &MainWindow::insertCalendar);

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document Writer"));
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

void MainWindow::insertCalendar()
{
//! [1]
    QTextCursor cursor(editor->textCursor());
    cursor.movePosition(QTextCursor::Start);

    QTextCharFormat format(cursor.charFormat());
    format.setFontFamily("Courier");

    QTextCharFormat boldFormat = format;
    boldFormat.setFontWeight(QFont::Bold);

    cursor.insertBlock();
    cursor.insertText(" ", boldFormat);

    QDate date = QDate::currentDate();
    int year = date.year(), month = date.month();

    for (int weekDay = 1; weekDay <= 7; ++weekDay) {
        cursor.insertText(QString("%1 ").arg(QLocale::system().dayName(weekDay), 3),
            boldFormat);
    }

    cursor.insertBlock();
    cursor.insertText(" ", format);

    for (int column = 1; column < QDate(year, month, 1).dayOfWeek(); ++column) {
        cursor.insertText("    ", format);
    }

    for (int day = 1; day <= date.daysInMonth(); ++day) {
//! [1] //! [2]
        int weekDay = QDate(year, month, day).dayOfWeek();

        if (QDate(year, month, day) == date)
            cursor.insertText(QString("%1 ").arg(day, 3), boldFormat);
        else
            cursor.insertText(QString("%1 ").arg(day, 3), format);

        if (weekDay == 7) {
            cursor.insertBlock();
            cursor.insertText(" ", format);
        }
//! [2] //! [3]
    }
//! [3]
}

