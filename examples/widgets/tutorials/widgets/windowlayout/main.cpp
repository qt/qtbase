// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [main program]
#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QWidget window;
//! [create, lay out widgets and show]
    QLabel *label = new QLabel(QApplication::translate("windowlayout", "Name:"));
    QLineEdit *lineEdit = new QLineEdit();

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(label);
    layout->addWidget(lineEdit);
    window.setLayout(layout);
//! [create, lay out widgets and show]
    window.setWindowTitle(
        QApplication::translate("windowlayout", "Window layout"));
    window.show();
    return app.exec();
}
//! [main program]
