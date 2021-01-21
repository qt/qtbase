/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
    \fn QGraphicsSceneLinearIndex::QGraphicsSceneLinearIndex(QGraphicsScene *scene = 0):

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
