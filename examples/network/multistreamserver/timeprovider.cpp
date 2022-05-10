// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "timeprovider.h"
#include <QDataStream>
#include <QTime>
#include <QByteArray>

TimeProvider::TimeProvider(QObject *parent)
    : Provider(parent)
{
}

void TimeProvider::readDatagram(QSctpSocket &from, const QByteArray &ba)
{
    QDataStream in_ds(ba);
    QTime curTime = QTime::currentTime();
    QTime clientTime;

    in_ds >> clientTime;
    // Send response only if a displayed part of the time is changed.
    // So, sub-second differences are ignored.
    if (!clientTime.isValid() || curTime.second() != clientTime.second()
        || curTime.minute() != clientTime.minute()
        || curTime.hour() != clientTime.hour()) {
        QByteArray buf;
        QDataStream out_ds(&buf, QIODeviceBase::WriteOnly);

        out_ds << curTime;
        emit writeDatagram(&from, buf);
    }
}
