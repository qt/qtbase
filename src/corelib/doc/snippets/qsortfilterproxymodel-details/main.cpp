// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <QApplication>
#include <QSortFilterProxyModel>

class MyItemModel : public QStandardItemModel
{
public:
    MyItemModel(QWidget *parent = nullptr);
};

MyItemModel::MyItemModel(QWidget *parent)
    : QStandardItemModel(parent)
{};

class Widget : public QWidget
{
public:
    Widget(QWidget *parent = nullptr);
};

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
//! [0] //! [1]
        QTreeView *treeView = new QTreeView;
//! [0]
        MyItemModel *model = new MyItemModel(this);

        treeView->setModel(model);
//! [1]

//! [2]
        MyItemModel *sourceModel = new MyItemModel(this);
        QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);

        proxyModel->setSourceModel(sourceModel);
        treeView->setModel(proxyModel);
//! [2]

//! [3]
        treeView->setSortingEnabled(true);
//! [3]

//! [4]
        proxyModel->sort(2, Qt::AscendingOrder);
//! [4] //! [5]
        proxyModel->setFilterRegularExpression(QRegularExpression("\.png", QRegularExpression::CaseInsensitiveOption));
        proxyModel->setFilterKeyColumn(1);
//! [5]
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Widget widget;
    widget.show();
    return app.exec();
}
