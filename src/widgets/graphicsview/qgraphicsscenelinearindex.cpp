// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*!
    \class QGraphicsSceneLinearIndex
    \brief The QGraphicsSceneLinearIndex class provides an implementation of
    a linear indexing algorithm for discovering items in QGraphicsScene.
    \since 4.6
    \ingroup graphicsview-api
    \internal

    QGraphicsSceneLinearIndex index is default linear implementation to discover items.
    It basically store all items in a list and return them to the scene.

    \sa QGraphicsScene, QGraphicsView, QGraphicsSceneIndex, QGraphicsSceneBspTreeIndex
*/

#include <private/qgraphicsscenelinearindex_p.h>

/*!
    \fn QGraphicsSceneLinearIndex::QGraphicsSceneLinearIndex(QGraphicsScene *scene = nullptr):

    Construct a linear index for the given \a scene.
*/

/*!
    \fn QList<QGraphicsItem *> QGraphicsSceneLinearIndex::items(Qt::SortOrder order = Qt::DescendingOrder) const;

    Return all items in the index and sort them using \a order.
*/


/*!
    \fn virtual QList<QGraphicsItem *> QGraphicsSceneLinearIndex::estimateItems(const QRectF &rect, Qt::SortOrder order) const

    Returns an estimation visible items that are either inside or
    intersect with the specified \a rect and return a list sorted using \a order.
*/

/*!
    \fn void QGraphicsSceneLinearIndex::clear()
    \internal
    Clear the all the BSP index.
*/

/*!
    \fn virtual void QGraphicsSceneLinearIndex::addItem(QGraphicsItem *item)

    Add the \a item into the index.
*/

/*!
    \fn virtual void QGraphicsSceneLinearIndex::removeItem(QGraphicsItem *item)

    Add the \a item from the index.
*/

#include "moc_qgraphicsscenelinearindex_p.cpp"
