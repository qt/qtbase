// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "dragwidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
#ifdef QT_KEYPAD_NAVIGATION
    QApplication::setNavigationMode(Qt::NavigationModeCursorAuto);
#endif
    DragWidget window;

    bool smallScreen = QApplication::arguments().contains(QStringLiteral("-small-screen"));
    if (smallScreen)
        window.showFullScreen();
    else
        window.show();

    return app.exec();
}
