// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FADEMESSAGE_H
#define FADEMESSAGE_H

#include <QGraphicsEffect>
#include <QGraphicsView>
#include <QPropertyAnimation>

#include "fademessage.h"

class FadeMessage: public QGraphicsView
{
    Q_OBJECT

public:
    FadeMessage(QWidget *parent = nullptr);

private:
    void setupScene();

private slots:
    void togglePopup();

private:
    QGraphicsScene m_scene;
    QGraphicsColorizeEffect *m_effect;
    QGraphicsItem *m_message;
    QPropertyAnimation *m_animation;
};

#endif // FADEMESSAGE_H
