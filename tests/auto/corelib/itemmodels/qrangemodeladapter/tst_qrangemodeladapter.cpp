// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define Q_ASSERT(cond) ((cond) ? static_cast<void>(0) : qCritical(#cond))
#define Q_ASSERT_X(cond, x, msg) ((cond) ? static_cast<void>(0) \
                                         : qCritical("%s: %s returned false - %s", x, #cond, msg))

#include "../qrangemodel/data.h"

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qpointer.h>
#include <QtCore/qrangemodeladapter.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qxptype_traits.h>

using namespace Qt::StringLiterals;

class tst_QRangeModelAdapter : public QRangeModelTest
{
    Q_OBJECT
public:
    using QRangeModelTest::QRangeModelTest;

    // compile tests
    void setRange_API();

    void indexOfRow_API();
    void indexOfCell_API();
    void indexOfPath_API();

    void dimension_API();

    void iterator_API();
    void access_API();

    void insertRow_API();
    void insertRows_API();
    void removeRow_API();
    void removeRows_API();
    void moveRow_API();
    void moveRows_API();

    void insertColumn_API();
    void insertColumns_API();
    void removeColumn_API();
    void removeColumns_API();
    void moveColumn_API();
    void moveColumns_API();

private slots:
    void init()
    {
        m_data.reset(new Data);
    }
    void cleanup()
    {
        m_data.reset();
    }

    void modelLifetime();
    void valueBehavior();
    void modelReset();

    void listIterate();
    void listAccess();
    void listWriteAccess();

    void tableIterate();
    void tableAccess();
    void tableWriteAccess();

    void treeIterate();
    void treeAccess();
    void treeWriteAccess();

    void insertRow();
    void insertRows();
    void removeRow();
    void removeRows();
    void moveRow();
    void moveRows();

    void insertColumn();
    void insertColumns();
    void removeColumn();
    void removeColumns();
    void moveColumn();
    void moveColumns();

    void buildValueTree();
    void buildPointerTree();

    void insertAutoConnectObjects();

private:
    void expectInvalidIndex(int count)
    {
#ifndef QT_NO_DEBUG
        static QRegularExpression invalidIndex{".* - Index at position is invalid"};

        for (int i = 0; i < count; ++i) // at and DataRef accesses when testing out-of-bounds
            QTest::ignoreMessage(QtCriticalMsg, invalidIndex);
#else
        Q_UNUSED(count);
#endif
    };

    static value_tree createValueTree()
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

    static pointer_tree createPointerTree()
    {
        pointer_tree pointerTree = {
            new tree_row("1", "one"),
            new tree_row("2", "two"),
            new tree_row("3", "three"),
            new tree_row("4", "four"),
            new tree_row("5", "five"),
        };

        pointerTree.at(1)->addChildPointer("2.1", "two.one");
        pointerTree.at(1)->addChildPointer("2.2", "two.two");
        tree_row *row23 = pointerTree.at(1)->addChildPointer("2.3", "two.three");

        row23->addChildPointer("2.3.1", "two.three.one");
        row23->addChildPointer("2.3.2", "two.three.two");

        return pointerTree;
    }

    std::unique_ptr<Data> m_data;
};

namespace {

template <typename Range, typename Protocol>
struct Adapter
{
    using type = decltype(QRangeModelAdapter(std::move(std::declval<Range>()), std::declval<Protocol>()));
};

template <typename Range>
struct Adapter<Range, void>
{
    using type = decltype(QRangeModelAdapter(std::move(std::declval<Range>())));
};

template <typename Range, typename Protocol = void>
using AdapterType = typename Adapter<Range, Protocol>::type;


#define HAS_API(API) \
template <typename Range> \
static constexpr bool has_##API(Range &&) { return API##Test<Range, void>::value; } \
template <typename Range, typename Protocol> \
static constexpr bool has_##API(Range &&, Protocol &&) { return API##Test<Range, Protocol>::value; } \
template <typename Range> auto API##_rt(Range &&) -> typename API##Test<Range, void>::return_type;

#define API_TEST(API, METHOD) \
template <typename Range, typename Protocol, typename = void> \
struct API##Test : std::false_type { using return_type = std::nullptr_t; }; \
template <typename Range, typename Protocol> \
struct API##Test<Range, Protocol, \
                std::void_t<decltype(std::declval<AdapterType<Range, Protocol>>(). METHOD)>> \
    : std::true_type { \
    using return_type = decltype(std::declval<AdapterType<Range, Protocol>>(). METHOD); \
    }; \
HAS_API(API)

API_TEST(setRange, setRange({}))

API_TEST(indexOfRow, index(0))
API_TEST(indexOfCell, index(0, 0))
API_TEST(indexOfPath, index(QList<int>{0, 0}, 0))

API_TEST(columnCount, columnCount())
API_TEST(rowCount, rowCount())
API_TEST(treeRowCount, rowCount(0))
API_TEST(branchRowCount, rowCount(QList<int>{0, 0}))
API_TEST(hasChildren, hasChildren(0))
API_TEST(treeHasChildren, hasChildren(QList<int>{0, 0}))

API_TEST(at, at(0))
API_TEST(subscript, operator[](0))
API_TEST(tableAt, at(0, 0))
API_TEST(tableSubscript, operator[](0, 0))
API_TEST(treeRowAt, at(QList<int>{0, 0}))
API_TEST(treeRowSubscript, operator[](QList<int>{0, 0}))
API_TEST(treeValueAt, at(QList<int>{0, 0}, 0))
API_TEST(treeValueSubscript, operator[](QList<int>{0, 0}, 0))

API_TEST(insertTableRow, insertRow(0))
API_TEST(insertTableRowWithData, insertRow(0, {}))
API_TEST(insertTableRows, insertRows(0, std::declval<Range&>()))
API_TEST(removeRow, removeRow(0))
API_TEST(removeRows, removeRows(0, 0))
API_TEST(moveRow, moveRow(0, 0))
API_TEST(moveTreeRow, moveRow({0}, {0}))
API_TEST(moveRows, moveRows(0, 0, 0))
API_TEST(moveTreeRows, moveRows(QList<int>{0, 0}, 0, QList<int>{0, 0}))

API_TEST(insertColumn, insertColumn(0))
API_TEST(insertColumnWithData, insertColumn(0, {}))
API_TEST(insertColumns, insertColumns(0, std::declval<Range&>()))
API_TEST(removeColumn, removeColumn(0))
API_TEST(removeColumns, removeColumns(0, 0))
API_TEST(moveColumn, moveColumn(0, 0))
API_TEST(moveTreeColumn, moveColumn(QList<int>{}, 0))
API_TEST(moveColumns, moveColumns(0, 0, 0))
API_TEST(moveTreeColumns, moveColumns(QList<int>{}, 0, 0))

API_TEST(getCellProperty, at(0).get()->at(0)->number())
API_TEST(setCellProperty, at(0).get()->at(0)->setNumber(5))

API_TEST(getCellRefProperty, at(0).at(0)->number())
API_TEST(setCellRefProperty, at(0).at(0)->setNumber(5))

API_TEST(getListItemProperty, at(0)->number())
API_TEST(setListItemProperty, at(0).get()->setNumber(5))
}

void tst_QRangeModelAdapter::setRange_API()
{
    Data d;
    auto tree = value_tree{};
    static_assert(has_setRange(d.vectorOfGadgets));
    static_assert(has_setRange(std::move(d.tableOfRowPointers)));
    static_assert(has_setRange(d.m_tree));
    static_assert(has_setRange(tree));
#if (!defined(Q_CC_GNU_ONLY) || Q_CC_GNU > 1303) && !defined(Q_OS_VXWORKS) && !defined(Q_OS_INTEGRITY)
    static_assert(has_setRange(std::ref(tree)));
#endif
    static_assert(has_setRange(std::move(tree)));
    static_assert(!has_setRange(std::as_const(d.vectorOfGadgets)));
    static_assert(!has_setRange(std::as_const(tree)));
}

void tst_QRangeModelAdapter::indexOfRow_API()
{
    Data d;
    static_assert(has_indexOfRow(d.fixedArrayOfNumbers));
    static_assert(!has_indexOfRow(d.vectorOfGadgets)); // table
    static_assert(has_indexOfRow(d.listOfMultiRoleGadgets));
    static_assert(!has_indexOfRow(d.tableOfMetaObjectTuple));

    // naughty cases: tuple<gadget> and tuple<object -> table with a single column
    static_assert(!has_indexOfRow(d.listOfGadgets));
    static_assert(!has_indexOfRow(d.listOfMetaObjectTuple));
}

void tst_QRangeModelAdapter::insertRow_API()
{
    Data d;
    static_assert(!has_insertTableRow(d.fixedArrayOfNumbers));
    static_assert(!has_insertTableRow(d.cArrayOfNumbers));
    static_assert(has_insertTableRow(d.vectorOfFixedColumns));

    static_assert(has_insertTableRow(d.vectorOfArrays));
    static_assert(has_insertTableRow(d.vectorOfGadgets));
    static_assert(has_insertTableRow(d.listOfGadgets));
    static_assert(has_insertTableRow(d.listOfMultiRoleGadgets));
    static_assert(has_insertTableRow(d.vectorOfStructs));
    static_assert(has_insertTableRow(d.listOfObjects));
    static_assert(has_insertTableRow(d.listOfMetaObjectTuple));
    static_assert(has_insertTableRow(d.tableOfMetaObjectTuple));
    static_assert(has_insertTableRow(d.vectorOfConstStructs));
    static_assert(has_insertTableRow(d.tableOfNumbers));
    static_assert(has_insertTableRow(d.tableOfPointers));
    static_assert(has_insertTableRow(d.tableOfRowPointers));
    static_assert(!has_insertTableRow(d.tableOfRowRefs));
    static_assert(!has_insertTableRow(d.arrayOfConstNumbers));
    static_assert(!has_insertTableRow(d.constListOfNumbers));
    static_assert(!has_insertTableRow(d.constTableOfNumbers));
    static_assert(has_insertTableRow(d.listOfNamedRoles));
    static_assert(has_insertTableRow(d.tableOfEnumRoles));
    static_assert(has_insertTableRow(d.tableOfIntRoles));
    static_assert(has_insertTableRow(d.stdTableOfIntRoles));
    static_assert(has_insertTableRow(d.stdTableOfIntRolesWithSharedRows));
    static_assert(has_insertTableRow(d.m_tree));

    // needs explicit protocol:
    // static_assert(has_insertRow(d.m_pointer_tree));
}

void tst_QRangeModelAdapter::indexOfCell_API()
{
    Data d;
    static_assert(!has_indexOfCell(d.fixedArrayOfNumbers));
    static_assert(has_indexOfCell(d.vectorOfGadgets));
    static_assert(!has_indexOfCell(d.listOfMultiRoleGadgets));

    static_assert(has_indexOfCell(d.tableOfNumbers));
    static_assert(has_indexOfCell(d.tableOfMetaObjectTuple));
    static_assert(has_indexOfCell(d.m_tree));

    // tuple<gadget> and tuple<object> -> table wiht a single column
    static_assert(has_indexOfCell(d.listOfGadgets));
    static_assert(has_indexOfCell(d.listOfMetaObjectTuple));
}

void tst_QRangeModelAdapter::indexOfPath_API()
{
    Data d;
    static_assert(!has_indexOfPath(d.fixedArrayOfNumbers));
    static_assert(!has_indexOfPath(d.listOfGadgets));
    static_assert(!has_indexOfPath(d.listOfMultiRoleGadgets));
    static_assert(!has_indexOfPath(d.listOfMetaObjectTuple));
    static_assert(!has_indexOfPath(d.tableOfMetaObjectTuple));

    static_assert(!has_indexOfPath(d.tableOfNumbers));
    static_assert(!has_indexOfPath(d.tableOfMetaObjectTuple));
    static_assert(has_indexOfPath(d.m_tree));
}

void tst_QRangeModelAdapter::dimension_API()
{
    Data d;
    {
        // list
        static_assert(has_columnCount(d.fixedArrayOfNumbers));
        static_assert(has_rowCount(d.fixedArrayOfNumbers));
        static_assert(!has_treeRowCount(d.fixedArrayOfNumbers));
        static_assert(!has_branchRowCount(d.fixedArrayOfNumbers));
        static_assert(!has_hasChildren(d.fixedArrayOfNumbers));
        static_assert(!has_treeHasChildren(d.fixedArrayOfNumbers));

        // tuple table
        static_assert(has_columnCount(d.vectorOfFixedColumns));
        static_assert(has_rowCount(d.vectorOfFixedColumns));
        static_assert(!has_treeRowCount(d.vectorOfFixedColumns));
        static_assert(!has_branchRowCount(d.vectorOfFixedColumns));
        static_assert(!has_hasChildren(d.vectorOfFixedColumns));
        static_assert(!has_treeHasChildren(d.vectorOfFixedColumns));

        // gadget table
        static_assert(has_columnCount(d.vectorOfGadgets));
        static_assert(has_rowCount(d.vectorOfGadgets));
        static_assert(!has_treeRowCount(d.vectorOfGadgets));
        static_assert(!has_branchRowCount(d.vectorOfGadgets));
        static_assert(!has_hasChildren(d.vectorOfGadgets));
        static_assert(!has_treeHasChildren(d.vectorOfGadgets));

        // tree
        static_assert(has_columnCount(d.m_tree));
        static_assert(has_rowCount(d.m_tree));
        static_assert(has_treeRowCount(d.m_tree));
        static_assert(has_branchRowCount(d.m_tree));
        static_assert(has_hasChildren(d.m_tree));
        static_assert(has_treeHasChildren(d.m_tree));
    }
}

#define HAS_OPERATOR_TEST(Name, Op) \
    template <typename It> using Name##_test = decltype(std::declval<It&>() Op std::declval<It&>())

HAS_OPERATOR_TEST(LessThan, <);
HAS_OPERATOR_TEST(GreaterThan, >);
HAS_OPERATOR_TEST(LessThanOrEquals, <=);
HAS_OPERATOR_TEST(GreaterThanOrEquals, <=);

#define HAS_OPERATOR(It, Name) qxp::is_detected_v<Name##_test, It>

#if defined (__cpp_concepts)
template <typename RowType, typename MinCategory>
static constexpr void iterator_API_helper()
{
    QRangeModelAdapter adapter = QRangeModelAdapter(std::vector<RowType>());
    using Adapter = decltype(adapter);

    // the row and column iterators always model random access
    using row_iterator = typename Adapter::iterator;
    static_assert(std::random_access_iterator<row_iterator>);
    using const_row_iterator = typename Adapter::const_iterator;
    static_assert(std::random_access_iterator<const_row_iterator>);

    using column_iterator = typename Adapter::ColumnIterator;
    static_assert(std::random_access_iterator<column_iterator>);
    using const_column_iterator = typename Adapter::ConstColumnIterator;
    static_assert(std::random_access_iterator<const_column_iterator>);

    // the iterator for the view of a row models the same category as the
    // row itself; at least forward iterator
    using rowtype_iterator = typename RowType::iterator;
    using rowview_iterator = decltype(adapter.at(0).get().begin());

    static_assert(std::is_base_of_v<MinCategory,
                  typename std::iterator_traits<rowtype_iterator>::iterator_category>);
    static_assert(std::forward_iterator<rowview_iterator>);
    static_assert(std::bidirectional_iterator<rowview_iterator>
               == std::bidirectional_iterator<rowtype_iterator>);
    static_assert(std::random_access_iterator<rowview_iterator>
               == std::random_access_iterator<rowtype_iterator>);

    static_assert(HAS_OPERATOR(rowview_iterator, LessThan)
               == HAS_OPERATOR(rowtype_iterator, LessThan));
    static_assert(HAS_OPERATOR(rowview_iterator, GreaterThan)
               == HAS_OPERATOR(rowtype_iterator, GreaterThan));
    static_assert(HAS_OPERATOR(rowview_iterator, LessThanOrEquals)
               == HAS_OPERATOR(rowtype_iterator, LessThanOrEquals));
    static_assert(HAS_OPERATOR(rowview_iterator, GreaterThanOrEquals)
               == HAS_OPERATOR(rowtype_iterator, GreaterThanOrEquals));
}
#endif

void tst_QRangeModelAdapter::iterator_API()
{
#if defined (__cpp_concepts)
    {
        using Row = std::array<int, 5>;
        iterator_API_helper<Row, std::random_access_iterator_tag>();
    }
    {
        using Row = std::vector<MultiRoleGadget *>;
        iterator_API_helper<Row, std::random_access_iterator_tag>();
    }

    {
        using Row = std::list<std::shared_ptr<MultiRoleGadget>>;
        iterator_API_helper<Row, std::bidirectional_iterator_tag>();
    }
#endif
}


#define has_with_type(fn, Range, Ret) \
    has_##fn(Range) && std::is_same_v<decltype(fn##_rt( Range)), Ret>

template <typename Range>
static constexpr auto iterator_type(Range r) -> decltype(QRangeModelAdapter(std::move(r)).begin());

template <typename Range>
static constexpr auto rowref_type(Range r) -> decltype(iterator_type(std::move(r)).operator*());

template <typename Range>
static constexpr auto dataref_type(Range r) -> typename decltype(QRangeModelAdapter(std::move(r)))::DataReference;

void tst_QRangeModelAdapter::access_API()
{
    Data d;
    { // list: std::array<int, 5>
        using data_ref = decltype(dataref_type(d.fixedArrayOfNumbers));
        static_assert(has_with_type(at, d.fixedArrayOfNumbers, data_ref));
        static_assert(has_with_type(at, std::as_const(d.fixedArrayOfNumbers), int));
        static_assert(has_with_type(subscript, d.fixedArrayOfNumbers, data_ref));
        static_assert(has_with_type(subscript, std::as_const(d.fixedArrayOfNumbers), int));

        static_assert(!has_tableAt(d.fixedArrayOfNumbers));
        static_assert(!has_treeRowAt(d.fixedArrayOfNumbers));
        static_assert(!has_treeValueAt(d.fixedArrayOfNumbers));
    }

    { // list: int[5]
        using data_ref = decltype(QRangeModelAdapter(std::move(d.cArrayOfNumbers)))::DataReference;
        static_assert(has_with_type(at, d.cArrayOfNumbers, data_ref));
        static_assert(has_with_type(at, std::as_const(d.cArrayOfNumbers), int));
        static_assert(has_with_type(subscript, d.cArrayOfNumbers, data_ref));
        static_assert(has_with_type(subscript, std::as_const(d.cArrayOfNumbers), int));

        static_assert(!has_tableAt(d.cArrayOfNumbers));
        static_assert(!has_treeRowAt(d.cArrayOfNumbers));
        static_assert(!has_treeValueAt(d.cArrayOfNumbers));
    }

    { // table: vector of tuple
        using row_ref = decltype(rowref_type(d.vectorOfFixedColumns));
        using row_type = std::tuple<int, QString>;
        using data_ref = decltype(dataref_type(d.vectorOfFixedColumns));
        static_assert(has_with_type(at, d.vectorOfFixedColumns, row_ref));
        static_assert(has_with_type(at, std::as_const(d.vectorOfFixedColumns), const row_type &));
        static_assert(has_with_type(subscript, d.vectorOfFixedColumns, row_ref));
        static_assert(has_with_type(subscript, std::as_const(d.vectorOfFixedColumns), const row_type &));

        static_assert(has_with_type(tableAt, d.vectorOfFixedColumns, data_ref));
        static_assert(has_with_type(tableAt, std::as_const(d.vectorOfFixedColumns), QVariant));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, d.vectorOfFixedColumns, data_ref));
        static_assert(has_with_type(tableSubscript, std::as_const(d.vectorOfFixedColumns), QVariant));
#endif

        static_assert(!has_treeRowAt(d.vectorOfFixedColumns));
        static_assert(!has_treeValueAt(d.vectorOfFixedColumns));
    }

    { // table: vector of shared_ptr<tuple>
        using row_type = std::shared_ptr<std::tuple<int, QString>>;
        using row_ref = decltype(rowref_type(d.vectorOfFixedSPtrColumns));
        using data_ref = decltype(dataref_type(d.vectorOfFixedSPtrColumns));
        static_assert(has_with_type(at, d.vectorOfFixedSPtrColumns, row_ref));
        static_assert(has_with_type(at, std::as_const(d.vectorOfFixedSPtrColumns), const row_type &));
        static_assert(has_with_type(subscript, d.vectorOfFixedSPtrColumns, row_ref));
        static_assert(has_with_type(subscript, std::as_const(d.vectorOfFixedSPtrColumns), const row_type &));

        static_assert(has_with_type(tableAt, d.vectorOfFixedSPtrColumns, data_ref));
        static_assert(has_with_type(tableAt, std::as_const(d.vectorOfFixedSPtrColumns), QVariant));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, d.vectorOfFixedSPtrColumns, data_ref));
        static_assert(has_with_type(tableSubscript, std::as_const(d.vectorOfFixedSPtrColumns), QVariant));
