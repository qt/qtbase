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

#include <QtTest/QtTest>

#include <qhash.h>

#include <iterator>
#include <sstream>
#include <algorithm>

#include <unordered_set>

class tst_QHashFunctions : public QObject
{
    Q_OBJECT
public:
    enum {
        // random value
        RandomSeed = 1045982819
    };
    uint seed;

public slots:
    void initTestCase();
    void init();

private Q_SLOTS:
    void consistent();
    void qhash();
    void qhash_of_empty_and_null_qstring();
    void qhash_of_empty_and_null_qbytearray();
    void fp_qhash_of_zero_is_seed();
    void qthash_data();
    void qthash();
    void range();
    void rangeCommutative();

    void stdHash();

    void setGlobalQHashSeed();
};

void tst_QHashFunctions::consistent()
{
    // QString-like
    {
        const QString s = QStringLiteral("abcdefghijklmnopqrstuvxyz").repeated(16);

        QCOMPARE(qHash(s), qHash(QStringRef(&s)));
        QCOMPARE(qHash(s), qHash(QStringView(s)));
    }
}

void tst_QHashFunctions::initTestCase()
{
    Q_STATIC_ASSERT(int(RandomSeed) > 0);

    QTest::addColumn<uint>("seedValue");
    QTest::newRow("zero-seed") << 0U;
    QTest::newRow("non-zero-seed") << uint(RandomSeed);
}

void tst_QHashFunctions::init()
{
    QFETCH_GLOBAL(uint, seedValue);
    seed = seedValue;
}

void tst_QHashFunctions::qhash()
{
    {
        QBitArray a1;
        QBitArray a2;
        QCOMPARE(qHash(a1, seed), seed);

        a1.resize(1);
        a1.setBit(0, true);

        a2.resize(1);
        a2.setBit(0, false);

        uint h1 = qHash(a1, seed);
        uint h2 = qHash(a2, seed);

        QVERIFY(h1 != h2);  // not guaranteed

        a2.setBit(0, true);
        QVERIFY(h1 == qHash(a2, seed));

        a1.fill(true, 8);
        a1.resize(7);

        h1 = qHash(a1, seed);

        a2.fill(true, 7);
        h2 = qHash(a2, seed);

        QVERIFY(h1 == h2);

        a2.setBit(0, false);
        uint h3 = qHash(a2, seed);
        QVERIFY(h2 != h3);  // not guaranteed

        a2.setBit(0, true);
        QVERIFY(h2 == qHash(a2, seed));

        a2.setBit(6, false);
        uint h4 = qHash(a2, seed);
        QVERIFY(h2 != h4);  // not guaranteed

        a2.setBit(6, true);
        QVERIFY(h2 == qHash(a2, seed));

        QVERIFY(h3 != h4);  // not guaranteed
    }

    {
        QPair<int, int> p12(1, 2);
        QPair<int, int> p21(2, 1);

        QVERIFY(qHash(p12, seed) == qHash(p12, seed));
        QVERIFY(qHash(p21, seed) == qHash(p21, seed));
        QVERIFY(qHash(p12, seed) != qHash(p21, seed));  // not guaranteed

        QPair<int, int> pA(0x12345678, 0x12345678);
        QPair<int, int> pB(0x12345675, 0x12345675);

        QVERIFY(qHash(pA, seed) != qHash(pB, seed));    // not guaranteed
    }

    {
        std::pair<int, int> p12(1, 2);
        std::pair<int, int> p21(2, 1);

        using QT_PREPEND_NAMESPACE(qHash);

        QVERIFY(qHash(p12, seed) == qHash(p12, seed));
        QVERIFY(qHash(p21, seed) == qHash(p21, seed));
        QVERIFY(qHash(p12, seed) != qHash(p21, seed));  // not guaranteed

        std::pair<int, int> pA(0x12345678, 0x12345678);
        std::pair<int, int> pB(0x12345675, 0x12345675);

        QVERIFY(qHash(pA, seed) != qHash(pB, seed));    // not guaranteed
    }
}

void tst_QHashFunctions::qhash_of_empty_and_null_qstring()
{
    QString null, empty("");
    QCOMPARE(null, empty);
    QCOMPARE(qHash(null, seed), qHash(empty, seed));

    QStringRef nullRef, emptyRef(&empty);
    QCOMPARE(nullRef, emptyRef);
    QCOMPARE(qHash(nullRef, seed), qHash(emptyRef, seed));

    QStringView nullView, emptyView(empty);
    QCOMPARE(nullView, emptyView);
    QCOMPARE(qHash(nullView, seed), qHash(emptyView, seed));
}

void tst_QHashFunctions::qhash_of_empty_and_null_qbytearray()
{
    QByteArray null, empty("");
    QCOMPARE(null, empty);
    QCOMPARE(qHash(null, seed), qHash(empty, seed));
}

