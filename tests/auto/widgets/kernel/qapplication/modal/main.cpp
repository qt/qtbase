// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include "base.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setAttribute(Qt::AA_NativeWindows); //QTBUG-15774
    base b;
    return app.exec();
}
