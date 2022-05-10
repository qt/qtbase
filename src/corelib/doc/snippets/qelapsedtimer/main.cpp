// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtCore>

void slowOperation1()
{
    static char buf[256];
    for (int i = 0; i < (1<<20); ++i)
        buf[i % sizeof buf] = i;
}

void slowOperation2(int) { slowOperation1(); }

void startExample()
{
//![0]
    QElapsedTimer timer;
    timer.start();

    slowOperation1();

    qDebug() << "The slow operation took" << timer.elapsed() << "milliseconds";
//![0]
}

//![1]
void executeSlowOperations(int timeout)
{
    QElapsedTimer timer;
    timer.start();
    slowOperation1();

    int remainingTime = timeout - timer.elapsed();
    if (remainingTime > 0)
        slowOperation2(remainingTime);
}
//![1]

//![2]
void executeOperationsForTime(int ms)
{
    QElapsedTimer timer;
    timer.start();

    while (!timer.hasExpired(ms))
        slowOperation1();
}
//![2]

int restartExample()
{
//![3]
    QElapsedTimer timer;

    int count = 1;
    timer.start();
    do {
        count *= 2;
        slowOperation2(count);
    } while (timer.restart() < 250);

    return count;
//![3]
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    startExample();
    restartExample();
    executeSlowOperations(5);
    executeOperationsForTime(5);
}
