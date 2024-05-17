// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QtWidgets>
#include "basica11ywidget.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    BasicA11yWidget a11yWidget;
    a11yWidget.show();

    return app.exec();
}

