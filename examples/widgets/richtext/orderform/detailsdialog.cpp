/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include "detailsdialog.h"

//! [0]
DetailsDialog::DetailsDialog(const QString &title, QWidget *parent)
    : QDialog(parent)
{
    nameLabel = new QLabel(tr("Name:"));
    addressLabel = new QLabel(tr("Address:"));
    addressLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    nameEdit = new QLineEdit;
    addressEdit = new QTextEdit;

    offersCheckBox = new QCheckBox(tr("Send information about products and "
                                      "special offers"));

    setupItemsTable();

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &DetailsDialog::verify);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DetailsDialog::reject);
//! [0]

//! [1]
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(nameLabel, 0, 0);
    mainLayout->addWidget(nameEdit, 0, 1);
    mainLayout->addWidget(addressLabel, 1, 0);
    mainLayout->addWidget(addressEdit, 1, 1);
    mainLayout->addWidget(itemsTable, 0, 2, 2, 1);
    mainLayout->addWidget(offersCheckBox, 2, 1, 1, 2);
    mainLayout->addWidget(buttonBox, 3, 0, 1, 3);
    setLayout(mainLayout);

    setWindowTitle(title);
}
//! [1]

//! [2]
void DetailsDialog::setupItemsTable()
{
    items << tr("T-shirt") << tr("Badge") << tr("Reference book")
          << tr("Coffee cup");

    itemsTable = new QTableWidget(items.count(), 2);

    for (int row = 0; row < items.count(); ++row) {
        QTableWidgetItem *name = new QTableWidgetItem(items[row]);
        name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        itemsTable->setItem(row, 0, name);
        QTableWidgetItem *quantity = new QTableWidgetItem("1");
        itemsTable->setItem(row, 1, quantity);
    }
}
//! [2]

//! [3]
QList<QPair<QString, int> > DetailsDialog::orderItems()
{
    QList<QPair<QString, int> > orderList;

    for (int row = 0; row < items.count(); ++row) {
        QPair<QString, int> item;
        item.first = itemsTable->item(row, 0)->text();
        int quantity = itemsTable->item(row, 1)->data(Qt::DisplayRole).toInt();
        item.second = qMax(0, quantity);
        orderList.append(item);
    }

    return orderList;
}
//! [3]

//! [4]
QString DetailsDialog::senderName() const
{
    return nameEdit->text();
}
//! [4]

//! [5]
QString DetailsDialog::senderAddress() const
{
    return addressEdit->toPlainText();
}
//! [5]

//! [6]
bool DetailsDialog::sendOffers()
{
    return offersCheckBox->isChecked();
}
//! [6]

//! [7]
void DetailsDialog::verify()
{
    if (!nameEdit->text().isEmpty() && !addressEdit->toPlainText().isEmpty()) {
        accept();
        return;
    }

    QMessageBox::StandardButton answer;
    answer = QMessageBox::warning(this, tr("Incomplete Form"),
        tr("The form does not contain all the necessary information.\n"
           "Do you want to discard it?"),
        QMessageBox::Yes | QMessageBox::No);

    if (answer == QMessageBox::Yes)
        reject();
}
//! [7]
