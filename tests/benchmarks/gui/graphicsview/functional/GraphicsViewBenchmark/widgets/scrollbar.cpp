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

#include <QGraphicsWidget>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

#include "scrollbar.h"
#include "theme.h"

class ScrollBarPrivate {
    Q_DECLARE_PUBLIC(ScrollBar)

public:

    ScrollBarPrivate(Qt::Orientation orientation, ScrollBar *scrollBar)
        : orientation(orientation)
        , sliderPosition(0.0)
        , sliderSize(0.0)
        , sliderDown(false)
        , q_ptr(scrollBar)
    {
         construct();
    }

    void themeChange()
    {
        construct();
        updateSlider();
    }

    void construct()
    {
        scrollerPixmap = Theme::p()->pixmap("scroll.svg");
        scrollBarPixmap = Theme::p()->pixmap("scrollbar.svg");

        if (orientation == Qt::Horizontal) {
            scrollerPixmap = scrollerPixmap.transformed(QTransform().rotate(90));
            scrollBarPixmap = scrollBarPixmap.transformed(QTransform().rotate(90));
        }
    }

    void setSliderPosition(qreal pos)
    {
        if (pos < 0.0)
            pos = 0.0;

        if (pos > sliderSize)
            pos = sliderSize;

        sliderPosition = pos;

        if (!qFuzzyCompare(pos, sliderPosition))
            updateSlider();
    }

    void updateSlider()
    {
        QRectF oldSlider = slider;
        slider = q_func()->boundingRect();

        qreal x = 0;
        qreal y = 0;
        qreal w = scrollerPixmap.width();
        qreal h = scrollerPixmap.height();

        //Adjust the scrollBar in relation to the scroller

        if (orientation == Qt::Horizontal) {
            qreal scrollBarHeight = scrollBarPixmap.height();

            if  (h > scrollBarHeight) {
                slider.setTop((h - scrollBarHeight)/2.0);
                slider.setHeight(scrollBarHeight);
            }
        } else {
            qreal scrollBarWidth = scrollBarPixmap.width();

            if  (w > scrollBarWidth) {
                slider.setLeft((w - scrollBarWidth)/2.0);
            }
            slider.setWidth(scrollBarWidth);
        }

        if(oldSlider != slider && (slider.size().width() > 0 &&slider.size().height() > 0 )) {
            scrollBarPixmap = Theme::p()->pixmap("scrollbar.svg", slider.size().toSize());
        }
        cursor = QRectF(x, y, w, h);

        if (orientation == Qt::Horizontal) {
            qreal dx = qreal(int(sliderPosition)) * (slider.width() - cursor.width()) / sliderSize;
            cursor.translate(dx, 0.0);
        } else {
            qreal dy = qreal(int(sliderPosition)) * (slider.height() - cursor.height()) / sliderSize;
            cursor.translate(0.0, dy);
        }
    }

    Qt::Orientation orientation;
    qreal sliderPosition;
    qreal sliderSize;

    QPointF pressPos;
    bool sliderDown;

    QRectF slider;
    QRectF cursor;
    QPixmap scrollerPixmap;
    QPixmap scrollBarPixmap;

    ScrollBar *q_ptr;
};

ScrollBar::ScrollBar(Qt::Orientation orientation, QGraphicsWidget *parent)
    : QGraphicsWidget(parent)
    , d_ptr(new ScrollBarPrivate(orientation, this))
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    setContentsMargins(0, 0, 0, 0);

    connect(Theme::p(), SIGNAL(themeChanged()), this, SLOT(themeChange()));
}

ScrollBar::~ScrollBar()
{
    delete d_ptr;
}

qreal ScrollBar::sliderSize() const
{
    Q_D(const ScrollBar);
    return d->sliderSize;
}

void ScrollBar::setSliderSize(const qreal s)
{
    Q_D(ScrollBar);
    d->sliderSize = s;
}

void ScrollBar::setSliderPosition(qreal pos)
{
    Q_D(ScrollBar);

    d->setSliderPosition(pos);
    prepareGeometryChange();
    emit sliderPositionChange(d->sliderPosition);
}

qreal ScrollBar::sliderPosition() const
{
    Q_D(const ScrollBar);
    return d->sliderPosition;
}

bool ScrollBar::sliderDown() const
{
    Q_D(const ScrollBar);
    return d->sliderDown;
}

void ScrollBar::paint(QPainter *painter,
        const QStyleOptionGraphicsItem *option,
        QWidget *widget)
{
    Q_D(ScrollBar);
    Q_UNUSED(widget);
    Q_UNUSED(option);

    d->updateSlider();

    QRect sliderRect = d->slider.toRect();
    painter->drawPixmap(sliderRect.topLeft(), d->scrollBarPixmap);

    QRect cursorRect = d->cursor.toRect();
    painter->drawPixmap(cursorRect.topLeft(), d->scrollerPixmap);
}

QSizeF ScrollBar::sizeHint(Qt::SizeHint which,
        const QSizeF &constraint) const
{
    Q_D(const ScrollBar);

    QSizeF s;

    if (d->orientation == Qt::Horizontal)
        s = QSizeF(-1, qMax(d->scrollBarPixmap.height(), d->scrollerPixmap.height()));
    else
        s = QSizeF(qMax(d->scrollBarPixmap.width(), d->scrollerPixmap.width()), -1);

    switch (which)
    {
    case Qt::MinimumSize:
        return s;

    case Qt::MaximumSize:
        return s;

    default:
        return QGraphicsWidget::sizeHint(which, constraint);
    }
}

void ScrollBar::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(ScrollBar);

    d->updateSlider();

    if (d->cursor.contains(event->pos())) {
        d->sliderDown = true;
        d->pressPos = event->pos();
        emit sliderPressed();
    }
}

void ScrollBar::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(ScrollBar);
    Q_UNUSED(event);

    d->sliderDown = false;
}

void ScrollBar::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(ScrollBar);

    if (!d->sliderDown)
        return;

    if (d->orientation == Qt::Horizontal) {
        qreal f = (event->pos().x() - d->pressPos.x())/(d->slider.width() - d->cursor.width());
        qreal dx = f * d->sliderSize;

        d->setSliderPosition(d->sliderPosition + dx);
    } else {
        qreal f = (event->pos().y() - d->pressPos.y())/(d->slider.height() - d->cursor.height());
        qreal dy = f * d->sliderSize;

        d->setSliderPosition(d->sliderPosition + dy);
    }

    d->pressPos = event->pos();

    prepareGeometryChange();
    emit sliderPositionChange(d->sliderPosition);
}

void ScrollBar::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QGraphicsWidget::resizeEvent(event);
}

void ScrollBar::themeChange()
{
    Q_D(ScrollBar);
    d->themeChange();
}

