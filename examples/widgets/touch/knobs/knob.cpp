// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "knob.h"

#include <QBrush>
#include <QTouchEvent>

Knob::Knob()
    : QGraphicsEllipseItem(-50, -50, 100, 100)
{
    setAcceptTouchEvents(true);
    setBrush(Qt::lightGray);

    QGraphicsEllipseItem *leftItem = new QGraphicsEllipseItem(0, 0, 20, 20, this);
    leftItem->setPos(-40, -10);
    leftItem->setBrush(Qt::darkGreen);

    QGraphicsEllipseItem *rightItem = new QGraphicsEllipseItem(0, 0, 20, 20, this);
    rightItem->setPos(20, -10);
    rightItem->setBrush(Qt::darkRed);
}

bool Knob::sceneEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);

        if (touchEvent->points().count() == 2) {
            const QEventPoint &touchPoint1 = touchEvent->points().first();
            const QEventPoint &touchPoint2 = touchEvent->points().last();

            QLineF line1(touchPoint1.sceneLastPosition(), touchPoint2.sceneLastPosition());
            QLineF line2(touchPoint1.scenePosition(), touchPoint2.scenePosition());

            setTransform(QTransform().rotate(line2.angleTo(line1)), true);
        }

        break;
    }

    default:
        return QGraphicsItem::sceneEvent(event);
    }

    return true;
}
