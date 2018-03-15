/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
