// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
    treeitem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include "treeitem.h"

//! [0]
TreeItem::TreeItem(const QVariantList &data, TreeItem *parent)
    : m_itemData(data), m_parentItem(parent)
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
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row).get();
}
//! [2]

//! [3]
int TreeItem::childCount() const
{
    return m_childItems.size();
}
//! [3]

//! [4]
int TreeItem::columnCount() const
{
    return m_itemData.count();
}
//! [4]

//! [5]
QVariant TreeItem::data(int column) const
{
    if (column < 0 || column >= m_itemData.size())
        return QVariant();
    return m_itemData.at(column);
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
    if (!m_parentItem)
        return 0;
    const auto it = std::find_if(m_parentItem->m_childItems.cbegin(), m_parentItem->m_childItems.cend(),
                                 [this](const std::unique_ptr<TreeItem> &treeItem) {
                                     return treeItem.get() == const_cast<TreeItem *>(this);
                                 });

    if (it != m_parentItem->m_childItems.cend())
        return std::distance(m_parentItem->m_childItems.cbegin(), it);
    Q_ASSERT(false); // should not happen
    return -1;
}
//! [7]
