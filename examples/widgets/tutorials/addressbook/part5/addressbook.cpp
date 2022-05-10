// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include "addressbook.h"

AddressBook::AddressBook(QWidget *parent)
    : QWidget(parent)
{
    QLabel *nameLabel = new QLabel(tr("Name:"));
    nameLine = new QLineEdit;
    nameLine->setReadOnly(true);

    QLabel *addressLabel = new QLabel(tr("Address:"));
    addressText = new QTextEdit;
    addressText->setReadOnly(true);

    addButton = new QPushButton(tr("&Add"));

    editButton = new QPushButton(tr("&Edit"));
    editButton->setEnabled(false);
    removeButton = new QPushButton(tr("&Remove"));
    removeButton->setEnabled(false);
//! [instantiating findButton]
    findButton = new QPushButton(tr("&Find"));
    findButton->setEnabled(false);
//! [instantiating findButton]
    submitButton = new QPushButton(tr("&Submit"));
    submitButton->hide();
    cancelButton = new QPushButton(tr("&Cancel"));
    cancelButton->hide();

    nextButton = new QPushButton(tr("&Next"));
    nextButton->setEnabled(false);
    previousButton = new QPushButton(tr("&Previous"));
    previousButton->setEnabled(false);

//! [instantiating FindDialog]
    dialog = new FindDialog(this);
//! [instantiating FindDialog]

    connect(addButton, &QPushButton::clicked,
            this, &AddressBook::addContact);
    connect(submitButton, &QPushButton::clicked,
            this, &AddressBook::submitContact);
    connect(editButton, &QPushButton::clicked,
            this, &AddressBook::editContact);
    connect(removeButton, &QPushButton::clicked,
            this, &AddressBook::removeContact);
    connect(cancelButton, &QPushButton::clicked,
            this, &AddressBook::cancel);
//! [signals and slots for find]
    connect(findButton, &QPushButton::clicked,
            this, &AddressBook::findContact);
//! [signals and slots for find]
    connect(nextButton, &QPushButton::clicked,
            this, &AddressBook::next);
    connect(previousButton, &QPushButton::clicked,
            this, &AddressBook::previous);

    QVBoxLayout *buttonLayout1 = new QVBoxLayout;
    buttonLayout1->addWidget(addButton);
    buttonLayout1->addWidget(editButton);
    buttonLayout1->addWidget(removeButton);
//! [adding findButton to layout]
    buttonLayout1->addWidget(findButton);
//! [adding findButton to layout]
    buttonLayout1->addWidget(submitButton);
    buttonLayout1->addWidget(cancelButton);
    buttonLayout1->addStretch();

    QHBoxLayout *buttonLayout2 = new QHBoxLayout;
    buttonLayout2->addWidget(previousButton);
    buttonLayout2->addWidget(nextButton);

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(nameLabel, 0, 0);
    mainLayout->addWidget(nameLine, 0, 1);
    mainLayout->addWidget(addressLabel, 1, 0, Qt::AlignTop);
    mainLayout->addWidget(addressText, 1, 1);
    mainLayout->addLayout(buttonLayout1, 1, 2);
    mainLayout->addLayout(buttonLayout2, 2, 1);

    setLayout(mainLayout);
    setWindowTitle(tr("Simple Address Book"));
}

void AddressBook::addContact()
{
    oldName = nameLine->text();
    oldAddress = addressText->toPlainText();

    nameLine->clear();
    addressText->clear();

    updateInterface(AddingMode);
}

void AddressBook::editContact()
{
    oldName = nameLine->text();
    oldAddress = addressText->toPlainText();

    updateInterface(EditingMode);
}

