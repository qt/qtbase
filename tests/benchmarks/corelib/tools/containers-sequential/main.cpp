/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

// Embedded devices typically have limited memory
#if defined(Q_OS_WINCE)
#  define LARGE_MAX_SIZE 2000
#else
#  define LARGE_MAX_SIZE 20000
#endif

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
        QTest::newRow(QByteArray("std::vector-int--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("QVector-int--" + sizeString).constData()) << false << size;
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
        QTest::newRow(QByteArray("std::vector-Large--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("QVector-Large--" + sizeString).constData()) << false << size;
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

void tst_vector_vs_std::lookup_int_data()
{
    QTest::addColumn<bool>("useStd");
    QTest::addColumn<int>("size");

    for (int size = 10; size < 20000; size += 100) {
        const QByteArray sizeString = QByteArray::number(size);
        QTest::newRow(QByteArray("std::vector-int--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("QVector-int--" + sizeString).constData()) << false << size;
    }
}

void tst_vector_vs_std::lookup_int()
{
    QFETCH(bool, useStd);
    QFETCH(int, size);

    if (useStd)
        useCases_stdvector_int->lookup(size);
    else
        useCases_QVector_int->lookup(size);
}

void tst_vector_vs_std::lookup_Large_data()
{
    QTest::addColumn<bool>("useStd");
    QTest::addColumn<int>("size");

    for (int size = 10; size < LARGE_MAX_SIZE; size += 100) {
        const QByteArray sizeString = QByteArray::number(size);
        QTest::newRow(QByteArray("std::vector-Large--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("QVector-Large--" + sizeString).constData()) << false << size;
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
