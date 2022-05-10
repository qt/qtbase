// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LAYOUTITEM_H
#define LAYOUTITEM_H

#include <QGraphicsLayoutItem>
#include <QGraphicsItem>
#include <QPixmap>

//! [0]
class LayoutItem : public QGraphicsLayoutItem, public QGraphicsItem
{
public:
    LayoutItem(QGraphicsItem *parent = nullptr);

    // Inherited from QGraphicsLayoutItem
    void setGeometry(const QRectF &geom) override;
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override;

    // Inherited from QGraphicsItem
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    QPixmap m_pix;
};
//! [0]

#endif // LAYOUTITEM_H
