// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QDir>
#include <QScreen>

#include "httpwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    HttpWindow httpWin;
    const QRect availableSize = httpWin.screen()->availableGeometry();
    httpWin.resize(availableSize.width() / 5, availableSize.height() / 5);
    httpWin.move((availableSize.width() - httpWin.width()) / 2, (availableSize.height() - httpWin.height()) / 2);
    httpWin.show();

    return app.exec();
}
