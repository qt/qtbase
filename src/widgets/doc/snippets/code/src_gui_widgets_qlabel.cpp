// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QLabel *label = new QLabel(this);
label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
label->setText("first line\nsecond line");
label->setAlignment(Qt::AlignBottom | Qt::AlignRight);
//! [0]


//! [1]
QLineEdit *phoneEdit = new QLineEdit(this);
QLabel *phoneLabel = new QLabel("&Phone:", this);
phoneLabel->setBuddy(phoneEdit);
//! [1]


//! [2]
QLineEdit *nameEdit  = new QLineEdit(this);
QLabel    *nameLabel = new QLabel("&Name:", this);
nameLabel->setBuddy(nameEdit);
QLineEdit *phoneEdit  = new QLineEdit(this);
QLabel    *phoneLabel = new QLabel("&Phone:", this);
phoneLabel->setBuddy(phoneEdit);
// (layout setup not shown)
//! [2]
