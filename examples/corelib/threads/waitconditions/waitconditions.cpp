// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore>

#include <stdio.h>
#include <stdlib.h>

//! [0]
const int DataSize = 100000;

const int BufferSize = 8192;
char buffer[BufferSize];

QWaitCondition bufferNotEmpty;
QWaitCondition bufferNotFull;
QMutex mutex;
int numUsedBytes = 0;
//! [0]

//! [1]
class Producer : public QThread
//! [1] //! [2]
{
public:
    Producer(QObject *parent = NULL) : QThread(parent)
    {
    }

    void run() override
    {
        for (int i = 0; i < DataSize; ++i) {
            mutex.lock();
            if (numUsedBytes == BufferSize)
                bufferNotFull.wait(&mutex);
            mutex.unlock();

            buffer[i % BufferSize] = "ACGT"[QRandomGenerator::global()->bounded(4)];

            mutex.lock();
            ++numUsedBytes;
            bufferNotEmpty.wakeAll();
            mutex.unlock();
        }
    }
};
//! [2]

//! [3]
class Consumer : public QThread
//! [3] //! [4]
{
    Q_OBJECT
public:
    Consumer(QObject *parent = NULL) : QThread(parent)
    {
    }

    void run() override
    {
        for (int i = 0; i < DataSize; ++i) {
            mutex.lock();
            if (numUsedBytes == 0)
                bufferNotEmpty.wait(&mutex);
            mutex.unlock();

            fprintf(stderr, "%c", buffer[i % BufferSize]);

            mutex.lock();
            --numUsedBytes;
            bufferNotFull.wakeAll();
            mutex.unlock();
        }
        fprintf(stderr, "\n");
    }

signals:
    void stringConsumed(const QString &text);
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

#include "waitconditions.moc"
