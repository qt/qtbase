/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QPainter>
#include <QTime>
#include "block.h"
#include "window.h"

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
    int starWidth = image.width()/3;
    int starHeight = image.height()/3;

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
    qsrand(QTime::currentTime().elapsed());

    Window window;
    window.show();

    window.loadImage(createImage(256, 256));
//! [main finish]
    return app.exec();
}
//! [main finish] //! [main function]
