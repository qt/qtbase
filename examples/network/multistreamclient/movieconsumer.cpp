// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "movieconsumer.h"
#include <QLabel>
#include <QDataStream>
#include <QImage>
#include <QPixmap>

MovieConsumer::MovieConsumer(QObject *parent)
    : Consumer(parent)
{
    label = new QLabel;
    label->setFrameStyle(QFrame::Box | QFrame::Raised);
    label->setFixedSize(128 + label->frameWidth() * 2,
                        64 + label->frameWidth() * 2);
}

QWidget *MovieConsumer::widget()
{
    return label;
}

void MovieConsumer::readDatagram(const QByteArray &ba)
{
    QDataStream ds(ba);
    QImage image;

    ds >> image;
    label->setPixmap(QPixmap::fromImage(image));
}

void MovieConsumer::serverDisconnected()
{
    label->setPixmap(QPixmap());
}
