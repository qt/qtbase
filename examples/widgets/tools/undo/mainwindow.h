// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

class Document;

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    void addDocument(Document *doc);
    void removeDocument(Document *doc);
    void setCurrentDocument(Document *doc);
    Document *currentDocument() const;

public slots:
    void openDocument();
    void saveDocument();
    void closeDocument();
    void newDocument();

    void addShape();
    void removeShape();
    void setShapeColor();

    void addSnowman();
    void addRobot();

    void about();
    void aboutQt();

private slots:
    void updateActions();

private:
    QUndoGroup *m_undoGroup;

    QString fixedWindowTitle(const Document *doc) const;
};

#endif // MAINWINDOW_H
