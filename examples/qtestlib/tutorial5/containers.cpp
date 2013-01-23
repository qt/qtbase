/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
// This file contains benchmarks for comparing QVector against std::vector

#include <QtCore>
#include <QVector>
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

// This subclass implements the use cases using QVector as efficiently as possible.
template <typename T>
class UseCases_QVector : public UseCases<T>
{
    void insert(int size)
    {
        QVector<T> v;
        T t;
        QBENCHMARK {
            for (int i = 0; i < size; ++i)
                v.append(t);
        }
    }

    void lookup(int size)
    {
        QVector<T> v;

        T t;
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
    void insert(int size)
    {
        std::vector<T> v;
        T t;
        QBENCHMARK {
            for (int i = 0; i < size; ++i)
                v.push_back(t);
        }
    }

    void lookup(int size)
    {
        std::vector<T> v;

        T t;
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

// Symbian devices typically have limited memory
#  define LARGE_MAX_SIZE 20000

class tst_vector_vs_std : public QObject
{
    Q_OBJECT
public:
    tst_vector_vs_std()
    {
        useCases_QVector_int = new UseCases_QVector<int>;
        useCases_stdvector_int = new UseCases_stdvector<int>;

        useCases_QVector_Large = new UseCases_QVector<Large>;
        useCases_stdvector_Large = new UseCases_stdvector<Large>;
    }

private:
    UseCases<int> *useCases_QVector_int;
    UseCases<int> *useCases_stdvector_int;
    UseCases<Large> *useCases_QVector_Large;
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
        QTest::newRow(("std::vector-int--" + sizeString).constData()) << true << size;
        QTest::newRow(("QVector-int--" + sizeString).constData()) << false << size;
    }
}

void tst_vector_vs_std::insert_int()
{
    QFETCH(bool, useStd);
    QFETCH(int, size);

    if (useStd)
        useCases_stdvector_int->insert(size);
    else
        useCases_QVector_int->insert(size);
}

void tst_vector_vs_std::insert_Large_data()
{
    QTest::addColumn<bool>("useStd");
    QTest::addColumn<int>("size");

    for (int size = 10; size < LARGE_MAX_SIZE; size += 100) {
        const QByteArray sizeString = QByteArray::number(size);
        QTest::newRow(("std::vector-Large--" + sizeString).constData()) << true << size;
        QTest::newRow(("QVector-Large--" + sizeString).constData()) << false << size;
    }
}

void tst_vector_vs_std::insert_Large()
{
    QFETCH(bool, useStd);
    QFETCH(int, size);

    if (useStd)
        useCases_stdvector_Large->insert(size);
    else
        useCases_QVector_Large->insert(size);
}

//! [1]
void tst_vector_vs_std::lookup_int_data()
{
    QTest::addColumn<bool>("useStd");
    QTest::addColumn<int>("size");

    for (int size = 10; size < 20000; size += 100) {
        const QByteArray sizeString = QByteArray::number(size);
        QTest::newRow(("std::vector-int--" + sizeString).constData()) << true << size;
        QTest::newRow(("QVector-int--" + sizeString).constData()) << false << size;
    }
}
//! [1]

//! [2]
void tst_vector_vs_std::lookup_int()
{
    QFETCH(bool, useStd);
    QFETCH(int, size);

    if (useStd)
        useCases_stdvector_int->lookup(size); // Create a std::vector and run the benchmark.
    else
        useCases_QVector_int->lookup(size); // Create a QVector and run the benchmark.
}
//! [2]

void tst_vector_vs_std::lookup_Large_data()
{
    QTest::addColumn<bool>("useStd");
    QTest::addColumn<int>("size");

    for (int size = 10; size < LARGE_MAX_SIZE; size += 100) {
        const QByteArray sizeString = QByteArray::number(size);
        QTest::newRow(("std::vector-Large--" + sizeString).constData()) << true << size;
        QTest::newRow(("QVector-Large--" + sizeString).constData()) << false << size;
    }
}

void tst_vector_vs_std::lookup_Large()
{
    QFETCH(bool, useStd);
    QFETCH(int, size);

    if (useStd)
        useCases_stdvector_Large->lookup(size);
    else
        useCases_QVector_Large->lookup(size);
}

QTEST_MAIN(tst_vector_vs_std)
#include "main.moc"
