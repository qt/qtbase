// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QVector>
#include <QDebug>
#include <QTest>

#include "qrawvector.h"

#include <vector>

/* Using 'extern accumulate' causes some load making the loop resembling a
   'simple inner loop' in 'real' applications.
*/

/* QRawVector::mutateToVector() hacks a semblance of a Qt 5 QVector.

   However, Qt 6's QVector is Qt 6's QList and completely different in internal
   layout.  The QTypedArrayData inside it is also completely rearranged.  Until
   QRawVector can be rewritten to do whatever it's supposed to do in a
   Qt6-compatible way, this test is suppressed, see QTBUG-95061.
*/
#define TEST_RAW 0

// TODO: is this still a thing ? (Dates from g++ 4.3.3 in 2009.)
// For some reason, both 'plain' and '-callgrind' create strange results
// (like varying instruction count for the same assembly code)
// So replace it by a plain loop and measure wall clock time.
//#undef QBENCHMARK
//#define QBENCHMARK for (int j = 0; j != 10000; ++j)

class tst_QVector: public QObject
{
    Q_OBJECT

private slots:
    void calibration();

    // Pure Qt solution
    void qvector_separator() { qWarning() << "QVector results: "; }
    void qvector_const_read_access();
    void qvector_mutable_read_access();
    void qvector_pop_back();
    void qvector_fill_and_return();

    // Purre Standard solution
    void stdvector() { qWarning() << "std::vector results: "; }
    void stdvector_const_read_access();
    void stdvector_mutable_read_access();
    void stdvector_pop_back();
    void stdvector_fill_and_return();

    // Build using std, pass as QVector
    void mixedvector() { qWarning() << "mixed results: "; }
    void mixedvector_fill_and_return();

    // Alternative implementation
    void qrawvector_separator() { qWarning() << "QRawVector results: "; }
    void qrawvector_const_read_access();
    void qrawvector_mutable_read_access();
#if TEST_RAW
    void qrawvector_fill_and_return();
#endif
};

void tst_QVector::calibration()
{
    QVector<double> v(million);
    for (int i = 0; i < million; ++i)
        v[i] = i;
    QBENCHMARK {
        for (int i = 0; i < million; ++i)
            accumulate += i;
    }
}

///////////////////// QVector /////////////////////

void tst_QVector::qvector_const_read_access()
{
    QVector<double> v(million);
    for (int i = 0; i < million; ++i)
        v[i] = i;

    const QVector<double> &vc = v;
    QBENCHMARK {
        for (int i = 0; i < million; ++i)
            accumulate += vc[i];
    }
}

void tst_QVector::qvector_mutable_read_access()
{
    QVector<double> v(million);
    for (int i = 0; i < million; ++i)
        v[i] = i;

    QBENCHMARK {
        for (int i = 0; i < million; ++i)
            accumulate += v[i];
    }
}

void tst_QVector::qvector_fill_and_return()
{
    QBENCHMARK {
        QVector<double> v = qvector_fill_and_return_helper();
        accumulate += v[1];
    }
}

///////////////////// QRawVector /////////////////////

void tst_QVector::qrawvector_const_read_access()
{
    QRawVector<double> v(million);
    for (int i = 0; i < million; ++i)
        v[i] = i;

    const QRawVector<double> &vc = v;
    QBENCHMARK {
        for (int i = vc.size(); --i >= 0;)
            accumulate += vc[i];
    }
}

void tst_QVector::qrawvector_mutable_read_access()
{
    QRawVector<double> v(million);
    for (int i = 0; i < million; ++i)
        v[i] = i;

    QBENCHMARK {
        for (int i = 0; i < million; ++i)
            accumulate += v[i];
    }
}

void tst_QVector::qvector_pop_back()
{
    const int c1 = 100000;
    QVERIFY(million % c1 == 0);

    QVector<int> v;
    v.resize(million);

    QBENCHMARK {
        for (int i = 0; i < c1; ++i)
            v.pop_back();
        if (v.size() == 0)
            v.resize(million);
    }
}

#if TEST_RAW
void tst_QVector::qrawvector_fill_and_return()
{
    QBENCHMARK {
        QVector<double> v = qrawvector_fill_and_return_helper();
        accumulate += v[1];
    }
}
#endif

///////////////////// std::vector /////////////////////

void tst_QVector::stdvector_const_read_access()
{
    std::vector<double> v(million);
    for (int i = 0; i < million; ++i)
        v[i] = i;

    const std::vector<double> &vc = v;
    QBENCHMARK {
        for (int i = 0; i < million; ++i)
            accumulate += vc[i];
    }
}

void tst_QVector::stdvector_mutable_read_access()
{
    std::vector<double> v(million);
    for (int i = 0; i < million; ++i)
        v[i] = i;

    QBENCHMARK {
        for (int i = 0; i < million; ++i)
            accumulate += v[i];
    }
}

void tst_QVector::stdvector_pop_back()
{
    const int size = million / 10;
    QVERIFY(million % size == 0);

    std::vector<int> v;
    v.resize(million);

    QBENCHMARK {
        for (int i = 0; i < size; ++i)
            v.pop_back();
        if (v.size() == 0)
            v.resize(million);
    }
}

void tst_QVector::stdvector_fill_and_return()
{
    QBENCHMARK {
        std::vector<double> v = stdvector_fill_and_return_helper();
        accumulate += v[1];
    }
}

///////////////////// mixed vector /////////////////////

void tst_QVector::mixedvector_fill_and_return()
{
    QBENCHMARK {
        std::vector<double> v = stdvector_fill_and_return_helper();
        accumulate += v[1];
    }
}

QTEST_MAIN(tst_QVector)

#include "tst_bench_qvector.moc"
