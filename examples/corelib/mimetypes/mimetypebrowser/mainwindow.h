// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QMainWindow>
#include <QModelIndexList>
#include <QTextEdit>
#include <QTreeView>

class MimetypeModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void currentChanged(const QModelIndex &);
    void detectFile();
    void find();
    void findNext();
    void findPrevious();

private:
    void selectAndGoTo(const QModelIndex &index);
    void updateFindActions();

    MimetypeModel *m_model;
    QTreeView *m_treeView;
    QTextEdit *m_detailsText;
    QAction *m_findNextAction;
    QAction *m_findPreviousAction;
    QModelIndexList m_findMatches;
    int m_findIndex;
};

#endif // MAINWINDOW_H
