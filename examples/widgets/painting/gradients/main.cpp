// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "gradients.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    GradientWidget gradientWidget;
    QStyle *arthurStyle = new ArthurStyle;
    gradientWidget.setStyle(arthurStyle);
    const QList<QWidget *> widgets = gradientWidget.findChildren<QWidget *>();
    for (QWidget *w : widgets) {
        w->setStyle(arthurStyle);
        w->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    gradientWidget.show();

    return app.exec();
}
