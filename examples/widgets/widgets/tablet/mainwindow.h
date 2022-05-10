// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QColorDialog;
QT_END_NAMESPACE
class TabletCanvas;

//! [0]
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(TabletCanvas *canvas);

private slots:
    void setBrushColor();
    void setAlphaValuator(QAction *action);
    void setLineWidthValuator(QAction *action);
    void setSaturationValuator(QAction *action);
    void setEventCompression(bool compress);
    bool save();
    void load();
    void clear();
    void about();

private:
    void createMenus();

    TabletCanvas *m_canvas;
    QColorDialog *m_colorDialog = nullptr;
};
//! [0]

#endif
