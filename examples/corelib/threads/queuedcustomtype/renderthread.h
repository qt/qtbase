// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QImage>
#include <QMutex>
#include <QThread>

class Block;

//! [RenderThread class definition]
class RenderThread : public QThread
{
    Q_OBJECT

public:
    RenderThread(QObject *parent = nullptr);
    ~RenderThread();

    void processImage(const QImage &image);
    void stopProcess();

signals:
    void sendBlock(const Block &block);

protected:
    void run();

private:
    bool m_abort;
    QImage m_image;
    QMutex mutex;
};
//! [RenderThread class definition]

#endif
