/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "sender.h"

Sender::Sender(QWidget *parent)
    : QDialog(parent),
      groupAddress4(QStringLiteral("239.255.43.21")),
      groupAddress6(QStringLiteral("ff12::2115"))
{
    // force binding to their respective families
    udpSocket4.bind(QHostAddress(QHostAddress::AnyIPv4), 0);
    udpSocket6.bind(QHostAddress(QHostAddress::AnyIPv6), udpSocket4.localPort());

    QString msg = tr("Ready to multicast datagrams to groups %1 and [%2] on port 45454").arg(groupAddress4.toString());
    if (udpSocket6.state() != QAbstractSocket::BoundState)
        msg = tr("IPv6 failed. Ready to multicast datagrams to group %1 on port 45454").arg(groupAddress4.toString());
    else
        msg = msg.arg(groupAddress6.toString());
    statusLabel = new QLabel(msg);

    auto ttlLabel = new QLabel(tr("TTL for IPv4 multicast datagrams:"));
    auto ttlSpinBox = new QSpinBox;
    ttlSpinBox->setRange(0, 255);

    auto ttlLayout = new QHBoxLayout;
    ttlLayout->addWidget(ttlLabel);
    ttlLayout->addWidget(ttlSpinBox);

    startButton = new QPushButton(tr("&Start"));
    auto quitButton = new QPushButton(tr("&Quit"));

    auto buttonBox = new QDialogButtonBox;
    buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    connect(ttlSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &Sender::ttlChanged);
    connect(startButton, &QPushButton::clicked, this, &Sender::startSending);
    connect(quitButton, &QPushButton::clicked, this, &Sender::close);
    connect(&timer, &QTimer::timeout, this, &Sender::sendDatagram);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(ttlLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle(tr("Multicast Sender"));
    ttlSpinBox->setValue(1);
}

void Sender::ttlChanged(int newTtl)
{
    // we only set the TTL on the IPv4 socket, as that changes the multicast scope
    udpSocket4.setSocketOption(QAbstractSocket::MulticastTtlOption, newTtl);
}

void Sender::startSending()
{
    startButton->setEnabled(false);
    timer.start(1000);
}

void Sender::sendDatagram()
{
    statusLabel->setText(tr("Now sending datagram %1").arg(messageNo));
    QByteArray datagram = "Multicast message " + QByteArray::number(messageNo);
    udpSocket4.writeDatagram(datagram, groupAddress4, 45454);
    if (udpSocket6.state() == QAbstractSocket::BoundState)
        udpSocket6.writeDatagram(datagram, groupAddress6, 45454);
    ++messageNo;
}
