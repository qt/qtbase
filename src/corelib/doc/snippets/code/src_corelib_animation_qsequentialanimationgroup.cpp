// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    QSequentialAnimationGroup *group = new QSequentialAnimationGroup;

    group->addAnimation(anim1);
    group->addAnimation(anim2);

    group->start();
//! [0]
