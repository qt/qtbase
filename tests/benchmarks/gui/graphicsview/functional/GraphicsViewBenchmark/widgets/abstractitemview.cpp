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
#include <QGraphicsLayout>

#include "abstractitemview.h"
#include "abstractviewitem.h"
#include "scrollbar.h"

AbstractItemView::AbstractItemView(QGraphicsWidget *parent)
    : AbstractScrollArea(parent),
    m_model(0),
    m_rootIndex(),
    m_container(0),
    m_selectionModel(0),
    m_currentIndex(),
    m_scroller()
{
    setRootIndex(QModelIndex());
}

/*virtual*/
AbstractItemView::~AbstractItemView()
{
}

/*virtual*/
void AbstractItemView::setModel(QAbstractItemModel *model, AbstractViewItem *prototype)
{
    if (m_model == model || !model)
        return;

    if (m_model) {
        disconnect(m_model, SIGNAL(destroyed()),
                   this, SLOT(_q_modelDestroyed()));
        disconnect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        disconnect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SLOT(rowsInserted(QModelIndex,int,int)));
        disconnect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(rowsRemoved(QModelIndex,int,int)));
        disconnect(m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                   this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
        disconnect(m_model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                   this, SLOT(rowsAboutToBeInserted(QModelIndex,int,int)));
        disconnect(m_model, SIGNAL(columnsInserted(QModelIndex,int,int)),
                   this, SLOT(columnsInserted(QModelIndex,int,int)));
        disconnect(m_model, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                   this, SLOT(columnsAboutToBeInserted(QModelIndex,int,int)));
        disconnect(m_model, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                   this, SLOT(columnsRemoved(QModelIndex,int,int)));
        disconnect(m_model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                   this, SLOT(columnsAboutToBeRemoved(QModelIndex,int,int)));
        disconnect(m_model, SIGNAL(modelReset()), this, SLOT(reset()));
        disconnect(m_model, SIGNAL(layoutChanged()), this, SLOT(_q_layoutChanged()));

        m_model = 0;
    }

    setSelectionModel(0);

    m_currentIndex = QModelIndex();
    m_rootIndex = QModelIndex();

    m_model = model;

    Q_ASSERT_X(m_model->index(0,0) == m_model->index(0,0),
               "AbstractItemView::setModel",
               "A model should return the exact same index "
               "(including its internal id/pointer) when asked for it twice in a row.");
    Q_ASSERT_X(m_model->index(0,0).parent() == QModelIndex(),
               "AbstractItemView::setModel",
               "The parent of a top level index should be invalid");


    connect(m_model, SIGNAL(destroyed()), this, SLOT(modelDestroyed()));
    connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    connect(m_model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
            this, SLOT(rowsAboutToBeInserted(QModelIndex,int,int)));
    connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(rowsInserted(QModelIndex,int,int)));
    connect(m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
    connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(rowsRemoved(QModelIndex,int,int)));
    connect(m_model, SIGNAL(modelReset()), this, SLOT(reset()));
    connect(m_model, SIGNAL(layoutChanged()), this, SLOT(layoutChanged()));

    setSelectionModel(new QItemSelectionModel(m_model));

    if (prototype && m_container) {
        m_container->setItemPrototype(prototype);
        m_container->reset();
    }
}

/*virtual*/
void AbstractItemView::setContainer(AbstractItemContainer *container)
{
    m_container = container;
    m_container->setItemView(this);
    m_container->setParentItem(viewport());

    viewport()->setFlag(
            QGraphicsItem::ItemClipsChildrenToShape, true);
    m_scroller.setScrollable(this);
    installEventFilter(&m_scroller);
}

/*virtual*/
void AbstractItemView::setRootIndex(const QModelIndex &index)
{
    m_rootIndex = index;
    // TODO fix this if we change index, container should be updated? Or do we need root index?
}

/*virtual*/
int AbstractItemView::indexCount() const
{
    if (m_model)
        return m_model->rowCount(m_rootIndex);
    return 0;
}

/*virtual*/
QAbstractItemModel* AbstractItemView::model() const
{
    return m_model;
}

/*virtual*/
QModelIndex AbstractItemView::nextIndex(const QModelIndex &index) const
{
    if (!m_model)
        return QModelIndex();

    if (index.isValid())
        return m_model->index(index.row() + 1, 0, m_rootIndex);
    else
        return m_model->index(0, 0, m_rootIndex);
}

/*virtual*/
QModelIndex AbstractItemView::previousIndex(const QModelIndex &index) const
{
    if (!m_model)
        return QModelIndex();

    if (index.isValid())
        return m_model->index(index.row() - 1, 0, m_rootIndex);
    else
        return m_model->index(m_model->rowCount(m_rootIndex) - 1, 0, m_rootIndex);
}

/*virtual*/
void AbstractItemView::setItemPrototype(AbstractViewItem* prototype)
{
    if (prototype && m_container) {
        m_container->setItemPrototype(prototype);
        m_container->reset();
    }
}

void AbstractItemView::setSelectionModel(QItemSelectionModel *smodel)
{
    if (smodel && smodel->model() != m_model) {
        return;
    }
    if (m_selectionModel) {
        disconnect(m_selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                   this, SLOT(currentSelectionChanged(QItemSelection,QItemSelection)));

        disconnect(m_selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                   this, SLOT(currentIndexChanged(QModelIndex,QModelIndex)));

        delete m_selectionModel;
        m_selectionModel = 0;
    }

    m_selectionModel = smodel;

    if (m_selectionModel) {
        connect(m_selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(currentSelectionChanged(QItemSelection,QItemSelection)));
        connect(m_selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(currentIndexChanged(QModelIndex,QModelIndex)));
    }
}

