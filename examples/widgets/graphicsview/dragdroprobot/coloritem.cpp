// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "coloritem.h"

#include <QApplication>
#include <QBitmap>
#include <QCursor>
#include <QDrag>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QPainter>
#include <QRandomGenerator>
#include <QWidget>

//! [0]
ColorItem::ColorItem()
    : color(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256))
{
    setToolTip(QString("QColor(%1, %2, %3)\n%4")
              .arg(color.red()).arg(color.green()).arg(color.blue())
              .arg("Click and drag this color onto the robot!"));
    setCursor(Qt::OpenHandCursor);
    setAcceptedMouseButtons(Qt::LeftButton);
}
//! [0]

//! [1]
QRectF ColorItem::boundingRect() const
{
    return QRectF(-15.5, -15.5, 34, 34);
}
//! [1]

//! [2]
void ColorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::darkGray);
    painter->drawEllipse(-12, -12, 30, 30);
    painter->setPen(QPen(Qt::black, 1));
    painter->setBrush(QBrush(color));
    painter->drawEllipse(-15, -15, 30, 30);
}
//! [2]

//! [3]
void ColorItem::mousePressEvent(QGraphicsSceneMouseEvent *)
{
    setCursor(Qt::ClosedHandCursor);
}
//! [3]

//! [5]
void ColorItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton))
        .length() < QApplication::startDragDistance()) {
        return;
    }

    QDrag *drag = new QDrag(event->widget());
    QMimeData *mime = new QMimeData;
    drag->setMimeData(mime);
//! [5]

//! [6]
    static int n = 0;
    if (n++ > 2 && QRandomGenerator::global()->bounded(3) == 0) {
        QImage image(":/images/head.png");
        mime->setImageData(image);

        drag->setPixmap(QPixmap::fromImage(image).scaled(30, 40));
        drag->setHotSpot(QPoint(15, 30));
//! [6]
//! [7]
    } else {
        mime->setColorData(color);
        mime->setText(QString("#%1%2%3")
                      .arg(color.red(), 2, 16, QLatin1Char('0'))
                      .arg(color.green(), 2, 16, QLatin1Char('0'))
                      .arg(color.blue(), 2, 16, QLatin1Char('0')));

        QPixmap pixmap(34, 34);
        pixmap.fill(Qt::white);

        QPainter painter(&pixmap);
        painter.translate(15, 15);
        painter.setRenderHint(QPainter::Antialiasing);
        paint(&painter, nullptr, nullptr);
        painter.end();

        pixmap.setMask(pixmap.createHeuristicMask());

        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(15, 20));
    }
//! [7]

//! [8]
    drag->exec();
    setCursor(Qt::OpenHandCursor);
}
//! [8]

//! [4]
void ColorItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    setCursor(Qt::OpenHandCursor);
}
//! [4]
