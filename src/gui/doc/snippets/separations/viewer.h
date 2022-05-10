// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef VIEWER_H
#define VIEWER_H

#include <QImage>
#include <QMainWindow>
#include <QMap>

class QAction;
class QFrame;
class QGridLayout;
class QLabel;
class QMenu;
class FinalWidget;
class ScreenWidget;

class Viewer : public QMainWindow
{
    Q_OBJECT
public:
    enum Brightness { None, Quarter, Half, ThreeQuarters, Full };
    Viewer();

public slots:
    void chooseFile();
    void setBrightness(QAction *action);
    void createImage();
    void saveImage();

private:
    void createMenus();
    QFrame *createCentralWidget();
    void openImageFile(QString &filePath);

    FinalWidget *finalWidget;
    int brightness;
    QAction *currentBrightness;
    QAction *saveAction;
    QGridLayout *grid;
    QImage scaledImage;
    QMap <QAction*,Brightness> menuMap;
    QMenu *brightnessMenu;
    QMenu *fileMenu;
    QString path;
    ScreenWidget *cyanWidget;
    ScreenWidget *magentaWidget;
    ScreenWidget *yellowWidget;
};

#endif
