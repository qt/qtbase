// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef LISTITEM_H
#define LISTITEM_H

#include <QPen>
#include "iconitem.h"
#include "label.h"
#include "gvbwidget.h"

class QGraphicsGridLayout;
class QGraphicsLinearLayout;
class QGraphicsSceneMouseEvent;
class QGraphicsItem;

class ListItem : public GvbWidget
{
    Q_OBJECT

public:

    enum TextPos {
        FirstPos = 0,
        SecondPos = 1,
        ThirdPos = 2,
        LastPos = 3
    };

    enum IconItemPos {
        LeftIcon = 0,
        RightIcon = 1
    };

    ListItem(QGraphicsWidget *parent = nullptr);
    virtual ~ListItem();

    void setIcon(IconItem *iconItem, const IconItemPos iconPos);
    IconItem* icon(const IconItemPos position)  const;
    void setText(const QString str, const TextPos position);
    QString text(const TextPos position) const;
    void setFont(const QFont font, const TextPos position);

    QVariant data(int role = Qt::DisplayRole) const;
    void setData(const QVariant &value, int role = Qt::DisplayRole);

    void setBorderPen(const QPen pen) { m_borderPen = pen; }
    void setBackgroundBrush(const QBrush brush) { m_backgroundBrush = brush; }
    void setBackgroundOpacity(const qreal opacity) { m_backgroundOpacity = opacity; }
    void setRounding(const QSize rounding) { m_rounding = rounding; }

protected:

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    Q_DISABLE_COPY(ListItem)
    QGraphicsGridLayout *m_txtlayout;
    QGraphicsLinearLayout *m_layout;
    QGraphicsLinearLayout *m_liconlayout;
    QGraphicsLinearLayout *m_riconlayout;
    QHash<TextPos, QFont> m_fonts;

    QPen m_borderPen;
    QBrush m_backgroundBrush;
    qreal m_backgroundOpacity;
    QSize m_rounding;
};

#endif // LISTITEM_H

