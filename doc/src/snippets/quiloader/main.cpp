// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <QtUiTools>

#include "mywidget.h"

//! [0]
QWidget *loadCustomWidget(QWidget *parent)
{
    QUiLoader loader;
    QWidget *myWidget = nullptr;

    QStringList availableWidgets = loader.availableWidgets();

    if (availableWidgets.contains("AnalogClock"))
        myWidget = loader.createWidget("AnalogClock", parent);

    return myWidget;
}
//! [0]

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MyWidget widget;
    widget.show();

    QWidget *customWidget = loadCustomWidget(0);
    customWidget->show();
    return app.exec();
}
