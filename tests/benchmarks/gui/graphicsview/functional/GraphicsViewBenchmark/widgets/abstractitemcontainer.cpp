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
#include <QDebug>
#include <QGraphicsLayout>

#include "abstractitemcontainer.h"
#include "abstractitemview.h"
#include "abstractviewitem.h"
#include "scrollbar.h"

AbstractItemContainer::AbstractItemContainer(int bufferSize, QGraphicsWidget *parent)
    : GvbWidget(parent),
    m_items(),
    m_itemView(0),
    m_prototype(0),
    m_bufferSize(bufferSize),
    m_twoColumns(false)
{
}

AbstractItemContainer::~AbstractItemContainer()
{
    delete m_prototype;
    m_prototype = 0;
}

AbstractViewItem *AbstractItemContainer::prototype()
{
    return m_prototype;
}

int AbstractItemContainer::bufferSize() const
{
    return m_bufferSize;
}

bool AbstractItemContainer::event(QEvent *e)
{
    if (e->type() == QEvent::LayoutRequest)
        updateItemBuffer();

    return QGraphicsWidget::event(e);
}


bool AbstractItemContainer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type()==QEvent::GraphicsSceneResize && m_itemView) {
        const bool caching = m_itemView->listItemCaching();
        m_itemView->setListItemCaching(false);

        QSizeF s = m_itemView->size();
        s.setWidth(s.width()-m_itemView->verticalScrollBar()->size().width());
        adjustVisibleContainerSize(s);

        m_itemView->updateViewContent();
        updateItemBuffer();

        m_itemView->setListItemCaching(caching);
    }

    return QGraphicsWidget::eventFilter(obj, event);
}

QVariant AbstractItemContainer::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant ichange = QGraphicsWidget::itemChange(change,value);

    if (change == ItemPositionChange) {
        if (m_itemView && layout() && !layout()->isActivated())
            m_itemView->refreshContainerGeometry();
    }
    return ichange;
 }

/*virtual*/
void AbstractItemContainer::setItemView(AbstractItemView *view)
{
    m_itemView = view;

    if (m_itemView) {
        setParentItem(m_itemView);
        m_itemView->installEventFilter(this);
    }
}
/*virtual*/
void AbstractItemContainer::setItemPrototype(AbstractViewItem *ptype)
{
    m_prototype = ptype;
    m_prototype->setParentItem(0);
    m_prototype->hide();
}

/*virtual*/
void AbstractItemContainer::reset()
{
    qDeleteAll(m_items);
    m_items.clear();
    updateItemBuffer();
}


/*virtual*/
void AbstractItemContainer::addItem(const QModelIndex &index)
{
    if (m_items.count() < maxItemCountInItemBuffer() ||
        (m_items.count() > 0 &&
            m_items.first()->modelIndex().row()-1 <= index.row() &&
            m_items.last()->modelIndex().row() >= index.row())) {
        int itemPos = 0;
        if (m_items.count() != 0)
            itemPos = qMax(0, index.row() - m_items.first()->modelIndex().row());

        if (itemPos >= m_items.count() || m_items.at(itemPos)->modelIndex() != index) {
            AbstractViewItem *item = 0;
            if (m_prototype)
                item = m_prototype->newItemInstance();

            if (item) {
                item->setModel(m_itemView->model());
                item->setTwoColumns(m_twoColumns);
                m_items.insert(itemPos, item);
                addItemToVisibleLayout(itemPos, item);

                if (item->modelIndex() != index) {
                    item->setModelIndex(index);
                }
            }
        }
        updateItemBuffer();
    }
}
void AbstractItemContainer::removeItem(const QModelIndex &index)
{
    AbstractViewItem *item = findItemByIndex(index);

    if (item) {
        if (maxItemCountInItemBuffer() < m_items.count()) {
            m_items.removeOne(item);
            removeItemFromVisibleLayout(item);

            delete item;
        }
        else {
            m_items.removeOne(item);
            removeItemFromVisibleLayout(item);

            QModelIndex newIndex = m_itemView->nextIndex(m_items.last()->modelIndex());
            if (newIndex.isValid()) {
                // Item readded as last item in buffer.
                m_items.append(item);
                addItemToVisibleLayout(m_items.count() - 1, item);
                item->setModelIndex(newIndex);
            } else {
                // Item readded as first item in buffer.
                newIndex = m_itemView->previousIndex(m_items.first()->modelIndex());

                m_items.prepend(item);
                addItemToVisibleLayout(0, item);
                item->setModelIndex(newIndex);
            }
        }
    }
}

/*virtual*/
int AbstractItemContainer::itemCount() const
{
    return m_items.count();
}

AbstractViewItem *AbstractItemContainer::firstItem()
{
    return m_items.first();
}

/*virtual*/
AbstractViewItem* AbstractItemContainer::itemAt(const int row) const
{
    if (row<0 || row >= m_items.count())
        return 0;
    return m_items.at(row);
}

