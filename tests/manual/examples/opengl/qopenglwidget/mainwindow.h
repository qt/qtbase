// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QGridLayout>

QT_FORWARD_DECLARE_CLASS(QOpenGLWidget)

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    void addNew();
    bool timerEnabled() const { return m_timer->isActive(); }

    void resizeEvent(QResizeEvent *);

public slots:
    void showNewWindow();

private slots:
    void updateIntervalChanged(int value);
    void timerUsageChanged(bool enabled);

private:
    QTimer *m_timer;
    QGridLayout *m_layout;
    int m_nextX;
    int m_nextY;
    QList<QOpenGLWidget *> m_glWidgets;
};

#endif
