// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QWidget>
#include <QMap>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QTextEdit;
QT_END_NAMESPACE

class AddressBook : public QWidget
{
    Q_OBJECT

public:
    AddressBook(QWidget *parent = nullptr);

//! [slots]
public slots:
    void addContact();
    void submitContact();
    void cancel();
//! [slots]

//! [pushbutton declaration]
private:
    QPushButton *addButton;
    QPushButton *submitButton;
    QPushButton *cancelButton;
    QLineEdit *nameLine;
    QTextEdit *addressText;
//! [pushbutton declaration]

//! [remaining private variables]
    QMap<QString, QString> contacts;
    QString oldName;
    QString oldAddress;
};
//! [remaining private variables]

#endif
