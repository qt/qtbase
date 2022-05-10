// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

class QTextEdit;
class QTextFrame;
class QTextBlock;
class QTextTable;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

public slots:
    void saveFile();
    void showTable();

private:
    bool writeXml(const QString &fileName);
    void processFrame(QTextFrame *);
    void processBlock(QTextBlock);
    void processTable(QTextTable *table);

    QTextEdit *editor = nullptr;
};

#endif
