// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "dialog.h"
#include "wigglywidget.h"

#include <QLineEdit>
#include <QVBoxLayout>

//! [0]
Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
{
    WigglyWidget *wigglyWidget = new WigglyWidget;
    QLineEdit *lineEdit = new QLineEdit;

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(wigglyWidget);
    layout->addWidget(lineEdit);

    connect(lineEdit, &QLineEdit::textChanged, wigglyWidget, &WigglyWidget::setText);
    lineEdit->setText(u8"🖖 " + tr("Hello world!"));

    setWindowTitle(tr("Wiggly"));
    resize(360, 145);
}
//! [0]
