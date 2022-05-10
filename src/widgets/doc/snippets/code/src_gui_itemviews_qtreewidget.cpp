// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QTreeWidget *treeWidget = new QTreeWidget();
treeWidget->setColumnCount(1);
QList<QTreeWidgetItem *> items;
for (int i = 0; i < 10; ++i)
    items.append(new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList(QString("item: %1").arg(i))));
treeWidget->insertTopLevelItems(0, items);
//! [0]
