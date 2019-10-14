/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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

#include <QtCore>
#include <QtWidgets>
#include <qmath.h>

#define WORLD_SIZE 8
int world_map[WORLD_SIZE][WORLD_SIZE] = {
    { 1, 1, 1, 1, 6, 1, 1, 1 },
    { 1, 0, 0, 1, 0, 0, 0, 7 },
    { 1, 1, 0, 1, 0, 1, 1, 1 },
    { 6, 0, 0, 0, 0, 0, 0, 3 },
    { 1, 8, 8, 0, 8, 0, 8, 1 },
    { 2, 2, 0, 0, 8, 8, 7, 1 },
    { 3, 0, 0, 0, 0, 0, 0, 5 },
    { 2, 2, 2, 2, 7, 4, 4, 4 },
};

#define TEXTURE_SIZE 64
#define TEXTURE_BLOCK (TEXTURE_SIZE * TEXTURE_SIZE)

class Raycasting: public QWidget
{
public:
    Raycasting(QWidget *parent = 0)
            : QWidget(parent)
            , angle(0.5)
            , playerPos(1.5, 1.5)
            , angleDelta(0)
            , moveDelta(0)
            , touchDevice(false) {

        // http://www.areyep.com/RIPandMCS-TextureLibrary.html
        textureImg.load(":/textures.png");
        textureImg = textureImg.convertToFormat(QImage::Format_ARGB32);
        Q_ASSERT(textureImg.width() == TEXTURE_SIZE * 2);
        Q_ASSERT(textureImg.bytesPerLine() == 4 * TEXTURE_SIZE * 2);
        textureCount = textureImg.height() / TEXTURE_SIZE;

        watch.start();
        ticker.start(25, this);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setMouseTracking(false);
    }

    void updatePlayer() {
        int interval = qBound(20ll, watch.elapsed(), 250ll);
        watch.start();
        angle += angleDelta * interval / 1000;
        qreal step = moveDelta * interval / 1000;
        qreal dx = cos(angle) * step;
        qreal dy = sin(angle) * step;
        QPointF pos = playerPos + 3 * QPointF(dx, dy);
        int xi = static_cast<int>(pos.x());
        int yi = static_cast<int>(pos.y());
        if (world_map[yi][xi] == 0)
            playerPos = playerPos + QPointF(dx, dy);
    }

    void showFps() {
        static QElapsedTimer frameTick;
        static int totalFrame = 0;
        if (!(totalFrame & 31)) {
            const qint64 elapsed = frameTick.elapsed();
            frameTick.start();
            int fps = 32 * 1000 / (1 + elapsed);
            setWindowTitle(QString("Raycasting (%1 FPS)").arg(fps));
        }
        totalFrame++;
    }

    void render() {

        // setup the screen surface
        if (buffer.size() != bufferSize)
            buffer = QImage(bufferSize, QImage::Format_ARGB32);
        int bufw = buffer.width();
        int bufh = buffer.height();
        if (bufw <= 0 || bufh <= 0)
            return;

        // we intentionally cheat here, to avoid detach
        const uchar *ptr = buffer.bits();
        QRgb *start = (QRgb*)(ptr);
        QRgb stride = buffer.bytesPerLine() / 4;
        QRgb *finish = start + stride * bufh;

        // prepare the texture pointer
        const uchar *src = textureImg.bits();
        const QRgb *texsrc = reinterpret_cast<const QRgb*>(src);

        // cast all rays here
        qreal sina = sin(angle);
        qreal cosa = cos(angle);
        qreal u = cosa - sina;
        qreal v = sina + cosa;
        qreal du = 2 * sina / bufw;
        qreal dv = -2 * cosa / bufw;

        for (int ray = 0; ray < bufw; ++ray, u += du, v += dv) {
            // every time this ray advances 'u' units in x direction,
            // it also advanced 'v' units in y direction
            qreal uu = (u < 0) ? -u : u;
            qreal vv = (v < 0) ? -v : v;
            qreal duu = 1 / uu;
            qreal dvv = 1 / vv;
            int stepx = (u < 0) ? -1 : 1;
            int stepy = (v < 0) ? -1 : 1;

            // the cell in the map that we need to check
            qreal px = playerPos.x();
            qreal py = playerPos.y();
            int mapx = static_cast<int>(px);
            int mapy = static_cast<int>(py);

            // the position and texture for the hit
            int texture = 0;
            qreal hitdist = 0.1;
            qreal texofs = 0;
            bool dark = false;

            // first hit at constant x and constant y lines
            qreal distx = (u > 0) ? (mapx + 1 - px) * duu : (px - mapx) * duu;
            qreal disty = (v > 0) ? (mapy + 1 - py) * dvv : (py - mapy) * dvv;

            // loop until we hit something
            while (texture <= 0) {
                if (distx > disty) {
                    // shorter distance to a hit in constant y line
                    hitdist = disty;
                    disty += dvv;
                    mapy += stepy;
                    texture = world_map[mapy][mapx];
                    if (texture > 0) {
                        dark = true;
                        if (stepy > 0) {
                            qreal ofs = px + u * (mapy - py) / v;
                            texofs = ofs - floor(ofs);
                        } else {
                            qreal ofs = px + u * (mapy + 1 - py) / v;
                            texofs = ofs - floor(ofs);
                        }
                    }
                } else {
                    // shorter distance to a hit in constant x line
                    hitdist = distx;
                    distx += duu;
                    mapx += stepx;
                    texture = world_map[mapy][mapx];
                    if (texture > 0) {
                        if (stepx > 0) {
                            qreal ofs = py + v * (mapx - px) / u;
                            texofs = ofs - floor(ofs);
                        } else {
                            qreal ofs = py + v * (mapx + 1 - px) / u;
                            texofs = ceil(ofs) - ofs;
                        }
                    }
                }
            }

            // get the texture, note that the texture image
            // has two textures horizontally, "normal" vs "dark"
            int col = static_cast<int>(texofs * TEXTURE_SIZE);
            col = qBound(0, col, TEXTURE_SIZE - 1);
            texture = (texture - 1) % textureCount;
            const QRgb *tex = texsrc + TEXTURE_BLOCK * texture * 2 +
                              (TEXTURE_SIZE * 2 * col);
            if (dark)
                tex += TEXTURE_SIZE;

            // start from the texture center (horizontally)
            int h = static_cast<int>(bufw / hitdist / 2);
            int dy = (TEXTURE_SIZE << 12) / h;
            int p1 = ((TEXTURE_SIZE / 2) << 12) - dy;
            int p2 = p1 + dy;

            // start from the screen center (vertically)
            // y1 will go up (decrease), y2 will go down (increase)
            int y1 = bufh / 2;
            int y2 = y1 + 1;
            QRgb *pixel1 = start + y1 * stride + ray;
            QRgb *pixel2 = pixel1 + stride;

            // map the texture to the sliver
            while (y1 >= 0 && y2 < bufh && p1 >= 0) {
                *pixel1 = tex[p1 >> 12];
                *pixel2 = tex[p2 >> 12];
                p1 -= dy;
                p2 += dy;
                --y1;
                ++y2;
                pixel1 -= stride;
                pixel2 += stride;
            }

            // ceiling and floor
            for (; pixel1 > start; pixel1 -= stride)
                *pixel1 = qRgb(0, 0, 0);
            for (; pixel2 < finish; pixel2 += stride)
                *pixel2 = qRgb(96, 96, 96);
        }

        update(QRect(QPoint(0, 0), bufferSize));
    }

protected:

