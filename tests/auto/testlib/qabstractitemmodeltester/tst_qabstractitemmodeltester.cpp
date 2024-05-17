// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include <QTest>
#include <QAbstractItemModelTester>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>

#include "dynamictreemodel.h"

class tst_QAbstractItemModelTester : public QObject
{
    Q_OBJECT

private slots:
    void stringListModel();
    void treeWidgetModel();
    void standardItemModel();
    void standardItemModelZeroColumns();
    void testInsertThroughProxy();
    void moveSourceItems();
    void testResetThroughProxy();
};

/*
  tests
*/

void tst_QAbstractItemModelTester::stringListModel()
{
    QStringListModel model;
    QSortFilterProxyModel proxy;

    QAbstractItemModelTester t1(&model);
    QAbstractItemModelTester t2(&proxy);

    proxy.setSourceModel(&model);

    model.setStringList(QStringList() << "2" << "3" << "1");
    model.setStringList(QStringList() << "a" << "e" << "plop" << "b" << "c");

    proxy.setDynamicSortFilter(true);
    proxy.setFilterRegularExpression(QRegularExpression("[^b]"));
}

void tst_QAbstractItemModelTester::treeWidgetModel()
{
    QTreeWidget widget;

    QAbstractItemModelTester t1(widget.model());

    QTreeWidgetItem *root = new QTreeWidgetItem(&widget, QStringList("root"));
    for (int i = 0; i < 20; ++i)
        new QTreeWidgetItem(root, QStringList(QString::number(i)));
    QTreeWidgetItem *remove = root->child(2);
    root->removeChild(remove);
    delete remove;
    QTreeWidgetItem *parent = new QTreeWidgetItem(&widget, QStringList("parent"));
    new QTreeWidgetItem(parent, QStringList("child"));
    parent->setHidden(true);

    widget.sortByColumn(0, Qt::AscendingOrder);
}

void tst_QAbstractItemModelTester::standardItemModel()
{
    QStandardItemModel model(10, 10);
    QSortFilterProxyModel proxy;

    QAbstractItemModelTester t1(&model);
    QAbstractItemModelTester t2(&proxy);

    proxy.setSourceModel(&model);

    model.insertRows(2, 5);
    model.removeRows(4, 5);

    model.insertColumns(2, 5);
    model.removeColumns(4, 5);

    model.insertRows(0, 5, model.index(1, 1));
    model.insertColumns(0, 5, model.index(1, 3));
}

void tst_QAbstractItemModelTester::standardItemModelZeroColumns()
{
    QStandardItemModel model;
    QAbstractItemModelTester t1(&model);
    // QTBUG-92220
    model.insertRows(0, 5);
    model.removeRows(0, 5);
    // QTBUG-92886
    model.insertRows(0, 5);
    model.removeRows(1, 2);

    const QModelIndex parentIndex = model.index(0, 0);
    model.insertRows(0, 5, parentIndex);
    model.removeRows(1, 2, parentIndex);
}

void tst_QAbstractItemModelTester::testInsertThroughProxy()
{
    DynamicTreeModel *model = new DynamicTreeModel(this);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);

    new QAbstractItemModelTester(proxy, this);

    ModelInsertCommand *insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setNumCols(4);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    // Parent is QModelIndex()
    insertCommand->doCommand();

    insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setNumCols(4);
    insertCommand->setAncestorRowNumbers(QList<int>() << 5);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();

    ModelMoveCommand *moveCommand = new ModelMoveCommand(model, this);
    moveCommand->setNumCols(4);
    moveCommand->setStartRow(0);
    moveCommand->setEndRow(0);
    moveCommand->setDestRow(9);
    moveCommand->setDestAncestors(QList<int>() << 5);
    moveCommand->doCommand();
}

/**
  Makes the persistent index list publicly accessible
*/
class AccessibleProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    using QSortFilterProxyModel::persistentIndexList;
};

class ObservingObject : public QObject
{
    Q_OBJECT
public:
    ObservingObject(AccessibleProxyModel *proxy, QObject *parent = nullptr) :
        QObject(parent),
        m_proxy(proxy),
        storePersistentFailureCount(0),
        checkPersistentFailureCount(0)
    {
        // moveRows signals can not come through because the proxy might sort/filter
        // out some of the moved rows. therefore layoutChanged signals are sent from
        // the QSFPM and we have to listen to them here to get properly notified
        connect(m_proxy, &QAbstractProxyModel::layoutAboutToBeChanged, this,
                &ObservingObject::storePersistent);
        connect(m_proxy, &QAbstractProxyModel::layoutChanged, this,
                &ObservingObject::checkPersistent);
    }

public slots:

