// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// This test is for checking that when there is padding set on the stylesheet and the elide mode is
// set that it is correctly shown as elided and not clipped.

#include <QApplication>
#include <QTabBar>
#include <QIcon>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setStyleSheet("QTabBar::tab { padding-left: 20px; }\n");
    QIcon icon(":/v.ico");

    QTabBar b;
    b.setElideMode(Qt::ElideRight);
    b.addTab(icon, "some text");
    b.resize(80,32);
    b.show();

    return app.exec();
}
