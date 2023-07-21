// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE
class QImage;
QT_END_NAMESPACE

//! [0]
class RenderThread : public QThread
{
    Q_OBJECT

public:
    RenderThread(QObject *parent = nullptr);
    ~RenderThread();

    void render(double centerX, double centerY, double scaleFactor, QSize resultSize,
                double devicePixelRatio);

    static void setNumPasses(int n) { numPasses = n; }

    static QString infoKey() { return QStringLiteral("info"); }

signals:
    void renderedImage(const QImage &image, double scaleFactor);

protected:
    void run() override;

private:
    static uint rgbFromWaveLength(double wave);

    QMutex mutex;
    QWaitCondition condition;
    double centerX;
    double centerY;
    double scaleFactor;
    double devicePixelRatio;
    QSize resultSize;
    static int numPasses;
    bool restart = false;
    bool abort = false;

    static constexpr int ColormapSize = 512;
    uint colormap[ColormapSize];
};
//! [0]

#endif // RENDERTHREAD_H
