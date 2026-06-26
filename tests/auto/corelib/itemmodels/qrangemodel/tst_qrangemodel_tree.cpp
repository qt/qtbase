// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qrangemodel.h"

#if QT_CONFIG(itemmodeltester)
#include <QtTest/qabstractitemmodeltester.h>
#endif

template <typename Tree>
static bool integrityCheck(const Tree &tree, int depth = 0)
{
    static constexpr bool pointerTree = std::is_pointer_v<typename std::remove_reference_t<Tree>::value_type>;
    bool result = true;
    for (const auto &row : tree) {
        if constexpr (pointerTree) {
            if (!row) {
                qCritical() << "Unexpected null pointer in tree!";
                return false;
            }
        }
        const auto &children = [&row]() -> const auto &{
            if constexpr (pointerTree) {
                const auto protocol = tree_row::ProtocolPointerImpl{};
                return protocol.childRows(*row);
            } else {
                return row.childRows();
            }
        }();
        if (children) {
            for (const auto &child : *children) {
                const bool match = [&child, &row]{
                    tree_row emptyRow;
                    if constexpr (pointerTree) {
                        if (child->parentRow() != row) {
                            qCritical().noquote() << "Parent out of sync for:" << *child;
                            qCritical().noquote() << "  Actual: " << child->parentRow()
                                    << std::move(child->parentRow() ? *child->parentRow()
                                                                    : emptyRow);
                            qCritical().noquote() << "Expected: " << row << *row;
                            return false;
                        }
                    } else {
                        if (child.parentRow() != std::addressof(row)) {
                            qCritical().noquote() << "Parent out of sync for:" << child;
                            qCritical().noquote() << "  Actual: " << child.parentRow()
                                    << std::move(child.parentRow() ? *child.parentRow()
                                                                    : emptyRow);
                            qCritical().noquote() << "Expected: " << std::addressof(row) << row;
                            return false;
                        }
                    }
                    return true;
                }();
                if (!match)
                    return false;
            }
            result &= integrityCheck(*children, depth + 1);
        }
    }
    return result;
}

bool tst_QRangeModel::treeIntegrityCheck()
{
    if (!integrityCheck(*m_data->m_tree)) {
        tree_row::prettyPrint(qDebug().nospace() << "\nTree of Values:\n", *m_data->m_tree);
        return false;
    }
    if (!integrityCheck(*m_data->m_pointer_tree)) {
        tree_row::prettyPrint(qDebug().nospace() << "\nTree of Pointers:\n", *m_data->m_tree);
        return false;
    }
    return true;
}

enum class TreeProtocol {
    ValueImplicit,
    ValueReadOnly,
    PointerExplicit,
    PointerExplicitMoved,
    ChildrenVector,
    ChildrenVectorPtr,
};

void tst_QRangeModel::createTree()
{
    tree_row root[] = {
        {"1", "one"},
        {"2", "two"},
        {"3", "three"},
        {"4", "four"},
        {"5", "five"},
    };
    m_data->m_tree.reset(new value_tree{std::make_move_iterator(std::begin(root)),
                                        std::make_move_iterator(std::end(root))});

    (*m_data->m_tree)[1].addChild("2.1", "two.one");
    (*m_data->m_tree)[1].addChild("2.2", "two.two");

    tree_row &row23 = (*m_data->m_tree)[1].addChild("2.3", "two.three");

    row23.addChild("2.3.1", "two.three.one");
    row23.addChild("2.3.2", "two.three.two");
    row23.addChild("2.3.3", "two.three.three");

    // assert the integrity of the tree; this is not a test.
    Q_ASSERT(!m_data->m_tree->at(0).childRows());
    Q_ASSERT(m_data->m_tree->at(1).childRows());
    Q_ASSERT(!m_data->m_tree->at(1).childRows()->at(1).childRows());
    Q_ASSERT(m_data->m_tree->at(1).childRows()->at(2).childRows());

    m_data->m_pointer_tree.reset(new pointer_tree{
        new tree_row("1", "one"),
        new tree_row("2", "two"),
        new tree_row("3", "three"),
        new tree_row("4", "four"),
        new tree_row("5", "five"),
    });

    m_data->m_pointer_tree->at(1)->addChildPointer("2.1", "two.one");
    m_data->m_pointer_tree->at(1)->addChildPointer("2.2", "two.two");
}

