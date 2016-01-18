/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
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
        return 0;
    return Qt::ItemIsDragEnabled|QAbstractItemModel::flags(index);
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
    return node - first;
}
