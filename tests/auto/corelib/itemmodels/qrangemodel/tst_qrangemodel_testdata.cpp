// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qrangemodel.h"


#define ADD_HELPER(Model, Tag, Policy, ColumnCount, Actions, HeaderData) \
    { \
        Factory factory = [this]() -> std::unique_ptr<QAbstractItemModel> { \
            auto result = std::make_unique<QRangeModel>(Policy(m_data->Model)); \
            createBackup(result.get(), m_data->Model); \
            return result; \
        }; \
        QTest::addRow(#Model #Tag) << std::move(factory) << int(std::size(m_data->Model)) \
                                   << int(ColumnCount) << ChangeActions(Actions) \
                                   << QVariant::fromValue(HeaderData); \
    }

#define ADD_POINTER(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, Pointer, &, ColumnCount, Actions, HeaderData)

#if defined(Q_OS_INTEGRITY) || defined(Q_OS_QNX)
# define ADD_COPY(Model, ColumnCount, Actions, HeaderData)
# define ADD_MOVE(Model, ColumnCount, Actions, HeaderData)
# define ADD_REF(Model, ColumnCount, Actions, HeaderData)
# define ADD_UPTR(Model, ColumnCount, Actions, HeaderData)
# define ADD_SPTR(Model, ColumnCount, Actions, HeaderData)
#else
# define ADD_COPY(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, Copy, *&, ColumnCount, Actions, HeaderData)
# define ADD_MOVE(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, Move, std::move, ColumnCount, Actions, HeaderData)
# define ADD_REF(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, Ref, std::ref, ColumnCount, Actions, HeaderData)
# define ADD_UPTR(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, UPtr, asUPtr, ColumnCount, Actions, HeaderData)
# define ADD_SPTR(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, SPtr, asSPtr, ColumnCount, Actions, HeaderData)
#endif

#define ADD_ALL(Model, ColumnCount, Actions, HeaderData) \
    ADD_COPY(Model, ColumnCount, Actions, HeaderData) \
    ADD_REF(Model, ColumnCount, Actions, HeaderData) \
    ADD_POINTER(Model, ColumnCount, Actions, HeaderData) \
    ADD_UPTR(Model, ColumnCount, Actions, HeaderData) \
    ADD_SPTR(Model, ColumnCount, Actions, HeaderData)

void tst_QRangeModel::createTestData()
{
    m_data.reset(new Data);

    createTree();

    QTest::addColumn<Factory>("factory");
    QTest::addColumn<int>("expectedRowCount");
    QTest::addColumn<int>("expectedColumnCount");
    QTest::addColumn<ChangeActions>("changeActions");
    QTest::addColumn<QVariant>("headerValue");

    // The entire test data is recreated for each test function, but test
    // functions must not change data structures other than the one tested.
    // For ranges that can't be copied, or that operate on pointers or
    // references, only adding either pointer, ref, or copy, as they all operate
    // on the same data.

    ADD_ALL(fixedArrayOfNumbers, 1, ChangeAction::SetData | ChangeAction::Sort, 1);

    ADD_POINTER(cArrayOfNumbers, 1, ChangeAction::SetData | ChangeAction::Sort, 1);
    ADD_REF(cArrayFixedColumns,
            std::tuple_size_v<Row>,
            ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort, u"Item"_s);

    ADD_ALL(vectorOfFixedColumns, 2,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::Sort, u"int"_s);

    // TODO: create a new instance with shared pointers inside for each test
    ADD_COPY(vectorOfFixedSPtrColumns, 2,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::Sort, u"int"_s);

    ADD_ALL(vectorOfArrays, 10,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::Sort, 0);

    ADD_ALL(vectorOfStructs,
            std::tuple_size_v<Row>,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"Item"_s);

    ADD_ALL(vectorOfConstStructs, std::tuple_size_v<ConstRow>, ChangeAction::ChangeRows | ChangeAction::Sort,
            u"QString"_s);

    ADD_ALL(vectorOfGadgets, 3,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"display"_s);

    ADD_ALL(listOfGadgets, 1,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"Item"_s);
    ADD_ALL(listOfMultiRoleGadgets, 1,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"The Gadget"_s);
    ADD_COPY(listOfSharedMultiRoleGadgets, 1,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"The Gadget"_s);
    ADD_POINTER(arrayOfUniqueMultiRoleGadgets, 1,
            ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"The Gadget"_s);

    ADD_ALL(vectorOfItemAccess, 1,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            1);

    ADD_MOVE(listOfObjects, 2,
             ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
             u"string"_s);
    ADD_REF(arrayOfUniqueObjects, 2, ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"string"_s);

    ADD_COPY(listOfMetaObjectTuple, 1,
             ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
             u"MetaObjectTuple"_s);
    ADD_REF(tableOfMetaObjectTuple,
            std::tuple_size_v<MetaObjectTuple>,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"QString"_s);
#if !defined(Q_OS_VXWORKS) && !defined(Q_OS_INTEGRITY)
    // don't use the correct createBackup overload and fails to build
    ADD_REF(arrayOfUniqueMultiObjectTuples, 1, ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"MetaObjectTuple"_s);
#endif

    ADD_ALL(tableOfNumbers, 5, ChangeAction::All | ChangeAction::Sort, 1);

    ADD_POINTER(tableOfPointers, 2, ChangeAction::All | ChangeAction::SetItemData | ChangeAction::Sort, 1);
    ADD_REF(tableOfRowRefs,
            std::tuple_size_v<Row>,
            ChangeAction::RemoveRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            u"Item"_s);

    ADD_ALL(arrayOfConstNumbers, 1, ChangeAction::ReadOnly, 1);

    ADD_ALL(constListOfNumbers, 1, ChangeAction::ReadOnly, 1);

    ADD_ALL(constTableOfNumbers, 5, ChangeAction::ReadOnly, 1);

    ADD_ALL(listOfNamedRoles, 1,
            ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::SetItemData | ChangeAction::Sort,
            1);

    ADD_ALL(tableOfEnumRoles, 1, ChangeAction::All | ChangeAction::SetItemData | ChangeAction::Sort, 1);

    ADD_ALL(tableOfIntRoles, 1, ChangeAction::All | ChangeAction::SetItemData | ChangeAction::Sort, 1);

    ADD_ALL(stdTableOfIntRoles, 1, ChangeAction::All | ChangeAction::SetItemData | ChangeAction::Sort, 1);

    ADD_COPY(stdTableOfIntRolesWithSharedRows, 1, ChangeAction::All | ChangeAction::SetItemData | ChangeAction::Sort,
             1);

    QTest::addRow("Moved table") << Factory([]{
        QList<std::vector<QString>> movedTable = {
            {"0/0", "0/1", "0/2", "0/3"},
            {"1/0", "1/1", "1/2", "1/3"},
            {"2/0", "2/1", "2/2", "2/3"},
            {"3/0", "3/1", "3/2", "3/3"},
        };
        return std::unique_ptr<QAbstractItemModel>(new QRangeModel(std::move(movedTable)));
    }) << 4 << 4 << ChangeActions(ChangeAction::All | ChangeAction::Sort) << QVariant(1);

    // moved list of pointers -> model takes ownership
    QTest::addRow("movedListOfObjects") << Factory([]{
        std::list<Object *> movedListOfObjects = {
            new Object("-3", -3), new Object("-2", -2), new Object("-1", -1),
            new Object("0", 0), new Object("1", 1), new Object("2", 2)
        };

        return std::unique_ptr<QAbstractItemModel>(
            new QRangeModel(std::move(movedListOfObjects))
        );
    }) << 6 << 2 << (ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::Sort)
       << QVariant(u"string"_s);

    // special case: tree
    QTest::addRow("value tree (ref)") << Factory([this]{
        return std::unique_ptr<QAbstractItemModel>(new QRangeModel(std::ref(*m_data->m_tree)));
    }) << int(std::size(*m_data->m_tree.get())) << int(std::tuple_size_v<tree_row>)
       << (ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::Sort) << QVariant(u"QString"_s);

    QTest::addRow("pointer tree") << Factory([this]{
        return std::unique_ptr<QAbstractItemModel>(
            new QRangeModel(m_data->m_pointer_tree.get(), tree_row::ProtocolPointerImpl{})
        );
    }) << int(std::size(*m_data->m_pointer_tree.get())) << int(std::tuple_size_v<tree_row>)
       << (ChangeAction::ChangeRows | ChangeAction::SetData | ChangeAction::Sort) << QVariant(u"QString"_s);
}

#undef ADD_COPY
#undef ADD_MOVE
#undef ADD_POINTER
#undef ADD_UPTR
#undef ADD_SPTR
#undef ADD_HELPER
#undef ADD_ALL
