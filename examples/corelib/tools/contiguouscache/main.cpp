// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "randomlistmodel.h"
#include <QListView>
#include <QApplication>

int main(int c, char **v)
{
    QApplication a(c, v);

    QListView view;
    view.setUniformItemSizes(true);
    view.setModel(new RandomListModel(&view));
    view.show();

    return a.exec();
}
