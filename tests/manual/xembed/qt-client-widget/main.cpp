// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QDebug>
#include <QWindow>

#include "window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStringList args = app.arguments();

    WId winId = 0;
    if (args.count() > 1) {
        bool ok;
        winId = args[1].toUInt(&ok);
        Q_ASSERT(ok);
    }

    Window window;

    QWindow *foreign = QWindow::fromWinId(winId);
    Q_ASSERT(foreign != 0);

    window.windowHandle()->setParent(foreign);
    window.show();

    return app.exec();
}
