// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [using a custom style]
#include <QtWidgets>

#include "customstyle.h"

int main(int argc, char *argv[])
{
    QApplication::setStyle(new CustomStyle);
    QApplication app(argc, argv);
    QSpinBox spinBox;
    spinBox.show();
    return app.exec();
}
//! [using a custom style]
