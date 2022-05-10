// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [main program]
#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QWidget window;
    window.resize(320, 240);
    window.setWindowTitle
          (QApplication::translate("childwidget", "Child widget"));
    window.show();

//! [create, position and show]
    QPushButton *button = new QPushButton(
        QApplication::translate("childwidget", "Press me"), &window);
    button->move(100, 100);
    button->show();
//! [create, position and show]
    return app.exec();
}
//! [main program]
