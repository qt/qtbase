// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BLUREFFECT_H
#define BLUREFFECT_H

#include <QGraphicsEffect>
#include <QGraphicsItem>

class BlurEffect: public QGraphicsBlurEffect
{
public:
    BlurEffect(QGraphicsItem *item);

    void setBaseLine(qreal y) { m_baseLine = y; }

    QRectF boundingRect() const;

    void draw(QPainter *painter) override;

private:
    void adjustForItem();

private:
    qreal m_baseLine;
    QGraphicsItem *item;
};

#endif // BLUREFFECT_H