    void storePersistentRecursive(const QModelIndex &parent)
    {
        for (int row = 0; row < m_proxy->rowCount(parent); ++row) {
            QModelIndex proxyIndex = m_proxy->index(row, 0, parent);
            QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
            if (!proxyIndex.isValid()) {
                qWarning("%s: Invalid proxy index", Q_FUNC_INFO);
                ++storePersistentFailureCount;
            }
            if (!sourceIndex.isValid()) {
                qWarning("%s: invalid source index", Q_FUNC_INFO);
                ++storePersistentFailureCount;
            }
            m_persistentSourceIndexes.append(sourceIndex);
            m_persistentProxyIndexes.append(proxyIndex);
            if (m_proxy->hasChildren(proxyIndex))
                storePersistentRecursive(proxyIndex);
        }
    }

    void storePersistent(const QList<QPersistentModelIndex> &parents = {})
    {
        // This method is called from source model rowsAboutToBeMoved. Persistent indexes should be valid
        foreach (const QModelIndex &idx, m_persistentProxyIndexes)
            if (!idx.isValid()) {
                qWarning("%s: persistentProxyIndexes contains invalid index", Q_FUNC_INFO);
                ++storePersistentFailureCount;
            }
        const auto validCount = std::count_if(parents.begin(), parents.end(),
                                              [](const auto &idx) { return idx.isValid(); });
        if (m_proxy->persistentIndexList().size() != validCount) {
            qWarning("%s: proxy should have no additional persistent indexes when storePersistent called",
                     Q_FUNC_INFO);
            ++storePersistentFailureCount;
        }
        storePersistentRecursive(QModelIndex());
        if (m_proxy->persistentIndexList().isEmpty()) {
            qWarning("%s: proxy should have persistent index after storePersistent called",
                     Q_FUNC_INFO);
            ++storePersistentFailureCount;
        }
    }

    void checkPersistent()
    {
        QVERIFY(!m_persistentProxyIndexes.isEmpty());
        QVERIFY(!m_persistentSourceIndexes.isEmpty());

        for (int row = 0; row < m_persistentProxyIndexes.size(); ++row) {
            m_persistentProxyIndexes.at(row);
            m_persistentSourceIndexes.at(row);
        }
        for (int row = 0; row < m_persistentProxyIndexes.size(); ++row) {
            QModelIndex updatedProxy = m_persistentProxyIndexes.at(row);
            QModelIndex updatedSource = m_persistentSourceIndexes.at(row);
            if (m_proxy->mapToSource(updatedProxy) != updatedSource) {
                qWarning("%s: check failed at row %d", Q_FUNC_INFO, row);
                ++checkPersistentFailureCount;
            }
        }
        m_persistentSourceIndexes.clear();
        m_persistentProxyIndexes.clear();
    }

private:
    AccessibleProxyModel *m_proxy;
    QList<QPersistentModelIndex> m_persistentSourceIndexes;
    QList<QPersistentModelIndex> m_persistentProxyIndexes;
public:
    int storePersistentFailureCount;
    int checkPersistentFailureCount;
};

void tst_QAbstractItemModelTester::moveSourceItems()
{
    DynamicTreeModel *model = new DynamicTreeModel(this);
    AccessibleProxyModel *proxy = new AccessibleProxyModel(this);
    proxy->setSourceModel(model);

    ModelInsertCommand *insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(2);
    insertCommand->doCommand();

    insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setAncestorRowNumbers(QList<int>() << 1);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(2);
    insertCommand->doCommand();

    ObservingObject observer(proxy);

    ModelMoveCommand *moveCommand = new ModelMoveCommand(model, this);
    moveCommand->setStartRow(0);
    moveCommand->setEndRow(0);
    moveCommand->setDestAncestors(QList<int>() << 1);
    moveCommand->setDestRow(0);
    moveCommand->doCommand();

    QCOMPARE(observer.storePersistentFailureCount, 0);
    QCOMPARE(observer.checkPersistentFailureCount, 0);
}

void tst_QAbstractItemModelTester::testResetThroughProxy()
{
    DynamicTreeModel *model = new DynamicTreeModel(this);

    ModelInsertCommand *insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(2);
    insertCommand->doCommand();

    QPersistentModelIndex persistent = model->index(0, 0);

    AccessibleProxyModel *proxy = new AccessibleProxyModel(this);
    proxy->setSourceModel(model);

    ObservingObject observer(proxy);
    observer.storePersistent();

    ModelResetCommand *resetCommand = new ModelResetCommand(model, this);
    resetCommand->setNumCols(0);
    resetCommand->doCommand();

    QCOMPARE(observer.storePersistentFailureCount, 0);
    QCOMPARE(observer.checkPersistentFailureCount, 0);
}

QTEST_MAIN(tst_QAbstractItemModelTester)
#include "tst_qabstractitemmodeltester.moc"
