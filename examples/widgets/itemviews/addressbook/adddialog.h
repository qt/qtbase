// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ADDDIALOG_H
#define ADDDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLabel;
class QPlainTextEdit;
class QLineEdit;
QT_END_NAMESPACE

struct Contact;

//! [0]
class AddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDialog(QWidget *parent = nullptr);

    Contact contact() const;
    void setContact(const Contact &c);

    void editAddress(const Contact &c);

private slots:
    void updateEnabled();

private:
    QDialogButtonBox *buttonBox;
    QLineEdit *nameText;
    QPlainTextEdit *addressText;
};
//! [0]

#endif // ADDDIALOG_H