#endif

        static_assert(!has_treeRowAt(d.vectorOfFixedSPtrColumns));
        static_assert(!has_treeValueAt(d.vectorOfFixedSPtrColumns));
    }

#ifndef Q_OS_INTEGRITY
    { // table: std::vector<std::array<int, 10>>
        using row_type = std::array<int, 10>;
        using row_ref = decltype(rowref_type(d.vectorOfArrays));
        using data_ref = decltype(dataref_type(d.vectorOfArrays));
        static_assert(has_with_type(at, d.vectorOfArrays, row_ref));
        static_assert(has_with_type(at, std::as_const(d.vectorOfArrays), const row_type &));
        static_assert(has_with_type(subscript, d.vectorOfArrays, row_ref));
        static_assert(has_with_type(subscript, std::as_const(d.vectorOfArrays), const row_type &));

        static_assert(has_with_type(tableAt, d.vectorOfArrays, data_ref));
        static_assert(has_with_type(tableAt, std::as_const(d.vectorOfArrays), int));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, d.vectorOfArrays, data_ref));
        static_assert(has_with_type(tableSubscript, std::as_const(d.vectorOfArrays), int));
#endif

        static_assert(!has_treeRowAt(d.vectorOfArrays));
        static_assert(!has_treeValueAt(d.vectorOfArrays));
    }
#endif

    { // table: std::vector<Item>
        using row_ref = decltype(rowref_type(d.vectorOfGadgets));
        using data_ref = decltype(dataref_type(d.vectorOfGadgets));
        static_assert(has_with_type(at, d.vectorOfGadgets, row_ref));
        static_assert(has_with_type(at, std::as_const(d.vectorOfGadgets), const Item &));
        static_assert(has_with_type(subscript, d.vectorOfGadgets, row_ref));
        static_assert(has_with_type(subscript, std::as_const(d.vectorOfGadgets), const Item &));

        static_assert(has_with_type(tableAt, d.vectorOfGadgets, data_ref));
        static_assert(has_with_type(tableAt, std::as_const(d.vectorOfGadgets), QVariant));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, d.vectorOfGadgets, data_ref));
        static_assert(has_with_type(tableSubscript, std::as_const(d.vectorOfGadgets), QVariant));
#endif

        static_assert(!has_treeRowAt(d.vectorOfGadgets));
        static_assert(!has_treeValueAt(d.vectorOfGadgets));
    }

    { // 1-column table: std::vector<std::tuple<Item>>
        using row_type = std::tuple<Item>;
        using row_ref = decltype(rowref_type(d.listOfGadgets));
        using data_ref = decltype(dataref_type(d.listOfGadgets));
        static_assert(has_with_type(at, d.listOfGadgets, row_ref));
        static_assert(has_with_type(at, std::as_const(d.listOfGadgets), const row_type &));
        static_assert(has_with_type(subscript, d.listOfGadgets, row_ref));
        static_assert(has_with_type(subscript, std::as_const(d.listOfGadgets), const row_type &));

        static_assert(has_with_type(tableAt, d.listOfGadgets, data_ref));
        static_assert(has_with_type(tableAt, std::as_const(d.listOfGadgets), Item));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, d.listOfGadgets, data_ref));
        static_assert(has_with_type(tableSubscript, std::as_const(d.listOfGadgets), Item));
#endif

        static_assert(!has_treeRowAt(d.listOfGadgets));
        static_assert(!has_treeValueAt(d.listOfGadgets));
    }

    { // list: std::vector<MultiRoleGadget>
        using row_type = MultiRoleGadget;
        using data_ref = decltype(dataref_type(d.listOfMultiRoleGadgets));
        static_assert(has_with_type(at, d.listOfMultiRoleGadgets, data_ref));
        static_assert(has_with_type(at, std::as_const(d.listOfMultiRoleGadgets), const row_type &));
        static_assert(has_with_type(subscript, d.listOfMultiRoleGadgets, data_ref));
        static_assert(has_with_type(subscript, std::as_const(d.listOfMultiRoleGadgets), const row_type &));

        static_assert(!has_tableAt(d.listOfMultiRoleGadgets));
        static_assert(!has_treeRowAt(d.listOfMultiRoleGadgets));
        static_assert(!has_treeValueAt(d.listOfMultiRoleGadgets));

        static_assert(has_getListItemProperty(d.listOfMultiRoleGadgets));
        static_assert(!has_setListItemProperty(d.listOfMultiRoleGadgets));
    }

#ifndef Q_OS_INTEGRITY
    { // list: std::vector<ItemAccessType>
        using row_type = ItemAccessType;
        using data_ref = decltype(dataref_type(d.vectorOfItemAccess));
        static_assert(has_with_type(at, d.vectorOfItemAccess, data_ref));
        static_assert(has_with_type(at, std::as_const(d.vectorOfItemAccess), row_type));
        static_assert(has_with_type(subscript, d.vectorOfItemAccess, data_ref));
        static_assert(has_with_type(subscript, std::as_const(d.vectorOfItemAccess), row_type));

        static_assert(!has_tableAt(d.vectorOfItemAccess));
        static_assert(!has_treeRowAt(d.listOfGadgets));
        static_assert(!has_treeValueAt(d.listOfGadgets));
    }
#endif

#if (!defined(Q_CC_GNU_ONLY) || Q_CC_GNU > 1303) && !defined(Q_OS_VXWORKS) && !defined(Q_OS_INTEGRITY)
    { // table: std::list<Object *>
        using row_ref = decltype(rowref_type(std::ref(d.listOfObjects)));
        using data_ref = decltype(dataref_type(std::ref(d.listOfObjects)));
        static_assert(has_with_type(at, std::ref(d.listOfObjects), row_ref));
        static_assert(has_with_type(at, std::ref(std::as_const(d.listOfObjects)), Object *const &));
        static_assert(has_with_type(subscript, std::ref(d.listOfObjects), row_ref));
        static_assert(has_with_type(subscript, std::ref(std::as_const(d.listOfObjects)), Object *const &));

        static_assert(has_with_type(tableAt, std::ref(d.listOfObjects), data_ref));
        static_assert(has_with_type(tableAt, std::ref(std::as_const(d.listOfObjects)), QVariant));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, std::ref(d.listOfObjects), data_ref));
        static_assert(has_with_type(tableSubscript, std::ref(std::as_const(d.listOfObjects)), QVariant));
#endif

        static_assert(!has_treeRowAt(std::ref(d.listOfObjects)));
        static_assert(!has_treeValueAt(std::ref(d.listOfObjects)));
    }
