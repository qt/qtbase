// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


//! [0]
    QPropertyAnimation *animation = new QPropertyAnimation(myWidget, "geometry");
    animation->setDuration(10000);
    animation->setStartValue(QRect(0, 0, 100, 30));
    animation->setEndValue(QRect(250, 250, 100, 30));

    animation->start();
//! [0]
