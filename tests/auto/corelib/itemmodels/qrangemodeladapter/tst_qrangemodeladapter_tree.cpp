// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define Q_ASSERT(cond) ((cond) ? static_cast<void>(0) : qCritical(#cond))
#define Q_ASSERT_X(cond, x, msg) ((cond) ? static_cast<void>(0) \
                                         : qCritical("%s: %s returned false - %s", x, #cond, msg))

#include "tst_qrangemodeladapter.h"

#include <QtTest/qtest.h>
#include <QtCore/qrangemodeladapter.h>
#include <QtTest/qsignalspy.h>

value_tree tst_QRangeModelAdapter::createValueTree()
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

pointer_tree tst_QRangeModelAdapter::createPointerTree()
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
        QVERIFY(!adapter.index(QList<int>{}, 0).isValid());
        QVERIFY(adapter.index(QList<int>{0}, 0).isValid());
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

void tst_QRangeModelAdapter::treeFindRecursively()
{
    QRangeModelAdapter adapter(createValueTree());
    QStringView needle = u"2.3.2";
    QList<int> path;
    while (true) {
        auto it = std::find_if(adapter.cbegin(), adapter.cend(), [needle](const auto &row){
            return needle.startsWith(row->value());
        });
        QVERIFY(it != adapter.cend());
        path.append(it - adapter.cbegin());
        if (it->value() == needle)
            break;
        if ((*it).hasChildren()) {
            adapter = (*it).children();
        } else {
            path = {};
            break;
        }
    }
    QCOMPARE(path, (QList<int>{1, 2, 1}));
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

    { // assign new range to an existing branch
        QCOMPARE(adapter.rowCount(0), 0);
        tree_row newChild[] = {
            {"0.1", "zero.one"}
        };
        adapter.at(0).children().assign(std::make_move_iterator(std::begin(newChild)),
                                        std::make_move_iterator(std::end(newChild)));
        QCOMPARE(adapter.rowCount(0), 1);
        QCOMPARE(rowsRemovedSpy.count(), 0);
        QCOMPARE(rowsInsertedSpy.count(), 1);
        rowsInsertedSpy.clear();

        tree_row newChildren[] = {
            {"0.1", "zero.one"},
            {"0.2", "zero.two"},
        };
        adapter.at(0).children().assign(std::make_move_iterator(std::begin(newChildren)),
                                        std::make_move_iterator(std::end(newChildren)));
        QVERIFY(adapter.at(0).children().index({}, 0).isValid());
        QCOMPARE(adapter.rowCount(0), 2);
        QCOMPARE(rowsRemovedSpy.count(), 1);
        QCOMPARE(rowsInsertedSpy.count(), 1);
        rowsRemovedSpy.clear();
        rowsInsertedSpy.clear();

        adapter.at(0).children().assign({});
        QCOMPARE(adapter.rowCount(0), 0);
        QCOMPARE(rowsRemovedSpy.count(), 1);
        QCOMPARE(rowsInsertedSpy.count(), 0);
        rowsRemovedSpy.clear();
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
        rowsInsertedSpy.clear();
    }

    { // insert rows
        QVERIFY(adapter.insertRow(QList<int>{0}, new tree_row("-1", "negative")));
        QCOMPARE(rowsInsertedSpy.count(), 1);
        rowsInsertedSpy.clear();
        QVERIFY(!adapter.hasChildren(QList<int>{0}));
        QVERIFY(adapter.insertRow({0, 0}, new tree_row("-1.0", "negative.null")));
        QVERIFY(adapter.hasChildren(QList<int>{0}));
        QCOMPARE(rowsInsertedSpy.count(), 1);
        rowsInsertedSpy.clear();
        QVERIFY(adapter.insertRows({0, 1}, std::vector{
            new tree_row("-1.1", "negative.one"),
            new tree_row("-1.2", "negative.two"),
        }));
        QCOMPARE(rowsInsertedSpy.count(), 1);
        QCOMPARE(rowsInsertedSpy.at(0).value(0), adapter.index(0, 0)); // parent
        QCOMPARE(rowsInsertedSpy.at(0).value(1), 1);
        QCOMPARE(rowsInsertedSpy.at(0).value(2), 2);
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
