// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "movieprovider.h"
#include <QMovie>
#include <QString>
#include <QDataStream>

MovieProvider::MovieProvider(QObject *parent)
    : Provider(parent)
{
    movie = new QMovie(this);
    movie->setCacheMode(QMovie::CacheAll);
    movie->setFileName(QLatin1String("animation.gif"));
    connect(movie, &QMovie::frameChanged, this, &MovieProvider::frameChanged);
    movie->start();
}

void MovieProvider::frameChanged()
{
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);

    ds << movie->currentImage();
    emit writeDatagram(0, buf);
}
