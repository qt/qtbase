// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "pathdeform.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    bool smallScreen = QApplication::arguments().contains("-small-screen");

    PathDeformWidget deformWidget(nullptr, smallScreen);

    if (smallScreen)
        deformWidget.showFullScreen();
    else
        deformWidget.show();
    return app.exec();
}
