// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include "widget.h"

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "-g"))
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
        else if (!qstrcmp(argv[i], "-s"))
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
        else if (!qstrcmp(argv[i], "-d"))
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    }

    QApplication app(argc, argv);

    Widget w;
    w.resize(700, 800);
    w.show();

    return app.exec();
}