void AddressBook::submitContact()
{
    QString name = nameLine->text();
    QString address = addressText->toPlainText();

    if (name.isEmpty() || address.isEmpty()) {
        QMessageBox::information(this, tr("Empty Field"),
            tr("Please enter a name and address."));
        return;
    }

    if (currentMode == AddingMode) {

        if (!contacts.contains(name)) {
            contacts.insert(name, address);
            QMessageBox::information(this, tr("Add Successful"),
                tr("\"%1\" has been added to your address book.").arg(name));
        } else {
            QMessageBox::information(this, tr("Add Unsuccessful"),
                tr("Sorry, \"%1\" is already in your address book.").arg(name));
        }
    } else if (currentMode == EditingMode) {

        if (oldName != name) {
            if (!contacts.contains(name)) {
                QMessageBox::information(this, tr("Edit Successful"),
                    tr("\"%1\" has been edited in your address book.").arg(oldName));
                contacts.remove(oldName);
                contacts.insert(name, address);
            } else {
                QMessageBox::information(this, tr("Edit Unsuccessful"),
                    tr("Sorry, \"%1\" is already in your address book.").arg(name));
            }
        } else if (oldAddress != address) {
            QMessageBox::information(this, tr("Edit Successful"),
                tr("\"%1\" has been edited in your address book.").arg(name));
            contacts[name] = address;
        }
    }

    updateInterface(NavigationMode);
}

void AddressBook::cancel()
{
    nameLine->setText(oldName);
    addressText->setText(oldAddress);
    updateInterface(NavigationMode);
}

void AddressBook::removeContact()
{
    QString name = nameLine->text();
    QString address = addressText->toPlainText();

    if (contacts.contains(name)) {

        int button = QMessageBox::question(this,
            tr("Confirm Remove"),
            tr("Are you sure you want to remove \"%1\"?").arg(name),
            QMessageBox::Yes | QMessageBox::No);

        if (button == QMessageBox::Yes) {

            previous();
            contacts.remove(name);

            QMessageBox::information(this, tr("Remove Successful"),
                tr("\"%1\" has been removed from your address book.").arg(name));
        }
    }

    updateInterface(NavigationMode);
}

void AddressBook::next()
{
    QString name = nameLine->text();
    QMap<QString, QString>::iterator i = contacts.find(name);

    if (i != contacts.end())
        i++;

    if (i == contacts.end())
        i = contacts.begin();

    nameLine->setText(i.key());
    addressText->setText(i.value());
}

void AddressBook::previous()
{
    QString name = nameLine->text();
    QMap<QString, QString>::iterator i = contacts.find(name);

    if (i == contacts.end()) {
        nameLine->clear();
        addressText->clear();
        return;
    }

    if (i == contacts.begin())
        i = contacts.end();

    i--;
    nameLine->setText(i.key());
    addressText->setText(i.value());
}
//! [findContact() function]
void AddressBook::findContact()
{
    dialog->show();

    if (dialog->exec() == QDialog::Accepted) {
        QString contactName = dialog->getFindText();

        if (contacts.contains(contactName)) {
            nameLine->setText(contactName);
            addressText->setText(contacts.value(contactName));
        } else {
            QMessageBox::information(this, tr("Contact Not Found"),
                tr("Sorry, \"%1\" is not in your address book.").arg(contactName));
            return;
        }
    }

    updateInterface(NavigationMode);
}
//! [findContact() function]

void AddressBook::updateInterface(Mode mode)
{
    currentMode = mode;

    switch (currentMode) {

    case AddingMode:
    case EditingMode:

        nameLine->setReadOnly(false);
        nameLine->setFocus(Qt::OtherFocusReason);
        addressText->setReadOnly(false);

        addButton->setEnabled(false);
        editButton->setEnabled(false);
        removeButton->setEnabled(false);

        nextButton->setEnabled(false);
        previousButton->setEnabled(false);

        submitButton->show();
        cancelButton->show();
        break;

    case NavigationMode:

        if (contacts.isEmpty()) {
            nameLine->clear();
            addressText->clear();
        }

        nameLine->setReadOnly(true);
        addressText->setReadOnly(true);
        addButton->setEnabled(true);

        int number = contacts.size();
        editButton->setEnabled(number >= 1);
        removeButton->setEnabled(number >= 1);
        findButton->setEnabled(number > 2);
        nextButton->setEnabled(number > 1);
        previousButton->setEnabled(number > 1);

        submitButton->hide();
        cancelButton->hide();
        break;
    }
}
