// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CHIPTESTER_H
#define CHIPTESTER_H

#include <QtWidgets/QGraphicsView>
#include <QtCore/QEventLoop>

QT_FORWARD_DECLARE_CLASS(QGraphicsScene)
QT_FORWARD_DECLARE_CLASS(QGraphicsView)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSlider)
QT_FORWARD_DECLARE_CLASS(QSplitter)

class ChipTester : public QGraphicsView
{
    Q_OBJECT
public:
    enum Operation {
        Rotate360,
        ZoomInOut,
        Translate
    };
    ChipTester(QWidget *parent = nullptr);

    void setAntialias(bool enabled);
    void runBenchmark();
    void setOperation(Operation operation);

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    void populateScene();

    QGraphicsView *view;
    QGraphicsScene *scene;
    int npaints;
    int timerId;
    QEventLoop eventLoop;
    Operation operation;
};

#endif
