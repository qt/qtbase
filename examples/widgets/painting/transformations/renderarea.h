// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QList>
#include <QPainterPath>
#include <QRect>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPaintEvent;
QT_END_NAMESPACE

//! [0]
enum Operation { NoTransformation, Translate, Rotate, Scale };
//! [0]

//! [1]
class RenderArea : public QWidget
{
    Q_OBJECT

public:
    RenderArea(QWidget *parent = nullptr);

    void setOperations(const QList<Operation> &operations);
    void setShape(const QPainterPath &shape);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
//! [1]

//! [2]
private:
    void drawCoordinates(QPainter &painter);
    void drawOutline(QPainter &painter);
    void drawShape(QPainter &painter);
    void transformPainter(QPainter &painter);

    QList<Operation> operations;
    QPainterPath shape;
    QRect xBoundingRect;
    QRect yBoundingRect;
};
//! [2]

#endif // RENDERAREA_H
