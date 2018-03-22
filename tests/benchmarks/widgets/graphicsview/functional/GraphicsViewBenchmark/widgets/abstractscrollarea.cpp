/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGraphicsSceneResizeEvent>
#include <QGraphicsWidget>
#include <QDebug>
#include "abstractscrollarea.h"
#include "scrollbar.h"

AbstractScrollArea::AbstractScrollArea(QGraphicsWidget *parent)
    : GvbWidget(parent)
    , m_viewport(0)
    , m_horizontalScrollBar(0)
    , m_verticalScrollBar(0)
    , m_prevHorizontalValue(0.0)
    , m_prevVerticalValue(0.0)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(0, 0, 0, 0);

    m_horizontalScrollBar = new ScrollBar(Qt::Horizontal, this);
    m_horizontalScrollBar->hide();
    m_horizontalScrollBar->setContentsMargins(0, 0, 0, 0);
    m_horizontalScrollBarPolicy = Qt::ScrollBarAsNeeded;
    m_horizontalScrollBar->setZValue(zValue()+1); // Raise scroll bar to top
    m_horizontalScrollBar->setVisible(false);

    connect(m_horizontalScrollBar, SIGNAL(sliderPositionChange(qreal)),
            this, SLOT(horizontalScroll(qreal)));
    connect(m_horizontalScrollBar, SIGNAL(sliderPressed()),
            this, SLOT(horizontalScrollStart()));

    m_verticalScrollBar = new ScrollBar(Qt::Vertical, this);
    m_verticalScrollBar->hide();
    m_verticalScrollBar->setContentsMargins(0, 0, 0, 0);
    m_verticalScrollBarPolicy = Qt::ScrollBarAsNeeded;
    m_verticalScrollBar->setZValue(zValue()+1); // Raise scroll bar to top
    m_verticalScrollBar->setVisible(false);

    connect(m_verticalScrollBar, SIGNAL(sliderPositionChange(qreal)),
            this, SLOT(verticalScroll(qreal)));
    connect(m_verticalScrollBar, SIGNAL(sliderPressed()),
            this, SLOT(verticalScrollStart()));

    QGraphicsWidget *viewport = new QGraphicsWidget;
    setViewport(viewport);
}

AbstractScrollArea::~AbstractScrollArea()
{
}

ScrollBar *AbstractScrollArea::verticalScrollBar() const
{
    return m_verticalScrollBar;
}

ScrollBar *AbstractScrollArea::horizontalScrollBar() const
{
    return m_horizontalScrollBar;
}

void AbstractScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy policy)
{
    m_horizontalScrollBarPolicy = policy;
}

void AbstractScrollArea::setVerticalScrollBarPolicy(Qt::ScrollBarPolicy policy)
{
    m_verticalScrollBarPolicy = policy;
}

Qt::ScrollBarPolicy AbstractScrollArea::verticalScrollBarPolicy() const
{
    return m_verticalScrollBarPolicy;
}

Qt::ScrollBarPolicy AbstractScrollArea::horizontalScrollBarPolicy() const
{
    return m_horizontalScrollBarPolicy;
}

QGraphicsWidget *AbstractScrollArea::viewport() const
{
    return m_viewport;
}

void AbstractScrollArea::setViewport(QGraphicsWidget *viewport)
{
    if (m_viewport) {
        m_viewport->setParentItem(0);

        QList<QGraphicsItem*> children = m_viewport->childItems();

        foreach (QGraphicsItem *child, children)
            child->setParentItem(0);

        delete m_viewport;
    }

    m_viewport = viewport;

    if (viewport) {

        m_viewport->setParentItem(this);
        m_viewport->setContentsMargins(0, 0, 0, 0);

        adjustScrollBars();
    }

    emit viewportChanged(viewport);
}

bool AbstractScrollArea::event(QEvent *e)
{
    if (e->type() == QEvent::ApplicationLayoutDirectionChange
       || e->type() == QEvent::LayoutDirectionChange) {
    } else if (e->type() == QEvent::GraphicsSceneResize) {
        QGraphicsSceneResizeEvent *event =
            static_cast<QGraphicsSceneResizeEvent*>(e);

        QSizeF newSize = event->newSize();
        QRectF hrect = m_horizontalScrollBar->boundingRect();
        QRectF vrect = m_verticalScrollBar->boundingRect();

        QSizeF vpSize = newSize;

        if (m_horizontalScrollBarPolicy != Qt::ScrollBarAlwaysOff)
            vpSize.setHeight(newSize.height() - hrect.height());
        if (m_verticalScrollBarPolicy != Qt::ScrollBarAlwaysOff)
            vpSize.setWidth(newSize.width() - vrect.width());

        m_viewport->resize(vpSize);

        adjustScrollBars();
    }

    return QGraphicsWidget::event(e);
}


void AbstractScrollArea::scrollContentsBy(qreal dx, qreal dy)
{
    Q_UNUSED(dx)
    Q_UNUSED(dy)
    prepareGeometryChange();
}

void AbstractScrollArea::verticalScrollStart()
{
    m_prevVerticalValue = m_verticalScrollBar->sliderPosition();
}

void AbstractScrollArea::verticalScroll(qreal value)
{
    qreal dy = value - m_prevVerticalValue;
    if (!qFuzzyCompare(dy,qreal(0.0))) {
        scrollContentsBy(0.0, dy);
        m_prevVerticalValue = value;
    }
}

void AbstractScrollArea::horizontalScrollStart()
{
    m_prevHorizontalValue = m_horizontalScrollBar->sliderPosition();
}

void AbstractScrollArea::horizontalScroll(qreal value)
{
    qreal dx = value - m_prevHorizontalValue;
    if (!qFuzzyCompare(dx,qreal(0.0))) {
        scrollContentsBy(dx, 0.0);
        m_prevHorizontalValue = value;
    }
}

void AbstractScrollArea::adjustScrollBars()
{
    if (m_horizontalScrollBarPolicy == Qt::ScrollBarAlwaysOff) {
        m_horizontalScrollBar->hide();
    } else {
        m_horizontalScrollBar->show();

        QRectF sbgeom = boundingRect();

        sbgeom.setTop(sbgeom.bottom() - m_horizontalScrollBar->boundingRect().height());
        sbgeom.setRight(sbgeom.right() - m_verticalScrollBar->boundingRect().width());
        m_horizontalScrollBar->setGeometry(sbgeom);
    }

    if (m_verticalScrollBarPolicy == Qt::ScrollBarAlwaysOff) {
        m_verticalScrollBar->hide();
        QRectF sbgeom = boundingRect();
        sbgeom.setLeft(sbgeom.right());
        sbgeom.setBottom(sbgeom.bottom());
        m_verticalScrollBar->setGeometry(sbgeom);
    } else {
        m_verticalScrollBar->show();

        QRectF sbgeom = boundingRect();

        sbgeom.setLeft(sbgeom.right() - m_verticalScrollBar->boundingRect().width());
        if (m_horizontalScrollBarPolicy != Qt::ScrollBarAlwaysOff)
            sbgeom.setBottom(sbgeom.bottom() - m_horizontalScrollBar->boundingRect().height());
        m_verticalScrollBar->setGeometry(sbgeom);
    }
}