#endif

    { // table: std::vector<std::vector<double>>
        using row_type = std::vector<double>;
        using row_ref = decltype(rowref_type(d.tableOfNumbers));
        using data_ref = decltype(dataref_type(d.tableOfNumbers));
        static_assert(has_with_type(at, d.tableOfNumbers, row_ref));
        static_assert(has_with_type(at, std::as_const(d.tableOfNumbers), const row_type &));
        static_assert(has_with_type(subscript, d.tableOfNumbers, row_ref));
        static_assert(has_with_type(subscript, std::as_const(d.tableOfNumbers), const row_type &));

        static_assert(has_with_type(tableAt, d.tableOfNumbers, data_ref));
        static_assert(has_with_type(tableAt, std::as_const(d.tableOfNumbers), double));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, d.tableOfNumbers, data_ref));
        static_assert(has_with_type(tableSubscript, std::as_const(d.tableOfNumbers), double));
#endif

        static_assert(!has_treeRowAt(d.tableOfNumbers));
        static_assert(!has_treeValueAt(d.tableOfNumbers));
    }

    { // table: std::vector<std::vector<Item *>>
        using row_type = std::vector<Item *>;
        using row_ref = decltype(rowref_type(d.tableOfPointers));
        using data_ref = decltype(dataref_type(d.tableOfPointers));
        static_assert(has_with_type(at, d.tableOfPointers, row_ref));
        static_assert(has_with_type(at, std::as_const(d.tableOfPointers), const row_type &));
        static_assert(has_with_type(subscript, d.tableOfPointers, row_ref));
        static_assert(has_with_type(subscript, std::as_const(d.tableOfPointers), const row_type &));

        static_assert(has_with_type(tableAt, d.tableOfPointers, data_ref));
        static_assert(has_with_type(tableAt, std::as_const(d.tableOfPointers), const Item *));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, d.tableOfPointers, data_ref));
        static_assert(has_with_type(tableSubscript, std::as_const(d.tableOfPointers), const Item *));
#endif

        static_assert(!has_treeRowAt(d.tableOfPointers));
        static_assert(!has_treeValueAt(d.tableOfPointers));
    }

    { // table: std::vector<std::ref<Row>>
        using row_type = std::reference_wrapper<Row>;
        using row_ref = decltype(rowref_type(d.tableOfRowRefs));
        using data_ref = decltype(dataref_type(d.tableOfRowRefs));
        static_assert(has_with_type(at, d.tableOfRowRefs, row_ref));
        static_assert(has_with_type(at, std::as_const(d.tableOfRowRefs), const row_type &));
        static_assert(has_with_type(subscript, d.tableOfRowRefs, row_ref));
        static_assert(has_with_type(subscript, std::as_const(d.tableOfRowRefs), const row_type &));

        static_assert(has_with_type(tableAt, d.tableOfRowRefs, data_ref));
        static_assert(has_with_type(tableAt, std::as_const(d.tableOfRowRefs), QVariant));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, d.tableOfRowRefs, data_ref));
        static_assert(has_with_type(tableSubscript, std::as_const(d.tableOfRowRefs), QVariant));
#endif

        static_assert(!has_treeRowAt(d.tableOfRowRefs));
        static_assert(!has_treeValueAt(d.tableOfRowRefs));
    }

    { // table of shared rows holding shared objects
        using data_type = std::shared_ptr<Object>;
        using row_type = std::shared_ptr<std::vector<data_type>>;
        std::vector<row_type> table;
        using row_ref = decltype(rowref_type(table));
        using data_ref = decltype(dataref_type(table));
        static_assert(has_with_type(at, table, row_ref));

        static_assert(has_with_type(at, std::as_const(table), const row_type &));
        static_assert(has_with_type(tableAt, table, data_ref));
        static_assert(has_with_type(tableAt, std::as_const(table), std::shared_ptr<const Object>));
    }

    { // table of raw rows holding raw objects
        using data_type = Object *;
        using row_type = std::vector<data_type> *;
        std::vector<row_type> table;
        using row_ref = decltype(rowref_type(&table));
        using data_ref = decltype(dataref_type(&table));
        static_assert(has_with_type(at, &table, row_ref));
        static_assert(has_with_type(at, &std::as_const(table), const row_type &));
        static_assert(has_with_type(tableAt, &table, data_ref));
        static_assert(has_with_type(tableAt, &std::as_const(table), const Object *));

        static_assert(has_getCellProperty(table));
        // we turn row pointers into pointers to const rows, but we don't make
        // the element of that pointer also const... ###
        static_assert(has_setCellProperty(table));
    }

    { // table of rows holding shared pointers
        using data_type = Object *;
        using row_type = std::vector<data_type>;
        std::vector<row_type> table;

        QRangeModelAdapter adapter(std::ref(table));
        adapter.at(0).at(0)->number();
        static_assert(has_getCellRefProperty(table));
        static_assert(!has_setCellRefProperty(table));
    }

    { // list: std::vector<QVariantMap>
        using row_type = QVariantMap;
        using data_ref = decltype(dataref_type(d.listOfNamedRoles));
        static_assert(has_with_type(at, d.listOfNamedRoles, data_ref));
        static_assert(has_with_type(at, std::as_const(d.listOfNamedRoles), row_type));
        static_assert(has_with_type(subscript, d.listOfNamedRoles, data_ref));
        static_assert(has_with_type(subscript, std::as_const(d.listOfNamedRoles), row_type));

        static_assert(!has_tableAt(d.listOfNamedRoles));
        static_assert(!has_treeRowAt(d.listOfNamedRoles));
        static_assert(!has_treeValueAt(d.listOfNamedRoles));
    }

    { // tree: std::vector<tree_row>
        const value_tree const_tree;
        using row_type = tree_row;
        using row_ref = decltype(QRangeModelAdapter(std::move(d.m_tree)).at(0));
        using data_ref = decltype(dataref_type(std::move(d.m_tree)));

        static_assert(has_with_type(at, d.m_tree, row_ref));
        static_assert(has_with_type(at, const_tree, const row_type &));
        static_assert(has_with_type(subscript, d.m_tree, row_ref));
        static_assert(has_with_type(subscript, const_tree, const row_type &));

        static_assert(has_with_type(tableAt, d.m_tree, data_ref));
        static_assert(has_with_type(tableAt, const_tree, QString));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(tableSubscript, d.m_tree, data_ref));
        static_assert(has_with_type(tableSubscript, const_tree, QString));
#endif

        static_assert(has_with_type(treeRowAt, d.m_tree, row_ref));
        // not a const ref, but a view of the row
        static_assert(has_with_type(treeRowAt, const_tree, const row_type &));

        static_assert(has_with_type(treeValueAt, d.m_tree, data_ref));
        static_assert(has_with_type(treeValueAt, const_tree, QString));
#if defined(__cpp_multidimensional_subscript)
        static_assert(has_with_type(treeValueSubscript, d.m_tree, data_ref));
        static_assert(has_with_type(treeValueSubscript, const_tree, QString));
#endif
    }
}


void tst_QRangeModelAdapter::insertRows_API()
{
    Data d;
    static_assert(!has_insertTableRows(d.fixedArrayOfNumbers));
    static_assert(has_insertTableRows(d.vectorOfGadgets));
    static_assert(has_insertTableRows(d.listOfMultiRoleGadgets));
    static_assert(has_insertTableRows(d.listOfNamedRoles));
    static_assert(has_insertTableRows(d.listOfObjects));
    static_assert(has_insertTableRows(d.stdTableOfIntRoles));

    static_assert(has_insertTableRowWithData(d.vectorOfFixedColumns));
}

void tst_QRangeModelAdapter::removeRow_API()
{
    Data d;
    static_assert(!has_removeRow(d.fixedArrayOfNumbers));
    static_assert(has_removeRow(d.vectorOfGadgets));
    static_assert(!has_removeRow(d.constListOfNumbers));
    static_assert(has_removeRow(d.m_tree));
}

void tst_QRangeModelAdapter::removeRows_API()
{
    Data d;
    static_assert(!has_removeRows(d.fixedArrayOfNumbers));
    static_assert(has_removeRows(d.vectorOfGadgets));
    static_assert(!has_removeRows(d.constListOfNumbers));
    static_assert(has_removeRows(d.m_tree));
}

void tst_QRangeModelAdapter::moveRow_API()
{
    Data d;
    static_assert(has_moveRow(d.fixedArrayOfNumbers));
    static_assert(has_moveRow(d.vectorOfGadgets));
    static_assert(!has_moveRow(d.constListOfNumbers));
    static_assert(has_moveRow(d.m_tree));
}

void tst_QRangeModelAdapter::moveRows_API()
{
    Data d;
    static_assert(has_moveRows(d.fixedArrayOfNumbers));
    static_assert(has_moveRows(d.vectorOfGadgets));
    static_assert(!has_moveRows(d.constListOfNumbers));
    static_assert(has_moveRows(d.m_tree));
    static_assert(!has_moveTreeRows(d.vectorOfGadgets));
    static_assert(has_moveTreeRows(d.m_tree));
}

void tst_QRangeModelAdapter::insertColumn_API()
{
    Data d;
    static_assert(!has_insertColumn(d.fixedArrayOfNumbers));
    static_assert(!has_insertColumn(d.vectorOfFixedColumns));
    static_assert(!has_insertColumn(d.vectorOfArrays));
    static_assert(!has_insertColumn(d.vectorOfGadgets));
    static_assert(!has_insertColumn(d.vectorOfConstStructs));

    static_assert(has_insertColumn(d.tableOfNumbers));
    static_assert(!has_insertColumn(d.constTableOfNumbers));
    static_assert(has_insertColumn(d.tableOfPointers));
    static_assert(!has_insertColumn(d.tableOfRowPointers));
    static_assert(!has_insertColumn(d.listOfNamedRoles));
    static_assert(!has_insertColumn(d.m_tree));

    static_assert(has_insertColumnWithData(d.tableOfNumbers));
    static_assert(!has_insertColumnWithData(d.constTableOfNumbers));
    static_assert(has_insertColumnWithData(d.tableOfPointers));
}

void tst_QRangeModelAdapter::insertColumns_API()
{
    Data d;
    static_assert(!has_insertColumns(d.fixedArrayOfNumbers));
    static_assert(!has_insertColumns(d.vectorOfFixedColumns));
    static_assert(!has_insertColumns(d.vectorOfArrays));
    static_assert(!has_insertColumns(d.vectorOfGadgets));
    static_assert(!has_insertColumns(d.vectorOfConstStructs));

    static_assert(has_insertColumns(d.tableOfNumbers));
    static_assert(!has_insertColumns(d.constTableOfNumbers));
    static_assert(has_insertColumns(d.tableOfPointers));
    static_assert(!has_insertColumns(d.tableOfRowPointers));
    static_assert(!has_insertColumns(d.listOfNamedRoles));
    static_assert(!has_insertColumns(d.m_tree));
}

void tst_QRangeModelAdapter::removeColumn_API()
{
    Data d;
    static_assert(!has_removeColumn(d.fixedArrayOfNumbers));
    static_assert(!has_removeColumn(d.vectorOfFixedColumns));
    static_assert(!has_removeColumn(d.vectorOfArrays));
    static_assert(!has_removeColumn(d.vectorOfGadgets));
    static_assert(!has_removeColumn(d.vectorOfConstStructs));

    static_assert(has_removeColumn(d.tableOfNumbers));
    static_assert(!has_removeColumn(d.constTableOfNumbers));
    static_assert(has_removeColumn(d.tableOfPointers));
    static_assert(!has_removeColumn(d.tableOfRowPointers));
    static_assert(!has_removeColumn(d.listOfNamedRoles));
    static_assert(!has_removeColumn(d.m_tree));
}

void tst_QRangeModelAdapter::removeColumns_API()
{
    Data d;
    static_assert(!has_removeColumns(d.fixedArrayOfNumbers));
    static_assert(!has_removeColumns(d.vectorOfFixedColumns));
    static_assert(!has_removeColumns(d.vectorOfArrays));
    static_assert(!has_removeColumns(d.vectorOfGadgets));
    static_assert(!has_removeColumns(d.vectorOfConstStructs));

    static_assert(has_removeColumns(d.tableOfNumbers));
    static_assert(!has_removeColumns(d.constTableOfNumbers));
    static_assert(has_removeColumns(d.tableOfPointers));
    static_assert(!has_removeColumns(d.tableOfRowPointers));
    static_assert(!has_removeColumns(d.listOfNamedRoles));
    static_assert(!has_removeColumns(d.m_tree));
}

void tst_QRangeModelAdapter::moveColumn_API()
{
    Data d;
    static_assert(!has_moveColumn(d.fixedArrayOfNumbers));
    static_assert(!has_moveColumn(d.vectorOfFixedColumns));
    static_assert(!has_moveColumn(d.vectorOfGadgets));
    static_assert(!has_moveColumn(d.vectorOfConstStructs));

    static_assert(has_moveColumn(d.vectorOfArrays));
    static_assert(has_moveColumn(d.tableOfNumbers));
    static_assert(!has_moveColumn(d.constTableOfNumbers));
    static_assert(has_moveColumn(d.tableOfPointers));
    static_assert(!has_moveColumn(d.tableOfRowPointers));
    static_assert(!has_moveColumn(d.listOfNamedRoles));
    static_assert(!has_moveColumn(d.m_tree));

    static_assert(!has_moveTreeColumn(d.m_tree));
}

