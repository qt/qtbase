// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindowbase.h"

#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#endif

#include <QList>
#include <QMap>
#include <QString>

QT_BEGIN_NAMESPACE
class QPrinter;
class QTextEdit;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

typedef QList<QTreeWidgetItem *> StyleItems;

class MainWindow : public QMainWindow, private Ui::MainWindowBase
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

public slots:
    void on_clearAction_triggered();
    void on_markAction_triggered();
    void on_unmarkAction_triggered();
    void on_printAction_triggered();
    void on_printPreviewAction_triggered();
    void printDocument(QPrinter *printer);
    void printPage(int index, QPainter *painter, QPrinter *printer);
    void showFont(QTreeWidgetItem *item);
    void updateStyles(QTreeWidgetItem *item, int column);

private:
    QMap<QString, StyleItems> currentPageMap();
    void markUnmarkFonts(Qt::CheckState state);
    void setupFontTree();

    QList<int> sampleSizes;
    QMap<QString, StyleItems> pageMap;
    int markedCount;
};

#endif // MAINWINDOW_H
