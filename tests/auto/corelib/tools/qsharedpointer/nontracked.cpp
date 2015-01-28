/****************************************************************************
**
** Copyright (C) 2013 Intel Corporation.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
