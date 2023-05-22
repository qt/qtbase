// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "norwegianwoodstyle.h"
#include "widgetgallery.h"

int main(int argc, char *argv[])
{
    QApplication::setStyle(new NorwegianWoodStyle);

    QApplication app(argc, argv);
    WidgetGallery gallery;
    gallery.show();
    return app.exec();
}
