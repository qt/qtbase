// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MODEL_H
#define MODEL_H

#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QIcon>
#include <QList>

class Model : public QAbstractItemModel
{
    Q_OBJECT

public:
    Model(int rows, int columns, QObject *parent = nullptr);
    ~Model();

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool hasChildren(const QModelIndex &parent) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:

    struct Node
    {
        Node(Node *parent = nullptr) : parent(parent), children(nullptr) {}
        ~Node() { delete children; }
        Node *parent;
        QList<Node> *children;
    };

    Node *node(int row, Node *parent) const;
    Node *parent(Node *child) const;
    int row(Node *node) const;

    QIcon services;
    int rc;
    int cc;
    QList<Node> *tree;
    QFileIconProvider iconProvider;
};

#endif // MODEL_H
