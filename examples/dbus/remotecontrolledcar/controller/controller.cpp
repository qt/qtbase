// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "controller.h"
#include <QGridLayout>
#include <QHBoxLayout>

using org::example::Examples::CarInterface;

Controller::Controller(QWidget *parent) : QWidget(parent)
{
    car = new CarInterface("org.example.CarExample", "/Car", QDBusConnection::sessionBus(), this);

    accelerate = new QPushButton(QIcon(":up.svg"), "", this);
    accelerate->setFixedSize(80, 64);
    accelerate->setIconSize(QSize(44, 44));
    decelerate = new QPushButton(QIcon(":down.svg"), "", this);
    decelerate->setFixedSize(80, 64);
    decelerate->setIconSize(QSize(44, 44));
    left = new QPushButton(QIcon(":left.svg"), "", this);
    left->setFixedSize(64, 80);
    left->setIconSize(QSize(44, 44));
    right = new QPushButton(QIcon(":right.svg"), "", this);
    right->setFixedSize(64, 80);
    right->setIconSize(QSize(44, 44));

    status = new QLabel(this);
    statusSymbol = new QLabel(this);
    statusSymbol->setFixedHeight(24);

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(accelerate, 1, 1);
    layout->addWidget(left, 2, 0);
    layout->addWidget(right, 2, 2);
    layout->addWidget(decelerate, 3, 1);

    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->addWidget(status);
    statusLayout->addWidget(statusSymbol);
    layout->addLayout(statusLayout, 0, 1, 1, 2, Qt::AlignTop | Qt::AlignRight);

    connect(accelerate, &QPushButton::clicked, car, &CarInterface::accelerate);
    connect(decelerate, &QPushButton::clicked, car, &CarInterface::decelerate);
    connect(left, &QPushButton::clicked, car, &CarInterface::turnLeft);
    connect(right, &QPushButton::clicked, car, &CarInterface::turnRight);

    startTimer(1000);
}

void Controller::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    static QPixmap connectedIcon = QPixmap::fromImage(QImage(":connected.svg"));
    static QPixmap connectingIcon = QPixmap::fromImage(QImage(":connecting.svg"));
    status->setText(car->isValid() ? tr("connected") : tr("searching..."));
    statusSymbol->setPixmap(car->isValid() ? connectedIcon : connectingIcon);
}
