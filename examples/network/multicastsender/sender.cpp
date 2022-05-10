// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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

    connect(ttlSpinBox, &QSpinBox::valueChanged, this, &Sender::ttlChanged);
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