void tst_QRangeModelAdapter::moveColumns_API()
{
    Data d;
    static_assert(!has_moveColumns(d.fixedArrayOfNumbers));
    static_assert(!has_moveColumns(d.vectorOfFixedColumns));
    static_assert(!has_moveColumns(d.vectorOfGadgets));
    static_assert(!has_moveColumns(d.vectorOfConstStructs));

    static_assert(has_moveColumns(d.vectorOfArrays));
    static_assert(has_moveColumns(d.tableOfNumbers));
    static_assert(!has_moveColumns(d.constTableOfNumbers));
    static_assert(has_moveColumns(d.tableOfPointers));
    static_assert(!has_moveColumns(d.tableOfRowPointers));
    static_assert(!has_moveColumns(d.listOfNamedRoles));
    static_assert(!has_moveColumns(d.m_tree));

    static_assert(!has_moveTreeColumns(d.m_tree));
}


void tst_QRangeModelAdapter::modelLifetime()
{
    std::vector<int> data;
    QPointer<QRangeModel> model;
    QPointer<QRangeModel> model2;

    {
        QRangeModelAdapter adapter(&data);
        model = adapter.model();
        QVERIFY(model);
    }
    QVERIFY(!model);

    {
        auto adapter = QRangeModelAdapter(&data);
        model = adapter.model();
        QVERIFY(model);

        {
            auto adapterCopy = adapter;
            QVERIFY(model);
            QCOMPARE(adapterCopy.model(), adapter.model());

            {
                std::vector<int> data2;
                adapterCopy = QRangeModelAdapter(&data2);
                model2 = adapterCopy.model();
                QVERIFY(model2);
                QCOMPARE_NE(adapterCopy.model(), adapter.model());
            }
            QVERIFY(model2);
        }
        QVERIFY(!model2);
        QVERIFY(model);

        auto movedToAdapter = std::move(adapter);
        QVERIFY(!adapter.model());
        QVERIFY(movedToAdapter.model());
        QVERIFY(model);
    }
    QVERIFY(!model);
}

void tst_QRangeModelAdapter::valueBehavior()
{
    QRangeModelAdapter adapter(QList<int>{});
    // make sure we don't construct from range, but make a copy
    QRangeModelAdapter adapter2(adapter);
    static_assert(std::is_same_v<decltype(adapter), decltype(adapter2)>);
    QCOMPARE(adapter.model(), adapter2.model());
    auto copy = adapter;
    static_assert(std::is_same_v<decltype(adapter), decltype(copy)>);
    QCOMPARE(adapter, copy);
    QCOMPARE(copy.model(), adapter.model());
    auto movedTo = std::move(adapter);
    QCOMPARE(movedTo, copy);
    QCOMPARE_NE(movedTo, adapter);
    QVERIFY(!adapter.model());
}

void tst_QRangeModelAdapter::modelReset()
{
    {
        QRangeModelAdapter adapter(std::vector<int>{});
        QSignalSpy modelAboutToBeResetSpy(adapter.model(), &QAbstractItemModel::modelAboutToBeReset);
        QSignalSpy modelResetSpy(adapter.model(), &QAbstractItemModel::modelReset);

        QCOMPARE(adapter.range(), std::vector<int>());

        adapter.setRange(std::vector<int>{1, 2, 3, 4, 5});
        QCOMPARE(modelAboutToBeResetSpy.count(), 1);
        QCOMPARE(modelResetSpy.count(), 1);

        QCOMPARE(adapter.rowCount(), 5);
        QCOMPARE(adapter[0], 1);

        adapter.setRange({3, 2, 1});
        QCOMPARE(modelAboutToBeResetSpy.count(), 2);
        QCOMPARE(modelResetSpy.count(), 2);
        QCOMPARE(adapter.rowCount(), 3);
        QCOMPARE(adapter[0], 3);

        QCOMPARE(adapter, (std::vector<int>{3, 2, 1}));
        modelAboutToBeResetSpy.clear();
        modelResetSpy.clear();

        std::vector<int> modifiedData = adapter;

        adapter.assign(modifiedData.begin(), modifiedData.end());
        QCOMPARE(modelResetSpy.count(), 1);
        adapter.setRange(std::vector<int>{3, 2, 1});
        QCOMPARE(modelResetSpy.count(), 2);
        std::vector<short> shorts = {10, 11, 12};
        adapter.assign(shorts.begin(), shorts.end());
        QCOMPARE(modelResetSpy.count(), 3);
    }

    {
        Object *object = new Object;
        QPointer<Object> watcher = object;

        QRangeModelAdapter adapter(QList<Object *>{object});
        adapter = {};
        QVERIFY(!watcher);
    }

    {
        QRangeModelAdapter adapter(createValueTree());
        adapter.at(0) = tree_row{};
        QCOMPARE(std::as_const(adapter).at(0, 0), "");
        QCOMPARE(std::as_const(adapter).at(0, 1), "");
        adapter.setRange(createValueTree());
        QCOMPARE(std::as_const(adapter).at(0, 0), "1");
        QCOMPARE(std::as_const(adapter).at(0, 1), "one");
    }

    {
        QStringList list;
        QRangeModelAdapter adapter(list);
        auto setList = [](const QStringList &) {};
        setList(adapter);
        QVariant var = list;
    }
}

void tst_QRangeModelAdapter::listIterate()
{
    {
        std::vector<int> data = {0, 1, 2, 3, 4};
        QRangeModelAdapter adapter(std::ref(data));

        QCOMPARE(adapter.end() - adapter.begin(), 5);
        QCOMPARE(adapter.end() - adapter.end(), 0);
        QCOMPARE(adapter.begin() - adapter.end(), -5);

        // test special handling of moving back from end()
        auto end = adapter.end();
        QCOMPARE(*(--end), 4);
        end = adapter.end();
        QCOMPARE(end--, adapter.end());
        QCOMPARE(*end, 4);
        end = adapter.end();
        end -= 2;
        QCOMPARE(*end, 3);
        QCOMPARE(*(adapter.end() - 1), 4);

        std::vector<int> values;
        for (const auto &d : std::as_const(adapter))
            values.push_back(d);
        QCOMPARE(values, data);

        for (auto d : adapter)
            d = d + 1;
        QCOMPARE(data, (std::vector{1, 2, 3, 4, 5}));
    }
}

void tst_QRangeModelAdapter::listAccess()
{
    {
        std::vector<int> data = {0, 1, 2, 3, 4};
        const int size = int(data.size());

        {
            QRangeModelAdapter adapter(data);
            QCOMPARE(adapter.at(1), 1);
            QCOMPARE(adapter.data(1).metaType(), QMetaType::fromType<int>());
            QCOMPARE(adapter.data(1), 1);
            QCOMPARE(adapter[1], 1);
            QCOMPARE(adapter.at(4), 4);
            QCOMPARE(adapter.data(4), 4);
            swap(adapter[0], adapter[4]);
            QCOMPARE(adapter.data(4), 0);
            QCOMPARE(adapter.data(0), 4);
            QVERIFY(adapter.setData(0, QVariant(0)));
            QVERIFY(adapter.setData(4, QVariant(4)));
            expectInvalidIndex(3);  // out-of-bounds access of vector and DataRef
            QCOMPARE(adapter.at(size), 0);
        }
        {
            QRangeModelAdapter adapter(std::as_const(data));
            QCOMPARE(adapter.at(1), 1);
            QCOMPARE(adapter.data(1), 1);
            QCOMPARE(adapter[1], 1);
            QCOMPARE(adapter.at(4), 4);
            expectInvalidIndex(1);  // out-of-bounds access of vector
            QCOMPARE(adapter.at(size), 0);
        }
        {
            const QRangeModelAdapter adapter(data);
            QCOMPARE(adapter.at(1), 1);
            QCOMPARE(adapter.data(1), 1);
            QCOMPARE(adapter[1], 1);
            QCOMPARE(adapter.at(4), 4);
            expectInvalidIndex(1);  // out-of-bounds access of vector
            QCOMPARE(adapter.at(size), 0);
        }
        {
            const QRangeModelAdapter adapter(std::as_const(data));
            QCOMPARE(adapter.at(1), 1);
            QCOMPARE(adapter.data(1), 1);
            QCOMPARE(adapter[1], 1);
            QCOMPARE(adapter.at(4), 4);
            expectInvalidIndex(1);  // out-of-bounds access of vector
            QCOMPARE(adapter[size], 0);
        }
    }

    { // this is a table (std::vector<Item>)
        QList<Item> gadgets = {m_data->vectorOfGadgets.begin(), m_data->vectorOfGadgets.end()};

        {
            const QRangeModelAdapter adapter(gadgets);
            QCOMPARE(adapter.at(1), gadgets.at(1));
            QCOMPARE(adapter.data(1, 0).metaType(), QMetaType::fromType<QString>());
            QCOMPARE(adapter.data(1, 1).metaType(), QMetaType::fromType<QColor>());
            QCOMPARE(adapter.data(1, 2).metaType(), QMetaType::fromType<QString>());
            QCOMPARE(adapter[1], gadgets[1]);
            QCOMPARE(adapter.at(2), gadgets.at(2));
        }
    }

    {
        auto gadgets = m_data->listOfMultiRoleGadgets;
        const int size = int(gadgets.size());

        {
            const QRangeModelAdapter adapter(gadgets);
            QCOMPARE(adapter.at(0), gadgets.at(0));
            QCOMPARE(adapter.data(0).metaType(), QMetaType::fromType<MultiRoleGadget>());
            QCOMPARE(adapter.data(0).value<MultiRoleGadget>(), gadgets.at(0));
            QCOMPARE(adapter.data(0, Qt::DisplayRole), gadgets.at(0).m_display);
            QCOMPARE(adapter.data(1, Qt::DecorationRole), gadgets.at(1).m_decoration);
            QCOMPARE(adapter.data(2, Qt::UserRole), gadgets.at(2).number());
            QCOMPARE(adapter.data(2, Qt::UserRole + 1), gadgets.at(2).m_user);
            QCOMPARE(adapter.at(size - 1), gadgets.at(size - 1));
            expectInvalidIndex(1);  // access of vector
            QCOMPARE(adapter.at(size), MultiRoleGadget{});
        }
    }
}

void tst_QRangeModelAdapter::listWriteAccess()
{
    auto gadgets = m_data->listOfMultiRoleGadgets;
    const int size = int(gadgets.size());

    QRangeModelAdapter adapter(&gadgets);
    QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

    MultiRoleGadget first = adapter.at(0);
    MultiRoleGadget last = adapter.at(size - 1);
    QCOMPARE(first, gadgets.at(0));
    QCOMPARE(last, gadgets.at(size - 1));
    QCOMPARE(dataChangedSpy.size(), 0);

    adapter[0] = last;
    QCOMPARE(dataChangedSpy.size(), 1);
    adapter[size - 1] = first;
    QCOMPARE(dataChangedSpy.size(), 2);
    QCOMPARE(last, gadgets.at(0));
    QCOMPARE(first, gadgets.at(size - 1));
    QCOMPARE(dataChangedSpy.size(), 2);

    swap(adapter.at(0), adapter.at(size - 1));
    QCOMPARE(dataChangedSpy.size(), 4);
    QCOMPARE(first, gadgets.at(0));
    QCOMPARE(last, gadgets.at(size - 1));
    QCOMPARE(dataChangedSpy.size(), 4);
    dataChangedSpy.clear();

    // DataRef(const DataRef &) should set the value on the model
    adapter[size - 1] = adapter.at(0);
    QCOMPARE(dataChangedSpy.size(), 1);
}

void tst_QRangeModelAdapter::tableIterate()
{
    {
        auto table = m_data->vectorOfFixedColumns;
        QRangeModelAdapter adapter(std::ref(table));
        QCOMPARE(adapter.end() - adapter.begin(), adapter.rowCount());

        QVariantList rowValues;
        QVariantList itemValues;
        { // const access
            for (const auto &row : std::as_const(adapter)) {
                std::tuple<int, QString> rowTuple = row;
                auto [number, string] = rowTuple;
                rowValues << number;
                rowValues << string;
                QCOMPARE(row.size(), 2);
                QCOMPARE(row.at(0), number);
                QCOMPARE(row.at(1), string);
                for (const auto &value : row)
                    itemValues << value;
            }
            QCOMPARE(rowValues, (QList<QVariant>{
                0, "null", 1, "one", 2, "two", 3, "three", 4, "four"
            }));
            QCOMPARE(itemValues, rowValues);
            rowValues.clear();
            itemValues.clear();
        }

        QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

        { // read access via mutable iterators
            for (auto row : adapter) {
                std::tuple<int, QString> rowTuple = row;
                auto [number, string] = rowTuple;
                rowValues << number;
                rowValues << string;
                for (auto value : row)
                    itemValues << value;
            }
            QCOMPARE(rowValues, (QList<QVariant>{
                0, "null", 1, "one", 2, "two", 3, "three", 4, "four"
            }));
            QCOMPARE(itemValues, rowValues);
        }

        { // write access via mutable iterators
            for (auto row : adapter) {
                row = {0, "0"};
                for (auto value : row) {
                    QCOMPARE(value, 0);
                    value = 42;
                }
            }
            for (auto tableRow : table) {
                QCOMPARE(tableRow, std::tuple(42, u"42"_s));
            }
        }
    }
}

template <typename Adapter, typename Table>
void verifyTupleTable(Adapter &&adapter, const Table &table)
{
    const int size = int(table.size());

    QCOMPARE(adapter.at(0), table.at(0));
    // QCOMPARE(adapter.at(size), {}); // asserts, as it should
    QCOMPARE(adapter.at(0, 0), std::get<0>(table.at(0)));
    QCOMPARE(adapter.data(0, 0), adapter.at(0, 0));
    QCOMPARE(adapter.at(1, 1), std::get<1>(table.at(1)));
    QCOMPARE(adapter.at(size, 1), QVariant{});
    QCOMPARE(adapter.at(1, 2), QVariant{});
}

