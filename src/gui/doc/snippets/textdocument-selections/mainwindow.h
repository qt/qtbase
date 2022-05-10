// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

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
    void cutSelection();
    void copySelection();
    void openFile();
    void pasteSelection();
    void selectWord();
    void selectLine();
    void selectBlock();
    void selectFrame();
    void updateMenus();

private:
    QAction *cutAction = nullptr;
    QAction *copyAction = nullptr;
    QAction *pasteAction = nullptr;
    QString currentFile;
    QTextEdit *editor = nullptr;
    QTextDocument *document = nullptr;
    QTextDocumentFragment selection;
};

#endif