void tst_QRangeModel::tree_data()
{
    m_data.reset(new Data);
    createTree();

    QTest::addColumn<TreeProtocol>("protocol");
    QTest::addColumn<int>("expectedRootRowCount");
    QTest::addColumn<int>("expectedColumnCount");
    QTest::addColumn<QList<int>>("rowsWithChildren");
    QTest::addColumn<ChangeActions>("changeActions");

    const int expectedRootRowCount = int(m_data->m_tree->size());
    const int expectedColumnCount = int(std::tuple_size_v<tree_row>);
    const auto rowsWithChildren = QList{1};

    QTest::addRow("ValueImplicit")
        << TreeProtocol::ValueImplicit
        << expectedRootRowCount << expectedColumnCount << rowsWithChildren
        << ChangeActions(ChangeAction::All);
    QTest::addRow("ValueReadOnly")
        << TreeProtocol::ValueReadOnly
        << expectedRootRowCount << expectedColumnCount << rowsWithChildren
        << ChangeActions(ChangeAction::ReadOnly);
    QTest::addRow("PointerExplicit")
        << TreeProtocol::PointerExplicit
        << expectedRootRowCount << expectedColumnCount << rowsWithChildren
        << ChangeActions(ChangeAction::All);
    QTest::addRow("PointerExplicitMoved")
        << TreeProtocol::PointerExplicitMoved
        << expectedRootRowCount << expectedColumnCount << rowsWithChildren
        << ChangeActions(ChangeAction::All);
    QTest::addRow("ChildrenVector")
        << TreeProtocol::ChildrenVector
        << expectedRootRowCount << expectedColumnCount << rowsWithChildren
        << ChangeActions(ChangeAction::All);
    QTest::addRow("ChildrenVectorPtr")
        << TreeProtocol::ChildrenVectorPtr
        << expectedRootRowCount << expectedColumnCount << rowsWithChildren
        << ChangeActions(ChangeAction::All);
}

std::unique_ptr<QAbstractItemModel> tst_QRangeModel::makeTreeModel()
{
    createTree();

    std::unique_ptr<QAbstractItemModel> model;

    QFETCH(const TreeProtocol, protocol);
    switch (protocol) {
    case TreeProtocol::ValueImplicit:
        model.reset(new QRangeModel(m_data->m_tree.get()));
        break;
    case TreeProtocol::ValueReadOnly: {
        struct { // minimal (read-only) implementation of the tree traversal protocol
            const tree_row *parentRow(const tree_row &row) const { return row.parentRow(); }
            const std::optional<value_tree> &childRows(const tree_row &row) const
            { return row.childRows(); }
        } readOnlyProtocol;
        model.reset(new QRangeModel(m_data->m_tree.get(), readOnlyProtocol));
        break;
    }
    case TreeProtocol::PointerExplicit:
        model.reset(new QRangeModel(m_data->m_pointer_tree.get(),
                    tree_row::ProtocolPointerImpl{}));
        break;
    case TreeProtocol::PointerExplicitMoved: {
        pointer_tree moved_tree{
            new tree_row("m1", "m_one"),
            new tree_row("m2", "m_two"),
            new tree_row("m3", "m_three"),
            new tree_row("m4", "m_four"),
            new tree_row("m5", "m_five"),
        };
        moved_tree.at(1)->addChildPointer("2.1", "two.one");
        moved_tree.at(1)->addChildPointer("2.2", "two.two");

        model.reset(new QRangeModel(std::move(moved_tree),
                    tree_row::ProtocolPointerImpl{}));
        break;
    }
    case TreeProtocol::ChildrenVector:
        model.reset(new QRangeModel(m_data->m_tree.get(),
                    tree_row::ProtocolWithChildrenVector{}));
        break;
    case TreeProtocol::ChildrenVectorPtr:
        model.reset(new QRangeModel(m_data->m_tree.get(),
                    tree_row::ProtocolWithChildrenVectorPtr{}));
        break;
    }

    return model;
}

