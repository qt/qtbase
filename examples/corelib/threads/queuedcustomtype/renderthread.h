// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QImage>
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

signals:
    void sendBlock(const Block &block);

protected:
    void run();

private:
    QImage m_image;
};
//! [RenderThread class definition]

#endif
