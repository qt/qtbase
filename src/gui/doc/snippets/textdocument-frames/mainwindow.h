// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

class QTextEdit;
class QTextFrame;
class QTextBlock;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

public slots:
    void saveFile();

private:
    bool writeXml(const QString &fileName);
    void processBlock(QTextBlock);
    void processFrame(QTextFrame *frame);

    QTextEdit *editor = nullptr;
};

#endif