void tst_QRangeModel::tree()
{
    auto model = makeTreeModel();
    QFETCH(const int, expectedRootRowCount);
    QFETCH(const int, expectedColumnCount);
    QFETCH(QList<int>, rowsWithChildren);
    QFETCH(ChangeActions, changeActions);

    QCOMPARE(model->rowCount(), expectedRootRowCount);
    QCOMPARE(model->columnCount(), expectedColumnCount);
    QCOMPARE(model->flags(model->index(0, 0)).testFlag(Qt::ItemIsEditable),
             !!(changeActions & ChangeAction::SetData));

    for (int row = 0; row < model->rowCount(); ++row) {
        const bool expectedChildren = rowsWithChildren.contains(row);
        const QModelIndex parent = model->index(row, 0);
        QVERIFY(parent.isValid());
        QCOMPARE(model->hasChildren(parent), expectedChildren);
        if (expectedChildren)
            QCOMPARE_GT(model->rowCount(parent), 0);
        else
            QCOMPARE(model->rowCount(parent), 0);
        QCOMPARE(model->columnCount(parent), expectedColumnCount);
        const QModelIndex child = model->index(0, 0, parent);
        QCOMPARE(child.isValid(), expectedChildren);
        if (expectedChildren)
            QCOMPARE(child.parent(), parent);
        else
            QCOMPARE(child.parent(), QModelIndex());
    }

#if QT_CONFIG(itemmodeltester)
    QAbstractItemModelTester modelTest(model.get());
#endif
}

void tst_QRangeModel::gadgetTree()
{
    GadgetTree tree;
    QRangeModel model(&tree);
    QCOMPARE(model.columnCount(), GadgetTreeItem::staticMetaObject.propertyCount());
    for (int c = 0; c < model.columnCount(); ++c) {
        QCOMPARE(model.headerData(c, Qt::Horizontal),
                 GadgetTreeItem::staticMetaObject.property(c).name());
    }
}

void tst_QRangeModel::treeModifyBranch()
{
    auto model = makeTreeModel();
    QFETCH(QList<int>, rowsWithChildren);
    QFETCH(const ChangeActions, changeActions);

    int rowWithChildren = rowsWithChildren.first();
    QCOMPARE_GT(rowWithChildren, 0);

    // removing or inserting a row adjusts the parents of the direct children
    // of the following branches
    {
        QVERIFY(treeIntegrityCheck());
        QCOMPARE(model->removeRow(--rowWithChildren),
                 changeActions.testFlag(ChangeAction::RemoveRows));
        QVERIFY(treeIntegrityCheck());
        QCOMPARE(model->insertRow(rowWithChildren++),
                 changeActions.testFlag(ChangeAction::InsertRows));
        QVERIFY(treeIntegrityCheck());
        if (!changeActions.testFlag(ChangeAction::ChangeRows))
            return; // nothing else to test with a read-only model
    }

    const QModelIndex parent = model->index(rowWithChildren, 0);
    int oldRowCount = model->rowCount(parent);

    // append
    {
        QVERIFY(model->insertRow(oldRowCount, parent));
        QModelIndex newChild = model->index(oldRowCount, 0, parent);
        QVERIFY(newChild.isValid());
        QCOMPARE(model->rowCount(parent), ++oldRowCount);
        QCOMPARE(newChild.parent(), parent);
    }

    // prepend
    {
        QVERIFY(model->insertRow(0, parent));
        QModelIndex newChild = model->index(0, 0, parent);
        QVERIFY(newChild.isValid());
        QCOMPARE(model->rowCount(parent), ++oldRowCount);
        QCOMPARE(newChild.parent(), parent);
    }

    // remove last
    {
        QVERIFY(model->removeRow(model->rowCount(parent) - 1, parent));
        QCOMPARE(model->rowCount(parent), --oldRowCount);
    }

    // remove first
    {
        QVERIFY(model->rowCount(parent) > 0);
        QVERIFY(model->removeRow(0, parent));
        QCOMPARE(model->rowCount(parent), --oldRowCount);
    }

#if QT_CONFIG(itemmodeltester)
    QAbstractItemModelTester modelTest(model.get());
#endif
}