void tst_QHashFunctions::fp_qhash_of_zero_is_seed()
{
    QCOMPARE(qHash(-0.0f, seed), seed);
    QCOMPARE(qHash( 0.0f, seed), seed);

    QCOMPARE(qHash(-0.0 , seed), seed);
    QCOMPARE(qHash( 0.0 , seed), seed);

#ifndef Q_OS_DARWIN
    QCOMPARE(qHash(-0.0L, seed), seed);
    QCOMPARE(qHash( 0.0L, seed), seed);
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
    QCOMPARE(qHashRange(ints, ints, seed), seed);
    // verify that order matters (test not guaranteed):
    QVERIFY(qHashRange(ints, ints + numInts, seed) !=
            qHashRange(std::reverse_iterator<const int*>(ints + numInts), std::reverse_iterator<const int*>(ints), seed));

    {
        // verify that the input iterator category suffices:
        std::stringstream sstream;
        Q_STATIC_ASSERT((std::is_same<std::input_iterator_tag, std::istream_iterator<int>::iterator_category>::value));
        std::copy(ints, ints + numInts, std::ostream_iterator<int>(sstream, " "));
        sstream.seekg(0);
        std::istream_iterator<int> it(sstream), end;
        QCOMPARE(qHashRange(ints, ints + numInts, seed), qHashRange(it, end, seed));
    }

    SomeNamespace::Hashable hashables[] = {{0}, {1}, {2}, {3}, {4}, {5}};
    static const size_t numHashables = sizeof hashables / sizeof *hashables;
    // compile check: is qHash() found using ADL?
    (void)qHashRange(hashables, hashables + numHashables, seed);
}

void tst_QHashFunctions::rangeCommutative()
{
    int ints[] = {0, 1, 2, 3, 4, 5};
    static const size_t numInts = sizeof ints / sizeof *ints;

    // empty range just gives the seed:
    QCOMPARE(qHashRangeCommutative(ints, ints, seed), seed);
    // verify that order doesn't matter (test not guaranteed):
    QCOMPARE(qHashRangeCommutative(ints, ints + numInts, seed),
             qHashRangeCommutative(std::reverse_iterator<int*>(ints + numInts), std::reverse_iterator<int*>(ints), seed));

    {
        // verify that the input iterator category suffices:
        std::stringstream sstream;
        std::copy(ints, ints + numInts, std::ostream_iterator<int>(sstream, " "));
        sstream.seekg(0);
        std::istream_iterator<int> it(sstream), end;
        QCOMPARE(qHashRangeCommutative(ints, ints + numInts, seed), qHashRangeCommutative(it, end, seed));
    }

    SomeNamespace::Hashable hashables[] = {{0}, {1}, {2}, {3}, {4}, {5}};
    static const size_t numHashables = sizeof hashables / sizeof *hashables;
    // compile check: is qHash() found using ADL?
    (void)qHashRangeCommutative(hashables, hashables + numHashables, seed);
}

void tst_QHashFunctions::stdHash()
{
    {
        std::unordered_set<QString> s = {QStringLiteral("Hello"), QStringLiteral("World")};
        QCOMPARE(s.size(), 2UL);
        s.insert(QStringLiteral("Hello"));
        QCOMPARE(s.size(), 2UL);
    }

    {
        std::unordered_set<QStringView> s = {QStringLiteral("Hello"), QStringLiteral("World")};
        QCOMPARE(s.size(), 2UL);
        s.insert(QStringLiteral("Hello"));
        QCOMPARE(s.size(), 2UL);
    }

    {
        std::unordered_set<QLatin1String> s = {QLatin1String("Hello"), QLatin1String("World")};
        QCOMPARE(s.size(), 2UL);
        s.insert(QLatin1String("Hello"));
        QCOMPARE(s.size(), 2UL);
    }

    {
        std::unordered_set<QByteArray> s = {QByteArrayLiteral("Hello"), QByteArrayLiteral("World")};
        QCOMPARE(s.size(), 2UL);
        s.insert(QByteArray("Hello"));
        QCOMPARE(s.size(), 2UL);
    }

}

void tst_QHashFunctions::setGlobalQHashSeed()
{
    // Setter works as advertised
    qSetGlobalQHashSeed(0);
    QCOMPARE(qGlobalQHashSeed(), 0);

    // Creating a new QHash doesn't reset the seed
    QHash<QString, int> someHash;
    someHash.insert("foo", 42);
    QCOMPARE(qGlobalQHashSeed(), 0);

    // Reset works as advertised
    qSetGlobalQHashSeed(-1);
    QVERIFY(qGlobalQHashSeed() > 0);
}

QTEST_APPLESS_MAIN(tst_QHashFunctions)
#include "tst_qhashfunctions.moc"
