/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "model.h"
#include <QIcon>
#include <QPixmap>

Model::Model(int rows, int columns, QObject *parent)
    : QAbstractItemModel(parent),
      services(QPixmap(":/images/services.png")),
      rc(rows), cc(columns),
      tree(new QVector<Node>(rows, Node(0)))
{

}

Model::~Model()
{
    delete tree;
}

QModelIndex Model::index(int row, int column, const QModelIndex &parent) const
{
    if (row < rc && row >= 0 && column < cc && column >= 0) {
        Node *p = static_cast<Node*>(parent.internalPointer());
        Node *n = node(row, p);
	if (n)
	    return createIndex(row, column, n);
    }
    return QModelIndex();
}

QModelIndex Model::parent(const QModelIndex &child) const
{
    if (child.isValid()) {
        Node *n = static_cast<Node*>(child.internalPointer());
        Node *p = parent(n);
        if (p)
            return createIndex(row(p), 0, p);
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
	return "Item " + QString::number(index.row()) + ":" + QString::number(index.column());
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
        return 0;
    return (Qt::ItemIsDragEnabled|Qt::ItemIsSelectable|Qt::ItemIsEnabled);
}

Model::Node *Model::node(int row, Node *parent) const
{
    if (parent && !parent->children)
	parent->children = new QVector<Node>(rc, Node(parent));
    QVector<Node> *v = parent ? parent->children : tree;
    return const_cast<Node*>(&(v->at(row)));
}

Model::Node *Model::parent(Node *child) const
{
    return child ? child->parent : 0;
}

int Model::row(Node *node) const
{
     const Node *first = node->parent ? &(node->parent->children->at(0)) : &(tree->at(0));
     return (node - first);
}
