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

    void modifyingListAlgorithms();
    void modifyingStringListAlgorithms();
    void modifyingTableAlgorithms();
    void modifyingTreeAlgorithms();
    void modifyingRangesAlgorithms();
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
            // skip the overloads for default arguments, the spy on the full
            // signal records those emissions as well
            if (method.methodType() == QMetaMethod::Signal
                && !(method.attributes() & QMetaMethod::Cloned)) {
                m_spies.emplace_back(std::make_unique<QSignalSpy>(model, method));
            }
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

    // the names of the emitted signals, without parameters
    QStringList emittedSignalNames() const
    {
        QStringList result = emittedSignals();
        for (QString &signal : result)
            signal.truncate(signal.indexOf(u'('));
        result.sort();
        return result;
    }

    void clear()
    {
        for (const auto &spy : m_spies)
            spy->clear();
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

namespace {

// The (row, column) of each cell that dataChanged reported, in the order of the
// emissions. A dataChanged emission for more than one cell is reported as
// (-1, -1), so that comparisons with single-cell lists fail.
QList<std::pair<int, int>> changedCells(const QSignalSpy &spy, const QModelIndex &parent = {})
{
    QList<std::pair<int, int>> result;
    for (const QList<QVariant> &arguments : spy) {
        const QModelIndex topLeft = arguments.at(0).value<QModelIndex>();
        const QModelIndex bottomRight = arguments.at(1).value<QModelIndex>();
        if (topLeft.parent() != parent)
            continue;
        if (topLeft == bottomRight)
            result.emplace_back(topLeft.row(), topLeft.column());
        else
            result.emplace_back(-1, -1);
    }
    return result;
}

// The rows that dataChanged reported as changed in all columns, in the order of
// the emissions. Any other dataChanged emission is reported as -1.
QList<int> changedRows(const QSignalSpy &spy)
{
    QList<int> result;
    for (const QList<QVariant> &arguments : spy) {
        const QModelIndex topLeft = arguments.at(0).value<QModelIndex>();
        const QModelIndex bottomRight = arguments.at(1).value<QModelIndex>();
        const int lastColumn = topLeft.model()->columnCount(topLeft.parent()) - 1;
        if (topLeft.row() == bottomRight.row() && topLeft.column() == 0
            && bottomRight.column() == lastColumn) {
            result << topLeft.row();
        } else {
            result << -1;
        }
    }
    return result;
}

// single cells in the first column
QList<std::pair<int, int>> cellsInRows(std::initializer_list<int> rows)
{
    QList<std::pair<int, int>> result;
    for (int row : rows)
        result.emplace_back(row, 0);
    return result;
}

QList<std::pair<int, int>> cellsInRows(int first, int last)
{
    QList<std::pair<int, int>> result;
    for (int row = first; row <= last; ++row)
        result.emplace_back(row, 0);
    return result;
}

template <typename Adapter, typename Range>
void verifyListModel(const Adapter &adapter, const Range &expected)
{
    const QAbstractItemModel *model = adapter.model();
    QCOMPARE(model->rowCount(), int(std::size(expected)));
    int row = 0;
    for (const auto &value : expected) {
        QCOMPARE(model->index(row, 0).data(Qt::RangeModelAdapterRole),
                 QVariant::fromValue(value));
        ++row;
    }
}

const QStringList dataChangedSignal = {u"dataChanged"_s};

template <typename Container>
void testModifyingListAlgorithms()
{
    using value_type = typename Container::value_type;
    Container data = {5, 3, 9, 1, 7, 7, 2, 8, 6, 4, 0};
    const int size = int(data.size());
    QRangeModelAdapter adapter(std::ref(data));
    ModelObserver observer(adapter.model());
    QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

    const auto verify = [&](const Container &expected,
                            const QList<std::pair<int, int>> &expectedCells) {
        QCOMPARE(data, expected);
        QCOMPARE(changedCells(dataChangedSpy), expectedCells);
        if (expectedCells.isEmpty())
            QCOMPARE(observer.emittedSignalNames(), QStringList());
        else
            QCOMPARE(observer.emittedSignalNames(), dataChangedSignal);
        VERIFY_HELPER(verifyListModel(adapter, expected));
        observer.clear();
        dataChangedSpy.clear();
    };

    std::fill(adapter.begin(), adapter.end(), value_type(7));
    VERIFY_HELPER(verify(Container(size, value_type(7)), cellsInRows(0, size - 1)));

    std::iota(adapter.begin(), adapter.end(), value_type(0));
    VERIFY_HELPER(verify({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, cellsInRows(0, size - 1)));

    // in-place transform
    std::transform(adapter.begin(), adapter.end(), adapter.begin(),
                   [](const value_type &value) { return value * 2; });
    VERIFY_HELPER(verify({0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20}, cellsInRows(0, size - 1)));

    // only the matching elements are written to
    std::replace(adapter.begin(), adapter.end(), value_type(4), value_type(5));
    VERIFY_HELPER(verify({0, 2, 5, 6, 8, 10, 12, 14, 16, 18, 20}, cellsInRows({2})));
    std::replace_if(adapter.begin(), adapter.end(),
                    [](const value_type &value) { return int(value) % 4 == 0; }, value_type(-1));
    VERIFY_HELPER(verify({-1, 2, 5, 6, -1, 10, -1, 14, -1, 18, -1}, cellsInRows({0, 4, 6, 8, 10})));
    std::replace(adapter.begin(), adapter.end(), value_type(100), value_type(0));
    VERIFY_HELPER(verify({-1, 2, 5, 6, -1, 10, -1, 14, -1, 18, -1}, {}));

    value_type next = 0;
    std::generate(adapter.begin(), adapter.end(), [&next] { return next += 3; });
    VERIFY_HELPER(verify({3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33}, cellsInRows(0, size - 1)));

    // for_each with a function that modifies the element through the reference
    std::for_each(adapter.begin(), adapter.end(), [](auto &&value) { value = value - 1; });
    VERIFY_HELPER(verify({2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32}, cellsInRows(0, size - 1)));
    // the same with a range-based for loop
    for (auto &&value : adapter)
        value = value + 1;
    VERIFY_HELPER(verify({3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33}, cellsInRows(0, size - 1)));

    // sub-ranges, and the _n variants
    std::fill(adapter.begin() + 2, adapter.begin() + 4, value_type(0));
    VERIFY_HELPER(verify({3, 6, 0, 0, 15, 18, 21, 24, 27, 30, 33}, cellsInRows({2, 3})));
    std::fill_n(adapter.begin() + 8, 3, value_type(1));
    VERIFY_HELPER(verify({3, 6, 0, 0, 15, 18, 21, 24, 1, 1, 1}, cellsInRows({8, 9, 10})));
    next = 0;
    std::generate_n(adapter.begin(), 2, [&next] { return --next; });
    VERIFY_HELPER(verify({-1, -2, 0, 0, 15, 18, 21, 24, 1, 1, 1}, cellsInRows({0, 1})));

    // through reverse iterators, starting at the end
    std::iota(adapter.rbegin(), adapter.rbegin() + 3, value_type(10));
    VERIFY_HELPER(verify({-1, -2, 0, 0, 15, 18, 21, 24, 12, 11, 10}, cellsInRows({10, 9, 8})));

    // copying into the model overwrites the existing elements
    const std::vector<value_type> source = {100, 101, 102};
    std::copy(source.begin(), source.end(), adapter.begin() + 1);
    VERIFY_HELPER(verify({-1, 100, 101, 102, 15, 18, 21, 24, 12, 11, 10}, cellsInRows({1, 2, 3})));
    std::copy_backward(source.begin(), source.end(), adapter.end());
    VERIFY_HELPER(verify({-1, 100, 101, 102, 15, 18, 21, 24, 100, 101, 102},
                         cellsInRows({10, 9, 8})));

    // transforming from one model into another
    Container otherData(size, value_type(0));
    QRangeModelAdapter other(std::ref(otherData));
    QSignalSpy otherDataChangedSpy(other.model(), &QAbstractItemModel::dataChanged);
    std::transform(adapter.cbegin(), adapter.cend(), other.begin(),
                   [](const value_type &value) { return -value; });
    QCOMPARE(otherData, (Container{1, -100, -101, -102, -15, -18, -21, -24, -100, -101, -102}));
    QCOMPARE(changedCells(otherDataChangedSpy), cellsInRows(0, size - 1));
    VERIFY_HELPER(verify({-1, 100, 101, 102, 15, 18, 21, 24, 100, 101, 102}, {}));
}

} // namespace

void tst_QRangeModelAdapterAlgorithms::modifyingListAlgorithms()
{
    VERIFY_HELPER(testModifyingListAlgorithms<std::vector<int>>());
    VERIFY_HELPER(testModifyingListAlgorithms<QList<int>>());
    VERIFY_HELPER(testModifyingListAlgorithms<std::list<int>>());
    VERIFY_HELPER(testModifyingListAlgorithms<std::vector<double>>());
}

void tst_QRangeModelAdapterAlgorithms::modifyingStringListAlgorithms()
{
    QStringList data = {u"one"_s, u"two"_s, u"three"_s, u"four"_s, u"two"_s};
    QRangeModelAdapter adapter(&data);
    ModelObserver observer(adapter.model());
    QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

    std::transform(adapter.begin(), adapter.end(), adapter.begin(),
                   [](const QString &value) { return value.toUpper(); });
    QCOMPARE(data, (QStringList{u"ONE"_s, u"TWO"_s, u"THREE"_s, u"FOUR"_s, u"TWO"_s}));
    QCOMPARE(changedCells(dataChangedSpy), cellsInRows(0, 4));
    dataChangedSpy.clear();

    std::replace(adapter.begin(), adapter.end(), u"TWO"_s, u"2"_s);
    QCOMPARE(data, (QStringList{u"ONE"_s, u"2"_s, u"THREE"_s, u"FOUR"_s, u"2"_s}));
    QCOMPARE(changedCells(dataChangedSpy), cellsInRows({1, 4}));
    dataChangedSpy.clear();

    std::for_each(adapter.begin(), adapter.end(), [](auto &&value) {
        value = QString(value).append(u'!');
    });
    QCOMPARE(data, (QStringList{u"ONE!"_s, u"2!"_s, u"THREE!"_s, u"FOUR!"_s, u"2!"_s}));
    QCOMPARE(changedCells(dataChangedSpy), cellsInRows(0, 4));

    QCOMPARE(observer.emittedSignalNames(), dataChangedSignal);
    VERIFY_HELPER(verifyListModel(adapter, data));
}

void tst_QRangeModelAdapterAlgorithms::modifyingTableAlgorithms()
{
    using Row = std::vector<int>;
    std::vector<Row> data = {
        {3, 30, 300},
        {1, 10, 100},
        {4, 40, 400},
        {1, 11, 111},
        {5, 50, 500},
    };
    QRangeModelAdapter adapter(std::ref(data));
    ModelObserver observer(adapter.model());
    QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);
    const auto verifyModel = [&adapter, &data] {
        const QAbstractItemModel *model = adapter.model();
        QCOMPARE(model->rowCount(), int(data.size()));
        for (int row = 0; row < model->rowCount(); ++row) {
            for (int column = 0; column < model->columnCount(); ++column)
                QCOMPARE(model->index(row, column).data(), data.at(row).at(column));
        }
    };

    // replacing entire rows emits dataChanged for all columns of the row
    std::fill(adapter.begin() + 1, adapter.begin() + 3, Row{0, 0, 0});
    QCOMPARE(data.at(1), (Row{0, 0, 0}));
    QCOMPARE(data.at(2), (Row{0, 0, 0}));
    QCOMPARE(changedRows(dataChangedSpy), (QList<int>{1, 2}));
    VERIFY_HELPER(verifyModel());
    dataChangedSpy.clear();

    std::transform(adapter.begin(), adapter.end(), adapter.begin(), [](const auto &row) {
        Row result(row.cbegin(), row.cend());
        std::reverse(result.begin(), result.end());
        return result;
    });
    QCOMPARE(data, (std::vector<Row>{{300, 30, 3}, {0, 0, 0}, {0, 0, 0},
                                     {111, 11, 1}, {500, 50, 5}}));
    QCOMPARE(changedRows(dataChangedSpy), (QList<int>{0, 1, 2, 3, 4}));
    VERIFY_HELPER(verifyModel());
    dataChangedSpy.clear();

    std::replace_if(adapter.begin(), adapter.end(),
                    [](const auto &row) { return row.at(0) == 0; }, Row{-1, -2, -3});
    QCOMPARE(data.at(1), (Row{-1, -2, -3}));
    QCOMPARE(data.at(2), (Row{-1, -2, -3}));
    QCOMPARE(changedRows(dataChangedSpy), (QList<int>{1, 2}));
    VERIFY_HELPER(verifyModel());
    dataChangedSpy.clear();

    // modifying the columns of a row emits dataChanged for each modified cell
    auto row = *(adapter.begin() + 3);
    std::fill(row.begin(), row.end(), 7);
    QCOMPARE(data.at(3), (Row{7, 7, 7}));
    QCOMPARE(changedCells(dataChangedSpy), (QList<std::pair<int, int>>{{3, 0}, {3, 1}, {3, 2}}));
    VERIFY_HELPER(verifyModel());
    dataChangedSpy.clear();

    std::iota(row.begin(), row.end(), 1);
    QCOMPARE(data.at(3), (Row{1, 2, 3}));
    dataChangedSpy.clear();
    std::transform(row.begin(), row.end(), row.begin(), [](int value) { return value * value; });
    QCOMPARE(data.at(3), (Row{1, 4, 9}));
    QCOMPARE(changedCells(dataChangedSpy), (QList<std::pair<int, int>>{{3, 0}, {3, 1}, {3, 2}}));
    VERIFY_HELPER(verifyModel());
    dataChangedSpy.clear();

    // copying within the row writes each target cell
    std::copy_backward(row.begin(), row.end() - 1, row.end());
    QCOMPARE(data.at(3), (Row{1, 1, 4}));
    QCOMPARE(changedCells(dataChangedSpy), (QList<std::pair<int, int>>{{3, 2}, {3, 1}}));
    VERIFY_HELPER(verifyModel());
    dataChangedSpy.clear();

    // the same column in every row
    std::for_each(adapter.begin(), adapter.end(), [](auto &&row) { row[1] = row.at(1) + 1; });
    QCOMPARE(data, (std::vector<Row>{{300, 31, 3}, {-1, -1, -3}, {-1, -1, -3},
                                     {1, 2, 4}, {500, 51, 5}}));
    QCOMPARE(changedCells(dataChangedSpy),
             (QList<std::pair<int, int>>{{0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}}));
    VERIFY_HELPER(verifyModel());

    QCOMPARE(observer.emittedSignalNames(), dataChangedSignal);
}

void tst_QRangeModelAdapterAlgorithms::modifyingTreeAlgorithms()
{
    auto tree = createValueTree();
    QRangeModelAdapter adapter(std::ref(tree));
    ModelObserver observer(adapter.model());
    QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

    // modify one column of each top-level row
    std::for_each(adapter.begin(), adapter.end(), [](auto &&row) {
        row[1] = row->description().toUpper();
    });
    QStringList descriptions;
    for (const tree_row &row : tree)
        descriptions << row.description();
    QCOMPARE(descriptions, (QStringList{u"ONE"_s, u"TWO"_s, u"THREE"_s, u"FOUR"_s, u"FIVE"_s}));
    QCOMPARE(changedCells(dataChangedSpy),
             (QList<std::pair<int, int>>{{0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}}));
    dataChangedSpy.clear();

    // modify the children of a branch
    auto branch = adapter.at(1).children();
    std::for_each(branch.begin(), branch.end(), [](auto &&row) {
        row[0] = row->value() + u'!';
    });
    const tree_row &branchRow = tree.at(1);
    QStringList values;
    for (const tree_row &child : *branchRow.childRows())
        values << child.value();
    QCOMPARE(values, (QStringList{u"2.1!"_s, u"2.2!"_s, u"2.3!"_s}));
    const QModelIndex branchIndex = adapter.index(1, 0);
    QCOMPARE(changedCells(dataChangedSpy, branchIndex), cellsInRows(0, 2));
    QCOMPARE(changedCells(dataChangedSpy), {});

    // the children still have the correct parent
    for (const tree_row &child : *branchRow.childRows())
        QCOMPARE(child.parentRow(), &branchRow);
    QCOMPARE(adapter.model()->index(2, 0, branchIndex).data().toString(), u"2.3!"_s);

    QCOMPARE(observer.emittedSignalNames(), dataChangedSignal);
}

#if defined(__cpp_lib_ranges)
namespace {
// the value of a tree row, from both the stored row and the row reference
struct TreeRowValue
{
    QString operator()(const tree_row &row) const { return row.value(); }
    template <typename Row>
    QString operator()(const Row &row) const { return row->value(); }
};
} // namespace
#endif

void tst_QRangeModelAdapterAlgorithms::modifyingRangesAlgorithms()
{
#if !defined(__cpp_lib_ranges)
    QSKIP("This test requires C++20 ranges");
#else
    // The range algorithms that write through the iterator, like ranges::fill,
    // or ranges::copy and ranges::transform into the adapter, require the
    // iterator to be std::indirectly_writable. That needs the proxy reference
    // to be assignable also when const, which it is not, so those algorithms
    // are not tested here. Writing through the reference that the algorithms
    // and views pass to us works.
    std::vector<int> data = {5, 3, 9, 1, 7, 2, 8, 6, 4, 0};
    QRangeModelAdapter adapter(&data);
    ModelObserver observer(adapter.model());
    QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);
    const auto verify = [&](const std::vector<int> &expected,
                            const QList<std::pair<int, int>> &expectedCells) {
        QCOMPARE(data, expected);
        QCOMPARE(changedCells(dataChangedSpy), expectedCells);
        VERIFY_HELPER(verifyListModel(adapter, expected));
        dataChangedSpy.clear();
    };

    // The callables need an explicit return type: with libstdc++ before GCC 15,
    // the concept check also calls them with a const reference, and deducing
    // the return type would instantiate the body.
    std::ranges::for_each(adapter, [](auto &&value) -> void { value = value * 10; });
    VERIFY_HELPER(verify({50, 30, 90, 10, 70, 20, 80, 60, 40, 0}, cellsInRows(0, 9)));

    // writing through views over the adapter
    for (auto &&value : adapter | std::views::filter([](int value) { return value > 50; }))
        value = value + 1;
    VERIFY_HELPER(verify({50, 30, 91, 10, 71, 20, 81, 61, 40, 0}, cellsInRows({2, 4, 6, 7})));
    for (auto &&value : adapter | std::views::reverse | std::views::take(3))
        value = -value;
    VERIFY_HELPER(verify({50, 30, 91, 10, 71, 20, 81, -61, -40, 0}, cellsInRows({9, 8, 7})));
    std::ranges::for_each(std::ranges::subrange(adapter.begin() + 1, adapter.begin() + 3),
                          [](auto &&value) -> void { value = 0; });
    VERIFY_HELPER(verify({50, 0, 0, 10, 71, 20, 81, -61, -40, 0}, cellsInRows({1, 2})));
    std::ranges::for_each(adapter | std::views::drop(8), [](auto &&value) -> void { value = 1; });
    VERIFY_HELPER(verify({50, 0, 0, 10, 71, 20, 81, -61, 1, 1}, cellsInRows({8, 9})));

    // writing through the iterator that an algorithm returns
    *std::ranges::max_element(adapter) = 100;
    *std::ranges::find(adapter, 10) = 11;
    VERIFY_HELPER(verify({50, 0, 0, 11, 71, 20, 100, -61, 1, 1}, cellsInRows({6, 3})));
    QCOMPARE(observer.emittedSignalNames(), dataChangedSignal);
    observer.clear();

    { // tables
        using Row = std::vector<int>;
        std::vector<Row> table = {{1, 10, 100}, {2, 20, 200}, {3, 30, 300}};
        QRangeModelAdapter tableAdapter(&table);
        ModelObserver tableObserver(tableAdapter.model());
        QSignalSpy tableSpy(tableAdapter.model(), &QAbstractItemModel::dataChanged);

        // a view of one column
        auto secondColumn = tableAdapter
                          | std::views::transform([](auto &&row) { return row[1]; });
        QVERIFY(std::ranges::equal(secondColumn, std::vector<int>{10, 20, 30}));
        for (auto &&cell : secondColumn)
            cell = cell + 1;
        QCOMPARE(table, (std::vector<Row>{{1, 11, 100}, {2, 21, 200}, {3, 31, 300}}));
        QCOMPARE(changedCells(tableSpy),
                 (QList<std::pair<int, int>>{{0, 1}, {1, 1}, {2, 1}}));
        tableSpy.clear();

        // the projection, like the predicates, gets called with both the
        // row reference and the row type
        const auto lastColumn = [](const auto &row) { return row.at(2); };
        auto maxRow = std::ranges::max_element(tableAdapter, {}, lastColumn);
        QCOMPARE(maxRow - tableAdapter.begin(), 2);
        (*maxRow)[0] = 0;
        *std::ranges::find(tableAdapter, 200, lastColumn) = Row{-2, -21, -200};
        QCOMPARE(table, (std::vector<Row>{{1, 11, 100}, {-2, -21, -200}, {0, 31, 300}}));
        QCOMPARE(changedCells(tableSpy), (QList<std::pair<int, int>>{{2, 0}, {-1, -1}}));
        QCOMPARE(changedRows(tableSpy), (QList<int>{-1, 1}));

        // reading rows into a container
        std::vector<Row> copy;
        std::ranges::copy(tableAdapter | std::views::reverse, std::back_inserter(copy));
        QCOMPARE(copy, (std::vector<Row>(table.rbegin(), table.rend())));
        QCOMPARE(tableObserver.emittedSignalNames(), dataChangedSignal);
    }

    { // trees
        auto tree = createValueTree();
        QRangeModelAdapter treeAdapter(std::ref(tree));
        ModelObserver treeObserver(treeAdapter.model());
        QSignalSpy treeSpy(treeAdapter.model(), &QAbstractItemModel::dataChanged);

        // The concepts of the range algorithms require that callables also
        // work with the stored row type, which for a tree does not have the
        // same interface as the row reference.
        using Value = TreeRowValue;
        auto values = treeAdapter | std::views::transform(Value{});
        QVERIFY(std::ranges::equal(values, QStringList{u"1"_s, u"2"_s, u"3"_s, u"4"_s, u"5"_s}));
        const auto branch = std::ranges::find(treeAdapter, u"2"_s, Value{});
        QVERIFY(branch != treeAdapter.end());
        QVERIFY((*branch).hasChildren());

        auto children = (*branch).children();
        QVERIFY(std::ranges::equal(children | std::views::transform(Value{}),
                                   QStringList{u"2.1"_s, u"2.2"_s, u"2.3"_s}));
        for (auto &&child : children | std::views::reverse | std::views::take(2))
            child[0] = child->value() + u'!';
        QVERIFY(std::ranges::equal(children | std::views::transform(Value{}),
                                   QStringList{u"2.1"_s, u"2.2!"_s, u"2.3!"_s}));
        const QModelIndex branchIndex = treeAdapter.index(1, 0);
        QCOMPARE(changedCells(treeSpy, branchIndex), (QList<std::pair<int, int>>{{2, 0}, {1, 0}}));
        QCOMPARE(treeObserver.emittedSignalNames(), dataChangedSignal);
    }
#endif
}

QTEST_MAIN(tst_QRangeModelAdapterAlgorithms)
#include "tst_qrangemodeladapteralgorithms.moc"
