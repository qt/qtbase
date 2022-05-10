// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "dialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Dialog dialog;
#ifdef Q_OS_ANDROID
    dialog.showMaximized();
#else
    dialog.show();
#endif

    return app.exec();
}
