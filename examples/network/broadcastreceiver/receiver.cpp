// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QLabel>
#include <QPushButton>
#include <QUdpSocket>
#include <QVBoxLayout>
#include <QCoreApplication>

#include "receiver.h"

Receiver::Receiver(QWidget *parent)
    : QWidget(parent)
{
    statusLabel = new QLabel(tr("Listening for broadcasted messages"));
    statusLabel->setWordWrap(true);

    auto quitButton = new QPushButton(tr("&Quit"));

//! [0]
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(45454, QUdpSocket::ShareAddress);
//! [0]

//! [1]
    connect(udpSocket, &QUdpSocket::readyRead,
            this, &Receiver::processPendingDatagrams);
//! [1]
    connect(quitButton, &QPushButton::clicked,
            qApp, &QCoreApplication::quit);

    auto buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Broadcast Receiver"));
}

void Receiver::processPendingDatagrams()
{
    QByteArray datagram;
//! [2]
    while (udpSocket->hasPendingDatagrams()) {
        datagram.resize(int(udpSocket->pendingDatagramSize()));
        udpSocket->readDatagram(datagram.data(), datagram.size());
        statusLabel->setText(tr("Received datagram: \"%1\"")
                             .arg(datagram.constData()));
    }
//! [2]
}
