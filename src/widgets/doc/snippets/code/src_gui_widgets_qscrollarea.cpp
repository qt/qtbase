// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QLabel *imageLabel = new QLabel;
QImage image("happyguy.png");
imageLabel->setPixmap(QPixmap::fromImage(image));

scrollArea = new QScrollArea;
scrollArea->setBackgroundRole(QPalette::Dark);
scrollArea->setWidget(imageLabel);
//! [0]
