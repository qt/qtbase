// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include "finddialog.h"

//! [constructor]
FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent)
{
    QLabel *findLabel = new QLabel(tr("Enter the name of a contact:"));
    lineEdit = new QLineEdit;

    findButton = new QPushButton(tr("&Find"));
    findText = "";

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(findLabel);
    layout->addWidget(lineEdit);
    layout->addWidget(findButton);

    setLayout(layout);
    setWindowTitle(tr("Find a Contact"));
    connect(findButton, &QPushButton::clicked,
            this, &FindDialog::findClicked);
    connect(findButton, &QPushButton::clicked,
            this, &FindDialog::accept);
}
//! [constructor]
//! [findClicked() function]
void FindDialog::findClicked()
{
    QString text = lineEdit->text();

    if (text.isEmpty()) {
        QMessageBox::information(this, tr("Empty Field"),
            tr("Please enter a name."));
        return;
    } else {
        findText = text;
        lineEdit->clear();
        hide();
    }
}
//! [findClicked() function]
//! [getFindText() function]
QString FindDialog::getFindText()
{
    return findText;
}
//! [getFindText() function]
