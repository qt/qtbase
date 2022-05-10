// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TIMECONSUMER_H
#define TIMECONSUMER_H

#include "consumer.h"
#include <QTime>

QT_BEGIN_NAMESPACE
class QLCDNumber;
QT_END_NAMESPACE

class TimeConsumer : public Consumer
{
    Q_OBJECT
public:
    explicit TimeConsumer(QObject *parent = nullptr);

    QWidget *widget() override;
    void readDatagram(const QByteArray &ba) override;
    void serverDisconnected() override;

private slots:
    void timerTick();

private:
    QTime lastTime;
    QLCDNumber *lcdNumber;
};

#endif
