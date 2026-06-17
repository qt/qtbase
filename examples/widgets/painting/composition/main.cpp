// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "composition.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    CompositionWidget compWidget(nullptr);
    compWidget.show();

    return app.exec();
}
