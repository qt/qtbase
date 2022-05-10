// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>

class PuzzleWidget;
class PiecesModel;
QT_BEGIN_NAMESPACE
class QListView;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void openImage();
    void loadImage(const QString &path);
    void setupPuzzle();

private slots:
    void setCompleted();

private:
    void setupMenus();
    void setupWidgets();

    QPixmap puzzleImage;
    QListView *piecesList;
    PuzzleWidget *puzzleWidget;
    PiecesModel *model;
};

#endif // MAINWINDOW_H