void tst_QRangeModel::treeCreateBranch()
{
    auto model = makeTreeModel();
    QFETCH(QList<int>, rowsWithChildren);
    QFETCH(const ChangeActions, changeActions);

#if QT_CONFIG(itemmodeltester)
    QAbstractItemModelTester modelTest(model.get());
#endif

    const QList<QPersistentModelIndex> pmiList = allIndexes(model.get());

    // new branch
    QVERIFY(!rowsWithChildren.contains(0));
    const QModelIndex parent = model->index(0, 0);
    QVERIFY(!model->hasChildren(parent));
    QCOMPARE(model->insertRows(0, 5, parent),
             changeActions.testFlag(ChangeAction::InsertRows));
    if (!changeActions.testFlag(ChangeAction::InsertRows))
        return; // nothing else to test with a read-only model
    QVERIFY(model->hasChildren(parent));
    QCOMPARE(model->rowCount(parent), 5);

    for (int i = 0; i < model->rowCount(parent); ++i) {
        QModelIndex newChild = model->index(i, 0, parent);
        QVERIFY(newChild.isValid());
        QCOMPARE(newChild.parent(), parent);
        QVERIFY(!model->hasChildren(newChild));
    }

    verifyPmiList(pmiList);
}

void tst_QRangeModel::treeRemoveBranch()
{
    auto model = makeTreeModel();
    QFETCH(QList<int>, rowsWithChildren);
    QFETCH(const ChangeActions, changeActions);

#if QT_CONFIG(itemmodeltester)
    QAbstractItemModelTester modelTest(model.get());
#endif

    const QModelIndex parent = model->index(rowsWithChildren.first(), 0);
    QVERIFY(parent.isValid());
    QVERIFY(model->hasChildren(parent));
    const int oldRowCount = model->rowCount(parent);
    QCOMPARE_GT(oldRowCount, 0);

    // out of bounds asserts in QAIM::removeRows
    // QVERIFY(model->removeRows(0, oldRowCount * 2, parent));

    QCOMPARE(model->removeRows(0, oldRowCount, parent),
             changeActions.testFlag(ChangeAction::RemoveRows));
    if (!changeActions.testFlag(ChangeAction::RemoveRows))
        return; // nothing else to test with a read-only model
    QVERIFY(!model->hasChildren(parent));
    QCOMPARE(model->rowCount(parent), 0);
}

void tst_QRangeModel::treeMoveRows()
{
    auto model = makeTreeModel();
    QFETCH(const QList<int>, rowsWithChildren);
    QFETCH(const ChangeActions, changeActions);
    if (!changeActions.testFlag(ChangeAction::ChangeRows))
        return;

    const QList<QPersistentModelIndex> pmiList = allIndexes(model.get());

    // move the first row down
    for (int currentRow = 0; currentRow < model->rowCount(); ++currentRow) {
        model->moveRow({}, currentRow, {}, currentRow + 2);
        QVERIFY(treeIntegrityCheck());
    }

    // move the last row back up
    for (int currentRow = model->rowCount() - 1; currentRow > 0; --currentRow) {
        model->moveRow({}, currentRow, {}, currentRow - 1);
        QVERIFY(treeIntegrityCheck());
    }

    verifyPmiList(pmiList);
}

void tst_QRangeModel::treeMoveRowBranches()
{
    auto model = makeTreeModel();
    QFETCH(const QList<int>, rowsWithChildren);
    QFETCH(const ChangeActions, changeActions);

#if QT_CONFIG(itemmodeltester)
    QAbstractItemModelTester modelTest(model.get());
#endif

    const QList<QPersistentModelIndex> pmiList = allIndexes(model.get());

    auto rowData = [model = model.get()](int row, const QModelIndex &parent = {}) -> QVariantList
    {
        QVariantList data;
        for (int i = 0; i < model->columnCount(parent); ++i)
            data << model->data(model->index(row, i, parent));
        return data;
    };

    int branchRow = rowsWithChildren.first();
    // those operations invalidate the model index, so get a fresh one every time
    auto branchParent = [&model, &branchRow] { return model->index(branchRow, 0); };

    int oldRootCount = model->rowCount();
    int oldBranchCount = model->rowCount(branchParent());

    QPersistentModelIndex pmi = model->index(0, 0);
    QVariantList oldData = rowData(0);

    QVERIFY(treeIntegrityCheck());

    // move the first toplevel child to the end of the branch
    QCOMPARE(model->moveRow({}, 0, branchParent(), oldBranchCount),
             changeActions.testFlag(ChangeAction::ChangeRows));
    if (!changeActions.testFlag(ChangeAction::ChangeRows))
        return; // nothing else to test with a read-only model

    QVERIFY(treeIntegrityCheck());
    QCOMPARE(model->rowCount(), --oldRootCount);
    // this moves the branch up
    --branchRow;
    QCOMPARE(model->rowCount(branchParent()), ++oldBranchCount);
    // verify that the data has been copied
    QCOMPARE(rowData(oldBranchCount - 1, branchParent()), oldData);
    // make sure that the moved row has the right parent
    QVERIFY(pmi.isValid());
    QCOMPARE(pmi.parent(), branchParent());

    pmi = model->index(0, 0, branchParent());
    oldData = rowData(0, branchParent());

    // move first child from the branch to the end of the toplevel list
    model->moveRow(branchParent(), 0, {}, model->rowCount());
    QCOMPARE(model->rowCount(branchParent()), --oldBranchCount);
    QCOMPARE(model->rowCount(), ++oldRootCount);
    QCOMPARE(rowData(oldRootCount - 1), oldData);
    QVERIFY(pmi.isValid());
    QVERIFY(!pmi.parent().isValid());

    // move the last child one level up, right before it's own parent
    {
        const QModelIndex parent = branchParent();
        const QModelIndex lastChild = model->index(model->rowCount(parent) - 1, 0, parent);
        const auto grandParent = parent.parent();
        QVERIFY(model->moveRow(parent, lastChild.row(), grandParent, parent.row()));
    }

    verifyPmiList(pmiList);
}

