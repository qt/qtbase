// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <private/qmakearray_p.h>


class tst_QMakeArray : public QObject
{
    Q_OBJECT

private slots:
    void quicksort();
};

struct Pair {
    unsigned int key;
    unsigned int val;

    constexpr bool operator <=(const Pair &that) const noexcept
    {
        return key <= that.key;
    }

    constexpr bool operator<(const Pair &that) const noexcept
    {
        return key < that.key;
    }

    constexpr bool operator==(const Pair &that) const noexcept
    {
        return key == that.key;
    }
};

template<std::size_t Key, std::size_t Val>
struct PairQuickSortElem
{
    using Type = Pair;
    static constexpr Type data() noexcept { return Type{Key, Val}; }
};

void tst_QMakeArray::quicksort()
{
    constexpr const auto sorted_array = qMakeArray(
        QSortedData<
            PairQuickSortElem<10, 0>,
            PairQuickSortElem<5, 0>,
            PairQuickSortElem<7, 0>,
            PairQuickSortElem<1, 0>,
            PairQuickSortElem<8, 0>,
            PairQuickSortElem<6, 0>,
            PairQuickSortElem<4, 0>,
            PairQuickSortElem<3, 0>,
            PairQuickSortElem<1, 0>,
            PairQuickSortElem<2, 0>,
            PairQuickSortElem<10, 0>,
            PairQuickSortElem<5, 0>
        >::Data{});
    static_assert(sorted_array.size() == 12, "sorted_array.size() != 12");
    QCOMPARE(sorted_array[0].key, 1u);
    QCOMPARE(sorted_array[1].key, 1u);
    QCOMPARE(sorted_array[2].key, 2u);
    QCOMPARE(sorted_array[3].key, 3u);
    QCOMPARE(sorted_array[4].key, 4u);
    QCOMPARE(sorted_array[5].key, 5u);
    QCOMPARE(sorted_array[6].key, 5u);
    QCOMPARE(sorted_array[7].key, 6u);
    QCOMPARE(sorted_array[8].key, 7u);
    QCOMPARE(sorted_array[9].key, 8u);
    QCOMPARE(sorted_array[10].key, 10u);
    QCOMPARE(sorted_array[11].key, 10u);
}


QTEST_APPLESS_MAIN(tst_QMakeArray)
#include "tst_qmakearray.moc"
