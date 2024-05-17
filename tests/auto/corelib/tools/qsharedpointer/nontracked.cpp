// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

/*
 * This file exists because tst_qsharedpointer.cpp is compiled with
 * QT_SHAREDPOINTER_TRACK_POINTERS. That changes some behavior.
 *
 * Note that most of these tests may yield false-positives in debug mode, but
 * they should not yield false negatives. That is, they may report PASS when
 * they are failing, but they should not produce FAILs.
 *
 * The reason for that is because of C++'s One Definition Rule: the macro
 * changes some functions and, in debug mode, they will not be inlined. At link
 * time, the two functions would be merged.
 */

#include <qsharedpointer.h>
#include <QTest>

#include "nontracked.h"

// We can't name our classes Data and DerivedData: those are in tst_qsharedpointer.cpp
namespace NonTracked {

class Data
{
public:
    static int destructorCounter;
    static int generationCounter;
    int generation;

    Data() : generation(++generationCounter)
    { }

    virtual ~Data()
    {
        if (generation <= 0)
            qFatal("tst_qsharedpointer: Double deletion!");
        generation = 0;
        ++destructorCounter;
    }
};
int Data::generationCounter = 0;
int Data::destructorCounter = 0;

class DerivedData: public Data
{
public:
    static int derivedDestructorCounter;
    int moreData;
    DerivedData() : moreData(0) { }
    ~DerivedData() { ++derivedDestructorCounter; }
};
int DerivedData::derivedDestructorCounter = 0;


#ifndef QTEST_NO_RTTI
void dynamicCastFailureNoLeak()
{
    Data::destructorCounter = DerivedData::derivedDestructorCounter = 0;

    // see QTBUG-28924
    QSharedPointer<Data> a(new Data);
    QSharedPointer<DerivedData> b = a.dynamicCast<DerivedData>();
    QVERIFY(!a.isNull());
    QVERIFY(b.isNull());

    a.clear();
    b.clear();
    QVERIFY(a.isNull());

    // verify that the destructors were called
    QCOMPARE(Data::destructorCounter, 1);
    QCOMPARE(DerivedData::derivedDestructorCounter, 0);
}

#endif
} // namespace NonTracked