AbstractViewItem* AbstractItemContainer::findItemByIndex(const QModelIndex &index) const
{
    AbstractViewItem *item = 0;
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items.at(i)->modelIndex() == index) {
            item = m_items.at(i);
            break;
        }
    }
    return item;
}

bool AbstractItemContainer::itemVisibleInView(AbstractViewItem* item, const QRectF &viewRect, bool fullyVisible) const
{
    if (!item || !m_itemView)
        return false;

    QRectF itemRectBoundingRect = item->mapToItem(m_itemView, item->boundingRect()).boundingRect();

    if (fullyVisible && viewRect.contains(itemRectBoundingRect))
        return true;
    else if (viewRect.intersects(itemRectBoundingRect))
        return true;

    return false;
}

void AbstractItemContainer::updateItemBuffer()
{
    if (!m_itemView || (m_itemView && !m_itemView->boundingRect().isValid()))
        return;

    int maxCount = maxItemCountInItemBuffer();

    if (m_items.count() < maxCount) {
        // New items needs to be added.
        QModelIndex index;
        if (m_items.count() > 0)
            index = m_items.last()->modelIndex();
        while (m_items.count() < maxCount) {
            index = m_itemView->nextIndex(index);

            if (!index.isValid())
                break;

            insertItem(m_items.count(), index);
        }

        index = m_items.first()->modelIndex();
        while (m_items.count() < maxCount) {
            index = m_itemView->previousIndex(index);

            if (!index.isValid())
                break;

            insertItem(0, index);
        }
    }

    QRectF viewRect = boundingRect();

    while (m_items.count() > maxCount) {
        int firstVisible = 0;
        int lastVisible = 0;
        findFirstAndLastVisibleBufferIndex(firstVisible, lastVisible, viewRect, false);

        AbstractViewItem* item = 0;
        if (lastVisible != m_items.count() - 1) {
            item = m_items.takeLast();
        }
        else if (firstVisible != 0 && m_items.first()->modelIndex().row() != firstVisible-1) {
            item = m_items.takeFirst();
        }
        else {
            // All the items are visible. Take the item at the end of the buffer.
            item = m_items.takeLast();
        }

        m_items.removeOne(item);
        removeItemFromVisibleLayout(item);
        delete item;
    }
}

void AbstractItemContainer::insertItem(int pos, const QModelIndex &index)
{
    AbstractViewItem *item = 0;
     if (m_prototype)
        item = m_prototype->newItemInstance();

    if (item) {
        item->setModel(m_itemView->model());
        item->setModelIndex(index);
        item->setTwoColumns(m_twoColumns);
        m_items.insert(pos, item);
        addItemToVisibleLayout(pos, item);
        item->updateItemContents();
        if (pos == 0)
            m_itemView->scrollContentsBy(qreal(0.0),
                                        item->effectiveSizeHint(Qt::PreferredSize).height());
    }
}

void AbstractItemContainer::findFirstAndLastVisibleBufferIndex(int &firstVisibleBufferIndex,
                                                               int &lastVisibleBufferIndex,
                                                               const QRectF &viewRect,
                                                               bool fullyVisible) const
{
    if (layout() && !layout()->isActivated())
        layout()->activate();

    firstVisibleBufferIndex = -1;
    lastVisibleBufferIndex = -1;

    int count = m_items.count();
    for (int i = 0; i < count; ++i) {
        if (itemVisibleInView(m_items.at(i), viewRect, fullyVisible)) {
            if (firstVisibleBufferIndex == -1)
                firstVisibleBufferIndex = i;
            lastVisibleBufferIndex = i;
        }
        else if ( lastVisibleBufferIndex != -1 )
            break; // lastVisibleBufferIndex is already set
    }
}

/*virtual*/
int AbstractItemContainer::maxItemCountInItemBuffer() const
{
    if (m_itemView && !m_itemView->boundingRect().isEmpty())
    {
        return m_itemView->indexCount();
    }
    return 0;
}


void AbstractItemContainer::themeChange()
{
    for (int i = 0; i <m_items.count(); ++i)
        m_items.at(i)->themeChange();
}

void AbstractItemContainer::updateContent()
{
    for (int i = 0; i <m_items.count(); ++i)
        m_items.at(i)->updateItemContents();
}

void AbstractItemContainer::setSubtreeCacheEnabled(bool enabled)
{
    for (int i = 0; i <m_items.count(); ++i)
        m_items.at(i)->setSubtreeCacheEnabled(enabled);
    if (m_prototype)
        m_prototype->setSubtreeCacheEnabled(enabled);
}

void AbstractItemContainer::setTwoColumns(const bool enabled)
{
    if (m_twoColumns == enabled)
        return;

    m_twoColumns = enabled;

    for (int i = 0; i < m_items.count(); ++i)
        m_items.at(i)->setTwoColumns(enabled);
}

bool AbstractItemContainer::twoColumns()
{
    return m_twoColumns;
}

