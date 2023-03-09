// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include <QtNetwork>

#include "receiver.h"

Receiver::Receiver(QWidget *parent)
    : QDialog(parent),
      groupAddress4(QStringLiteral("239.255.43.21")),
      groupAddress6(QStringLiteral("ff12::2115"))
{
    statusLabel = new QLabel(tr("Listening for multicast messages on both IPv4 and IPv6"));
    auto quitButton = new QPushButton(tr("&Quit"));

    auto buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Multicast Receiver"));

    udpSocket4.bind(QHostAddress::AnyIPv4, 45454, QUdpSocket::ShareAddress);
    udpSocket4.joinMulticastGroup(groupAddress4);

    if (!udpSocket6.bind(QHostAddress::AnyIPv6, 45454, QUdpSocket::ShareAddress) ||
            !udpSocket6.joinMulticastGroup(groupAddress6))
        statusLabel->setText(tr("Listening for multicast messages on IPv4 only"));

    connect(&udpSocket4, &QUdpSocket::readyRead,
            this, &Receiver::processPendingDatagrams);
    connect(&udpSocket6, &QUdpSocket::readyRead,
            this, &Receiver::processPendingDatagrams);
    connect(quitButton, &QPushButton::clicked,
            qApp, &QCoreApplication::quit);
}

void Receiver::processPendingDatagrams()
{
    QByteArray datagram;

    // using QUdpSocket::readDatagram (API since Qt 4)
    while (udpSocket4.hasPendingDatagrams()) {
        datagram.resize(qsizetype(udpSocket4.pendingDatagramSize()));
        udpSocket4.readDatagram(datagram.data(), datagram.size());
        statusLabel->setText(tr("Received IPv4 datagram: \"%1\"")
                             .arg(datagram.constData()));
    }

    // using QUdpSocket::receiveDatagram (API since Qt 5.8)
    while (udpSocket6.hasPendingDatagrams()) {
        QNetworkDatagram dgram = udpSocket6.receiveDatagram();
        statusLabel->setText(statusLabel->text() +
                             tr("\nReceived IPv6 datagram from [%2]:%3: \"%1\"")
                             .arg(dgram.data().constData(), dgram.senderAddress().toString())
                             .arg(dgram.senderPort()));
    }
}
