// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BLURPICKER_H
#define BLURPICKER_H

#include <QGraphicsEffect>
#include <QGraphicsView>
#include <QPropertyAnimation>

#include "blureffect.h"

class BlurPicker: public QGraphicsView
{
    Q_OBJECT
    Q_PROPERTY(qreal index READ index WRITE setIndex)

public:
    BlurPicker(QWidget *parent = nullptr);

    qreal index() const;
    void setIndex(qreal);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupScene();

private:
    qreal m_index;
    QList<QGraphicsItem*> m_icons;
    QPropertyAnimation m_animation;
};

#endif // BLURPICKER_H
