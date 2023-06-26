// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "model.h"

#include <QPixmap>

Model::Model(int rows, int columns, QObject *parent)
    : QAbstractItemModel(parent),
      services(QPixmap(":/images/services.png")),
      rc(rows),
      cc(columns),
      tree(new QList<Node>(rows, Node()))
{

}

Model::~Model()
{
    delete tree;
}

QModelIndex Model::index(int row, int column, const QModelIndex &parent) const
{
    if (row < rc && row >= 0 && column < cc && column >= 0) {
        Node *parentNode = static_cast<Node*>(parent.internalPointer());
        Node *childNode = node(row, parentNode);
        if (childNode)
            return createIndex(row, column, childNode);
    }
    return QModelIndex();
}

QModelIndex Model::parent(const QModelIndex &child) const
{
    if (child.isValid()) {
        Node *childNode = static_cast<Node*>(child.internalPointer());
        Node *parentNode = parent(childNode);
        if (parentNode)
            return createIndex(row(parentNode), 0, parentNode);
    }
    return QModelIndex();
}

int Model::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid() && parent.column() != 0) ? 0 : rc;
}

int Model::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return cc;
}

QVariant Model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole)
        return QVariant("Item " + QString::number(index.row()) + ':' + QString::number(index.column()));
    if (role == Qt::DecorationRole) {
        if (index.column() == 0)
            return iconProvider.icon(QFileIconProvider::Folder);
        return iconProvider.icon(QFileIconProvider::File);
    }
    return QVariant();
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
        return QString::number(section);
    if (role == Qt::DecorationRole)
        return QVariant::fromValue(services);
    return QAbstractItemModel::headerData(section, orientation, role);
}

bool Model::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return false;
    return rc > 0 && cc > 0;
}

Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    return Qt::ItemIsDragEnabled|QAbstractItemModel::flags(index);
}

Model::Node *Model::node(int row, Node *parent) const
{
    if (parent && !parent->children)
        parent->children = new QList<Node>(rc, Node(parent));
    QList<Node> *v = parent ? parent->children : tree;
    return const_cast<Node*>(&(v->at(row)));
}

Model::Node *Model::parent(Node *child) const
{
    return child ? child->parent : nullptr;
}

int Model::row(Node *node) const
{
    const Node *first = node->parent ? &(node->parent->children->at(0)) : &(tree->at(0));
    return node - first;
}
