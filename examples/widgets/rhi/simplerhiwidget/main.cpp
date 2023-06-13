// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "examplewidget.h"

//![0]
int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    ExampleRhiWidget *rhiWidget = new ExampleRhiWidget;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(rhiWidget);

    QWidget w;
    w.setLayout(layout);
    w.resize(1280, 720);
    w.show();

    return app.exec();
}
//![0]
