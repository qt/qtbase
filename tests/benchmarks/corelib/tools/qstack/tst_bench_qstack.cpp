// Copyright (C) 2015 Robin Burchell <robin.burchell@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QStack>
#include <QDebug>
#include <QTest>

#include <vector>

class tst_QStack: public QObject
{
    Q_OBJECT

private slots:
    void qstack_push();
    void qstack_pop();
    void qstack_pushpopone();
};

const int N = 1000000;

void tst_QStack::qstack_push()
{
    QStack<int> v;
    QBENCHMARK {
        for (int i = 0; i != N; ++i)
            v.push(i);
        v = QStack<int>();
    }
}

void tst_QStack::qstack_pop()
{
    QStack<int> v;
    for (int i = 0; i != N; ++i)
        v.push(i);

    QBENCHMARK {
        QStack<int> v2 = v;
        for (int i = 0; i != N; ++i) {
            v2.pop();
        }
    }
}

void tst_QStack::qstack_pushpopone()
{
    QBENCHMARK {
        QStack<int> v;
        for (int i = 0; i != N; ++i) {
            v.push(0);
            v.pop();
        }
    }
}

QTEST_MAIN(tst_QStack)

#include "tst_bench_qstack.moc"
