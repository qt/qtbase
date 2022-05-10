// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "sortingbox.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(tooltips);

    QApplication app(argc, argv);
    SortingBox sortingBox;
    sortingBox.show();
    return app.exec();
}
