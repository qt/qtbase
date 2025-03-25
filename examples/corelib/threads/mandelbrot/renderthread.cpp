// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "renderthread.h"

#include <QImage>
#include <QElapsedTimer>
#include <QTextStream>

#include <cmath>

int RenderThread::numPasses = 8;

//! [0]
RenderThread::RenderThread(QObject *parent)
    : QThread(parent)
{
    for (int i = 0; i < ColormapSize; ++i)
        colormap[i] = rgbFromWaveLength(380.0 + (i * 400.0 / ColormapSize));
}
//! [0]

//! [1]
RenderThread::~RenderThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}
//! [1]

//! [2]
void RenderThread::render(double centerX, double centerY, double scaleFactor,
                          QSize resultSize, double devicePixelRatio)
{
    QMutexLocker locker(&mutex);

    this->centerX = centerX;
    this->centerY = centerY;
    this->scaleFactor = scaleFactor;
    this->devicePixelRatio = devicePixelRatio;
    this->resultSize = resultSize;

    if (!isRunning()) {
        start(LowPriority);
    } else {
        restart = true;
        condition.wakeOne();
    }
}
//! [2]

//! [3]
void RenderThread::run()
{
    QElapsedTimer timer;
    forever {
        mutex.lock();
        const double devicePixelRatio = this->devicePixelRatio;
        const QSize resultSize = this->resultSize * devicePixelRatio;
        const double requestedScaleFactor = this->scaleFactor;
        const double scaleFactor = requestedScaleFactor / devicePixelRatio;
        const double centerX = this->centerX;
        const double centerY = this->centerY;
        mutex.unlock();
//! [3]

//! [4]
        const int halfWidth = resultSize.width() / 2;
//! [4] //! [5]
        const int halfHeight = resultSize.height() / 2;
        QImage image(resultSize, QImage::Format_RGB32);
        image.setDevicePixelRatio(devicePixelRatio);

        int pass = 0;
        while (pass < numPasses) {
            const int MaxIterations = (1 << (2 * pass + 6)) + 32;
            constexpr int Limit = 4;
            bool allBlack = true;

            timer.start();

            for (int y = -halfHeight; y < halfHeight; ++y) {
                if (restart)
                    break;
                if (abort)
                    return;

                auto scanLine =
                        reinterpret_cast<uint *>(image.scanLine(y + halfHeight));
                const double ay = centerY + (y * scaleFactor);

                for (int x = -halfWidth; x < halfWidth; ++x) {
                    const double ax = centerX + (x * scaleFactor);
                    double a1 = ax;
                    double b1 = ay;
                    int numIterations = 0;

                    do {
                        ++numIterations;
                        const double a2 = (a1 * a1) - (b1 * b1) + ax;
                        const double b2 = (2 * a1 * b1) + ay;
                        if ((a2 * a2) + (b2 * b2) > Limit)
                            break;

                        ++numIterations;
                        a1 = (a2 * a2) - (b2 * b2) + ax;
                        b1 = (2 * a2 * b2) + ay;
                        if ((a1 * a1) + (b1 * b1) > Limit)
                            break;
                    } while (numIterations < MaxIterations);

                    if (numIterations < MaxIterations) {
                        *scanLine++ = colormap[numIterations % ColormapSize];
                        allBlack = false;
                    } else {
                        *scanLine++ = qRgb(0, 0, 0);
                    }
                }
            }

            if (allBlack && pass == 0) {
                pass = 4;
            } else {
                if (!restart) {
                    QString message;
                    QTextStream str(&message);
                    str << " Pass " << (pass + 1) << '/' << numPasses
                        << ", max iterations: " << MaxIterations << ", time: ";
                    const auto elapsed = timer.elapsed();
                    if (elapsed > 2000)
                        str << (elapsed / 1000) << 's';
                    else
                        str << elapsed << "ms";
                    image.setText(infoKey(), message);

                    emit renderedImage(image, requestedScaleFactor);
                }
//! [5] //! [6]
                ++pass;
            }
//! [6] //! [7]
        }
//! [7]

//! [8]
        mutex.lock();
//! [8] //! [9]
        if (!restart)
            condition.wait(&mutex);
        restart = false;
        mutex.unlock();
    }
}
//! [9]

//! [10]
uint RenderThread::rgbFromWaveLength(double wave)
{
    double r = 0;
    double g = 0;
    double b = 0;

    if (wave >= 380.0 && wave <= 440.0) {
        r = -1.0 * (wave - 440.0) / (440.0 - 380.0);
        b = 1.0;
    } else if (wave >= 440.0 && wave <= 490.0) {
        g = (wave - 440.0) / (490.0 - 440.0);
        b = 1.0;
    } else if (wave >= 490.0 && wave <= 510.0) {
        g = 1.0;
        b = -1.0 * (wave - 510.0) / (510.0 - 490.0);
    } else if (wave >= 510.0 && wave <= 580.0) {
        r = (wave - 510.0) / (580.0 - 510.0);
        g = 1.0;
    } else if (wave >= 580.0 && wave <= 645.0) {
        r = 1.0;
        g = -1.0 * (wave - 645.0) / (645.0 - 580.0);
    } else if (wave >= 645.0 && wave <= 780.0) {
        r = 1.0;
    }

    double s = 1.0;
    if (wave > 700.0)
        s = 0.3 + 0.7 * (780.0 - wave) / (780.0 - 700.0);
    else if (wave <  420.0)
        s = 0.3 + 0.7 * (wave - 380.0) / (420.0 - 380.0);

    r = std::pow(r * s, 0.8);
    g = std::pow(g * s, 0.8);
    b = std::pow(b * s, 0.8);
    return qRgb(int(r * 255), int(g * 255), int(b * 255));
}
//! [10]
