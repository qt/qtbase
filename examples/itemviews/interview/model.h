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

#ifndef MODEL_H
#define MODEL_H

#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QIcon>
#include <QVector>

class Model : public QAbstractItemModel
{
    Q_OBJECT

public:
    Model(int rows, int columns, QObject *parent = 0);
    ~Model();

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    bool hasChildren(const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

private:

    struct Node
    {
	Node(Node *parent = 0) : parent(parent), children(0) {}
	~Node() { delete children; }
	Node *parent;
	QVector<Node> *children;
    };

    Node *node(int row, Node *parent) const;
    Node *parent(Node *child) const;
    int row(Node *node) const;

    QIcon services;
    int rc, cc;
    QVector<Node> *tree;
    QFileIconProvider iconProvider;
};

#endif
