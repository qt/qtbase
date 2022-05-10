// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include "addressbook.h"

AddressBook::AddressBook(QWidget *parent)
    : QWidget(parent)
{
    QLabel *nameLabel = new QLabel(tr("Name:"));
    nameLine = new QLineEdit;
//! [setting readonly 1]
    nameLine->setReadOnly(true);
//! [setting readonly 1]
    QLabel *addressLabel = new QLabel(tr("Address:"));
    addressText = new QTextEdit;
//! [setting readonly 2]
    addressText->setReadOnly(true);
//! [setting readonly 2]

//! [pushbutton declaration]
    addButton = new QPushButton(tr("&Add"));
    addButton->show();
    submitButton = new QPushButton(tr("&Submit"));
    submitButton->hide();
    cancelButton = new QPushButton(tr("&Cancel"));
    cancelButton->hide();
//! [pushbutton declaration]
//! [connecting signals and slots]
    connect(addButton, &QPushButton::clicked,
            this, &AddressBook::addContact);
    connect(submitButton, &QPushButton::clicked,
            this, &AddressBook::submitContact);
    connect(cancelButton, &QPushButton::clicked,
            this, &AddressBook::cancel);
//! [connecting signals and slots]
//! [vertical layout]
    QVBoxLayout *buttonLayout1 = new QVBoxLayout;
    buttonLayout1->addWidget(addButton, Qt::AlignTop);
    buttonLayout1->addWidget(submitButton);
    buttonLayout1->addWidget(cancelButton);
    buttonLayout1->addStretch();
//! [vertical layout]
//! [grid layout]
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(nameLabel, 0, 0);
    mainLayout->addWidget(nameLine, 0, 1);
    mainLayout->addWidget(addressLabel, 1, 0, Qt::AlignTop);
    mainLayout->addWidget(addressText, 1, 1);
    mainLayout->addLayout(buttonLayout1, 1, 2);
//! [grid layout]
    setLayout(mainLayout);
    setWindowTitle(tr("Simple Address Book"));
}
//! [addContact]
void AddressBook::addContact()
{
    oldName = nameLine->text();
    oldAddress = addressText->toPlainText();

    nameLine->clear();
    addressText->clear();

    nameLine->setReadOnly(false);
    nameLine->setFocus(Qt::OtherFocusReason);
    addressText->setReadOnly(false);

    addButton->setEnabled(false);
    submitButton->show();
    cancelButton->show();
}
//! [addContact]

//! [submitContact part1]
void AddressBook::submitContact()
{
    QString name = nameLine->text();
    QString address = addressText->toPlainText();

    if (name.isEmpty() || address.isEmpty()) {
        QMessageBox::information(this, tr("Empty Field"),
            tr("Please enter a name and address."));
        return;
    }
//! [submitContact part1]
//! [submitContact part2]
    if (!contacts.contains(name)) {
        contacts.insert(name, address);
        QMessageBox::information(this, tr("Add Successful"),
            tr("\"%1\" has been added to your address book.").arg(name));
    } else {
        QMessageBox::information(this, tr("Add Unsuccessful"),
            tr("Sorry, \"%1\" is already in your address book.").arg(name));
        return;
    }
//! [submitContact part2]
//! [submitContact part3]
    if (contacts.isEmpty()) {
        nameLine->clear();
        addressText->clear();
    }

    nameLine->setReadOnly(true);
    addressText->setReadOnly(true);
    addButton->setEnabled(true);
    submitButton->hide();
    cancelButton->hide();
}
//! [submitContact part3]
//! [cancel]
void AddressBook::cancel()
{
    nameLine->setText(oldName);
    nameLine->setReadOnly(true);

    addressText->setText(oldAddress);
    addressText->setReadOnly(true);

    addButton->setEnabled(true);
    submitButton->hide();
    cancelButton->hide();
}
//! [cancel]
