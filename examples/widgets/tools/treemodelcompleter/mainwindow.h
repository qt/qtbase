// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>

class TreeModelCompleter;
QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QComboBox;
class QLabel;
class QLineEdit;
class QTreeView;
QT_END_NAMESPACE

//! [0]
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void about();
    void changeCase(int);
    void changeMode(int);
    void highlight(const QModelIndex &index);
    void updateContentsLabel(const QString &sep);
//! [0]

//! [1]
private:
    void createMenu();
    QAbstractItemModel *modelFromFile(const QString &fileName);

    QTreeView *treeView = nullptr;
    QComboBox *caseCombo = nullptr;
    QComboBox *modeCombo = nullptr;
    QLabel *contentsLabel = nullptr;
    TreeModelCompleter *completer = nullptr;
    QLineEdit *lineEdit = nullptr;
};
//! [1]

#endif // MAINWINDOW_H
