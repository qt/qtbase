// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "widgetgallery.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    WidgetGallery gallery;
    gallery.show();
    return QCoreApplication::exec();
}
