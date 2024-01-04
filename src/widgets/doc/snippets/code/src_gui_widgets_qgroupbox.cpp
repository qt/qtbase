// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
g->setTitle("&User information");
//! [0]

//! [Set up QGroupBox with layout]
QGroupBox *groupBox = new QGroupBox(tr("Group Box with Layout"));

QRadioButton *radio1 = new QRadioButton(tr("&Radio button 1"));
QRadioButton *radio2 = new QRadioButton(tr("R&adio button 2"));
QRadioButton *radio3 = new QRadioButton(tr("Ra&dio button 3"));

radio1->setChecked(true);

QVBoxLayout *vbox = new QVBoxLayout;
vbox->addWidget(radio1);
vbox->addWidget(radio2);
vbox->addWidget(radio3);
vbox->addStretch(1);
groupBox->setLayout(vbox);
//! [Set up QGroupBox with layout]
