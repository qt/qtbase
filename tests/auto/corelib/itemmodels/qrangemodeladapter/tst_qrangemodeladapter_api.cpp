// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qrangemodeladapter.h"

#include <QtCore/qlist.h>
#include <QtCore/qrangemodeladapter.h>
#include <QtCore/qxptype_traits.h>

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

API_TEST(assign, assign({}))

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
API_TEST(insertColumnWithData, insertColumn(0, QList<int>{0}))
API_TEST(insertColumns, insertColumns(0, QList<int>{0}))
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

template <typename ...Args>
using construct_test = decltype(QRangeModelAdapter(std::declval<Args &&>() ...));

void tst_QRangeModelAdapter::construct_API()
{
    static_assert(qxp::is_detected_v<construct_test, QList<int>>);
    static_assert(qxp::is_detected_v<construct_test, QList<QList<QString>>>);
    static_assert(qxp::is_detected_v<construct_test, QList<tree_row *>, tree_row::ProtocolPointerImpl>);

    static_assert(!qxp::is_detected_v<construct_test, int>);
}

void tst_QRangeModelAdapter::assign_API()
{
    Data d;
    auto tree = value_tree{};
    static_assert(has_assign(d.vectorOfGadgets));
    static_assert(has_assign(std::move(d.tableOfRowPointers)));
    static_assert(has_assign(d.m_tree));
    static_assert(has_assign(tree));
#if (!defined(Q_CC_GNU_ONLY) || Q_CC_GNU > 1303) && !defined(Q_OS_VXWORKS) && !defined(Q_OS_INTEGRITY)
    static_assert(has_assign(std::ref(tree)));
#endif
    static_assert(has_assign(std::move(tree)));
    static_assert(!has_assign(std::as_const(d.vectorOfGadgets)));
    static_assert(!has_assign(std::as_const(tree)));
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
    static_assert(!has_insertColumnWithData(d.tableOfPointers));
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
    static_assert(!has_insertColumns(d.tableOfPointers));
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
