/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtCore/qarraydata.h>

class tst_QArrayData : public QObject
{
    Q_OBJECT

private slots:
    void referenceCounting();
    void sharedNullEmpty();
    void staticData();
};

void tst_QArrayData::referenceCounting()
{
    {
        // Reference counting initialized to 1 (owned)
        QArrayData array = { Q_REFCOUNT_INITIALIZER(1), 0, 0, 0, 0 };

        QCOMPARE(int(array.ref), 1);

        array.ref.ref();
        QCOMPARE(int(array.ref), 2);

        QVERIFY(array.ref.deref());
        QCOMPARE(int(array.ref), 1);

        array.ref.ref();
        QCOMPARE(int(array.ref), 2);

        QVERIFY(array.ref.deref());
        QCOMPARE(int(array.ref), 1);

        QVERIFY(!array.ref.deref());
        QCOMPARE(int(array.ref), 0);

        // Now would be a good time to free/release allocated data
    }

    {
        // Reference counting initialized to -1 (static read-only data)
        QArrayData array = { Q_REFCOUNT_INITIALIZER(-1), 0, 0, 0, 0 };

        QCOMPARE(int(array.ref), -1);

        array.ref.ref();
        QCOMPARE(int(array.ref), -1);

        QVERIFY(array.ref.deref());
        QCOMPARE(int(array.ref), -1);
    }
}

void tst_QArrayData::sharedNullEmpty()
{
    QArrayData *null = const_cast<QArrayData *>(&QArrayData::shared_null);
    QArrayData *empty = const_cast<QArrayData *>(&QArrayData::shared_empty);

    QCOMPARE(int(null->ref), -1);
    QCOMPARE(int(empty->ref), -1);

    null->ref.ref();
    empty->ref.ref();

    QCOMPARE(int(null->ref), -1);
    QCOMPARE(int(empty->ref), -1);

    QVERIFY(null->ref.deref());
    QVERIFY(empty->ref.deref());

    QCOMPARE(int(null->ref), -1);
    QCOMPARE(int(empty->ref), -1);

    QVERIFY(null != empty);

    QCOMPARE(null->size, 0);
    QCOMPARE(null->alloc, 0u);
    QCOMPARE(null->capacityReserved, 0u);

    QCOMPARE(empty->size, 0);
    QCOMPARE(empty->alloc, 0u);
    QCOMPARE(empty->capacityReserved, 0u);
}

void tst_QArrayData::staticData()
{
    QStaticArrayData<char, 10> charArray = {
        Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(char, 10),
        { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j' }
    };
    QStaticArrayData<int, 10> intArray = {
        Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(int, 10),
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }
    };
    QStaticArrayData<double, 10> doubleArray = {
        Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(double, 10),
        { 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f }
    };

    QCOMPARE(charArray.header.size, 10);
    QCOMPARE(intArray.header.size, 10);
    QCOMPARE(doubleArray.header.size, 10);

    QCOMPARE(charArray.header.data(), reinterpret_cast<void *>(&charArray.data));
    QCOMPARE(intArray.header.data(), reinterpret_cast<void *>(&intArray.data));
    QCOMPARE(doubleArray.header.data(), reinterpret_cast<void *>(&doubleArray.data));
}

QTEST_APPLESS_MAIN(tst_QArrayData)
#include "tst_qarraydata.moc"
