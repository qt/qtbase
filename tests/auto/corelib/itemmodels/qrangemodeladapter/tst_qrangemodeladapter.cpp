// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define Q_ASSERT(cond) ((cond) ? static_cast<void>(0) : qCritical(#cond))
#define Q_ASSERT_X(cond, x, msg) ((cond) ? static_cast<void>(0) \
                                         : qCritical("%s: %s returned false - %s", x, #cond, msg))

#include "tst_qrangemodeladapter.h"

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qpointer.h>
#include <QtCore/qrangemodeladapter.h>
#include <QtCore/qregularexpression.h>

using namespace Qt::StringLiterals;

void tst_QRangeModelAdapter::expectInvalidIndex(int count)
{
#ifndef QT_NO_DEBUG
    static QRegularExpression invalidIndex{".* - Index at .* is invalid"};

    for (int i = 0; i < count; ++i) // at and DataRef accesses when testing out-of-bounds
        QTest::ignoreMessage(QtCriticalMsg, invalidIndex);
#else
    Q_UNUSED(count);
#endif
}

void tst_QRangeModelAdapter::init()
{
    m_data.reset(new Data);
}

void tst_QRangeModelAdapter::cleanup()
{
    m_data.reset();
}

void tst_QRangeModelAdapter::construct()
{
    std::vector<int> data = { 1, 2, 3 };
    {
        QRangeModelAdapter<const std::vector<int>> const_adapter(data);
        QCOMPARE(const_adapter[0], data[0]); // unchanged, we operate on a local copy
    }

    {
        QRangeModelAdapter<std::vector<int>> adapter(std::as_const(data));
        adapter[0] = 0; // we can assign, but operate on a copy of data
        QCOMPARE(adapter[0], 0);
        QCOMPARE(data[0], 1); // unchanged
    }

    {
        std::initializer_list<int> list = { 1, 2, 3, 4 };
        QRangeModelAdapter<std::vector<int>> adapter(list);
    }
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

        adapter.assign(std::vector<int>{1, 2, 3, 4, 5});
        QCOMPARE(modelAboutToBeResetSpy.count(), 1);
        QCOMPARE(modelResetSpy.count(), 1);

        QCOMPARE(adapter.rowCount(), 5);
        QCOMPARE(adapter[0], 1);

        adapter.assign({3, 2, 1});
        QCOMPARE(modelAboutToBeResetSpy.count(), 2);
        QCOMPARE(modelResetSpy.count(), 2);
        QCOMPARE(adapter.rowCount(), 3);
        QCOMPARE(adapter[0], 3);

        QCOMPARE(adapter, (std::vector<int>{3, 2, 1}));
        modelAboutToBeResetSpy.clear();
        modelResetSpy.clear();

        std::vector<int> modifiedData = adapter.range();

        adapter.assign(modifiedData.begin(), modifiedData.end());
        QCOMPARE(modelResetSpy.count(), 1);
        adapter.assign(std::vector<int>{3, 2, 1});
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
        adapter.assign(createValueTree());
        QCOMPARE(std::as_const(adapter).at(0, 0), "1");
        QCOMPARE(std::as_const(adapter).at(0, 1), "one");
    }

    {
        QStringList list;
        QRangeModelAdapter adapter(list);
        auto setList = [](const QStringList &) {};
        setList(adapter.range());
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

QTEST_MAIN(tst_QRangeModelAdapter)
#include "moc_tst_qrangemodeladapter.cpp"
