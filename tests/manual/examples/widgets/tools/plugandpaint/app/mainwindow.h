// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDir>
#include <QMainWindow>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QMenu;
class QScrollArea;
QT_END_NAMESPACE
class PaintArea;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private slots:
    void open();
    bool saveAs();
    void brushColor();
    void brushWidth();
    void changeBrush();
    void insertShape();
    void applyFilter();
    void about();
    void aboutPlugins();

private:
    typedef void (MainWindow::*Member)();

    void createActions();
    void createMenus();
    void loadPlugins();
    void populateMenus(QObject *plugin);
    void addToMenu(QObject *plugin, const QStringList &texts, QMenu *menu,
                   Member member, QActionGroup *actionGroup = nullptr);

    PaintArea *paintArea = nullptr;
    QScrollArea *scrollArea = nullptr;
    QDir pluginsDir;
    QStringList pluginFileNames;

    QMenu *fileMenu = nullptr;
    QMenu *brushMenu = nullptr;
    QMenu *shapesMenu = nullptr;
    QMenu *filterMenu = nullptr;
    QMenu *helpMenu = nullptr;
    QActionGroup *brushActionGroup = nullptr;
    QAction *openAct = nullptr;
    QAction *saveAsAct = nullptr;
    QAction *exitAct = nullptr;
    QAction *brushWidthAct = nullptr;
    QAction *brushColorAct = nullptr;
    QAction *aboutAct = nullptr;
    QAction *aboutQtAct = nullptr;
    QAction *aboutPluginsAct = nullptr;
};

#endif
