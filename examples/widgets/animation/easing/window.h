// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QtWidgets>

#include "ui_form.h"
#include "animation.h"

class PixmapItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)
public:
    PixmapItem(const QPixmap &pix) : QGraphicsPixmapItem(pix)
    {
    }
};

class Window : public QWidget {
    Q_OBJECT
public:
    Window(QWidget *parent = nullptr);
private slots:
    void curveChanged(int row);
    void pathChanged(QAbstractButton *button);
    void periodChanged(double);
    void amplitudeChanged(double);
    void overshootChanged(double);

private:
    void createCurveIcons();
    void startAnimation();

    Ui::Form m_ui;
    QGraphicsScene m_scene;
    PixmapItem *m_item;
    Animation *m_anim;
    QSize m_iconSize;
};

#endif // WINDOW_H
