// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "searchbox.h"

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);
    SearchBox searchEdit;
    searchEdit.show();
    return app.exec();
}
