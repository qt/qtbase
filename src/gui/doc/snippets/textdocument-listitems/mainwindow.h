// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QList>
#include <QMainWindow>
#include <QString>

class QAction;
class QTextDocument;
class QTextEdit;
class QTextList;
class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

public slots:
    void insertList();
    void highlightListItems();
    void showList();

private:
    QString currentFile;
    QTextEdit *editor = nullptr;
    QTextDocument *document = nullptr;
    QList<QTextList*> listStructures;
    QList<QTreeWidgetItem*> previousItems;
    QList<QTreeWidgetItem*> parentItems;
};

#endif