    void resizeEvent(QResizeEvent*) {
        touchDevice = false;
        if (touchDevice) {
            if (width() < height()) {
                trackPad = QRect(0, height() / 2, width(), height() / 2);
                centerPad = QPoint(width() / 2, height() * 3 / 4);
                bufferSize = QSize(width(), height() / 2);
            } else {
                trackPad = QRect(width() / 2, 0, width() / 2, height());
                centerPad = QPoint(width() * 3 / 4, height() / 2);
                bufferSize = QSize(width() / 2, height());
            }
        } else {
            trackPad = QRect();
            bufferSize = size();
        }
        update();
   }

    void timerEvent(QTimerEvent*) {
        updatePlayer();
        render();
        showFps();
    }

    void paintEvent(QPaintEvent *event) {
        QPainter p(this);
        p.setCompositionMode(QPainter::CompositionMode_Source);

        p.drawImage(event->rect(), buffer, event->rect());

        if (touchDevice && event->rect().intersects(trackPad)) {
            p.fillRect(trackPad, Qt::white);
            p.setPen(QPen(QColor(224, 224, 224), 6));
            int rad = qMin(trackPad.width(), trackPad.height()) * 0.3;
            p.drawEllipse(centerPad, rad, rad);

            p.setPen(Qt::NoPen);
            p.setBrush(Qt::gray);

            QPolygon poly;
            poly << QPoint(-30, 0);
            poly << QPoint(0, -40);
            poly << QPoint(30, 0);

            p.translate(centerPad);
            for (int i = 0; i < 4; ++i) {
                p.rotate(90);
                p.translate(0, 20 - rad);
                p.drawPolygon(poly);
                p.translate(0, rad - 20);
            }
        }

        p.end();
    }

    void keyPressEvent(QKeyEvent *event) {
        event->accept();
        if (event->key() == Qt::Key_Left)
            angleDelta = 1.3 * M_PI;
        if (event->key() == Qt::Key_Right)
            angleDelta = -1.3 * M_PI;
        if (event->key() == Qt::Key_Up)
            moveDelta = 2.5;
        if (event->key() == Qt::Key_Down)
            moveDelta = -2.5;
    }

    void keyReleaseEvent(QKeyEvent *event) {
        event->accept();
        if (event->key() == Qt::Key_Left)
            angleDelta = (angleDelta > 0) ? 0 : angleDelta;
        if (event->key() == Qt::Key_Right)
            angleDelta = (angleDelta < 0) ? 0 : angleDelta;
        if (event->key() == Qt::Key_Up)
            moveDelta = (moveDelta > 0) ? 0 : moveDelta;
        if (event->key() == Qt::Key_Down)
            moveDelta = (moveDelta < 0) ? 0 : moveDelta;
    }

    void mousePressEvent(QMouseEvent *event) {
        qreal dx = centerPad.x() - event->pos().x();
        qreal dy = centerPad.y() - event->pos().y();
        angleDelta = dx * 2 * M_PI / width();
        moveDelta = dy * 10 / height();
    }

    void mouseMoveEvent(QMouseEvent *event) {
        qreal dx = centerPad.x() - event->pos().x();
        qreal dy = centerPad.y() - event->pos().y();
        angleDelta = dx * 2 * M_PI / width();
        moveDelta = dy * 10 / height();
    }

    void mouseReleaseEvent(QMouseEvent*) {
        angleDelta = 0;
        moveDelta = 0;
    }

private:
    QElapsedTimer watch;
    QBasicTimer ticker;
    QImage buffer;
    qreal angle;
    QPointF playerPos;
    qreal angleDelta;
    qreal moveDelta;
    QImage textureImg;
    int textureCount;
    bool touchDevice;
    QRect trackPad;
    QPoint centerPad;
    QSize bufferSize;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Raycasting w;
    w.setWindowTitle("Raycasting");
    w.resize(640, 480);
    w.show();

    return app.exec();
}
