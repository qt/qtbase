// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include <QtNetwork>
#include <QtCore>

#include "sender.h"

Sender::Sender(QWidget *parent)
    : QWidget(parent)
{
    statusLabel = new QLabel(tr("Ready to broadcast datagrams on port 45454"));
    statusLabel->setWordWrap(true);

    startButton = new QPushButton(tr("&Start"));
    auto quitButton = new QPushButton(tr("&Quit"));

    auto buttonBox = new QDialogButtonBox;
    buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

//! [0]
    udpSocket = new QUdpSocket(this);
//! [0]

    connect(startButton, &QPushButton::clicked, this, &Sender::startBroadcasting);
    connect(quitButton, &QPushButton::clicked, qApp, &QCoreApplication::quit);
    connect(&timer, &QTimer::timeout, this, &Sender::broadcastDatagram);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle(tr("Broadcast Sender"));
}

void Sender::startBroadcasting()
{
    startButton->setEnabled(false);
    timer.start(1000);
}

void Sender::broadcastDatagram()
{
    statusLabel->setText(tr("Now broadcasting datagram %1").arg(messageNo));
//! [1]
    QByteArray datagram = "Broadcast message " + QByteArray::number(messageNo);
    udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, 45454);
//! [1]
    ++messageNo;
}
