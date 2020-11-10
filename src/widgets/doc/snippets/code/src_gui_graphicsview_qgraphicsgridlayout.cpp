// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QGraphicsScene scene;
QGraphicsWidget *textEdit = scene.addWidget(new QTextEdit);
QGraphicsWidget *pushButton = scene.addWidget(new QPushButton);

QGraphicsWidget *form = new QGraphicsWidget;
scene.addItem(form);

QGraphicsGridLayout *layout = new QGraphicsGridLayout(form);
layout->addItem(textEdit, 0, 0);
layout->addItem(pushButton, 0, 1);
//! [0]
