// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
class QClipboard;
class QComboBox;
class QFontComboBox;
class QLineEdit;
class QScrollArea;
class QCheckBox;
QT_END_NAMESPACE
class CharacterWidget;

//! [0]
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

public slots:
    void filterChanged(int);
    void findStyles(const QFont &font);
    void findSizes(const QFont &font);
    void insertCharacter(const QString &character);
#ifndef QT_NO_CLIPBOARD
    void updateClipboard();
#endif
    void showInfo();

private:
    CharacterWidget *characterWidget;
    QComboBox *filterCombo;
    QComboBox *styleCombo;
    QComboBox *sizeCombo;
    QFontComboBox *fontCombo;
    QLineEdit *lineEdit;
    QScrollArea *scrollArea;
    QCheckBox *fontMerging;
};
//! [0]

#endif
