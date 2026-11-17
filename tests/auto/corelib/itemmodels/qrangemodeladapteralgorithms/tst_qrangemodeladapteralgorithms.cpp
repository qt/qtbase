// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../qrangemodel/data.h"

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qabstractitemmodeltester.h>
#include <QtCore/qrangemodeladapter.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <numeric>
#include <vector>

#if defined(__cpp_lib_ranges)
#include <ranges>
#endif

// std::ranges::min() and max() of a range store the result in a temporary.
// libstdc++ before GCC 16 declares that temporary as auto, so with a mutable
// adapter it is a copy of the proxy reference, and assigning to it modifies the
// model (GCC bug 111861). Other implementations use the range's value_type.
#if defined(__cpp_lib_ranges) && (!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE >= 16)
#  define QRMA_RANGES_MINMAX_USES_VALUE_TYPE
#endif

using namespace Qt::StringLiterals;

class tst_QRangeModelAdapterAlgorithms : public QObject
{
    Q_OBJECT
private slots:
    void readOnlyListAlgorithms();
    void readOnlyStringListAlgorithms();
    void readOnlyTableAlgorithms();
    void readOnlyTreeAlgorithms();
    void readOnlyRangesAlgorithms();
};

// Calls a helper function that verifies with the QtTest macros, and returns
// from the calling function if the test failed. The failure is reported at the
// location in the helper, so also log the call that failed.
#define VERIFY_HELPER(...) \
    do { \
        __VA_ARGS__; \
        if (QTest::currentTestFailed()) { \
            qWarning("%s:%d: %s failed", __FILE__, __LINE__, #__VA_ARGS__); \
            return; \
        } \
    } while (false)

namespace {

value_tree createValueTree()
{
    tree_row root[] = {
        {"1", "one"},
        {"2", "two"},
        {"3", "three"},
        {"4", "four"},
        {"5", "five"},
    };
    value_tree tree{std::make_move_iterator(std::begin(root)),
                    std::make_move_iterator(std::end(root))};

    tree[1].addChild("2.1", "two.one");
    tree[1].addChild("2.2", "two.two");
    tree_row &row23 = tree[1].addChild("2.3", "two.three");

    row23.addChild("2.3.1", "two.three.one");
    row23.addChild("2.3.2", "two.three.two");

    return tree;
}

// Records every signal that the model emits, and verifies the model with
// QAbstractItemModelTester.
class ModelObserver
{
public:
    explicit ModelObserver(QAbstractItemModel *model)
        : m_tester(model, QAbstractItemModelTester::FailureReportingMode::QtTest)
    {
        const QMetaObject &mo = QAbstractItemModel::staticMetaObject;
        for (int i = QObject::staticMetaObject.methodCount(); i < mo.methodCount(); ++i) {
            const QMetaMethod method = mo.method(i);
            if (method.methodType() == QMetaMethod::Signal)
                m_spies.emplace_back(std::make_unique<QSignalSpy>(model, method));
        }
    }

    QStringList emittedSignals() const
    {
        QStringList result;
        for (const auto &spy : m_spies) {
            if (!spy->isEmpty())
                result << QString::fromLatin1(spy->signal());
        }
        return result;
    }

private:
    QAbstractItemModelTester m_tester;
    std::vector<std::unique_ptr<QSignalSpy>> m_spies;
};

// Runs read-only algorithms on the range [first, last) of an adapter, and
// compares the results with the same algorithms applied to the expected
// values.
template <typename It, typename Expected>
void verifyReadOnlyListAlgorithms(It first, It last, const Expected &expected)
{
    using value_type = typename Expected::value_type;
    const auto ebegin = std::begin(expected);
    const auto eend = std::end(expected);
    const auto pos = [first](It it) { return int(std::distance(first, it)); };
    const auto epos = [ebegin](auto it) { return int(std::distance(ebegin, it)); };

    QCOMPARE(std::distance(first, last), std::distance(ebegin, eend));
    QVERIFY(std::equal(first, last, ebegin, eend));

    // searching
    for (const auto &value : expected)
        QCOMPARE(pos(std::find(first, last, value)), epos(std::find(ebegin, eend, value)));
    QCOMPARE(std::find(first, last, value_type(-1)), last);

    const auto isOdd = [](const value_type &value) { return int(value) % 2 != 0; };
    QCOMPARE(pos(std::find_if(first, last, isOdd)), epos(std::find_if(ebegin, eend, isOdd)));
    QCOMPARE(pos(std::find_if_not(first, last, isOdd)),
             epos(std::find_if_not(ebegin, eend, isOdd)));
    QCOMPARE(pos(std::adjacent_find(first, last, std::greater<>())),
             epos(std::adjacent_find(ebegin, eend, std::greater<>())));

    const std::vector<value_type> needle(std::next(ebegin, 2), std::next(ebegin, 4));
    QCOMPARE(pos(std::search(first, last, needle.begin(), needle.end())), 2);
    QCOMPARE(pos(std::find_first_of(first, last, needle.begin(), needle.end())), 2);

    // counting and predicates
    QCOMPARE(std::count(first, last, *first), std::count(ebegin, eend, *ebegin));
    QCOMPARE(std::count_if(first, last, isOdd), std::count_if(ebegin, eend, isOdd));
    QCOMPARE(std::all_of(first, last, isOdd), std::all_of(ebegin, eend, isOdd));
    QCOMPARE(std::any_of(first, last, isOdd), std::any_of(ebegin, eend, isOdd));
    QCOMPARE(std::none_of(first, last, isOdd), std::none_of(ebegin, eend, isOdd));

    // folding
    QCOMPARE(std::accumulate(first, last, value_type(0)),
             std::accumulate(ebegin, eend, value_type(0)));
    value_type sum = 0;
    std::for_each(first, last, [&sum](const value_type &value) { sum += value; });
    QCOMPARE(sum, std::accumulate(ebegin, eend, value_type(0)));

    // minimum and maximum
    QCOMPARE(pos(std::min_element(first, last)), epos(std::min_element(ebegin, eend)));
    QCOMPARE(pos(std::max_element(first, last)), epos(std::max_element(ebegin, eend)));
    const auto [minIt, maxIt] = std::minmax_element(first, last);
    QCOMPARE(*minIt, *std::min_element(ebegin, eend));
    QCOMPARE(*maxIt, *std::max_element(ebegin, eend));

    // comparisons
    std::vector<value_type> different(ebegin, eend);
    different.back() += 1;
    QCOMPARE(pos(std::mismatch(first, last, different.begin()).first),
             int(different.size()) - 1);
    QVERIFY(!std::equal(first, last, different.begin(), different.end()));
    QVERIFY(std::lexicographical_compare(first, last, different.begin(), different.end()));
    QVERIFY(!std::lexicographical_compare(different.begin(), different.end(), first, last));
    QVERIFY(std::is_permutation(first, last, std::rbegin(expected), std::rend(expected)));

    // properties of the sequence
    QCOMPARE(std::is_sorted(first, last), std::is_sorted(ebegin, eend));
    QCOMPARE(pos(std::is_sorted_until(first, last)), epos(std::is_sorted_until(ebegin, eend)));

    // copying out of the model
    std::vector<value_type> copy;
    std::copy(first, last, std::back_inserter(copy));
    QVERIFY(std::equal(copy.begin(), copy.end(), ebegin, eend));
    std::vector<value_type> transformed;
    std::transform(first, last, std::back_inserter(transformed),
                   [](const value_type &value) { return value * 2; });
    QCOMPARE(transformed.size(), copy.size());
    QCOMPARE(transformed.front(), copy.front() * 2);
}

// Binary search algorithms on a sorted range
template <typename It, typename Expected>
void verifyBinarySearchAlgorithms(It first, It last, const Expected &expected)
{
    using value_type = typename Expected::value_type;
    const auto ebegin = std::begin(expected);
    const auto eend = std::end(expected);
    const auto pos = [first](It it) { return int(std::distance(first, it)); };
    const auto epos = [ebegin](auto it) { return int(std::distance(ebegin, it)); };

    QVERIFY(std::is_sorted(first, last));
    for (const value_type &value : {value_type(-1), value_type(0), value_type(3),
                                    value_type(4), value_type(100)}) {
        QCOMPARE(std::binary_search(first, last, value), std::binary_search(ebegin, eend, value));
        QCOMPARE(pos(std::lower_bound(first, last, value)),
                 epos(std::lower_bound(ebegin, eend, value)));
        QCOMPARE(pos(std::upper_bound(first, last, value)),
                 epos(std::upper_bound(ebegin, eend, value)));
        const auto range = std::equal_range(first, last, value);
        const auto erange = std::equal_range(ebegin, eend, value);
        QCOMPARE(pos(range.first), epos(erange.first));
        QCOMPARE(pos(range.second), epos(erange.second));
    }
}

template <typename Container>
void testReadOnlyListAlgorithms()
{
    const Container expected = {5, 3, 9, 1, 7, 7, 2, 8, 6, 4, 0};

    { // const iterators of a mutable adapter
        Container data = expected;
        QRangeModelAdapter adapter(std::ref(data));
        ModelObserver observer(adapter.model());

        VERIFY_HELPER(verifyReadOnlyListAlgorithms(adapter.cbegin(), adapter.cend(), expected));
        // reverse iterators
        VERIFY_HELPER(verifyReadOnlyListAlgorithms(adapter.crbegin(), adapter.crend(),
                                                   Container(expected.rbegin(), expected.rend())));
        // const adapter
        VERIFY_HELPER(verifyReadOnlyListAlgorithms(std::as_const(adapter).begin(),
                                                   std::as_const(adapter).end(), expected));

        QCOMPARE(data, expected);
        QCOMPARE(observer.emittedSignals(), QStringList());
    }

    { // adapter on a const range
        const Container data = expected;
        QRangeModelAdapter adapter(std::ref(data));
        ModelObserver observer(adapter.model());

        VERIFY_HELPER(verifyReadOnlyListAlgorithms(adapter.begin(), adapter.end(), expected));
        QCOMPARE(observer.emittedSignals(), QStringList());
    }

    { // sorted
        Container sorted = expected;
        if constexpr (std::is_same_v<Container, std::list<typename Container::value_type>>)
            sorted.sort();
        else
            std::sort(sorted.begin(), sorted.end());
        const Container expectedSorted = sorted;

        QRangeModelAdapter adapter(std::ref(sorted));
        ModelObserver observer(adapter.model());

        VERIFY_HELPER(verifyBinarySearchAlgorithms(adapter.cbegin(), adapter.cend(),
                                                   expectedSorted));
        QCOMPARE(sorted, expectedSorted);
        QCOMPARE(observer.emittedSignals(), QStringList());
    }
}

} // namespace

void tst_QRangeModelAdapterAlgorithms::readOnlyListAlgorithms()
{
    VERIFY_HELPER(testReadOnlyListAlgorithms<std::vector<int>>());
    VERIFY_HELPER(testReadOnlyListAlgorithms<QList<int>>());
    VERIFY_HELPER(testReadOnlyListAlgorithms<std::list<int>>());
    VERIFY_HELPER(testReadOnlyListAlgorithms<std::vector<double>>());
}

void tst_QRangeModelAdapterAlgorithms::readOnlyStringListAlgorithms()
{
    QStringList data = {u"one"_s, u"two"_s, u"three"_s, u"four"_s, u"two"_s};
    const QStringList expected = data;
    QRangeModelAdapter adapter(&data);
    ModelObserver observer(adapter.model());

    QCOMPARE(std::find(adapter.cbegin(), adapter.cend(), u"three"_s) - adapter.cbegin(), 2);
    QCOMPARE(std::count(adapter.cbegin(), adapter.cend(), u"two"_s), 2);
    QCOMPARE(std::count_if(adapter.cbegin(), adapter.cend(), [](const QString &value) {
                 return value.startsWith(u't');
             }), 3);
    QCOMPARE(std::accumulate(adapter.cbegin(), adapter.cend(), QString()),
             u"onetwothreefourtwo"_s);
    QCOMPARE(*std::max_element(adapter.cbegin(), adapter.cend()), u"two"_s);
    QCOMPARE(*std::min_element(adapter.cbegin(), adapter.cend(),
                               [](const QString &lhs, const QString &rhs) {
                                   return lhs.size() < rhs.size();
                               }), u"one"_s);
    QVERIFY(std::equal(adapter.cbegin(), adapter.cend(), expected.begin(), expected.end()));
    QCOMPARE(QStringList(adapter.crbegin(), adapter.crend()),
             QStringList(expected.rbegin(), expected.rend()));

    QCOMPARE(data, expected);
    QCOMPARE(observer.emittedSignals(), QStringList());
}

void tst_QRangeModelAdapterAlgorithms::readOnlyTableAlgorithms()
{
    using Row = std::vector<int>;
    const std::vector<Row> expected = {
        {3, 30, 300},
        {1, 10, 100},
        {4, 40, 400},
        {1, 11, 111},
        {5, 50, 500},
    };
    std::vector<Row> data = expected;
    QRangeModelAdapter adapter(std::ref(data));
    ModelObserver observer(adapter.model());

    const auto first = adapter.cbegin();
    const auto last = adapter.cend();
    const auto pos = [first](auto it) { return int(std::distance(first, it)); };

    // rows compare equal to the row type
    QVERIFY(std::equal(first, last, expected.begin(), expected.end()));
    QCOMPARE(pos(std::find(first, last, expected.at(2))), 2);
    QCOMPARE(std::find(first, last, Row{0, 0, 0}), last);
    QCOMPARE(std::count(first, last, expected.at(1)), 1);

    // predicates and comparators get the row wrapper
    const auto firstColumnIs1 = [](const auto &row) { return row.at(0) == 1; };
    QCOMPARE(pos(std::find_if(first, last, firstColumnIs1)), 1);
    QCOMPARE(std::count_if(first, last, firstColumnIs1), 2);
    QVERIFY(std::all_of(first, last, [](const auto &row) { return row.size() == 3; }));
    QVERIFY(!std::any_of(first, last, [](const auto &row) { return row.at(0) > 5; }));

    const auto bySecondColumn = [](const auto &lhs, const auto &rhs) {
        return lhs.at(1) < rhs.at(1);
    };
    QCOMPARE(pos(std::max_element(first, last, bySecondColumn)), 4);
    QCOMPARE(pos(std::min_element(first, last, bySecondColumn)), 1);
    QVERIFY(!std::is_sorted(first, last, bySecondColumn));

    // reverse iteration
    QVERIFY(std::equal(adapter.crbegin(), adapter.crend(), expected.rbegin(), expected.rend()));
    QCOMPARE(std::distance(adapter.crbegin(), std::find_if(adapter.crbegin(), adapter.crend(),
                                                            firstColumnIs1)), 1);

    // folding over rows, and over the columns of each row
    const int total = std::accumulate(first, last, 0, [](int sum, const auto &row) {
        return std::accumulate(row.cbegin(), row.cend(), sum);
    });
    int expectedTotal = 0;
    for (const auto &row : expected)
        expectedTotal = std::accumulate(row.begin(), row.end(), expectedTotal);
    QCOMPARE(total, expectedTotal);

    // algorithms on the columns of a row
    const auto row = *std::next(first, 2);
    QCOMPARE(std::distance(row.cbegin(), row.cend()), 3);
    QCOMPARE(std::distance(row.cbegin(), std::find(row.cbegin(), row.cend(), 40)), 1);
    QCOMPARE(*std::max_element(row.cbegin(), row.cend()), 400);
    QVERIFY(std::is_sorted(row.cbegin(), row.cend()));
    QVERIFY(std::equal(row.cbegin(), row.cend(), expected.at(2).begin(), expected.at(2).end()));

    // copying rows out of the model
    std::vector<Row> copy;
    std::copy(first, last, std::back_inserter(copy));
    QCOMPARE(copy, expected);

    QCOMPARE(data, expected);
    QCOMPARE(observer.emittedSignals(), QStringList());
}

void tst_QRangeModelAdapterAlgorithms::readOnlyTreeAlgorithms()
{
    auto tree = createValueTree();
    const QRangeModelAdapter adapter(std::ref(tree));
    ModelObserver observer(adapter.model());

    const auto valueIs = [](const QString &value) {
        return [value](const auto &row) { return row->value() == value; };
    };

    // top level
    QCOMPARE(std::distance(adapter.cbegin(), adapter.cend()), 5);
    QCOMPARE(std::distance(adapter.cbegin(),
                           std::find_if(adapter.cbegin(), adapter.cend(), valueIs(u"3"_s))), 2);
    QCOMPARE(std::find_if(adapter.cbegin(), adapter.cend(), valueIs(u"2.1"_s)), adapter.cend());
    QCOMPARE(std::count_if(adapter.cbegin(), adapter.cend(),
                           [](const auto &row) { return row.hasChildren(); }), 1);

    QStringList values;
    std::transform(adapter.cbegin(), adapter.cend(), std::back_inserter(values),
                   [](const auto &row) { return row->value(); });
    QCOMPARE(values, (QStringList{u"1"_s, u"2"_s, u"3"_s, u"4"_s, u"5"_s}));
    QVERIFY(std::is_sorted(adapter.cbegin(), adapter.cend(),
                           [](const auto &lhs, const auto &rhs) {
                               return lhs->value() < rhs->value();
                           }));

    // a branch
    const auto branchRow = std::find_if(adapter.cbegin(), adapter.cend(),
                                        [](const auto &row) { return row.hasChildren(); });
    QVERIFY(branchRow != adapter.cend());
    const auto branch = (*branchRow).children();
    QCOMPARE(std::distance(branch.cbegin(), branch.cend()), 3);
    QCOMPARE(std::distance(branch.cbegin(),
                           std::find_if(branch.cbegin(), branch.cend(), valueIs(u"2.2"_s))), 1);
    values.clear();
    std::transform(branch.crbegin(), branch.crend(), std::back_inserter(values),
                   [](const auto &row) { return row->value(); });
    QCOMPARE(values, (QStringList{u"2.3"_s, u"2.2"_s, u"2.1"_s}));

    QCOMPARE(observer.emittedSignals(), QStringList());
}

void tst_QRangeModelAdapterAlgorithms::readOnlyRangesAlgorithms()
{
#if !defined(__cpp_lib_ranges)
    QSKIP("This test requires C++20 ranges");
#else
    using Adapter = decltype(QRangeModelAdapter(std::declval<std::vector<int> *>()));
    static_assert(std::ranges::random_access_range<Adapter>);
    static_assert(std::ranges::random_access_range<const Adapter>);
    static_assert(std::ranges::sized_range<Adapter>);
    static_assert(std::ranges::common_range<Adapter>);
    static_assert(std::ranges::viewable_range<Adapter &>);

    using TableAdapter = decltype(QRangeModelAdapter(std::declval<std::vector<std::vector<int>> *>()));
    static_assert(std::ranges::random_access_range<TableAdapter>);
    static_assert(std::ranges::sized_range<TableAdapter>);

    std::vector<int> data = {5, 3, 9, 1, 7, 2, 8, 6, 4, 0};
    const std::vector<int> expected = data;
    QRangeModelAdapter adapter(&data);
    ModelObserver observer(adapter.model());

    QCOMPARE(std::ranges::size(adapter), data.size());
    QVERIFY(std::ranges::equal(adapter, expected));
    QCOMPARE(std::ranges::distance(adapter.cbegin(), std::ranges::find(adapter, 7)), 4);
    QCOMPARE(std::ranges::count_if(adapter, [](int value) { return value % 2; }), 5);
#ifdef QRMA_RANGES_MINMAX_USES_VALUE_TYPE
    QCOMPARE(std::ranges::min(adapter), 0);
    QCOMPARE(std::ranges::max(adapter), 9);
#endif
    QCOMPARE(std::ranges::min(std::as_const(adapter)), 0);
    QCOMPARE(std::ranges::max(std::as_const(adapter)), 9);
    QVERIFY(std::ranges::any_of(adapter, [](int value) { return value > 8; }));

    std::vector<int> copy;
    std::ranges::copy(adapter, std::back_inserter(copy));
    QCOMPARE(copy, expected);

    // views over the adapter
    std::vector<int> evens;
    std::ranges::copy(adapter | std::views::filter([](int value) { return value % 2 == 0; }),
                      std::back_inserter(evens));
    QCOMPARE(evens, (std::vector<int>{2, 8, 6, 4, 0}));

    std::vector<int> lastThreeDoubled;
    std::ranges::copy(adapter | std::views::reverse | std::views::take(3)
                              | std::views::transform([](int value) { return value * 2; }),
                      std::back_inserter(lastThreeDoubled));
    QCOMPARE(lastThreeDoubled, (std::vector<int>{0, 8, 12}));

    QCOMPARE(data, expected);
    QCOMPARE(observer.emittedSignals(), QStringList());
#endif
}

QTEST_MAIN(tst_QRangeModelAdapterAlgorithms)
#include "tst_qrangemodeladapteralgorithms.moc"
