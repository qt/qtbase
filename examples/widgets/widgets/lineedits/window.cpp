// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>

//! [0]
Window::Window(QWidget *parent)
    : QWidget(parent)
{
    QGroupBox *echoGroup = new QGroupBox(tr("Echo"));

    QLabel *echoLabel = new QLabel(tr("Mode:"));
    QComboBox *echoComboBox = new QComboBox;
    echoComboBox->addItem(tr("Normal"));
    echoComboBox->addItem(tr("Password"));
    echoComboBox->addItem(tr("PasswordEchoOnEdit"));
    echoComboBox->addItem(tr("No Echo"));

    echoLineEdit = new QLineEdit;
    echoLineEdit->setPlaceholderText("Placeholder Text");
    echoLineEdit->setFocus();
//! [0]

//! [1]
    QGroupBox *validatorGroup = new QGroupBox(tr("Validator"));

    QLabel *validatorLabel = new QLabel(tr("Type:"));
    QComboBox *validatorComboBox = new QComboBox;
    validatorComboBox->addItem(tr("No validator"));
    validatorComboBox->addItem(tr("Integer validator"));
    validatorComboBox->addItem(tr("Double validator"));

    validatorLineEdit = new QLineEdit;
    validatorLineEdit->setPlaceholderText("Placeholder Text");
//! [1]

//! [2]
    QGroupBox *alignmentGroup = new QGroupBox(tr("Alignment"));

    QLabel *alignmentLabel = new QLabel(tr("Type:"));
    QComboBox *alignmentComboBox = new QComboBox;
    alignmentComboBox->addItem(tr("Left"));
    alignmentComboBox->addItem(tr("Centered"));
    alignmentComboBox->addItem(tr("Right"));

    alignmentLineEdit = new QLineEdit;
    alignmentLineEdit->setPlaceholderText("Placeholder Text");
//! [2]

//! [3]
    QGroupBox *inputMaskGroup = new QGroupBox(tr("Input mask"));

    QLabel *inputMaskLabel = new QLabel(tr("Type:"));
    QComboBox *inputMaskComboBox = new QComboBox;
    inputMaskComboBox->addItem(tr("No mask"));
    inputMaskComboBox->addItem(tr("Phone number"));
    inputMaskComboBox->addItem(tr("ISO date"));
    inputMaskComboBox->addItem(tr("License key"));

    inputMaskLineEdit = new QLineEdit;
    inputMaskLineEdit->setPlaceholderText("Placeholder Text");
//! [3]

//! [4]
    QGroupBox *accessGroup = new QGroupBox(tr("Access"));

    QLabel *accessLabel = new QLabel(tr("Read-only:"));
    QComboBox *accessComboBox = new QComboBox;
    accessComboBox->addItem(tr("False"));
    accessComboBox->addItem(tr("True"));

    accessLineEdit = new QLineEdit;
    accessLineEdit->setPlaceholderText("Placeholder Text");
//! [4]

//! [5]
    connect(echoComboBox, &QComboBox::activated,
            this, &Window::echoChanged);
    connect(validatorComboBox, &QComboBox::activated,
            this, &Window::validatorChanged);
    connect(alignmentComboBox, &QComboBox::activated,
            this, &Window::alignmentChanged);
    connect(inputMaskComboBox, &QComboBox::activated,
            this, &Window::inputMaskChanged);
    connect(accessComboBox, &QComboBox::activated,
            this, &Window::accessChanged);
//! [5]

//! [6]
    QGridLayout *echoLayout = new QGridLayout;
    echoLayout->addWidget(echoLabel, 0, 0);
    echoLayout->addWidget(echoComboBox, 0, 1);
    echoLayout->addWidget(echoLineEdit, 1, 0, 1, 2);
    echoGroup->setLayout(echoLayout);
//! [6]

//! [7]
    QGridLayout *validatorLayout = new QGridLayout;
    validatorLayout->addWidget(validatorLabel, 0, 0);
    validatorLayout->addWidget(validatorComboBox, 0, 1);
    validatorLayout->addWidget(validatorLineEdit, 1, 0, 1, 2);
    validatorGroup->setLayout(validatorLayout);

    QGridLayout *alignmentLayout = new QGridLayout;
    alignmentLayout->addWidget(alignmentLabel, 0, 0);
    alignmentLayout->addWidget(alignmentComboBox, 0, 1);
    alignmentLayout->addWidget(alignmentLineEdit, 1, 0, 1, 2);
    alignmentGroup-> setLayout(alignmentLayout);

    QGridLayout *inputMaskLayout = new QGridLayout;
    inputMaskLayout->addWidget(inputMaskLabel, 0, 0);
    inputMaskLayout->addWidget(inputMaskComboBox, 0, 1);
    inputMaskLayout->addWidget(inputMaskLineEdit, 1, 0, 1, 2);
    inputMaskGroup->setLayout(inputMaskLayout);

    QGridLayout *accessLayout = new QGridLayout;
    accessLayout->addWidget(accessLabel, 0, 0);
    accessLayout->addWidget(accessComboBox, 0, 1);
    accessLayout->addWidget(accessLineEdit, 1, 0, 1, 2);
    accessGroup->setLayout(accessLayout);
//! [7]

//! [8]
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(echoGroup, 0, 0);
    layout->addWidget(validatorGroup, 1, 0);
    layout->addWidget(alignmentGroup, 2, 0);
    layout->addWidget(inputMaskGroup, 0, 1);
    layout->addWidget(accessGroup, 1, 1);
    setLayout(layout);

    setWindowTitle(tr("Line Edits"));
}
//! [8]

