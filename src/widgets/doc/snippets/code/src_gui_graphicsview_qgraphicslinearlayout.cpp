// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QGraphicsScene scene;
QGraphicsWidget *textEdit = scene.addWidget(new QTextEdit);
QGraphicsWidget *pushButton = scene.addWidget(new QPushButton);

QGraphicsWidget *form = new QGraphicsWidget;
scene.addItem(form);

QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(form);
layout->addItem(textEdit);
layout->addItem(pushButton);
//! [0]
