// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QWidget>
#include <QMap>
//! [include finddialog's header]
#include "finddialog.h"
//! [include finddialog's header]
QT_BEGIN_NAMESPACE
class QPushButton;
class QLabel;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE


class AddressBook : public QWidget
{
    Q_OBJECT

public:
    AddressBook(QWidget *parent = nullptr);
    enum Mode { NavigationMode, AddingMode, EditingMode };

public slots:
    void addContact();
    void editContact();
    void submitContact();
    void cancel();
    void removeContact();
//! [findContact() declaration]
    void findContact();
//! [findContact() declaration]
    void next();
    void previous();

private:
    void updateInterface(Mode mode);

    QPushButton *addButton;
    QPushButton *editButton;
    QPushButton *removeButton;
//! [findButton declaration]
    QPushButton *findButton;
//! [findButton declaration]
    QPushButton *submitButton;
    QPushButton *cancelButton;
    QPushButton *nextButton;
    QPushButton *previousButton;
    QLineEdit *nameLine;
    QTextEdit *addressText;

    QMap<QString, QString> contacts;
//! [FindDialog declaration]
    FindDialog *dialog;
//! [FindDialog declaration]
    QString oldName;
    QString oldAddress;
    Mode currentMode;
};

#endif
