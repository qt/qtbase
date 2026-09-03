// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qhttpheaders.h>

#include <QtTest/qtest.h>

#include <optional>

using namespace Qt::StringLiterals;

using Ranges = QList<QHttpHeaderRange>;

static constexpr QHttpHeaderRange range(qint64 start, qint64 end) noexcept
{
    return {start, end};
}

static constexpr QHttpHeaderRange from(qint64 start) noexcept
{
    return {start, std::nullopt};
}

static constexpr QHttpHeaderRange last(qint64 n) noexcept
{
    return {std::nullopt, n};
}

static constexpr QHttpHeaderRange invalid() noexcept
{
    return {std::nullopt, std::nullopt};
}

static Ranges toList(QSpan<const QHttpHeaderRange> ranges)
{
    return Ranges(ranges.begin(), ranges.end());
}

class tst_QHttpHeaderRangeSet : public QObject
{
    Q_OBJECT

private slots:
    void defaultConstructed();
    void construction_data();
    void construction();
    void initializerList();
    void setRanges();
    void storage();
    void sharing();
    void comparison();
    void swapAndMove();
    void debugStream();
};

void tst_QHttpHeaderRangeSet::defaultConstructed()
{
    QHttpHeaderRangeSet ranges;
    QVERIFY(ranges.ranges().isEmpty());
    QCOMPARE_EQ(ranges, QHttpHeaderRangeSet{});
    QCOMPARE_EQ(ranges, QHttpHeaderRangeSet{Ranges{}}); // an empty object is an empty object
    QCOMPARE_EQ(qHash(ranges), qHash(QHttpHeaderRangeSet{}));
}

void tst_QHttpHeaderRangeSet::construction_data()
{
    QTest::addColumn<Ranges>("input");

    QTest::newRow("empty") << Ranges{};
    QTest::newRow("single") << Ranges{range(0, 99)};
    QTest::newRow("open-end") << Ranges{from(500)};
    QTest::newRow("suffix") << Ranges{last(500)};
    QTest::newRow("disjoint") << Ranges{range(0, 99), range(200, 299)};
    QTest::newRow("descending") << Ranges{range(200, 299), range(0, 99)};
    QTest::newRow("overlapping") << Ranges{range(0, 99), range(50, 199)};
    QTest::newRow("contiguous") << Ranges{range(0, 99), range(100, 199)};
    QTest::newRow("duplicates") << Ranges{range(0, 99), range(0, 99), range(0, 99)};
    QTest::newRow("mixed") << Ranges{range(0, 99), from(500), last(50)};
    QTest::newRow("invalid") << Ranges{range(500, 400), invalid(), range(-5, -1)};
    QTest::newRow("many") << Ranges{range(0, 9), range(20, 29), range(40, 49),
                                    range(60, 69), range(80, 89)};
}

void tst_QHttpHeaderRangeSet::construction()
{
    QFETCH(const Ranges, input);

    const QHttpHeaderRangeSet ranges{input};
    QCOMPARE(toList(ranges.ranges()), input); // the given order is kept, as-is

    const QHttpHeaderRangeSet copy = ranges;
    QCOMPARE_EQ(copy, ranges);
    QCOMPARE_EQ(qHash(copy), qHash(ranges));
    QCOMPARE(toList(copy.ranges()), input);
}

void tst_QHttpHeaderRangeSet::initializerList()
{
    const QHttpHeaderRangeSet ranges = { {0, 99}, {200, std::nullopt}, {std::nullopt, 100} };
    QCOMPARE(toList(ranges.ranges()), Ranges({range(0, 99), from(200), last(100)}));

    QCOMPARE_EQ(ranges, QHttpHeaderRangeSet(Ranges{range(0, 99), from(200), last(100)}));
    QCOMPARE_EQ(QHttpHeaderRangeSet({}), QHttpHeaderRangeSet{});
}

void tst_QHttpHeaderRangeSet::setRanges()
{
    QHttpHeaderRangeSet ranges;

    ranges.setRanges({range(200, 299), range(0, 99)});
    QCOMPARE(toList(ranges.ranges()), Ranges({range(200, 299), range(0, 99)}));

    // setting from a view of the object's own ranges is fine:
    ranges.setRanges(ranges.ranges().last(1));
    QCOMPARE(toList(ranges.ranges()), Ranges{range(0, 99)});

    ranges.setRanges({});
    QVERIFY(ranges.ranges().isEmpty());
    QCOMPARE_EQ(ranges, QHttpHeaderRangeSet{});
}

