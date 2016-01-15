/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
#include <QtTest>

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
