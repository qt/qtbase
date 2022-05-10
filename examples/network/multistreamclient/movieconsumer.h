// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MOVIECONSUMER_H
#define MOVIECONSUMER_H

#include "consumer.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class MovieConsumer : public Consumer
{
    Q_OBJECT
public:
    explicit MovieConsumer(QObject *parent = nullptr);

    QWidget *widget() override;
    void readDatagram(const QByteArray &ba) override;
    void serverDisconnected() override;

private:
    QLabel *label;
};

#endif
