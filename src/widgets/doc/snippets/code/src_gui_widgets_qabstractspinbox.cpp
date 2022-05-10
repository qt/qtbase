// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QSpinBox *spinBox = new QSpinBox(this);
spinBox->setRange(0, 100);
spinBox->setWrapping(true);
spinBox->setValue(100);
spinBox->stepBy(1);
// value is 0
//! [0]
