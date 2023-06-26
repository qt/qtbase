// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QString>
#include <QSize>

class ToolBar;
QT_FORWARD_DECLARE_CLASS(QMenu)

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    typedef QMap<QString, QSize> CustomSizeHintMap;

    explicit MainWindow(const CustomSizeHintMap &customSizeHints,
                        QWidget *parent = nullptr,
                        Qt::WindowFlags flags = { });

public slots:
    void actionTriggered(QAction *action);
    void saveLayout();
    void loadLayout();
    void switchLayoutDirection();
    void setDockOptions();

    void createDockWidget();
    void destroyDockWidget(QAction *action);

    void about();

private:
    void setupToolBar();
    void setupMenuBar();
    void setupDockWidgets(const CustomSizeHintMap &customSizeHints);

    QList<ToolBar*> toolBars;
    QMenu *dockWidgetMenu;
    QMenu *mainWindowMenu;
    QList<QDockWidget *> extraDockWidgets;
    QMenu *destroyDockWidgetMenu;
};

#endif // MAINWINDOW_H
