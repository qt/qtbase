// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "glwidget.h"
#include "widget.h"
#include "window.h"

#include <QGridLayout>
#include <QLabel>
#include <QTimer>

//! [0]
Window::Window()
{
    setWindowTitle(tr("2D Painting on Native and OpenGL Widgets"));

    Widget *native = new Widget(&helper, this);
    GLWidget *openGL = new GLWidget(&helper, this);
    QLabel *nativeLabel = new QLabel(tr("Native"));
    nativeLabel->setAlignment(Qt::AlignHCenter);
    QLabel *openGLLabel = new QLabel(tr("OpenGL"));
    openGLLabel->setAlignment(Qt::AlignHCenter);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(native, 0, 0);
    layout->addWidget(openGL, 0, 1);
    layout->addWidget(nativeLabel, 1, 0);
    layout->addWidget(openGLLabel, 1, 1);
    setLayout(layout);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, native, &Widget::animate);
    connect(timer, &QTimer::timeout, openGL, &GLWidget::animate);
    timer->start(50);
}
//! [0]
