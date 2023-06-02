// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "blurpicker.h"
#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    BlurPicker blurPicker;
    blurPicker.setWindowTitle(QT_TRANSLATE_NOOP(QGraphicsView, "Application Picker"));

    blurPicker.setFixedSize(400, 300);
    blurPicker.show();

    return app.exec();
}
