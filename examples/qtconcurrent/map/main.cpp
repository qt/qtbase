// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QImage>
#include <QList>
#include <QThread>
#include <QDebug>
#include <QGuiApplication>
#include <qtconcurrentmap.h>

#include <functional>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    const int imageCount = 20;

    // Create a list containing imageCount images.
    QList<QImage> images;
    for (int i = 0; i < imageCount; ++i)
        images.append(QImage(1600, 1200, QImage::Format_ARGB32_Premultiplied));

    std::function<QImage(const QImage&)> scale = [](const QImage &image) -> QImage
    {
        qDebug() << "Scaling image in thread" << QThread::currentThread();
        return image.scaled(QSize(100, 100), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    };

    // Use QtConcurrentBlocking::mapped to apply the scale function to all the
    // images in the list.
    QList<QImage> thumbnails = QtConcurrent::blockingMapped(images, scale);

    return 0;
}