void tst_QRangeModel::sortTree()
{
    auto model = makeTreeModel();
    QFETCH(const QList<int>, rowsWithChildren);

    int branchRow = rowsWithChildren.first();
#if QT_CONFIG(itemmodeltester)
    QAbstractItemModelTester modelTest(model.get());
#endif

    QPersistentModelIndex pmiParent = model->index(branchRow, 0, {});
    const QVariant parentData = pmiParent.data();
    QPersistentModelIndex pmiChild = model->index(0, 1, pmiParent);
    const QVariant childData = pmiChild.data();

    model->sort(0, Qt::AscendingOrder);
    QCOMPARE(pmiParent.data(), parentData);
    QCOMPARE(pmiChild.data(), childData);

    model->sort(0, Qt::DescendingOrder);
    QCOMPARE(pmiParent.data(), parentData);
    QCOMPARE(pmiChild.data(), childData);
}

void tst_QRangeModel::matchRecursive()
{
    auto model = makeTreeModel();
    QFETCH(QList<int>, rowsWithChildren);

    const QModelIndex parent = model->index(rowsWithChildren.first(), 0);
    QVERIFY(parent.isValid());
    QVERIFY(model->hasChildren(parent));

    const QModelIndex child = model->index(0, 0, parent);
    QVERIFY(child.isValid());
    const QVariant childValue = model->data(child);

    const QModelIndex start = model->index(0, 0);
    // Flat search - only root level rows
    const auto resultsFlat = model->match(start, Qt::DisplayRole, childValue,
                                          -1, Qt::MatchExactly);
    // Descends into children - more matches
    const auto results = model->match(start, Qt::DisplayRole, childValue,
                                      -1, Qt::MatchExactly | Qt::MatchRecursive);
    QVERIFY(results.size() > 0);
    QVERIFY(results.size() > resultsFlat.size());
}

void tst_QRangeModel::matchCollatorRecursive()
{
    auto model = makeTreeModel();
    auto *rangeModel = static_cast<QRangeModel *>(model.get());

    QCollator collator(QLocale::English);
    collator.setIgnorePunctuation(true);
    if (collator.compare(u"2.1", u"21") != 0)
        QSKIP("Ignoring diacritic marks unsupported on this platform.");

    const QModelIndex start = model->index(0, 0);
    const auto plain = model->match(start, Qt::DisplayRole, "21", -1,
                                    Qt::MatchFixedString | Qt::MatchRecursive);
    QCOMPARE(plain.size(), 0);

    rangeModel->setMatchCollator(collator);
    // Flat search - only root level rows
    const auto flat = model->match(start, Qt::DisplayRole, "21", -1,
                                   Qt::MatchFixedString);
    QCOMPARE(flat.size(), 0);
    // Descends into children - match found
    const auto recursive = model->match(start, Qt::DisplayRole, "21", -1,
                                        Qt::MatchFixedString | Qt::MatchRecursive);
    QCOMPARE(recursive.size(), 1);
    QCOMPARE(model->data(recursive.first()).toString(), "2.1");
}
