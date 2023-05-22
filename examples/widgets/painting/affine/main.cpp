// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "xform.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    XFormWidget xformWidget(nullptr);
    QStyle *arthurStyle = new ArthurStyle;
    xformWidget.setStyle(arthurStyle);

    const QList<QWidget *> widgets = xformWidget.findChildren<QWidget *>();
    for (QWidget *w : widgets) {
        w->setStyle(arthurStyle);
        w->setAttribute(Qt::WA_AcceptTouchEvents);
    }

    xformWidget.show();

    return app.exec();
}
