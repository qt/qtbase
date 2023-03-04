// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtCore>

QThreadStorage<int *> dummy[8];

QThreadStorage<QString *> tls1;

class tst_QThreadStorage : public QObject
{
    Q_OBJECT

public:
    tst_QThreadStorage();
    virtual ~tst_QThreadStorage();

public slots:
    void init();
    void cleanup();

private slots:
    void construct();
    void get();
    void set();
};

tst_QThreadStorage::tst_QThreadStorage()
{
}

tst_QThreadStorage::~tst_QThreadStorage()
{
}

void tst_QThreadStorage::init()
{
    dummy[1].setLocalData(new int(5));
    dummy[2].setLocalData(new int(4));
    dummy[3].setLocalData(new int(3));
    tls1.setLocalData(new QString());
}

void tst_QThreadStorage::cleanup()
{
}

void tst_QThreadStorage::construct()
{
    QBENCHMARK {
        QThreadStorage<int *> ts;
    }
}


void tst_QThreadStorage::get()
{
    QThreadStorage<int *> ts;
    ts.setLocalData(new int(45));

    int count = 0;
    QBENCHMARK {
        int *i = ts.localData();
        count += *i;
    }
    QVERIFY(count > 0);
    ts.setLocalData(0);
}

void tst_QThreadStorage::set()
{
    QThreadStorage<int *> ts;

    int count = 0;
    QBENCHMARK {
        ts.setLocalData(new int(count));
        count++;
    }
    ts.setLocalData(0);
}

QTEST_MAIN(tst_QThreadStorage)

#include "tst_bench_qthreadstorage.moc"
