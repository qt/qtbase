/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QSet>
#include <QTest>

class tst_QSet : public QObject
{
    Q_OBJECT

private slots:
    void intersect_int_data();
    void intersect_int();
    void intersect_complexType_data();
    void intersect_complexType();
};

void tst_QSet::intersect_int_data()
{
    QTest::addColumn<int>("lhsSize");
    QTest::addColumn<int>("rhsSize");
    QTest::addColumn<int>("intersectSize");

    QTest::newRow("1000000.intersect(1000) = empty") << 1000000 << 1000 << 0;
    QTest::newRow("1000.intersect(1000000) = empty") << 1000 << 1000000 << 0;
    QTest::newRow("1000000.intersect(1000) = 500") << 1000000 << 1000 << 500;
    QTest::newRow("1000.intersect(1000000) = 500") << 1000 << 1000000 << 500;
    QTest::newRow("1000000.intersect(1000) = 1000") << 1000000 << 1000 << 1000;
    QTest::newRow("1000.intersect(1000000) = 1000") << 1000 << 1000000 << 1000;
}

void tst_QSet::intersect_int()
{
    QFETCH(int, lhsSize);
    QFETCH(int, rhsSize);
    QFETCH(int, intersectSize);

    // E.g. when lhsSize = 1000, rhsSize = 1000000 and intersectSize = 500:
    // lhsSize = { 0, 1, ... 1000 }
    // rhsSize = { 500, 501, ... 1000500 }

    QSet<int> lhs;
    for (int i = 0; i < lhsSize; ++i)
        lhs.insert(i);

    QSet<int> rhs;
    const int start = lhsSize - intersectSize;
    for (int i = start; i < start + rhsSize; ++i)
        rhs.insert(i);

    QBENCHMARK {
        lhs.intersect(rhs);
    }

    QVERIFY(lhs.size() == intersectSize);
}

struct ComplexType
{
    ComplexType(int a) : a(a) {}
    int a;
    int b;
    int c;
};

inline uint qHash(const ComplexType &key, uint seed = 0)
{
    return uint(key.a) ^ seed;
}

inline bool operator==(const ComplexType &lhs, const ComplexType &rhs)
{
    return lhs.a == rhs.a;
}

void tst_QSet::intersect_complexType_data()
{
    intersect_int_data();
}

void tst_QSet::intersect_complexType()
{
    QFETCH(int, lhsSize);
    QFETCH(int, rhsSize);
    QFETCH(int, intersectSize);

    QSet<ComplexType> lhs;
    for (int i = 0; i < lhsSize; ++i)
        lhs.insert(ComplexType(i));

    QSet<ComplexType> rhs;
    const int start = lhsSize - intersectSize;
    for (int i = start; i < start + rhsSize; ++i)
        rhs.insert(ComplexType(i));

    QBENCHMARK {
        lhs.intersect(rhs);
    }
}

QTEST_MAIN(tst_QSet)

#include "main.moc"
