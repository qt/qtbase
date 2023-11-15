// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
    treemodel.cpp

    Provides a simple tree model to show how to create and use hierarchical
    models.
*/

#include "treemodel.h"
#include "treeitem.h"

#include <QStringList>

using namespace Qt::StringLiterals;

//! [0]
TreeModel::TreeModel(const QString &data, QObject *parent)
    : QAbstractItemModel(parent)
    , rootItem(std::make_unique<TreeItem>(QVariantList{tr("Title"), tr("Summary")}))
{
    setupModelData(QStringView{data}.split(u'\n'), rootItem.get());
}
//! [0]

//! [1]
TreeModel::~TreeModel() = default;
//! [1]

//! [2]
int TreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    return rootItem->columnCount();
}
//! [2]

//! [3]
QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};

    const auto *item = static_cast<const TreeItem*>(index.internalPointer());
    return item->data(index.column());
}
//! [3]

//! [4]
Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    return index.isValid()
        ? QAbstractItemModel::flags(index) : Qt::ItemFlags(Qt::NoItemFlags);
}
//! [4]

//! [5]
QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    return orientation == Qt::Horizontal && role == Qt::DisplayRole
        ? rootItem->data(section) : QVariant{};
}
//! [5]

//! [6]
QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    TreeItem *parentItem = parent.isValid()
        ? static_cast<TreeItem*>(parent.internalPointer())
        : rootItem.get();

    if (auto *childItem = parentItem->child(row))
        return createIndex(row, column, childItem);
    return {};
}
//! [6]

//! [7]
QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    auto *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    return parentItem != rootItem.get()
        ? createIndex(parentItem->row(), 0, parentItem) : QModelIndex{};
}
//! [7]

//! [8]
int TreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    const TreeItem *parentItem = parent.isValid()
        ? static_cast<const TreeItem*>(parent.internalPointer())
        : rootItem.get();

    return parentItem->childCount();
}
//! [8]

void TreeModel::setupModelData(const QList<QStringView> &lines, TreeItem *parent)
{
    struct ParentIndentation
    {
        TreeItem *parent;
        qsizetype indentation;
    };

    QList<ParentIndentation> state{{parent, 0}};

    for (const auto &line : lines) {
        qsizetype position = 0;
        for ( ; position < line.length() && line.at(position).isSpace(); ++position) {
        }

        const QStringView lineData = line.sliced(position).trimmed();
        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            const auto columnStrings = lineData.split(u'\t', Qt::SkipEmptyParts);
            QVariantList columnData;
            columnData.reserve(columnStrings.count());
            for (const auto &columnString : columnStrings)
                columnData << columnString.toString();

            if (position > state.constLast().indentation) {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.
                auto *lastParent = state.constLast().parent;
                if (lastParent->childCount() > 0)
                    state.append({lastParent->child(lastParent->childCount() - 1), position});
            } else {
                while (position < state.constLast().indentation && !state.isEmpty())
                    state.removeLast();
            }

            // Append a new item to the current parent's list of children.
            auto *lastParent = state.constLast().parent;
            lastParent->appendChild(std::make_unique<TreeItem>(columnData, lastParent));
        }
    }
}
