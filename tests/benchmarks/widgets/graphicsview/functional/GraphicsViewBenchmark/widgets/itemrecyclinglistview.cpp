// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "itemrecyclinglistview.h"

ItemRecyclingListView::ItemRecyclingListView(QGraphicsWidget * parent)
    : AbstractItemView(parent), m_rootIndex()
{
}

/*virtual*/
ItemRecyclingListView::~ItemRecyclingListView()
{
}
void ItemRecyclingListView::setCurrentRow(const int row)
{
    setCurrentIndex(model()->index(row,0));
}

int ItemRecyclingListView::rows() const
{
    if (m_model)
        return m_model->rowCount();
    return 0;
}

/*virtual*/
void ItemRecyclingListView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if (parent == m_rootIndex) {
        AbstractItemView::rowsInserted(parent, start, end);
    }
}

/*virtual*/
void ItemRecyclingListView::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    if (parent == m_rootIndex) {
        AbstractItemView::rowsRemoved(parent, start, end);
    }
}
