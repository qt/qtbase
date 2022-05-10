// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <limits>

#include <QtCore>
#include <QtNetwork>

#include "nicselector.h"
#include "ui_nicselector.h"

NicSelector::NicSelector(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NicSelector)
{
    ui->setupUi(this);
    auto portValidator = new QIntValidator(0, int(std::numeric_limits<quint16>::max()),
                                           ui->portSelector);
    ui->portSelector->setValidator(portValidator);
    ui->portSelector->setText(QStringLiteral("22334"));

    const QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    availableAddresses.reserve(ipAddressesList.size());
    for (const QHostAddress &ip : ipAddressesList) {
        if (ip != QHostAddress::LocalHost && ip.toIPv4Address()) {
            availableAddresses.push_back(ip);
            ui->ipSelector->addItem(ip.toString());
        }
    }
}

NicSelector::~NicSelector()
{
    delete ui;
}

QHostAddress NicSelector::selectedIp() const
{
    if (!availableAddresses.size())
        return {};

    return availableAddresses[ui->ipSelector->currentIndex()];
}

quint16 NicSelector::selectedPort() const
{
    return quint16(ui->portSelector->text().toUInt());
}
