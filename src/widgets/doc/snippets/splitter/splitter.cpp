// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>

void processSize(int)
{
}

int main()
{
    QWidget *parent = nullptr;

//! [0]
    QSplitter *splitter = new QSplitter(parent);
    QListView *listview = new QListView;
    QTreeView *treeview = new QTreeView;
    QTextEdit *textedit = new QTextEdit;
    splitter->addWidget(listview);
    splitter->addWidget(treeview);
    splitter->addWidget(textedit);
//! [0]

    {
    // SAVE STATE
//! [1]
    QSettings settings;
    settings.setValue("splitterSizes", splitter->saveState());
//! [1]
    }

    {
    // RESTORE STATE
//! [2]
    QSettings settings;
    splitter->restoreState(settings.value("splitterSizes").toByteArray());
//! [2]
    }

//! [3]
    QListIterator<int> it(splitter->sizes());
    while (it.hasNext())
        processSize(it.next());
//! [3]

    return 0;
}
