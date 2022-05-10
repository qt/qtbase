// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "timeconsumer.h"
#include <QLCDNumber>
#include <QString>
#include <QDataStream>
#include <QTimer>

TimeConsumer::TimeConsumer(QObject *parent)
    : Consumer(parent)
{
    lcdNumber = new QLCDNumber(8);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TimeConsumer::timerTick);
    timer->start(100);

    serverDisconnected();
}

QWidget *TimeConsumer::widget()
{
    return lcdNumber;
}

void TimeConsumer::readDatagram(const QByteArray &ba)
{
    QDataStream ds(ba);

    ds >> lastTime;
    lcdNumber->display(lastTime.toString("hh:mm:ss"));
}

void TimeConsumer::timerTick()
{
    QByteArray buf;
    QDataStream ds(&buf, QIODeviceBase::WriteOnly);

    ds << lastTime;
    emit writeDatagram(buf);
}

void TimeConsumer::serverDisconnected()
{
    lcdNumber->display(QLatin1String("--:--:--"));
}
