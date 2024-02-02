// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// This file contains benchmarks for comparing QList against std::vector

#include <QtCore>
#include <QList>
#include <vector>

#include <qtest.h>

template <typename T> // T is the item type
class UseCases {
public:
    virtual ~UseCases() {}

    // Use case: Insert \a size items into the vector.
    virtual void insert(int size) = 0;

    // Use case: Lookup \a size items from the vector.
    virtual void lookup(int size) = 0;
};

template <typename T>
T * f(T *ts) // dummy function to prevent code from being optimized away by the compiler
{
    return ts;
}

// This subclass implements the use cases using QList as efficiently as possible.
template<typename T>
class UseCases_QList : public UseCases<T>
{
    void insert(int size) override
    {
        QList<T> v;
        T t{};
        QBENCHMARK {
            for (int i = 0; i < size; ++i)
                v.append(t);
        }
    }

    void lookup(int size) override
    {
        QList<T> v;

        T t{};
        for (int i = 0; i < size; ++i)
            v.append(t);

        T *ts = new T[size];
        QBENCHMARK {
            for (int i = 0; i < size; ++i)
                ts[i] = v.value(i);
        }
        f<T>(ts);
        delete[] ts;
    }
};

// This subclass implements the use cases using std::vector as efficiently as possible.
template <typename T>
class UseCases_stdvector : public UseCases<T>
{
    void insert(int size) override
    {
        std::vector<T> v;
        T t = {};
        QBENCHMARK {
            for (int i = 0; i < size; ++i)
                v.push_back(t);
        }
    }

    void lookup(int size) override
    {
        std::vector<T> v;

        T t = {};
        for (int i = 0; i < size; ++i)
            v.push_back(t);

        T *ts = new T[size];
        QBENCHMARK {
            for (int i = 0; i < size; ++i)
                ts[i] = v[i];
        }
        f<T>(ts);
        delete[] ts;
    }
};

struct Large { // A "large" item type
    int x[1000];
};

#define LARGE_MAX_SIZE 20000

class tst_vector_vs_std : public QObject
{
    Q_OBJECT
public:
    tst_vector_vs_std()
    {
        useCases_QList_int = new UseCases_QList<int>;
        useCases_stdvector_int = new UseCases_stdvector<int>;

        useCases_QList_Large = new UseCases_QList<Large>;
        useCases_stdvector_Large = new UseCases_stdvector<Large>;
    }

private:
    UseCases<int> *useCases_QList_int;
    UseCases<int> *useCases_stdvector_int;
    UseCases<Large> *useCases_QList_Large;
    UseCases<Large> *useCases_stdvector_Large;

private slots:
    void insert_int_data();
    void insert_int();
    void insert_Large_data();
    void insert_Large();
    void lookup_int_data();
    void lookup_int();
    void lookup_Large_data();
    void lookup_Large();
};

void tst_vector_vs_std::insert_int_data()
{
    QTest::addColumn<bool>("useStd");
    QTest::addColumn<int>("size");

    for (int size = 10; size < 20000; size += 100) {
        const QByteArray sizeString = QByteArray::number(size);
        QTest::newRow(QByteArray("std::vector-int--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("QList-int--" + sizeString).constData()) << false << size;
    }
}

void tst_vector_vs_std::insert_int()
{
    QFETCH(bool, useStd);
    QFETCH(int, size);

    if (useStd)
        useCases_stdvector_int->insert(size);
    else
        useCases_QList_int->insert(size);
}

void tst_vector_vs_std::insert_Large_data()
{
    QTest::addColumn<bool>("useStd");
    QTest::addColumn<int>("size");

    for (int size = 10; size < LARGE_MAX_SIZE; size += 100) {
        const QByteArray sizeString = QByteArray::number(size);
        QTest::newRow(QByteArray("std::vector-Large--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("QList-Large--" + sizeString).constData()) << false << size;
    }
}

void tst_vector_vs_std::insert_Large()
{
    QFETCH(bool, useStd);
    QFETCH(int, size);

    if (useStd)
        useCases_stdvector_Large->insert(size);
    else
        useCases_QList_Large->insert(size);
}

void tst_vector_vs_std::lookup_int_data()
{
    QTest::addColumn<bool>("useStd");
    QTest::addColumn<int>("size");

    for (int size = 10; size < 20000; size += 100) {
        const QByteArray sizeString = QByteArray::number(size);
        QTest::newRow(QByteArray("std::vector-int--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("QList-int--" + sizeString).constData()) << false << size;
    }
}

void tst_vector_vs_std::lookup_int()
{
    QFETCH(bool, useStd);
    QFETCH(int, size);

    if (useStd)
        useCases_stdvector_int->lookup(size);
    else
        useCases_QList_int->lookup(size);
}

void tst_vector_vs_std::lookup_Large_data()
{
    QTest::addColumn<bool>("useStd");
    QTest::addColumn<int>("size");

    for (int size = 10; size < LARGE_MAX_SIZE; size += 100) {
        const QByteArray sizeString = QByteArray::number(size);
        QTest::newRow(QByteArray("std::vector-Large--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("QList-Large--" + sizeString).constData()) << false << size;
    }
}

void tst_vector_vs_std::lookup_Large()
{
    QFETCH(bool, useStd);
    QFETCH(int, size);

    if (useStd)
        useCases_stdvector_Large->lookup(size);
    else
        useCases_QList_Large->lookup(size);
}

QTEST_MAIN(tst_vector_vs_std)

#include "tst_bench_containers_sequential.moc"
