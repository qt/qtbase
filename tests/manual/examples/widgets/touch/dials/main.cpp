// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QWidget>
#include <QDial>

#include "ui_dials.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QWidget window;
    Ui::Dials dialsUi;
    dialsUi.setupUi(&window);
    const QList<QAbstractSlider *> sliders = window.findChildren<QAbstractSlider *>();
    for (QAbstractSlider *slider : sliders)
        slider->setAttribute(Qt::WA_AcceptTouchEvents);
    window.showMaximized();
    return app.exec();
}
