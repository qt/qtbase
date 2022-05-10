// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QMainWindow>
#include <QScrollArea>

#include "../shared/shared.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QScrollArea scrollView;

    QWidget * staticWidget = new StaticWidget();
    staticWidget->resize(400, 200);
    scrollView.setWidget(staticWidget);

    scrollView.setAttribute(Qt::WA_StaticContents);

    scrollView.resize(600, 400);
    scrollView.show();

    return app.exec();
}