template <typename Adapter, typename Table>
void verifyGadgetTable(const Adapter &adapter, const Table &table)
{
    const int size = int(table.size());

    QCOMPARE(adapter.at(0), table.at(0));
    // QCOMPARE(adapter.at(size), {}); // asserts, as it should
    QCOMPARE(adapter.at(0, 0), table.at(0).display());
    QCOMPARE(adapter.data(0, 0).metaType(), QMetaType::fromType<QString>());
    QCOMPARE(adapter.data(0, 1).metaType(), QMetaType::fromType<QColor>());
    QCOMPARE(adapter.data(0, 0), table.at(0).display());
    QCOMPARE(adapter.at(1, 1), table.at(1).decoration());
    QCOMPARE(adapter.at(2, 2), table.at(2).toolTip());
    QCOMPARE(adapter.at(size, 1), QVariant{});
    QCOMPARE(adapter.at(0, 3), QVariant{});
}

template <typename Adapter, typename Table>
void verifyPointerTable(const Adapter &adapter, const Table &table)
{
    [[maybe_unused]] const int size = int(table.size());

    using ItemType = std::remove_reference_t<decltype(*table.at(0).at(0))>;

    // row
    QCOMPARE(adapter.at(0), table.at(0));

    // cell
    QCOMPARE(adapter.data(0, 0).metaType(), QMetaType::fromType<ItemType *>());
    QCOMPARE(adapter.data(0, 0), QVariant::fromValue(table.at(0).at(0)));
    QCOMPARE(adapter.at(0, 0), table.at(0).at(0));
}

void tst_QRangeModelAdapter::tableAccess()
{
    {
        auto table = m_data->vectorOfFixedColumns;
        {
            QRangeModelAdapter adapter(table);
            expectInvalidIndex(6); // at and DataRef accesses when testing out-of-bounds
            verifyTupleTable(adapter, table);
        }

        {
            QRangeModelAdapter adapter(std::as_const(table));
            expectInvalidIndex(2); // at and DataRef accesses when testing out-of-bounds
            verifyTupleTable(adapter, table);
        }

        {
            const QRangeModelAdapter adapter(table);
            expectInvalidIndex(2); // at and DataRef accesses when testing out-of-bounds
            verifyTupleTable(adapter, table);
        }

        {
            const QRangeModelAdapter adapter(std::as_const(table));
            expectInvalidIndex(2); // at and DataRef accesses when testing out-of-bounds
            verifyTupleTable(adapter, table);
        }
    }

    {
        auto table = m_data->vectorOfGadgets;
        {
            QRangeModelAdapter adapter(table);
            expectInvalidIndex(2); // at and DataRef accesses when testing out-of-bounds
            verifyGadgetTable(adapter, table);
        }

        {
            QRangeModelAdapter adapter(std::as_const(table));
            expectInvalidIndex(2); // at and DataRef accesses when testing out-of-bounds
            verifyGadgetTable(adapter, table);
        }

        {
            const QRangeModelAdapter adapter(table);
            expectInvalidIndex(2); // at and DataRef accesses when testing out-of-bounds
            verifyGadgetTable(adapter, table);
        }

        {
            const QRangeModelAdapter adapter(std::as_const(table));
            expectInvalidIndex(2); // at and DataRef accesses when testing out-of-bounds
            verifyGadgetTable(adapter, table);
        }
    }

    {
        auto table = m_data->tableOfPointers;
        {
            QRangeModelAdapter adapter(table);
            verifyPointerTable(adapter, table);
        }
        {
            QRangeModelAdapter adapter(std::as_const(table));
            verifyPointerTable(adapter, table);
        }
        {
            const QRangeModelAdapter adapter(table);
            verifyPointerTable(adapter, table);
        }
        {
            const QRangeModelAdapter adapter(std::as_const(table));
            verifyPointerTable(adapter, table);
        }
    }

    {
        std::vector<std::vector<Object *>> table = {
            {new Object, new Object},
            {new Object, new Object}
        };
        {
            QRangeModelAdapter adapter(table);
            verifyPointerTable(adapter, table);
        }
    }
}

void tst_QRangeModelAdapter::tableWriteAccess()
{
    using std::swap;
    {
        auto table = m_data->vectorOfFixedColumns;
        const int size = int(table.size());

        QRangeModelAdapter adapter(&table);
        QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

        adapter[0] = {0, "null"};
        QCOMPARE(dataChangedSpy.size(), 1);
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), adapter.index(0, 0));
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), adapter.index(0, 1));

        dataChangedSpy.clear();
        QCOMPARE(adapter.at(0, 0), 0);
        QCOMPARE(adapter.at(0, 1), "null");

        { // model outlives adapter
            QRangeModelAdapter adapterCopy = adapter;
            adapterCopy.at(0) = {-1, "dirty"};
            adapterCopy.at(0) = {0, "dirty"};
        }
        QCOMPARE(dataChangedSpy.size(), 2);
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), adapter.index(0, 0));
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), adapter.index(0, 1));
        dataChangedSpy.clear();

        { // all modifications result in notification
            QRangeModelAdapter adapterCopy = adapter;
            adapterCopy.at(0) = {0, "null"};
            adapter.at(1) = {1, "dirty"};
        }
        QCOMPARE(dataChangedSpy.size(), 2);

        // order of signal emissions is defined
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), adapter.index(0, 0));
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), adapter.index(0, 1));
        QCOMPARE(dataChangedSpy.at(1).at(0).value<QModelIndex>(), adapter.index(1, 0));
        QCOMPARE(dataChangedSpy.at(1).at(1).value<QModelIndex>(), adapter.index(1, 1));
        dataChangedSpy.clear();

        swap(adapter[0], adapter[size - 1]);
        QCOMPARE(dataChangedSpy.size(), 2);
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), adapter.index(0, 0));
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), adapter.index(0, 1));
        QCOMPARE(dataChangedSpy.at(1).at(0).value<QModelIndex>(), adapter.index(size - 1, 0));
        QCOMPARE(dataChangedSpy.at(1).at(1).value<QModelIndex>(), adapter.index(size - 1, 1));
        dataChangedSpy.clear();

        QVERIFY(adapter.setData(0, 0, -1, Qt::DisplayRole));
        QVERIFY(adapter.setData(0, 1, "Minus one", Qt::DisplayRole));
        QCOMPARE(dataChangedSpy.size(), 2);
    }

    {
        auto table = m_data->tableOfNumbers;
        const int lastRow = int(table.size() - 1);
        const int lastColumn = int(table.at(0).size() - 1);

        QRangeModelAdapter adapter(&table);
        QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

        QCOMPARE(adapter[0], table.at(0));

        adapter[lastRow] = adapter[0];
        QCOMPARE(dataChangedSpy.size(), 1);
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), adapter.index(lastRow, 0));
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), adapter.index(lastRow, lastColumn));
        dataChangedSpy.clear();

        adapter[lastRow] = {21.1, 22.1, 23.1, 24.1, 25.1};
        QCOMPARE(dataChangedSpy.size(), 1);
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), adapter.index(lastRow, 0));
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), adapter.index(lastRow, lastColumn));
        dataChangedSpy.clear();

        // this breaks table topology, and would assert; we have to do it last
#ifndef QT_NO_DEBUG
        QTest::ignoreMessage(QtCriticalMsg,
                             QRegularExpression(".* The new row has the wrong size!"));
#endif
        adapter[0] = std::vector<double>{1.0};
    }

    { // table with raw row pointers
        std::vector<Object *> table = {
            new Object,
            new Object,
        };
        QRangeModelAdapter adapter(std::ref(table));
        QCOMPARE(adapter.rowCount(), 2);
        QCOMPARE(adapter.columnCount(), 2);

        QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

        adapter.at(0, 0) = "1/1";
        adapter.at(0, 1) = 10;
        QCOMPARE(table.at(0)->string(), "1/1");
        QCOMPARE(table.at(0)->number(), 10);
        QCOMPARE(dataChangedSpy.count(), 2);
        dataChangedSpy.clear();

        QVERIFY(adapter.at(0) != nullptr);
        QCOMPARE(dataChangedSpy.count(), 0); // nothing written to the wrapper

        adapter.at(0) = new Object;
        QCOMPARE(dataChangedSpy.count(), 1);
        // data in entire row changed
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), adapter.index(0, 0));
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), adapter.index(0, 1));
    }

    { // table with item pointers
        std::vector<std::vector<Object *>> table = {
            {new Object, new Object},
            {new Object, new Object},
        };
        QRangeModelAdapter adapter(&table);
        QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

        QVERIFY(adapter.at(0, 0) != nullptr);
        QCOMPARE(dataChangedSpy.count(), 0);
#ifndef QT_NO_DEBUG
        // we can't replace items that are pointers
        QTest::ignoreMessage(QtCriticalMsg,
                             QRegularExpression("Not able to assign QVariant"));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Writing value of type Object\\* to "
                             "role Qt::RangeModelAdapterRole at index .* failed"));
#endif
        adapter.at(0, 0) = new Object;
        QCOMPARE(dataChangedSpy.count(), 0);
    }

    { // table with smart item pointers
        std::vector<std::vector<std::shared_ptr<Object>>> table = {
            {std::make_shared<Object>("1.1", 1), std::make_shared<Object>("1.2", 2)},
            {std::make_shared<Object>("2.1", 3), std::make_shared<Object>("2.2", 4)},
        };
        QRangeModelAdapter adapter(&table);
        QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

        // we only allow read-access to objects, as otherwise we'd not update
        // the model
        std::shared_ptr<const Object> topLeft = adapter.at(0, 0);
        QCOMPARE(topLeft, table.at(0).at(0));
        QCOMPARE(dataChangedSpy.count(), 0);
        adapter.at(0, 0) = std::make_shared<Object>("0", 0);
        QCOMPARE(dataChangedSpy.count(), 1);
        QCOMPARE(table.at(0).at(0)->string(), "0");
        QCOMPARE(table.at(0).at(0)->number(), 0);

        // we get a shared_ptr<const Object> and want to assign as a
        // shared_ptr<Object>. This is not possible - and that's ok, because
        // we'd end up with the same object in multiple places.
        // adapter.at(0, 0) = adapter.at(1, 1);
        // adapter.at(0, 0) = topLeft;

        // Explicitly getting a row yields a view-like wrapper around the
        // vector, preventing direct write access to the objects stored in the
        // table.
        auto row = adapter.at(0).get();
        QCOMPARE(row.at(0)->number(), table.at(0).at(0)->number());
        // row.at(0)->setNumber(3);
        auto begin = row.begin();
        QCOMPARE(begin->number(), row.at(0)->number());
        // not allowed (P2836R1)
        // std::vector<std::shared_ptr<Object>>::const_iterator it = begin;
        // (*it)->setNumber(3); // this would be possible

        int column = 0;
        for (const auto &cell : row) {
            QCOMPARE(row.at(column)->string(), cell->string());
            ++column;
            // cell.setNumber(3);
        }
    }
}

template <typename Adapter>
QStringList rowValues(Adapter &&adapter)
{
    QStringList result;
    for (auto row : adapter) {
        result << row->value() << row->description();
        if (row.hasChildren())
            result << rowValues(row.children());
    }
    return result;
}

template <typename Adapter>
QStringList itemValues(Adapter &&adapter)
{
    QStringList result;
    for (auto row : adapter) {
        for (auto value : row)
            result << value;
        if (row.hasChildren())
            result << itemValues(row.children());
    }
    return result;
}

