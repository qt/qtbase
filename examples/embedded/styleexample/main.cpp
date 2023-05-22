// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>

#include "stylewidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("style");
    app.setOrganizationName("QtProject");
    app.setOrganizationDomain("www.qt-project.org");

    StyleWidget widget;
    widget.showFullScreen();

    return app.exec();
}

