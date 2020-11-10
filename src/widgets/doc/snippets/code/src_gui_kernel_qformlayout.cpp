// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QFormLayout *formLayout = new QFormLayout(this);
formLayout->addRow(tr("&Name:"), nameLineEdit);
formLayout->addRow(tr("&Email:"), emailLineEdit);
formLayout->addRow(tr("&Age:"), ageSpinBox);
//! [0]


//! [1]
QGridLayout *gridLayout = new QGridLayout(this);

nameLabel = new QLabel(tr("&Name:"));
nameLabel->setBuddy(nameLineEdit);

emailLabel = new QLabel(tr("&Name:"));
emailLabel->setBuddy(emailLineEdit);

ageLabel = new QLabel(tr("&Name:"));
ageLabel->setBuddy(ageSpinBox);

gridLayout->addWidget(nameLabel, 0, 0);
gridLayout->addWidget(nameLineEdit, 0, 1);
gridLayout->addWidget(emailLabel, 1, 0);
gridLayout->addWidget(emailLineEdit, 1, 1);
gridLayout->addWidget(ageLabel, 2, 0);
gridLayout->addWidget(ageSpinBox, 2, 1);
//! [1]


//! [2]
formLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
formLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
formLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
formLayout->setLabelAlignment(Qt::AlignLeft);
//! [2]

//! [3]
QFormLayout *flay = ...;
QPointer<QLineEdit> le = new QLineEdit;
flay->insertRow(2, "User:", le);
// later:
flay->removeRow(2); // le == nullptr at this point
//! [3]

//! [4]
QFormLayout *flay = ...;
QPointer<QLineEdit> le = new QLineEdit;
flay->insertRow(2, "User:", le);
// later:
flay->removeRow(le); // le == nullptr at this point
//! [4]

//! [5]
QFormLayout *flay = ...;
QPointer<QVBoxLayout> vbl = new QVBoxLayout;
flay->insertRow(2, "User:", vbl);
// later:
flay->removeRow(layout); // vbl == nullptr at this point
//! [5]

//! [6]
QFormLayout *flay = ...;
QPointer<QLineEdit> le = new QLineEdit;
flay->insertRow(2, "User:", le);
// later:
QFormLayout::TakeRowResult result = flay->takeRow(2);
//! [6]

//! [7]
QFormLayout *flay = ...;
QPointer<QLineEdit> le = new QLineEdit;
flay->insertRow(2, "User:", le);
// later:
QFormLayout::TakeRowResult result = flay->takeRow(widget);
//! [7]

//! [8]
QFormLayout *flay = ...;
QPointer<QVBoxLayout> vbl = new QVBoxLayout;
flay->insertRow(2, "User:", vbl);
// later:
QFormLayout::TakeRowResult result = flay->takeRow(widget);
//! [8]
