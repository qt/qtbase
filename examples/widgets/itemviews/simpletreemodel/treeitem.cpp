// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
    treeitem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include "treeitem.h"

//! [0]
TreeItem::TreeItem(QVariantList data, TreeItem *parent)
    : m_itemData(std::move(data)), m_parentItem(parent)
{}
//! [0]

//! [1]
void TreeItem::appendChild(std::unique_ptr<TreeItem> &&child)
{
    m_childItems.push_back(std::move(child));
}
//! [1]

//! [2]
TreeItem *TreeItem::child(int row)
{
    return row >= 0 && row < childCount() ? m_childItems.at(row).get() : nullptr;
}
//! [2]

//! [3]
int TreeItem::childCount() const
{
    return int(m_childItems.size());
}
//! [3]

//! [4]
int TreeItem::columnCount() const
{
    return int(m_itemData.count());
}
//! [4]

//! [5]
QVariant TreeItem::data(int column) const
{
    return m_itemData.value(column);
}
//! [5]

//! [6]
TreeItem *TreeItem::parentItem()
{
    return m_parentItem;
}
//! [6]

//! [7]
int TreeItem::row() const
{
    if (m_parentItem == nullptr)
        return 0;
    const auto it = std::find_if(m_parentItem->m_childItems.cbegin(), m_parentItem->m_childItems.cend(),
                                 [this](const std::unique_ptr<TreeItem> &treeItem) {
                                     return treeItem.get() == this;
                                 });

    if (it != m_parentItem->m_childItems.cend())
        return std::distance(m_parentItem->m_childItems.cbegin(), it);
    Q_ASSERT(false); // should not happen
    return -1;
}
//! [7]
