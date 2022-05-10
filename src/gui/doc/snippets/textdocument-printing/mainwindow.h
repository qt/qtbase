// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QMainWindow>
#include <QTextDocumentFragment>

class QAction;
class QTextDocument;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

public slots:
    void openFile();
    void printFile();
    void printPdf();
    void updateMenus();

private:
    QAction *printAction = nullptr;
    QAction *pdfPrintAction = nullptr;
    QString currentFile;
    QTextEdit *editor = nullptr;
    QTextDocument *document = nullptr;
};

#endif
