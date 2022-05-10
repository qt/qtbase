// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [main program]
#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
//! [create, resize and show]
    QWidget window;
    window.resize(320, 240);
    window.show();
//! [create, resize and show]
    window.setWindowTitle(
        QApplication::translate("toplevel", "Top-level widget"));
    return app.exec();
}
//! [main program]