void tst_QRangeModelAdapter::treeIterate()
{
    const QStringList expectedValues = {
            "1", "one",
            "2", "two",
                "2.1", "two.one",
                "2.2", "two.two",
                "2.3", "two.three",
                    "2.3.1", "two.three.one",
                    "2.3.2", "two.three.two",
            "3", "three",
            "4", "four",
            "5", "five"
    };

    { // read from const adapter over const tree
        const auto tree = createValueTree();
        auto printTreeOnError = qScopeGuard([&tree]{
            tree_row::prettyPrint(qDebug().nospace() << "tree at test failure:\n", tree);
        });

        const QRangeModelAdapter adapter(std::cref(tree));

        auto top = adapter.begin();
        QCOMPARE(top->value(), expectedValues.front());
        QCOMPARE(top, adapter.cbegin());

        auto topLeft = (*top).cbegin();
        QCOMPARE(topLeft, (*top).begin());
        QVERIFY(!topLeft->isEmpty());
        QCOMPARE(*topLeft, top->value());

        QStringList allRows = rowValues(adapter);
        QStringList allItems = itemValues(adapter);

        QCOMPARE(allRows, expectedValues);
        QCOMPARE(allItems, expectedValues);

        printTreeOnError.dismiss();
    }

    { // read from const adapter over mutable tree
        auto tree = createValueTree();
        auto printTreeOnError = qScopeGuard([&tree]{
            tree_row::prettyPrint(qDebug().nospace() << "tree at test failure:\n", tree);
        });

        const QRangeModelAdapter adapter(std::ref(tree));

        auto top = adapter.begin();
        QCOMPARE(top->value(), expectedValues.front());
        QCOMPARE(top, adapter.cbegin());

        auto topLeft = (*top).cbegin();
        QCOMPARE(topLeft, (*top).begin());
        QVERIFY(!topLeft->isEmpty());
        QCOMPARE(*topLeft, top->value());

        QStringList allRows = rowValues(adapter);
        QStringList allItems = itemValues(adapter);

        QCOMPARE(allRows, expectedValues);
        QCOMPARE(allItems, expectedValues);

        printTreeOnError.dismiss();
    }

    { // mutable adapter over const tree
        const auto tree = createValueTree();
        auto printTreeOnError = qScopeGuard([&tree]{
            tree_row::prettyPrint(qDebug().nospace() << "tree at test failure:\n", tree);
        });

        QRangeModelAdapter adapter(std::ref(tree));

        auto top = adapter.begin();
        QCOMPARE(top->value(), expectedValues.front());
        QCOMPARE(top, adapter.cbegin());

        auto topLeft = (*top).cbegin();
        QCOMPARE(topLeft, (*top).begin());
        QVERIFY(!topLeft->isEmpty());
        QCOMPARE(*topLeft, top->value());

        QStringList allRows = rowValues(adapter);
        QStringList allItems = itemValues(adapter);

        QCOMPARE(allRows, expectedValues);
        QCOMPARE(allItems, expectedValues);

        // We can safely access children on a const model, even if there is no
        // range to back it up.
        const auto &topRow = *top;
        QVERIFY(!topRow.hasChildren());
        QCOMPARE(topRow.children().size(), 0);
        int iterCount = 0;
        for (const auto &child : topRow.children()) {
            Q_UNUSED(child);
            ++iterCount;
        }
        QCOMPARE(iterCount, 0);

        ++top;
        const auto &secondRow = *top;
        QVERIFY(secondRow.hasChildren());
        QCOMPARE_NE(secondRow.children().size(), 0);

        printTreeOnError.dismiss();
    }

    { // mutable adapter over mutable tree
        auto tree = createValueTree();
        auto printTreeOnError = qScopeGuard([&tree]{
            tree_row::prettyPrint(qDebug().nospace() << "tree at test failure:\n", tree);
        });

        QRangeModelAdapter adapter(std::ref(tree));
        QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);
        QSignalSpy rowsRemovedSpy(adapter.model(), &QAbstractItemModel::rowsRemoved);
        QSignalSpy rowsInsertedSpy(adapter.model(), &QAbstractItemModel::rowsInserted);

        auto top = adapter.begin();
        QCOMPARE(top->value(), expectedValues.front());
        QCOMPARE(top, adapter.cbegin());
        QCOMPARE((*top).at(0), top->value());

        auto topLeft = (*top).cbegin();
        QCOMPARE(topLeft, (*top).begin());
        QVERIFY(!topLeft->isEmpty());
        QCOMPARE(*topLeft, top->value());

        QStringList allRows = rowValues(adapter);
        QStringList allItems = itemValues(adapter);

        QCOMPARE(allRows, expectedValues);
        QCOMPARE(allItems, expectedValues);

        // nothing changed so far
        QCOMPARE(dataChangedSpy.count(), 0);
        QCOMPARE(rowsRemovedSpy.count(), 0);
        QCOMPARE(rowsInsertedSpy.count(), 0);

        // add zero children - no change to rows
        auto topRow = *top;
        QVERIFY(!topRow.hasChildren());
        topRow.children() = {};
        QVERIFY(!topRow.hasChildren());
        QCOMPARE(rowsRemovedSpy.count(), 0);
        QCOMPARE(rowsInsertedSpy.count(), 0);

        // replace children
        auto secondRow = *(top + 1);
        QVERIFY(secondRow.hasChildren());

        secondRow.at(0) = "reset";
        QCOMPARE(dataChangedSpy.count(), 1);
        secondRow[1] = "clear";
        QCOMPARE(dataChangedSpy.count(), 2);
        dataChangedSpy.clear();

        secondRow.children() = createValueTree();
        QCOMPARE(rowsRemovedSpy.count(), 1);
        QCOMPARE(rowsInsertedSpy.count(), 1);

        // clear children
        secondRow.children() = {};
        QCOMPARE(rowsRemovedSpy.count(), 2);
        QCOMPARE(rowsInsertedSpy.count(), 1);

        // add children
        secondRow.children() = createValueTree();
        QCOMPARE(rowsRemovedSpy.count(), 2);
        QCOMPARE(rowsInsertedSpy.count(), 2);

        printTreeOnError.dismiss();
    }
}

template <typename Adapter, typename Tree>
void verifyTree(const Adapter &adapter, Tree &&tree)
{
    using QRangeModelDetails::refTo;
    const int size = int(tree.size());

    QVERIFY(!adapter.hasChildren(0));
    QVERIFY(adapter.hasChildren(1));
    QVERIFY(!adapter.hasChildren(2));
    QVERIFY(!adapter.hasChildren(3));

    // row access
    QCOMPARE(refTo(adapter.at(0)).value(), refTo(tree.at(0)).value());
    QVERIFY(!refTo(adapter.at({1, 1})).description().isEmpty());
    QCOMPARE(refTo(adapter.at(1)).description(), refTo(tree.at(1)).description());
    // QCOMPARE(adapter.at(size), {}); // asserts, as it should

    // value access
    QCOMPARE(adapter.at(0, 0), refTo(tree.at(0)).value());
    QCOMPARE(adapter.data(0, 0).metaType(), QMetaType::fromType<QString>());
    QCOMPARE(adapter.data(0, 0), refTo(tree.at(0)).value());
    QCOMPARE(adapter.at(1, 1), refTo(tree.at(1)).description());
    QCOMPARE(adapter.at(size, 0), QString{});
    QCOMPARE(adapter.at(0, adapter.columnCount()), QString{});

    QVERIFY(!adapter.data({0, 0}, 0).isValid());
    QCOMPARE(adapter.at({0, 0}, 0), QString{});
    QCOMPARE(adapter.at(0, 0), "1");
    QCOMPARE(adapter.at(0, 1), "one");
    QCOMPARE(adapter.at({1, 0}, 0), "2.1");
    QVERIFY(adapter.data({1, 0}, 0).isValid());
    QCOMPARE(adapter.at({1, 0}, 1), "two.one");
    QCOMPARE(adapter.at({1, 2, 0}, 0), "2.3.1");
    QCOMPARE(adapter.at({1, 2, 1}, 1), "two.three.two");
}

void tst_QRangeModelAdapter::treeAccess()
{
    {
        auto tree = createValueTree();
        QRangeModelAdapter adapter(std::ref(tree));
        expectInvalidIndex(4); // row, column, and non-existing children
        verifyTree(adapter, tree);
        // adapter.at(0).value() = u"123"_s;
        adapter.at(0) = tree_row{"1", "eins"};
        adapter.at(0, 1) = "1";
    }

    {
        auto tree = createValueTree();
        QRangeModelAdapter adapter(std::ref(std::as_const(tree)));
        expectInvalidIndex(4); // row, column, and non-existing children
        verifyTree(adapter, tree);
    }

    {
        auto tree = createValueTree();
        const QRangeModelAdapter adapter(std::ref(tree));
        expectInvalidIndex(4); // row, column, and non-existing children
        verifyTree(adapter, tree);
    }

    {
        auto tree = createValueTree();
        const QRangeModelAdapter adapter(std::ref(std::as_const(tree)));
        expectInvalidIndex(4); // row, column, and non-existing children
        verifyTree(adapter, tree);
    }

    using PointerProtocol = tree_row::ProtocolPointerImpl;
    {
        auto tree = createPointerTree();
        QRangeModelAdapter adapter(std::ref(tree), PointerProtocol{});
        expectInvalidIndex(4); // row, column, and non-existing children
        verifyTree(adapter, tree);
    }

    {
        auto tree = createPointerTree();
        QRangeModelAdapter adapter(std::ref(std::as_const(tree)), PointerProtocol{});
        expectInvalidIndex(4); // row, column, and non-existing children
        verifyTree(adapter, tree);
    }

    {
        auto tree = createPointerTree();
        const QRangeModelAdapter adapter(std::ref(tree), PointerProtocol{});
        expectInvalidIndex(4); // row, column, and non-existing children
        verifyTree(adapter, tree);
    }

    {
        auto tree = createPointerTree();
        const QRangeModelAdapter adapter(std::ref(std::as_const(tree)), PointerProtocol{});
        expectInvalidIndex(4); // row, column, and non-existing children
        verifyTree(adapter, tree);
    }
}

void tst_QRangeModelAdapter::treeWriteAccess()
{
    {
        auto tree = createValueTree();
        QRangeModelAdapter adapter(std::ref(tree));
        const int lastColumn = adapter.columnCount() - 1;
        QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

        adapter.at(0) = tree_row{};
        QCOMPARE(dataChangedSpy.size(), 1);
        QCOMPARE(adapter.at(0, 0), "");
        QCOMPARE(adapter.at(0, 1), "");
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), adapter.index(0, 0));
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), adapter.index(0, lastColumn));
        dataChangedSpy.clear();

        adapter.at({1, 0}) = {"x", "X"};
        QCOMPARE(dataChangedSpy.size(), 1);
        QCOMPARE(adapter.at({1, 0}, 0), "x");
        QCOMPARE(adapter.at({1, 0}, 1), "X");
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), adapter.index({1, 0}, 0));
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), adapter.index({1, 0}, lastColumn));
        dataChangedSpy.clear();

        adapter.at({1, 2, 1}) = {"y", "Y"};
        const auto changedLeft = adapter.index({1, 2, 1}, 0);
        const QPersistentModelIndex trackedLeft = changedLeft;
        const auto changedRight = adapter.index({1, 2, 1}, lastColumn);
        const QPersistentModelIndex trackedRight = changedRight;
        QVERIFY(adapter.removeRow({1, 2, 0}));
        QCOMPARE(dataChangedSpy.size(), 1);
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), changedLeft);
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), changedRight);
        QCOMPARE_NE(changedLeft, trackedLeft);
        QCOMPARE_NE(changedRight, trackedRight);
        dataChangedSpy.clear();

        adapter.at({1, 2, 0}, 0) = "z";
        QCOMPARE(dataChangedSpy.size(), 1);
        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), trackedLeft);
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), trackedLeft);
        adapter.at({1, 2, 0}, 1) = "Z";
        QCOMPARE(dataChangedSpy.size(), 2);
        QCOMPARE(dataChangedSpy.at(1).at(0).value<QModelIndex>(), trackedRight);
        QCOMPARE(dataChangedSpy.at(1).at(1).value<QModelIndex>(), trackedRight);
        dataChangedSpy.clear();

        QVERIFY(adapter.setData({1, 2, 0}, 0, "y"));
        QCOMPARE(dataChangedSpy.size(), 1);
        QVERIFY(adapter.setData({1, 2, 0}, 1, "Y"));
        QCOMPARE(dataChangedSpy.size(), 2);
        dataChangedSpy.clear();
    }

    {
        auto tree = createPointerTree();
        // use a special protocol to check for row deletion
        struct MarkDirtyProtocol : tree_row::ProtocolPointerImpl {
            void deleteRow(tree_row *row) {
                row->value() = "deleted";
                row->description() = "deleted";
            }
        };

        QRangeModelAdapter adapter(std::ref(tree), MarkDirtyProtocol{});
        const QRangeModelAdapter constAdapter = adapter;
        QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);

        QCOMPARE(constAdapter.at(0, 0), "1");
        QCOMPARE(constAdapter.at(0, 1), "one");

        // adapter.at(0) = nullptr; // would corrupt the tree, so not allowed

        // overwriting the tree row value would not inform the model
        // *adapter.at(0) = {};

        // but we can overwrite individual items
        adapter.at(0, 0) = "";
        adapter.at(0, 1) = "";
        QCOMPARE(constAdapter.at(0, 0), "");
        QCOMPARE(constAdapter.at(0, 1), "");

        auto row = constAdapter.at(4);
        QCOMPARE(row->value(), "5");
        QCOMPARE(row->description(), "five");

        // not allowed, as we get a const tree_row * and can't assign to
        // a tree_row *. Good, as otherwise we'd have the same pointer twice!
        // adapter.at(0) = row;

        // we can replace the old tree row with a new one
        row = adapter.at(0);
        adapter.at(0) = new tree_row{"new", "row"};
        QCOMPARE(constAdapter.at(0, 0), "new");
        QCOMPARE(constAdapter.at(0, 1), "row");

        // and the old row got deleted
        QCOMPARE(row->value(), "deleted");
        QCOMPARE(row->description(), "deleted");

    }
}

void tst_QRangeModelAdapter::insertRow()
{
    {
        QList<int> data;
        QRangeModelAdapter adapter(std::ref(data));

        for (int i = 0; i < 2; ++i) {
            QVERIFY(adapter.insertRow(data.size(), i));
            if (i)
                QVERIFY(adapter.insertRow(0, -i));
        }

        QCOMPARE(data, (QList<int>{-1, 0, 1}));
    }

    {
        auto data = m_data->vectorOfFixedColumns;
        auto oldSize = data.size();

        QRangeModelAdapter adapter(std::ref(data));
        // append
        QVERIFY(adapter.insertRow(int(oldSize), {5, "five"}));
        QCOMPARE(data.size(), ++oldSize);

        // inserted
        std::tuple<int, QString> newRow = {6, "six"};
        QVERIFY(adapter.insertRow(int(oldSize / 2), newRow));
        // not moved
        QVERIFY(!std::get<QString>(newRow).isEmpty());
        QCOMPARE(data.size(), ++oldSize);

        // prepend
        QVERIFY(adapter.insertRow(0, newRow));
        QCOMPARE(data.size(), ++oldSize);

        // move
        QVERIFY(adapter.insertRow(0, std::move(newRow)));
        QCOMPARE(data.size(), ++oldSize);
        QVERIFY(std::get<QString>(newRow).isEmpty());
    }
}

void tst_QRangeModelAdapter::insertRows()
{
#if defined Q_CC_MSVC && _MSC_VER < 1944
    QSKIP("Internal compiler error with older MSVC versions");
#else
    {
        QList<QString> data;
        QList<QString> newData = {u"one"_s, u"two"_s, u"three"_s};
        QRangeModelAdapter adapter(&data);

        QVERIFY(adapter.insertRows(0, newData));
        QCOMPARE(data, newData);
        data.clear();

        // move newData into data
        const auto oldNewData = newData;
        QVERIFY(adapter.insertRows(0, std::move(newData)));
        QVERIFY(newData.at(0).isEmpty());
        QCOMPARE(data, oldNewData);
    }

    {
        auto data = m_data->vectorOfFixedColumns;
        QRangeModelAdapter adapter(std::ref(data));

        // std::vector has insert(pos, first, last)
        for (int i = 0; i < 10; ++i) {
            auto localCopy = data;
            const size_t oldSize = data.size();
            QVERIFY(adapter.insertRows(0, localCopy));
            QCOMPARE(data.size(), oldSize * 2);
        }

        // inserting into self is UB, so verify that we handle that gracefully. However,
        // the inner inserter returning false doesn't abort the begin/endInsertRows, as we
        // don't have a way of canceling such an operation - so expect_fail here until we
        // have a solution.
        QEXPECT_FAIL("", "QAIM has no way to cancel an ongoing insertion operation", Continue);
        QVERIFY(!adapter.insertRows(0, data));
    }
#endif
}

