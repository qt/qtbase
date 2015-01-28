/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    ListItem(QGraphicsWidget *parent = 0);
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

