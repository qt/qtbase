// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QHBoxLayout>

#include "dragwidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget mainWidget;
    QHBoxLayout *horizontalLayout = new QHBoxLayout(&mainWidget);
    horizontalLayout->addWidget(new DragWidget);
    horizontalLayout->addWidget(new DragWidget);

    mainWidget.setWindowTitle(QObject::tr("Draggable Icons"));
    mainWidget.show();

    return app.exec();
}
