// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QPair>

QT_BEGIN_NAMESPACE
class QAction;
class QTabWidget;
QT_END_NAMESPACE

//! [0]
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    void createSample();

public slots:
    void openDialog();
    void printFile();

private:
    void createLetter(const QString &name, const QString &address,
                      QList<QPair<QString,int> > orderItems,
                      bool sendOffers);

    QAction *printAction;
    QTabWidget *letters;
};
//! [0]

#endif // MAINWINDOW_H
