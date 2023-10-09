// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>

class Block;
class RenderThread;

//! [Window class definition]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);
    void loadImage(const QImage &image);

public slots:
    void addBlock(const Block &block);

private slots:
    void loadImage();
    void resetUi();

private:
    QLabel *label;
    QPixmap pixmap;
    QPushButton *loadButton;
    QPushButton *resetButton;
    QString path;
    RenderThread *thread;
};
//! [Window class definition]

#endif
