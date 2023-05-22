// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "pathdeform.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    bool smallScreen = QApplication::arguments().contains("-small-screen");

    PathDeformWidget deformWidget(nullptr, smallScreen);

    QStyle *arthurStyle = new ArthurStyle;
    deformWidget.setStyle(arthurStyle);
    const QList<QWidget *> widgets = deformWidget.findChildren<QWidget *>();
    for (QWidget *w : widgets)
        w->setStyle(arthurStyle);

    if (smallScreen)
        deformWidget.showFullScreen();
    else
        deformWidget.show();

#ifdef QT_KEYPAD_NAVIGATION
    QApplication::setNavigationMode(Qt::NavigationModeCursorAuto);
#endif
    return app.exec();
}
