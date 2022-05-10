// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QComboBox;
class QCompleter;
class QLabel;
class QLineEdit;
class QCheckBox;
class QSpinBox;
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
    void changeModel();
    void changeMaxVisible(int);
//! [0]

//! [1]
private:
    void createMenu();
    QAbstractItemModel *modelFromFile(const QString &fileName);

    QComboBox *caseCombo = nullptr;
    QComboBox *modeCombo = nullptr;
    QComboBox *modelCombo = nullptr;
    QSpinBox *maxVisibleSpinBox = nullptr;
    QCheckBox *wrapCheckBox = nullptr;
    QCompleter *completer = nullptr;
    QLabel *contentsLabel = nullptr;
    QLineEdit *lineEdit = nullptr;
};
//! [1]

#endif // MAINWINDOW_H
