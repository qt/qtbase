// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ROBOT_H
#define ROBOT_H

#include <QGraphicsItem>

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
class QParallelAnimationGroup;
QT_END_NAMESPACE

//! [0]
class RobotPart : public QGraphicsObject
{
public:
    RobotPart(QGraphicsItem *parent = nullptr);

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

    QColor color = Qt::lightGray;
    bool dragOver = false;
};
//! [0]

//! [1]
class RobotHead : public RobotPart
{
public:
    RobotHead(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

private:
    QPixmap pixmap;
};
//! [1]

//! [2]
class RobotTorso : public RobotPart
{
public:
    using RobotPart::RobotPart;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
};
//! [2]

//! [3]
class RobotLimb : public RobotPart
{
public:
    RobotLimb(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
};
//! [3]

//! [4]
class Robot : public RobotPart
{
public:
    Robot(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
};
//! [4]

#endif
