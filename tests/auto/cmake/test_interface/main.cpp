// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>

#include "mainwindow.h"

#if !defined(QT_CORE_LIB) || !defined(QT_GUI_LIB) || !defined(QT_WIDGETS_LIB)
#error Missing module definitions
#endif

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MainWindow mw;
    mw.show();

    return app.exec();
}