// exercises both the inline buffer and the heap allocation behind it
void tst_QHttpHeaderRangeSet::storage()
{
    for (qsizetype n = 0; n < 8; ++n) {
        Ranges input;
        for (qsizetype i = 0; i < n; ++i)
            input.push_back(range(i * 10, i * 10 + 5));

        const QHttpHeaderRangeSet ranges{input};
        QCOMPARE(toList(ranges.ranges()), input);

        const QHttpHeaderRangeSet copy = ranges;
        QCOMPARE(toList(copy.ranges()), input);
        QCOMPARE_EQ(copy, ranges);
    }

    Ranges input;
    for (qint64 i = 20; i > 0; --i)
        input.push_back(range(i * 10, i * 10 + 15));

    QHttpHeaderRangeSet ranges{input};
    QCOMPARE(ranges.ranges().size(), 20);
    QCOMPARE(toList(ranges.ranges()), input);

    // a uniquely-owned buffer is reused, even for a view of its own ranges:
    ranges.setRanges(ranges.ranges().last(3));
    QCOMPARE(toList(ranges.ranges()), input.last(3));
    ranges.setRanges(input);
    QCOMPARE(toList(ranges.ranges()), input);
    ranges.setRanges(ranges.ranges().first(1));
    QCOMPARE(toList(ranges.ranges()), input.first(1));
}

void tst_QHttpHeaderRangeSet::sharing()
{
    const Ranges input{range(0, 99), range(200, 299)};
    const QHttpHeaderRangeSet original{input};

    QHttpHeaderRangeSet copy = original;
    QCOMPARE(copy.ranges().data(), original.ranges().data()); // shares the ranges

    copy.setRanges({from(1000)});
    QCOMPARE(toList(copy.ranges()), Ranges{from(1000)});
    QCOMPARE(toList(original.ranges()), input);
    QCOMPARE_NE(copy, original);
}

void tst_QHttpHeaderRangeSet::comparison()
{
    const QHttpHeaderRangeSet ranges{{range(0, 99), range(200, 299)}};
    const QHttpHeaderRangeSet same{{range(0, 99), range(200, 299)}};
    const QHttpHeaderRangeSet reordered{{range(200, 299), range(0, 99)}};
    const QHttpHeaderRangeSet merged{{range(0, 299)}};
    const QHttpHeaderRangeSet shorter{{range(0, 99)}};

    QCOMPARE_EQ(ranges, ranges);
    QCOMPARE_EQ(ranges, same);
    QCOMPARE_EQ(qHash(ranges), qHash(same));

    // the ranges are compared element by element, like QList's:
    QCOMPARE_NE(ranges, reordered);
    QCOMPARE_NE(ranges, merged);
    QCOMPARE_NE(ranges, shorter);
    QCOMPARE_NE(ranges, QHttpHeaderRangeSet{});
}

void tst_QHttpHeaderRangeSet::swapAndMove()
{
    static_assert(std::is_nothrow_move_constructible_v<QHttpHeaderRangeSet>);
    static_assert(std::is_nothrow_move_assignable_v<QHttpHeaderRangeSet>);
    static_assert(std::is_nothrow_swappable_v<QHttpHeaderRangeSet>);

    const Ranges input{range(0, 99)};

    QHttpHeaderRangeSet lhs{input};
    QHttpHeaderRangeSet rhs;

    lhs.swap(rhs);
    QVERIFY(lhs.ranges().isEmpty());
    QCOMPARE(toList(rhs.ranges()), input);

    const QHttpHeaderRangeSet moved = std::move(rhs);
    QCOMPARE(toList(moved.ranges()), input);

    lhs = moved;
    QCOMPARE(toList(lhs.ranges()), input);
}

void tst_QHttpHeaderRangeSet::debugStream()
{
    QString buffer;

    QDebug{&buffer} << QHttpHeaderRangeSet{{range(0, 99), from(200), last(100)}};
    QCOMPARE(buffer.trimmed(), u"QHttpHeaderRangeSet(bytes=0-99, 200-, -100)"_s);

    buffer.clear();
    QDebug{&buffer} << QHttpHeaderRangeSet{};
    QCOMPARE(buffer.trimmed(), u"QHttpHeaderRangeSet(bytes=)"_s);
}

QTEST_APPLESS_MAIN(tst_QHttpHeaderRangeSet)
#include "tst_qhttpheaderrangeset.moc"
