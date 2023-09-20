// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "block.h"
#include "window.h"

#include <QApplication>
#include <QBrush>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRect>

QImage createImage(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB16);
    QPainter painter;
    QPen pen;
    pen.setStyle(Qt::NoPen);
    QBrush brush(Qt::blue);

    painter.begin(&image);
    painter.fillRect(image.rect(), brush);
    brush.setColor(Qt::white);
    painter.setPen(pen);
    painter.setBrush(brush);

    static const QPointF points1[3] = {
        QPointF(4, 4),
        QPointF(7, 4),
        QPointF(5.5, 1)
    };

    static const QPointF points2[3] = {
        QPointF(1, 4),
        QPointF(7, 4),
        QPointF(10, 10)
    };

    static const QPointF points3[3] = {
        QPointF(4, 4),
        QPointF(10, 4),
        QPointF(1, 10)
    };

    painter.setWindow(0, 0, 10, 10);

    int x = 0;
    int y = 0;
    const int starWidth = image.width()/3;
    const int starHeight = image.height()/3;

    QRect rect(x, y, starWidth, starHeight);

    for (int i = 0; i < 9; ++i) {

        painter.setViewport(rect);
        painter.drawPolygon(points1, 3);
        painter.drawPolygon(points2, 3);
        painter.drawPolygon(points3, 3);

        if (i % 3 == 2) {
            y = y + starHeight;
            rect.moveTop(y);

            x = 0;
            rect.moveLeft(x);

        } else {
            x = x + starWidth;
            rect.moveLeft(x);
        }
    }

    painter.end();
    return image;
}

//! [main function] //! [main start]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
//! [main start] //! [register meta-type for queued communications]
    qRegisterMetaType<Block>();
//! [register meta-type for queued communications]

    Window window;
    window.show();

    window.loadImage(createImage(256, 256));
//! [main finish]
    return app.exec();
}
//! [main finish] //! [main function]
