// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QGraphicsScene scene;
QGraphicsWidget *textEdit = scene.addWidget(new QTextEdit);
QGraphicsWidget *pushButton = scene.addWidget(new QPushButton);

QGraphicsLinearLayout *layout = new QGraphicsLinearLayout;
layout->addItem(textEdit);
layout->addItem(pushButton);

QGraphicsWidget *form = new QGraphicsWidget;
form->setLayout(layout);
scene.addItem(form);
//! [0]
