// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Ivan Komissarov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QShortcut>
#include <QTreeView>

#include "storagemodel.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTreeView view;
    view.resize(640, 480);
    view.setWindowTitle("Storage View");
    view.setSelectionBehavior(QAbstractItemView::SelectRows);

    StorageModel *model = new StorageModel(&view);
    model->refresh();
    QShortcut *refreshShortcut = new QShortcut(QKeySequence::Refresh, &view);
    QObject::connect(refreshShortcut, &QShortcut::activated, model, &StorageModel::refresh);
    view.setModel(model);

    int columnCount = view.model()->columnCount();
    for (int c = 0; c < columnCount; ++c)
        view.resizeColumnToContents(c);
    view.show();

    return a.exec();
}
