// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "addressdialog.h"
#include "ui_addressdialog.h"

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>

#include <limits>

AddressDialog::AddressDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::AddressDialog)
{
    ui->setupUi(this);
    setupHostSelector();
    setupPortSelector();
}

AddressDialog::~AddressDialog()
{
    delete ui;
}

QString AddressDialog::remoteName() const
{
    if (ui->addressSelector->count())
        return ui->addressSelector->currentText();
    return {};
}

quint16 AddressDialog::remotePort() const
{
    return quint16(ui->portSelector->text().toUInt());
}

void AddressDialog::setupHostSelector()
{
    QString name(QHostInfo::localHostName());
    if (!name.isEmpty()) {
        ui->addressSelector->addItem(name);
        const QString domain = QHostInfo::localDomainName();
        if (!domain.isEmpty())
            ui->addressSelector->addItem(name + QChar('.') + domain);
    }

    if (name != QStringLiteral("localhost"))
        ui->addressSelector->addItem(QStringLiteral("localhost"));

    const QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    for (const QHostAddress &ipAddress : ipAddressesList) {
        if (!ipAddress.isLoopback())
            ui->addressSelector->addItem(ipAddress.toString());
    }

    ui->addressSelector->insertSeparator(ui->addressSelector->count());

    for (const QHostAddress &ipAddress : ipAddressesList) {
        if (ipAddress.isLoopback())
            ui->addressSelector->addItem(ipAddress.toString());
    }
}

void AddressDialog::setupPortSelector()
{
    ui->portSelector->setValidator(new QIntValidator(0, std::numeric_limits<quint16>::max(),
                                                     ui->portSelector));
    ui->portSelector->setText(QStringLiteral("22334"));
}
