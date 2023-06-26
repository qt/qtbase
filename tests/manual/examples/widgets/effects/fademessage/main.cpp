// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "fademessage.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    FadeMessage widget;
    widget.setWindowTitle(QT_TRANSLATE_NOOP(QGraphicsView, "Popup Message with Effect"));
    widget.setFixedSize(400, 600);
    widget.show();

    return app.exec();
}
