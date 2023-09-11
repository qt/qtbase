// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore>

#include <stdio.h>
#include <stdlib.h>

//! [0]
constexpr int DataSize = 100000;

constexpr int BufferSize = 8192;
char buffer[BufferSize];

QSemaphore freeBytes(BufferSize);
QSemaphore usedBytes;
//! [0]

//! [1]
class Producer : public QThread
//! [1] //! [2]
{
public:
    void run() override
    {
        for (int i = 0; i < DataSize; ++i) {
            freeBytes.acquire();
            buffer[i % BufferSize] = "ACGT"[QRandomGenerator::global()->bounded(4)];
            usedBytes.release();
        }
    }
};
//! [2]

//! [3]
class Consumer : public QThread
//! [3] //! [4]
{
public:
    void run() override
    {
        for (int i = 0; i < DataSize; ++i) {
            usedBytes.acquire();
            fprintf(stderr, "%c", buffer[i % BufferSize]);
            freeBytes.release();
        }
        fprintf(stderr, "\n");
    }
};
//! [4]

//! [5]
int main(int argc, char *argv[])
//! [5] //! [6]
{
    QCoreApplication app(argc, argv);
    Producer producer;
    Consumer consumer;
    producer.start();
    consumer.start();
    producer.wait();
    consumer.wait();
    return 0;
}
//! [6]
