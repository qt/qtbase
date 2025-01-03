// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgraphicsgridlayoutengine_p.h"

#include "qgraphicslayoutitem_p.h"
#include "qgraphicslayout_p.h"
#include "qgraphicswidget.h"

QT_BEGIN_NAMESPACE

/*
  returns \c true if the size policy returns \c true for either hasHeightForWidth()
  or hasWidthForHeight()
 */
bool QGraphicsGridLayoutEngineItem::hasDynamicConstraint() const
{
    return QGraphicsLayoutItemPrivate::get(q_layoutItem)->hasHeightForWidth()
        || QGraphicsLayoutItemPrivate::get(q_layoutItem)->hasWidthForHeight();
}

Qt::Orientation QGraphicsGridLayoutEngineItem::dynamicConstraintOrientation() const
{
    if (QGraphicsLayoutItemPrivate::get(q_layoutItem)->hasHeightForWidth())
        return Qt::Vertical;
    else //if (QGraphicsLayoutItemPrivate::get(q_layoutItem)->hasWidthForHeight())
        return Qt::Horizontal;
}

/*!
  \internal

  If this returns true, the layout will arrange just as if the item was never added to the layout.
  (Note that this shouldn't lead to a "double spacing" where the item was hidden)
*/
bool QGraphicsGridLayoutEngineItem::isEmpty() const
{
    return q_layoutItem->isEmpty();
}

void QGraphicsGridLayoutEngine::setAlignment(QGraphicsLayoutItem *graphicsLayoutItem, Qt::Alignment alignment)
{
    if (QGraphicsGridLayoutEngineItem *gridEngineItem = findLayoutItem(graphicsLayoutItem)) {
        gridEngineItem->setAlignment(alignment);
        invalidate();
    }
}

Qt::Alignment QGraphicsGridLayoutEngine::alignment(QGraphicsLayoutItem *graphicsLayoutItem) const
{
    if (QGraphicsGridLayoutEngineItem *gridEngineItem = findLayoutItem(graphicsLayoutItem))
        return gridEngineItem->alignment();
    return { };
}


void QGraphicsGridLayoutEngine::setStretchFactor(QGraphicsLayoutItem *layoutItem, int stretch,
                                         Qt::Orientation orientation)
{
    Q_ASSERT(stretch >= 0);

    if (QGraphicsGridLayoutEngineItem *item = findLayoutItem(layoutItem))
        item->setStretchFactor(stretch, orientation);
}

int QGraphicsGridLayoutEngine::stretchFactor(QGraphicsLayoutItem *layoutItem, Qt::Orientation orientation) const
{
    if (QGraphicsGridLayoutEngineItem *item = findLayoutItem(layoutItem))
        return item->stretchFactor(orientation);
    return 0;
}

QT_END_NAMESPACE
