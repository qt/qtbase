// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../shared/shared.h"

#include <QApplication>
#include <QMainWindow>
#include <QSplitter>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QSplitter splitter;

    splitter.addWidget(new StaticWidget());
    splitter.addWidget(new StaticWidget());

    splitter.resize(600, 400);
    splitter.show();

    return app.exec();
}
