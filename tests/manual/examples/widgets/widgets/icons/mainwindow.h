// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QIcon>
#include <QList>
#include <QMainWindow>
#include <QPixmap>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QLabel;
class QButtonGroup;
class QTableWidget;
class QAbstractButton;
QT_END_NAMESPACE
class IconPreviewArea;
class IconSizeSpinBox;

//! [0]
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    void loadImages(const QStringList &fileNames);

    void show();

private slots:
    void about();
    void changeStyle(bool checked);
    void changeSize(QAbstractButton *button, bool);
    void triggerChangeSize();
    void changeIcon();
    void addSampleImages();
    void addOtherImages();
    void removeAllImages();
    void screenChanged();

private:
    QWidget *createImagesGroupBox();
    QWidget *createIconSizeGroupBox();
    QWidget *createHighDpiIconSizeGroupBox();
    void createActions();
    void createContextMenu();
    void checkCurrentStyle();
    void addImages(const QString &directory);

    IconPreviewArea *previewArea;

    QTableWidget *imagesTable;

    QButtonGroup *sizeButtonGroup;
    IconSizeSpinBox *otherSpinBox;

    QLabel *devicePixelRatioLabel;
    QLabel *screenNameLabel;

    QAction *addOtherImagesAct;
    QAction *addSampleImagesAct;
    QAction *removeAllImagesAct;
    QAction *guessModeStateAct;
    QAction *nativeFileDialogAct;
    QActionGroup *styleActionGroup;
};
//! [0]

#endif