void tst_QRangeModelAdapter::removeRow()
{
    QList<int> data = {0, 1, 2, 3, 4};
    QRangeModelAdapter adapter(&data);
    QVERIFY(adapter.removeRow(0));
    QCOMPARE(data, (QList<int>{1, 2, 3, 4}));
}

void tst_QRangeModelAdapter::removeRows()
{
    std::vector<std::vector<int>> data = {
        {0},
        {1},
        {2},
        {3},
        {4},
    };
    QRangeModelAdapter adapter(&data);
    QVERIFY(adapter.removeRows(1, 3));
    QVERIFY(!adapter.removeRows(1, 7));
    QCOMPARE(data, (std::vector<std::vector<int>>{{0},{4}}));
}

void tst_QRangeModelAdapter::moveRow()
{
    std::list<int> data = {0, 1, 2, 3, 4};
    QRangeModelAdapter adapter(&data);
    QVERIFY(adapter.moveRow(0, 4));
    QCOMPARE(data, (std::list<int>{1, 2, 3, 0, 4}));
}

void tst_QRangeModelAdapter::moveRows()
{
    std::list<int> data = {0, 1, 2, 3, 4};
    QRangeModelAdapter adapter(&data);
    QVERIFY(adapter.moveRows(3, 2, 0));
    QCOMPARE(data, (std::list<int>{3, 4, 0, 1, 2}));
}

void tst_QRangeModelAdapter::insertColumn()
{
    std::vector<std::vector<QString>> table = {
        {"1"},
        {"11"},
        {"21"}
    };
    QRangeModelAdapter adapter(std::ref(table));
    QVERIFY(adapter.insertColumn(0));

    QCOMPARE(table, (std::vector<std::vector<QString>>{
        {"", "1"},
        {"", "11"},
        {"", "21"}
    }));

    QVERIFY(adapter.insertColumn(2, u"100"_s));
    QCOMPARE(table, (std::vector<std::vector<QString>>{
        {"", "1", "100"},
        {"", "11", "100"},
        {"", "21", "100"}
    }));

    QVERIFY(adapter.insertColumn(1, QList<QString>{
        "one", "eleven"
    }));
    QCOMPARE(table, (std::vector<std::vector<QString>>{
        {"", "one", "1", "100"},
        {"", "eleven", "11", "100"},
        {"", "one", "21", "100"}
    }));
}

void tst_QRangeModelAdapter::insertColumns()
{
    { // with insert(range)
        std::vector<std::vector<int>> table = {
            {0},
            {10},
            {20}
        };
        QRangeModelAdapter adapter(std::ref(table));
        QVERIFY(adapter.insertColumns(1, QList{1, 2}));
        QCOMPARE(table, (std::vector<std::vector<int>>{
            {0, 1, 2},
            {10, 1, 2},
            {20, 1, 2}
        }));
    }

    { // without insert(range)
        QList<QList<int>> table = {
            {0},
            {10},
            {20}
        };

        QRangeModelAdapter adapter(std::ref(table));
        QVERIFY(adapter.insertColumns(1, QList{1, 2}));
        QCOMPARE(table, (QList<QList<int>>{
            {0, 1, 2},
            {10, 1, 2},
            {20, 1, 2}
        }));

        QVERIFY(adapter.insertColumns(0, QList<QList<int>>{
            {-2, -1},
            {-12, -11}
        }));

        QCOMPARE(table, (QList<QList<int>>{
            {-2, -1, 0, 1, 2},
            {-12, -11, 10, 1, 2},
            {-2, -1, 20, 1, 2}
        }));
    }
}

void tst_QRangeModelAdapter::removeColumn()
{
    {
        QList<QList<QString>> table = {
            {"1"},
            {"11"},
            {"21"}
        };
        QRangeModelAdapter adapter(&table);
        QVERIFY(adapter.removeColumn(0));
        QVERIFY(!adapter.removeColumn(0));
        QCOMPARE(table, (QList<QList<QString>>{{}, {}, {}}));
    }
    {
        QList<QList<QString>> table = {
            {"01", "02"},
            {"11", "12"},
            {"21", "22"}
        };
        QRangeModelAdapter adapter(&table);
        QVERIFY(adapter.removeColumn(1));
        QCOMPARE(table, (QList<QList<QString>>{
            {"01"},
            {"11"},
            {"21"}
        }));
    }
}

void tst_QRangeModelAdapter::removeColumns()
{
    {
        QList<QList<QString>> table = {
            {"1"},
            {"11"},
            {"21"}
        };
        QRangeModelAdapter adapter(&table);
        QVERIFY(!adapter.removeColumns(0, 5));
        QVERIFY(adapter.removeColumns(0, 1));
        QCOMPARE(table, (QList<QList<QString>>{{}, {}, {}}));
    }
    {
        QList<QList<QString>> table = {
            {"01", "02"},
            {"11", "12"},
            {"21", "22"}
        };
        QRangeModelAdapter adapter(&table);
        QVERIFY(adapter.removeColumns(0, 2));
        QCOMPARE(table, (QList<QList<QString>>{{}, {}, {}}));
    }
    {
        QList<QList<QString>> table = {
            {"01", "02", "03", "04"},
            {"11", "12", "13", "14"},
            {"21", "22", "23", "24"}
        };
        QRangeModelAdapter adapter(&table);
        QVERIFY(adapter.removeColumns(1, 2));
        QCOMPARE(table, (QList<QList<QString>>{
            {"01", "04"},
            {"11", "14"},
            {"21", "24"}
        }));
    }
}

void tst_QRangeModelAdapter::moveColumn()
{
    QList<QList<QString>> table = {
        {"01", "02", "03", "04"},
        {"11", "12", "13", "14"},
        {"21", "22", "23", "24"}
    };
    QRangeModelAdapter adapter(&table);
    QVERIFY(adapter.moveColumn(0, 2));
    QCOMPARE(table, (QList<QList<QString>>{
        {"02", "01", "03", "04"},
        {"12", "11", "13", "14"},
        {"22", "21", "23", "24"}
    }));
}

void tst_QRangeModelAdapter::moveColumns()
{
    std::vector<std::vector<int>> table = {
        {1, 2, 3, 4},
        {11, 12, 13, 14},
        {21, 22, 23, 24}
    };
    QRangeModelAdapter adapter(&table);
    adapter.moveColumns(0, 2, 3);
    QCOMPARE(table, (std::vector<std::vector<int>>{
        {3, 1, 2, 4},
        {13, 11, 12, 14},
        {23, 21, 22, 24}
    }));
}

void tst_QRangeModelAdapter::buildValueTree()
{
    auto tree = std::make_unique<value_tree>();
    auto printTreeOnError = qScopeGuard([&tree]{
        tree_row::prettyPrint(qDebug().nospace() << "tree at test failure:\n", *tree);
    });

    QRangeModelAdapter adapter(std::ref(*tree));
    QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);
    QSignalSpy rowsRemovedSpy(adapter.model(), &QAbstractItemModel::rowsRemoved);
    QSignalSpy rowsInsertedSpy(adapter.model(), &QAbstractItemModel::rowsInserted);

    auto oldCount = tree->size();

    // create top level item
    QVERIFY(adapter.insertRow(0));
    QCOMPARE(tree->size(), ++oldCount);
    QCOMPARE(rowsInsertedSpy.count(), 1);
    QCOMPARE(rowsInsertedSpy.at(0).value(0), QModelIndex()); // parent
    QCOMPARE(rowsInsertedSpy.at(0).value(1), 0); // first
    QCOMPARE(rowsInsertedSpy.at(0).value(2), 0); // last
    QCOMPARE(dataChangedSpy.count(), 0);
    rowsInsertedSpy.clear();

    // append one more, explicitly constructed
    QVERIFY(adapter.insertRow(int(tree->size()), {"1", "one"}));
    QCOMPARE(tree->size(), ++oldCount);
    QCOMPARE(rowsInsertedSpy.count(), 1);
    QCOMPARE(rowsInsertedSpy.at(0).value(0), QModelIndex()); // parent
    QCOMPARE(rowsInsertedSpy.at(0).value(1), 1);
    QCOMPARE(rowsInsertedSpy.at(0).value(2), 1);
    QCOMPARE(dataChangedSpy.count(), 0);
    rowsInsertedSpy.clear();

#if defined Q_CC_MSVC && _MSC_VER < 1944
    printTreeOnError.dismiss();
    QSKIP("Buggy compiler, get a later version of MSVC 2022");
