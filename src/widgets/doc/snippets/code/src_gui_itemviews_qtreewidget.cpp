// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <qtreewidget.h>

class TreeMainWindow :public QWidget // Provide tr()
{
public:
    TreeMainWindow()
    {
        //! [0]
        auto *treeWidget = new QTreeWidget();
        treeWidget->setColumnCount(1);
        for (int i = 0; i < 10; ++i)
            new QTreeWidgetItem(treeWidget, { tr("item: %1").arg(i) });
        //! [0]
    }
};