/*virtual*/
void AbstractItemView::currentIndexChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)

    if (current != m_currentIndex)
        m_currentIndex = current;
}

/*virtual*/
void AbstractItemView::currentSelectionChanged(const QItemSelection &selected,
                                               const QItemSelection &deselected)
{
   Q_UNUSED(selected)
   Q_UNUSED(deselected)
}

/*virtual*/
void AbstractItemView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    // TODO implement if we like to edit view items.
}

/*virtual*/
void AbstractItemView::rowsAboutToBeInserted(const QModelIndex &index, int start, int end)
{
    Q_UNUSED(index)
    Q_UNUSED(start)
    Q_UNUSED(end)

    // TODO implement
}

/*virtual*/
void AbstractItemView::rowsAboutToBeRemoved(const QModelIndex &index,int start, int end)
{
    Q_UNUSED(index)
    Q_UNUSED(start)
    Q_UNUSED(end)
}

/*virtual*/
void AbstractItemView::rowsRemoved(const QModelIndex &parent,int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    if (start <= m_currentIndex.row() && m_currentIndex.row() <= end) {
        QModelIndex newCurrentIndex = m_model->index(start, 0, m_rootIndex);
        if (!newCurrentIndex.isValid()) {
            newCurrentIndex = m_model->index(qMax(0,start - 1), 0, m_rootIndex);
        }

        if (m_selectionModel) {
            m_selectionModel->setCurrentIndex(newCurrentIndex, QItemSelectionModel::NoUpdate);
        }
    }
    for (int i = end; i >= start; --i) //The items are already removed from the model.
        m_container->removeItem(QModelIndex()); // Indexes are already invalid.
}

/*virtual*/
void AbstractItemView::reset()
{
    m_rootIndex = QModelIndex();

    if (m_container)
        m_container->reset();

    setCurrentIndex(QModelIndex());

    ScrollBar *sb = verticalScrollBar();

    if (sb)
        sb->setSliderSize(0);
}

/*virtual*/
void AbstractItemView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if (!m_container)
        return;

    for (int i = start; i <= end; ++i)
        m_container->addItem(m_model->index(i, 0, parent));

    refreshContainerGeometry();
}

/*virtual*/
void AbstractItemView::modelDestroyed()
{
    m_model = 0;
    setSelectionModel(0);
    reset();
}

/*virtual*/
void AbstractItemView::layoutChanged()
{
    m_container->reset();
}

bool AbstractItemView::event(QEvent *e)
{
    bool result = AbstractScrollArea::event(e);
    if (e && e->type()==QEvent::LayoutRequest) {
        refreshContainerGeometry();
        result = true;
    }
    if (e && e->type()==QEvent::GraphicsSceneResize) {
        m_scroller.stopScrolling();
        refreshContainerGeometry();

        m_container->resize(this->size().width()-verticalScrollBar()->size().width(),
                            m_container->preferredHeight());

        if (verticalScrollBar()->sliderPosition() > verticalScrollBar()->sliderSize())
            verticalScrollBar()->setSliderPosition(verticalScrollBar()->sliderSize());

        result = true;
    }
    return result;
}

void AbstractItemView::setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags selectionFlag)
{
    if (m_selectionModel)
        m_selectionModel->setCurrentIndex(index, selectionFlag);
}

void AbstractItemView::refreshContainerGeometry()
{
    if (!m_container || !m_model)
        return;

    if (m_container->layout() && !m_container->layout()->isActivated())
        m_container->layout()->activate();

    ScrollBar *sb = verticalScrollBar();

    if (sb) {
        AbstractViewItem *item = m_container->itemAt(0);
        if (item) {
            qreal oneItemH = item->size().height();
            sb->setSliderSize(oneItemH*m_model->rowCount(m_rootIndex)-size().height());
        }
        if (!sb->isVisible() && verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff &&
            contentsRect().height() < m_container->boundingRect().height())
            sb->show();
    }
}

void AbstractItemView::scrollContentsBy(qreal dx, qreal dy)
{
    AbstractScrollArea::scrollContentsBy(dx, dy);

    if (!viewport() || !m_container || (m_container && m_container->itemCount() <= 0) ||
        !m_model || (m_model && m_model->rowCount() <= 0) ||
        (viewport() && viewport()->boundingRect().height() < contentsRect().height()))
        return;

    qreal itemH = 1;

    AbstractViewItem *item = m_container->itemAt(0);
    if (item && item->size().height() > 1) {
        itemH = item->size().height();
    } else if (item && item->preferredHeight() > 1) {
        itemH = item->preferredHeight();
    }

    qreal vpx = m_container->pos().x();
    qreal vpy = m_container->pos().y();

    if ((vpy+m_container->size().height()-dy > pos().y()+size().height()) &&
        (qAbs(dy) < itemH) && (vpy-dy <= 0)) {
        m_container->setPos(vpx, vpy-dy);
    } else {
        qreal vPos = verticalScrollBar()->sliderPosition();
        int startRow =  m_model->index(vPos/itemH, 0).row();
        int itemsInContainer = m_container->itemCount();

        for (int i = 0; i<itemsInContainer; ++i) {
            AbstractViewItem *changedItem = m_container->itemAt(i);
            changedItem->setModelIndex(m_model->index(startRow+i,0));
            m_container->setListItemCaching(listItemCaching(), i);
        }

        qreal diff = vPos-startRow*itemH;

        if (diff < 0)
            m_container->setPos(vpx, diff);
        else
            m_container->setPos(vpx, -diff);
    }
}

void AbstractItemView::changeTheme()
{
    if (m_container)
        m_container->themeChange();
}

void AbstractItemView::updateViewContent()
{
    if (m_container)
        m_container->updateContent();
}