//! [9]
void Window::echoChanged(int index)
{
    switch (index) {
    case 0:
        echoLineEdit->setEchoMode(QLineEdit::Normal);
        break;
    case 1:
        echoLineEdit->setEchoMode(QLineEdit::Password);
        break;
    case 2:
        echoLineEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
        break;
    case 3:
        echoLineEdit->setEchoMode(QLineEdit::NoEcho);
        break;
    }
}
//! [9]

//! [10]
void Window::validatorChanged(int index)
{
    switch (index) {
    case 0:
        validatorLineEdit->setValidator(nullptr);
        break;
    case 1:
        validatorLineEdit->setValidator(new QIntValidator(
            validatorLineEdit));
        break;
    case 2:
        validatorLineEdit->setValidator(new QDoubleValidator(-999.0,
            999.0, 2, validatorLineEdit));
        break;
    }

    validatorLineEdit->clear();
}
//! [10]

//! [11]
void Window::alignmentChanged(int index)
{
    switch (index) {
    case 0:
        alignmentLineEdit->setAlignment(Qt::AlignLeft);
        break;
    case 1:
        alignmentLineEdit->setAlignment(Qt::AlignCenter);
        break;
    case 2:
        alignmentLineEdit->setAlignment(Qt::AlignRight);
        break;
    }
}
//! [11]

//! [12]
void Window::inputMaskChanged(int index)
{
    switch (index) {
    case 0:
        inputMaskLineEdit->setInputMask("");
        break;
    case 1:
        inputMaskLineEdit->setInputMask("+99 99 99 99 99;_");
        break;
    case 2:
        inputMaskLineEdit->setInputMask("0000-00-00");
        inputMaskLineEdit->setText("00000000");
        inputMaskLineEdit->setCursorPosition(0);
        break;
    case 3:
        inputMaskLineEdit->setInputMask(">AAAAA-AAAAA-AAAAA-AAAAA-AAAAA;#");
        break;
    }
}
//! [12]

//! [13]
void Window::accessChanged(int index)
{
    switch (index) {
    case 0:
        accessLineEdit->setReadOnly(false);
        break;
    case 1:
        accessLineEdit->setReadOnly(true);
        break;
    }
}
//! [13]
