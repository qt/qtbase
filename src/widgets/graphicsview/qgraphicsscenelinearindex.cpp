/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
