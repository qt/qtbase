// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "adddialog.h"
#include "contact.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

//! [0]
AddDialog::AddDialog(QWidget *parent)
    : QDialog(parent),
      buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this)),
      nameText(new QLineEdit),
      addressText(new QPlainTextEdit)
{
    auto *formLayout = new QFormLayout;
    formLayout->addRow(tr("Name"), nameText);
    formLayout->addRow(tr("Address"), addressText);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(nameText, &QLineEdit::textChanged, this, &AddDialog::updateEnabled);
    connect(addressText, &QPlainTextEdit::textChanged, this, &AddDialog::updateEnabled);

    setWindowTitle(tr("Add a Contact"));

    updateEnabled();
}

void AddDialog::updateEnabled()
{
    Contact c = contact();
    const bool valid = !c.name.isEmpty() && c.name.front().isLetter() && !c.address.isEmpty();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

Contact AddDialog::contact() const
{
    return { nameText->text().trimmed(), addressText->toPlainText().trimmed() };
}

void AddDialog::setContact(const Contact &c)
{
    nameText->setText(c.name);
    addressText->setPlainText(c.address);
    updateEnabled();
}

void AddDialog::editAddress(const Contact &c)
{
    nameText->setReadOnly(true);
    setContact(c);
}
//! [0]