#else
    // append two more, implicitly constructed
    QVERIFY(adapter.insertRows(int(tree->size()), std::array{
        u"2"_s,
        u"3"_s
    }));
    QCOMPARE(tree->size(), oldCount += 2);
    QCOMPARE(rowsInsertedSpy.count(), 1);
    QCOMPARE(rowsInsertedSpy.at(0).value(0), QModelIndex());
    QCOMPARE(rowsInsertedSpy.at(0).value(1), 2);
    QCOMPARE(rowsInsertedSpy.at(0).value(2), 3);
    QCOMPARE(dataChangedSpy.count(), 0);
    rowsInsertedSpy.clear();

    QVERIFY(!adapter.hasChildren(0));
    QVERIFY(adapter.insertRow({0, 0}));
    QVERIFY(adapter.hasChildren(0));
    QCOMPARE(adapter.rowCount(0), 1);

    QCOMPARE(rowsInsertedSpy.count(), 1);
    QCOMPARE(rowsInsertedSpy.at(0).value(0), adapter.index(0, 0));
    QCOMPARE(rowsInsertedSpy.at(0).value(1), 0);
    QCOMPARE(rowsInsertedSpy.at(0).value(2), 0);
    QCOMPARE(dataChangedSpy.count(), 0);
    rowsInsertedSpy.clear();

    {
        auto firstChild = adapter.at({0, 0});

        QVERIFY(firstChild->parentRow());
        QVERIFY(firstChild->value().isEmpty());
        QVERIFY(firstChild->description().isEmpty());

        adapter.at({0, 0}, 0) = "0.0";
        QCOMPARE(dataChangedSpy.count(), 1);
        QCOMPARE(rowsInsertedSpy.count(), 0);
        adapter.at({0, 0}, 1) = "zero.null";
        QCOMPARE(dataChangedSpy.count(), 2);
        dataChangedSpy.clear();

        QCOMPARE(adapter.at({0, 0}, 0), firstChild->value());
        QCOMPARE(adapter.at({0, 0}, 1), firstChild->description());

        adapter.at({0, 0}) = {"0,0", "null.nix"};
        QCOMPARE(firstChild->value(), "0,0");
        QCOMPARE(firstChild->description(), "null.nix");
        QCOMPARE(dataChangedSpy.count(), 1);
        QCOMPARE(rowsInsertedSpy.count(), 0);
        dataChangedSpy.clear();

        adapter.at({0, 0}, 0) = "1.0";
        adapter.at({0, 0}, 1) = "one.zero";
        QCOMPARE(firstChild->value(), "1.0");
        QCOMPARE(firstChild->description(), "one.zero");
        QCOMPARE(dataChangedSpy.count(), 2);
        dataChangedSpy.clear();

#if defined(__cpp_multidimensional_subscript)
/*!
    Current state of support
    * MSVC chokes on initializer list within [] operator, so have to call operator
      explicitly as a member function
    * gcc 13.3 compiles, but the returned DataRef is default-constructed
    * gcc 14.2.0 works
    * (Apple) clang 17 works fine
*/
#if (!defined(Q_CC_GNU_ONLY) || Q_CC_GNU > 1303)
#if defined(Q_CC_MSVC_ONLY)
        adapter.operator[]({0, 0}, 0) = "1.0";
        adapter.operator[]({0, 0}, 1) = "one.null";
#else
        adapter[{0, 0}, 0] = "1.0";
        adapter[{0, 0}, 1] = "one.null";
#endif

        QCOMPARE(firstChild->value(), "1.0");
        QCOMPARE(firstChild->description(), "one.null");
        QCOMPARE(dataChangedSpy.count(), 2);
        dataChangedSpy.clear();
#else
        qInfo("C++23 multidimensional subscript support available, but broken.");
#endif
#else
        qInfo("C++23 multidimensional subscript support not available.");
#endif // __cpp_multidimensional_subscript
    }

    // insert move-only rows
    QVERIFY(adapter.insertRows({0, 1}, std::array{
        tree_row{u"1.1"_s, u"one.one"_s},
        tree_row{u"1.2"_s, u"one.two"_s},
    }));
    QCOMPARE(adapter.rowCount(0), 3);
    QCOMPARE(adapter.index({0, 1}, 0).parent(), adapter.index(0, 0));
    QVERIFY((adapter.at({0, 1}))->parentRow());
    QCOMPARE(rowsInsertedSpy.count(), 1);
    QCOMPARE(rowsInsertedSpy.at(0).value(0), adapter.index(0, 0));
    QCOMPARE(rowsInsertedSpy.at(0).value(1), 1);
    QCOMPARE(rowsInsertedSpy.at(0).value(2), 2);
    QCOMPARE(dataChangedSpy.count(), 0);
    rowsInsertedSpy.clear();

    adapter.moveRow(2, 1);
    // adapter.moveRow({0, 0}, {1, 1}); // out of bounds -> crash
    while (adapter.hasChildren(0))
        adapter.moveRow({0, 0}, {1, 0});
    QCOMPARE(adapter.rowCount(0), 0);
    QCOMPARE(adapter.rowCount(1), 3);
    adapter.moveRows({1, 0}, 3, {2, 0});
    QCOMPARE(adapter.rowCount(1), 0);
    QCOMPARE(adapter.rowCount(2), 3);

    QPersistentModelIndex firstRowPMI;
    QPersistentModelIndex firstChildPMI;
    QPersistentModelIndex firstGrandchildPMI;

    { // replace existing row with branch
        tree_row newRow = {u"0"_s, u"zero"_s};
        tree_row &firstChild = newRow.addChild(u"0.1"_s, u"zero.one"_s);
        firstChild.addChild("0.1.1", u"zero.one.one"_s);

        adapter.at(0) = std::move(newRow);
        QCOMPARE(dataChangedSpy.count(), 1); // whole row data changed
        QCOMPARE(dataChangedSpy.at(0).value(0), adapter.index(0, 0));
        QCOMPARE(dataChangedSpy.at(0).value(1), adapter.index(0, 1));
        QCOMPARE(rowsInsertedSpy.count(), 1); // and a new row was added underneath
        QCOMPARE(rowsInsertedSpy.at(0).value(0), adapter.index(0, 0));
        QCOMPARE(rowsInsertedSpy.at(0).value(1), 0);
        QCOMPARE(rowsInsertedSpy.at(0).value(2), 0);
        QCOMPARE(rowsRemovedSpy.count(), 0); // no rows removed
        dataChangedSpy.clear();
        rowsInsertedSpy.clear();

        firstRowPMI = adapter.index(0, 0);
        QVERIFY(firstRowPMI.isValid());
        QCOMPARE(firstRowPMI.data(), "0");
        firstChildPMI = adapter.index({0, 0}, 1);
        QVERIFY(firstChildPMI.isValid());
        QCOMPARE(firstChildPMI.data(), "zero.one");
        firstGrandchildPMI = adapter.index({0, 0, 0}, 0);
        QVERIFY(firstGrandchildPMI.isValid());
        QCOMPARE(firstGrandchildPMI.data(), "0.1.1");
    }

    { // replace existing branch with new branch
        tree_row newRow = {"0", u"null"_s};
        tree_row &firstChild = newRow.addChild(u"0.1"_s, u"null.one"_s);
        firstChild.addChild(u"0.1.1"_s, u"null.one.one"_s);

        adapter.at(0) = std::move(newRow);
        QCOMPARE(dataChangedSpy.count(), 1); // whole row data changed
        QCOMPARE(dataChangedSpy.at(0).value(0), adapter.index(0, 0));
        QCOMPARE(dataChangedSpy.at(0).value(1), adapter.index(0, 1));
        QCOMPARE(rowsRemovedSpy.count(), 1); // old child row was removed
        QCOMPARE(rowsRemovedSpy.at(0).value(0), adapter.index(0, 0));
        QCOMPARE(rowsRemovedSpy.at(0).value(1), 0);
        QCOMPARE(rowsRemovedSpy.at(0).value(2), 0);
        QCOMPARE(rowsInsertedSpy.count(), 1); // old child row was removed
        QCOMPARE(rowsInsertedSpy.at(0).value(0), adapter.index(0, 0));
        QCOMPARE(rowsInsertedSpy.at(0).value(1), 0);
        QCOMPARE(rowsInsertedSpy.at(0).value(2), 0);
        dataChangedSpy.clear();
        rowsInsertedSpy.clear();
        rowsRemovedSpy.clear();

        // only data has changed
        QVERIFY(firstRowPMI.isValid());
        // (grand)children are replaced
        QVERIFY(!firstChildPMI.isValid());
        firstChildPMI = adapter.index({0, 0}, 0);
        QVERIFY(firstChildPMI.isValid());
        QVERIFY(!firstGrandchildPMI.isValid());
        firstGrandchildPMI = adapter.index({0, 0, 0}, 0);
        QVERIFY(firstGrandchildPMI.isValid());
    }

    { // replace existing branch with new row
        tree_row newRow = {"0", u"zero.zero"_s};
        adapter.at(0) = std::move(newRow);
        QCOMPARE(dataChangedSpy.count(), 1); // whole row data changed
        QCOMPARE(dataChangedSpy.at(0).value(0), adapter.index(0, 0));
        QCOMPARE(dataChangedSpy.at(0).value(1), adapter.index(0, 1));
        QCOMPARE(rowsRemovedSpy.count(), 1); // old child row was removed
        QCOMPARE(rowsRemovedSpy.at(0).value(0), adapter.index(0, 0));
        QCOMPARE(rowsRemovedSpy.at(0).value(1), 0);
        QCOMPARE(rowsRemovedSpy.at(0).value(2), 0);
        QCOMPARE(rowsInsertedSpy.count(), 0); // no new children inserted
        dataChangedSpy.clear();
        rowsRemovedSpy.clear();

        // only data has changed
        QVERIFY(firstRowPMI.isValid());
        // (grand)children are replaced
        QVERIFY(!firstChildPMI.isValid());
        QVERIFY(!firstGrandchildPMI.isValid());
    }

    dataChangedSpy.clear();
    rowsInsertedSpy.clear();
#endif // old Q_CC_MSVC

    printTreeOnError.dismiss();
}

void tst_QRangeModelAdapter::buildPointerTree()
{
    struct MarkDirtyProtocol : tree_row::ProtocolPointerImpl {
        void deleteRow(tree_row *row) {
            row->value() = "deleted";
            row->description() = "deleted";
            deletedRows << row;
        }
        QList<tree_row *> deletedRows;

        ~MarkDirtyProtocol()
        {
            qDeleteAll(deletedRows);
        }
    };

    auto tree = createPointerTree();
    QRangeModelAdapter adapter(std::move(tree), MarkDirtyProtocol{});

    QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);
    QSignalSpy rowsRemovedSpy(adapter.model(), &QAbstractItemModel::rowsRemoved);
    QSignalSpy rowsInsertedSpy(adapter.model(), &QAbstractItemModel::rowsInserted);

    {
        const tree_row *secondRow = adapter.at(1);
        QVERIFY(secondRow);
        QCOMPARE(secondRow->value(), adapter.data(1, 0));
        const tree_row *row21 = adapter.at({1, 0});
        QVERIFY(row21);
        const tree_row *row230 = adapter.at({1, 2, 0});
        QVERIFY(row230);

        tree_row *newRow = new tree_row{"0", "null"};
        newRow->addChildPointer("0.0", "");
        newRow->addChildPointer("0.1", "");
        tree_row *newChild = newRow->addChildPointer("0.2", "");
        newChild->addChildPointer("0.2.0", "");
        newChild->addChildPointer("0.2.1", "");
        newChild->addChildPointer("0.2.2", "");
        newRow->addChildPointer("0.3", "");

        // replace branch with new branch
        adapter.at(1) = newRow;
        QCOMPARE(dataChangedSpy.count(), 1); // top row changed - ### actually, replaced - should we invalidate?
        QCOMPARE(dataChangedSpy.at(0).value(0), adapter.index(1, 0));
        QCOMPARE(dataChangedSpy.at(0).value(1), adapter.index(1, 1));
        QCOMPARE(rowsRemovedSpy.count(), 1);
        QCOMPARE(rowsRemovedSpy.at(0).value(0), adapter.index(1, 0)); // parent
        QCOMPARE(rowsRemovedSpy.at(0).value(1), 0);
        QCOMPARE(rowsRemovedSpy.at(0).value(2), 2); // three children removed
        QCOMPARE(rowsInsertedSpy.count(), 1);
        QCOMPARE(rowsInsertedSpy.at(0).value(0), adapter.index(1, 0)); // parent
        QCOMPARE(rowsInsertedSpy.at(0).value(1), 0);
        QCOMPARE(rowsInsertedSpy.at(0).value(2), 3); // four children added
        dataChangedSpy.clear();
        rowsRemovedSpy.clear();
        rowsInsertedSpy.clear();

        // all old rows marked as deleted
        QCOMPARE(secondRow->value(), "deleted");
        QCOMPARE(row21->value(), "deleted");
        QCOMPARE(row230->value(), "deleted");
    }

    // now do the same thing with iterator access
    {
        auto secondRow = *(adapter.begin() + 1);
        QVERIFY(secondRow.hasChildren());
        secondRow.children() = createPointerTree();

        QCOMPARE(dataChangedSpy.count(), 0); // no existing row was changed
        QCOMPARE(rowsRemovedSpy.count(), 1);
        QCOMPARE(rowsRemovedSpy.at(0).value(0), adapter.index(1, 0)); // parent
        QCOMPARE(rowsRemovedSpy.at(0).value(1), 0);
        QCOMPARE(rowsRemovedSpy.at(0).value(2), 3); // four children removed
        QCOMPARE(rowsInsertedSpy.count(), 1);
        QCOMPARE(rowsInsertedSpy.at(0).value(0), adapter.index(1, 0)); // parent
        QCOMPARE(rowsInsertedSpy.at(0).value(1), 0);
        QCOMPARE(rowsInsertedSpy.at(0).value(2), 4); // five children added
    }
}

class ObjectTreeItem;
using ObjectTree = std::vector<ObjectTreeItem>;

class ObjectTreeItem : public ObjectRow
{
public:
    ObjectTreeItem() = default;

    explicit ObjectTreeItem(Object *item)
    {
        m_objects[0] = item;
    }

    ObjectTreeItem(const ObjectTreeItem &other) = delete;
    ObjectTreeItem &operator=(const ObjectTreeItem &other) = delete;
    ObjectTreeItem(ObjectTreeItem &&other) noexcept
    {
        m_children = std::move(other.m_children);
        m_objects = std::move(other.m_objects);
        other.m_objects = {};
    }

    ObjectTreeItem &operator=(ObjectTreeItem &&other) noexcept
    {
        m_children = std::move(other.m_children);
        m_objects = std::move(other.m_objects);
        other.m_objects = {};
        return *this;
    }

    ~ObjectTreeItem()
    {
       qDeleteAll(m_objects);
    }

    ObjectTreeItem *parentRow() const { return m_parentRow; }
    void setParentRow(ObjectTreeItem *parentRow) { m_parentRow = parentRow; }
    const auto &childRows() const { return m_children; }
    auto &childRows() { return m_children; }

private:
    template <std::size_t I, typename Item,
        std::enable_if_t<std::is_same_v<q20::remove_cvref_t<Item>, ObjectTreeItem>, bool> = true>
    friend decltype(auto) get(Item &&row) { return q23::forward_like<Item>(row.m_objects[I]); }

    ObjectTreeItem *m_parentRow = nullptr;
    std::optional<ObjectTree> m_children = std::nullopt;
};

namespace std {
    template <> struct tuple_size<ObjectTreeItem> : tuple_size<ObjectRow> {};
    template <std::size_t I> struct tuple_element<I, ObjectTreeItem> : tuple_element<I, ObjectRow> {};
}

void tst_QRangeModelAdapter::insertAutoConnectObjects()
{
    QRangeModelAdapter adapter(ObjectTree{});
    QSignalSpy dataChangedSpy(adapter.model(), &QAbstractItemModel::dataChanged);
    adapter.model()->setAutoConnectPolicy(QRangeModel::AutoConnectPolicy::Full);

    Object *newObject = new Object;
    adapter.insertRow(0, ObjectTreeItem{newObject});
    newObject->setString("0");
    newObject->setNumber(0);

    QCOMPARE(dataChangedSpy.count(), 2);
    dataChangedSpy.clear();

    Object *newChild = new Object;
    auto firstRow = adapter.begin();
    {
        ObjectTree children(3);
        children[0] = ObjectTreeItem(newChild);
        (*firstRow).children() = std::move(children);
    }
    QCOMPARE(dataChangedSpy.count(), 0);
    QVERIFY(adapter.hasChildren(0));
    newChild->setString("0.0");
    QCOMPARE(dataChangedSpy.count(), 1);
    dataChangedSpy.clear();

    newChild = new Object;
    newChild->setString("0.1");
    adapter.at({0, 1}) = ObjectTreeItem(newChild);
    QCOMPARE(dataChangedSpy.count(), 1);
    newChild->setNumber(1);
    QCOMPARE(dataChangedSpy.count(), 2);
    dataChangedSpy.clear();

    newChild = new Object;
    Object *newGrandChild = new Object;
    ObjectTreeItem newBranch(newChild);
    {
        ObjectTree children(3);
        // skip the first row to verify that we continue through nullptr
        children[1] = ObjectTreeItem(newGrandChild);
        newBranch.childRows() = std::move(children);
    }
    adapter.at({0, 2}) = std::move(newBranch);
    QCOMPARE(adapter.rowCount({0, 2}), 3);
    QCOMPARE(dataChangedSpy.count(), 1);
    newChild->setNumber(1);
    QCOMPARE(dataChangedSpy.count(), 2);
    dataChangedSpy.clear();

    newGrandChild->setString("0.2.1");
    QCOMPARE(dataChangedSpy.count(), 1);
    dataChangedSpy.clear();

    newGrandChild = new Object;
    adapter.at({0, 2, 0}, 0) = newGrandChild;
    QCOMPARE(dataChangedSpy.count(), 1);
    newGrandChild->setString("0.2.0");
    QCOMPARE(dataChangedSpy.count(), 2);
}

QTEST_MAIN(tst_QRangeModelAdapter)
#include "tst_qrangemodeladapter.moc"

#undef HAS_API
#undef ADD_COPY
#undef ADD_POINTER
#undef ADD_UPTR
#undef ADD_SPTR
#undef ADD_HELPER
#undef ADD_ALL
