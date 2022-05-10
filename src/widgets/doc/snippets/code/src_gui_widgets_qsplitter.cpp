// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QWidget *widget = splitter->widget(index);
QSizePolicy policy = widget->sizePolicy();
policy.setHorizontalStretch(stretch);
policy.setVerticalStretch(stretch);
widget->setSizePolicy(policy);
//! [0]
