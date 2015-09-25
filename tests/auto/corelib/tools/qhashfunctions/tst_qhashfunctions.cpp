/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include <QtTest/QtTest>

#include <qhash.h>
#include <qtypetraits.h>

#include <iterator>
#include <sstream>
#include <algorithm>

class tst_QHashFunctions : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void qhash();
    void fp_qhash_of_zero_is_zero();
    void qthash_data();
    void qthash();
    void range();
    void rangeCommutative();

    void setGlobalQHashSeed();
};

void tst_QHashFunctions::qhash()
{
    {
        QBitArray a1;
        QBitArray a2;
        QVERIFY(qHash(a1) == 0);

        a1.resize(1);
        a1.setBit(0, true);

        a2.resize(1);
        a2.setBit(0, false);

        uint h1 = qHash(a1);
        uint h2 = qHash(a2);

        QVERIFY(h1 != h2);

        a2.setBit(0, true);
        QVERIFY(h1 == qHash(a2));

        a1.fill(true, 8);
        a1.resize(7);

        h1 = qHash(a1);

        a2.fill(true, 7);
        h2 = qHash(a2);

        QVERIFY(h1 == h2);

        a2.setBit(0, false);
        uint h3 = qHash(a2);
        QVERIFY(h2 != h3);

        a2.setBit(0, true);
        QVERIFY(h2 == qHash(a2));

        a2.setBit(6, false);
        uint h4 = qHash(a2);
        QVERIFY(h2 != h4);

        a2.setBit(6, true);
        QVERIFY(h2 == qHash(a2));

        QVERIFY(h3 != h4);
    }

    {
        QPair<int, int> p12(1, 2);
        QPair<int, int> p21(2, 1);

        QVERIFY(qHash(p12) == qHash(p12));
        QVERIFY(qHash(p21) == qHash(p21));
        QVERIFY(qHash(p12) != qHash(p21));

        QPair<int, int> pA(0x12345678, 0x12345678);
        QPair<int, int> pB(0x12345675, 0x12345675);

        QVERIFY(qHash(pA) != qHash(pB));
    }
}

void tst_QHashFunctions::fp_qhash_of_zero_is_zero()
{
    QCOMPARE(qHash(-0.0f), 0U);
    QCOMPARE(qHash( 0.0f), 0U);

    QCOMPARE(qHash(-0.0 ), 0U);
    QCOMPARE(qHash( 0.0 ), 0U);

#ifndef Q_OS_DARWIN
    QCOMPARE(qHash(-0.0L), 0U);
    QCOMPARE(qHash( 0.0L), 0U);
#endif
}

void tst_QHashFunctions::qthash_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<uint>("hash");

    QTest::newRow("null") << QString() << 0u;
    QTest::newRow("empty") << QStringLiteral("") << 0u;
    QTest::newRow("abcdef") << QStringLiteral("abcdef") << 108567222u;
    QTest::newRow("tqbfjotld") << QStringLiteral("The quick brown fox jumps over the lazy dog") << 140865879u;
    QTest::newRow("42") << QStringLiteral("42") << 882u;
}

void tst_QHashFunctions::qthash()
{
    QFETCH(QString, key);
    const uint result = qt_hash(key);
    QTEST(result, "hash");
}

namespace SomeNamespace {
    struct Hashable { int i; };
    inline uint qHash(Hashable h, uint seed = 0)
    { return QT_PREPEND_NAMESPACE(qHash)(h.i, seed); }
}

void tst_QHashFunctions::range()
{
    static const int ints[] = {0, 1, 2, 3, 4, 5};
    static const size_t numInts = sizeof ints / sizeof *ints;

    // empty range just gives the seed:
    QCOMPARE(qHashRange(ints, ints, 0xdeadbeefU), 0xdeadbeefU);
    // verify that order matters:
    QVERIFY(qHashRange(ints, ints + numInts) !=
            qHashRange(std::reverse_iterator<const int*>(ints + numInts), std::reverse_iterator<const int*>(ints)));

    {
        // verify that the input iterator category suffices:
        std::stringstream sstream;
        Q_STATIC_ASSERT((QtPrivate::is_same<std::input_iterator_tag, std::istream_iterator<int>::iterator_category>::value));
        std::copy(ints, ints + numInts, std::ostream_iterator<int>(sstream, " "));
        sstream.seekg(0);
        std::istream_iterator<int> it(sstream), end;
        QCOMPARE(qHashRange(ints, ints + numInts), qHashRange(it, end));
    }

    SomeNamespace::Hashable hashables[] = {{0}, {1}, {2}, {3}, {4}, {5}};
    static const size_t numHashables = sizeof hashables / sizeof *hashables;
    // compile check: is qHash() found using ADL?
    (void)qHashRange(hashables, hashables + numHashables);
}

void tst_QHashFunctions::rangeCommutative()
{
    int ints[] = {0, 1, 2, 3, 4, 5};
    static const size_t numInts = sizeof ints / sizeof *ints;

    // empty range just gives the seed:
    QCOMPARE(qHashRangeCommutative(ints, ints, 0xdeadbeefU), 0xdeadbeefU);
    // verify that order doesn't matter:
    QCOMPARE(qHashRangeCommutative(ints, ints + numInts),
             qHashRangeCommutative(std::reverse_iterator<int*>(ints + numInts), std::reverse_iterator<int*>(ints)));

    {
        // verify that the input iterator category suffices:
        std::stringstream sstream;
        std::copy(ints, ints + numInts, std::ostream_iterator<int>(sstream, " "));
        sstream.seekg(0);
        std::istream_iterator<int> it(sstream), end;
        QCOMPARE(qHashRangeCommutative(ints, ints + numInts), qHashRangeCommutative(it, end));
    }

    SomeNamespace::Hashable hashables[] = {{0}, {1}, {2}, {3}, {4}, {5}};
    static const size_t numHashables = sizeof hashables / sizeof *hashables;
    // compile check: is qHash() found using ADL?
    (void)qHashRangeCommutative(hashables, hashables + numHashables);
}

void tst_QHashFunctions::setGlobalQHashSeed()
{
    // Setter works as advertised
    qSetGlobalQHashSeed(0x10101010);
    QCOMPARE(qGlobalQHashSeed(), 0x10101010);

    // Creating a new QHash doesn't reset the seed
    QHash<QString, int> someHash;
    someHash.insert("foo", 42);
    QCOMPARE(qGlobalQHashSeed(), 0x10101010);

    // Reset works as advertised
    qSetGlobalQHashSeed(-1);
    QVERIFY(qGlobalQHashSeed() != -1);
}

QTEST_APPLESS_MAIN(tst_QHashFunctions)
#include "tst_qhashfunctions.moc"
