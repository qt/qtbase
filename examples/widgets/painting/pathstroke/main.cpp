// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "pathstroke.h"

#include <QApplication>

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(pathstroke);
#ifdef Q_OS_ANDROID
    qputenv("QT_SCALE_FACTOR", "2");
#endif

    QApplication app(argc, argv);

    bool smallScreen = QApplication::arguments().contains("-small-screen");

    PathStrokeWidget pathStrokeWidget(smallScreen);
    QStyle *arthurStyle = new ArthurStyle();
    pathStrokeWidget.setStyle(arthurStyle);
    const QList<QWidget *> widgets = pathStrokeWidget.findChildren<QWidget *>();
    for (QWidget *w : widgets) {
        w->setStyle(arthurStyle);
        w->setAttribute(Qt::WA_AcceptTouchEvents);
    }

    if (smallScreen)
        pathStrokeWidget.showFullScreen();
    else
        pathStrokeWidget.show();

#ifdef QT_KEYPAD_NAVIGATION
    QApplication::setNavigationMode(Qt::NavigationModeCursorAuto);
#endif
    return app.exec();
}
