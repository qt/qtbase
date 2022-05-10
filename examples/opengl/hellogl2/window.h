// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSlider;
class QPushButton;
QT_END_NAMESPACE

class GLWidget;
class MainWindow;

class Window : public QWidget
{
    Q_OBJECT

public:
    Window(MainWindow *mw);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void dockUndock();

private:
    QSlider *createSlider();

    GLWidget *glWidget;
    QSlider *xSlider;
    QSlider *ySlider;
    QSlider *zSlider;
    QPushButton *dockBtn;
    MainWindow *mainWindow;
};

#endif
