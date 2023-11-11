// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QStandardItemModel>
#include <QTreeView>
#include <QMap>
#include <QSignalSpy>
#include <QAbstractItemModelTester>

#include <private/qabstractitemmodel_p.h>
#include <private/qpropertytesthelper_p.h>
#include <private/qtreeview_p.h>

#include <algorithm>

using namespace Qt::StringLiterals;

class tst_QStandardItemModel : public QObject
{
    Q_OBJECT

public:
    tst_QStandardItemModel();

    enum ModelChanged {
        RowsAboutToBeInserted,
        RowsInserted,
        RowsAboutToBeRemoved,
        RowsRemoved,
        ColumnsAboutToBeInserted,
        ColumnsInserted,
        ColumnsAboutToBeRemoved,
        ColumnsRemoved
    };

public slots:
    void init();
    void cleanup();

protected slots:
    void checkAboutToBeRemoved();
    void checkRemoved();
    void updateRowAboutToBeRemoved();

    void rowsAboutToBeInserted(const QModelIndex &parent, int first, int last)
        { modelChanged(RowsAboutToBeInserted, parent, first, last); }
    void rowsInserted(const QModelIndex &parent, int first, int last)
        { modelChanged(RowsInserted, parent, first, last); }
    void rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
        { modelChanged(RowsAboutToBeRemoved, parent, first, last); }
    void rowsRemoved(const QModelIndex &parent, int first, int last)
        { modelChanged(RowsRemoved, parent, first, last); }
    void columnsAboutToBeInserted(const QModelIndex &parent, int first, int last)
        { modelChanged(ColumnsAboutToBeInserted, parent, first, last); }
    void columnsInserted(const QModelIndex &parent, int first, int last)
        { modelChanged(ColumnsInserted, parent, first, last); }
    void columnsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
        { modelChanged(ColumnsAboutToBeRemoved, parent, first, last); }
    void columnsRemoved(const QModelIndex &parent, int first, int last)
        { modelChanged(ColumnsRemoved, parent, first, last); }

    void modelChanged(ModelChanged change, const QModelIndex &parent, int first, int last);

private slots:
    void insertRow_data();
    void insertRow();
    void insertRows();
    void insertRowsItems();
    void insertRowInHierarcy();
    void insertColumn_data();
    void insertColumn();
    void insertColumns();
    void removeRows();
    void removeColumns();
    void setHeaderData();
    void persistentIndexes();
    void removingPersistentIndexes();
    void updatingPersistentIndexes();

    void checkChildren();
    void data();
    void clear();
    void clearItemData();
    void sort_data();
    void sort();
    void sortRole_data();
    void sortRole();
    void sortRoleBindings();
    void findItems();
    void getSetHeaderItem();
    void indexFromItem();
    void itemFromIndex();
    void getSetItemPrototype();
    void getSetItemData_data();
    void getSetItemData();
    void setHeaderLabels_data();
    void setHeaderLabels();
    void itemDataChanged();
    void takeHeaderItem();
    void useCase1();
    void useCase2();
    void useCase3();

    void setNullChild();
    void deleteChild();

    void rootItemFlags();
#ifdef QT_BUILD_INTERNAL
    void treeDragAndDrop();
#endif
    void removeRowsAndColumns();

    void defaultItemRoles();
    void itemRoleNames();
    void getMimeDataWithInvalidModelIndex();
    void supportedDragDropActions();

    void taskQTBUG_45114_setItemData();
    void setItemPersistentIndex();
    void signalsOnTakeItem();
    void takeChild();
    void createPersistentOnLayoutAboutToBeChanged();
private:
    QStandardItemModel *m_model = nullptr;
    QPersistentModelIndex persistent;
    QList<QModelIndex> rcParent = QList<QModelIndex>(8);
    QList<int> rcFirst = QList<int>(8, 0);
    QList<int> rcLast = QList<int>(8, 0);
    QList<int> currentRoles;

    //return true if models have the same structure, and all child have the same text
    static bool compareModels(QStandardItemModel *model1, QStandardItemModel *model2);
    //return true if models have the same structure, and all child have the same text
    static bool compareItems(QStandardItem *item1, QStandardItem *item2);
};

static constexpr int defaultSize = 3;

Q_DECLARE_METATYPE(QStandardItem*)
Q_DECLARE_METATYPE(Qt::Orientation)

tst_QStandardItemModel::tst_QStandardItemModel()
{
    qRegisterMetaType<QStandardItem*>("QStandardItem*");
    qRegisterMetaType<Qt::Orientation>("Qt::Orientation");
    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint");
    qRegisterMetaType<QList<QPersistentModelIndex>>("QList<QPersistentModelIndex>");
}

/*
  This test usually uses a model with a 3x3 table
  ---------------------------
  |  0,0  |  0,1    |  0,2  |
  ---------------------------
  |  1,0  |  1,1    |  1,2  |
  ---------------------------
  |  2,0  |  2,1    |  2,2  |
  ---------------------------
*/
void tst_QStandardItemModel::init()
{
    m_model = new QStandardItemModel(defaultSize, defaultSize);
    connect(m_model, &QStandardItemModel::rowsAboutToBeInserted,
            this, &tst_QStandardItemModel::rowsAboutToBeInserted);
    connect(m_model, &QStandardItemModel::rowsInserted,
            this, &tst_QStandardItemModel::rowsInserted);
    connect(m_model, &QStandardItemModel::rowsAboutToBeRemoved,
            this, &tst_QStandardItemModel::rowsAboutToBeRemoved);
    connect(m_model, &QStandardItemModel::rowsRemoved,
            this, &tst_QStandardItemModel::rowsRemoved);

    connect(m_model, &QStandardItemModel::columnsAboutToBeInserted,
            this, &tst_QStandardItemModel::columnsAboutToBeInserted);
    connect(m_model, &QStandardItemModel::columnsInserted,
            this, &tst_QStandardItemModel::columnsInserted);
    connect(m_model, &QStandardItemModel::columnsAboutToBeRemoved,
            this, &tst_QStandardItemModel::columnsAboutToBeRemoved);
    connect(m_model, &QStandardItemModel::columnsRemoved,
            this, &tst_QStandardItemModel::columnsRemoved);

    connect(m_model, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &, const QModelIndex &, const QList<int> &roles) {
                currentRoles = roles;
            });

    rcFirst.fill(-1);
    rcLast.fill(-1);
}

void tst_QStandardItemModel::cleanup()
{
    m_model->disconnect(this);
    delete m_model;
    m_model = nullptr;
}

void tst_QStandardItemModel::insertRow_data()
{
    QTest::addColumn<int>("insertRow");
    QTest::addColumn<int>("expectedRow");

    QTest::newRow("Insert less then 0") << -1 << 0;
    QTest::newRow("Insert at 0")  << 0 << 0;
    QTest::newRow("Insert beyond count")  << defaultSize+1 << defaultSize;
    QTest::newRow("Insert at count") << defaultSize << defaultSize;
    QTest::newRow("Insert in the middle") << 1 << 1;
}

void tst_QStandardItemModel::insertRow()
{
    QFETCH(int, insertRow);
    QFETCH(int, expectedRow);

    QIcon icon;
    // default all initial items to DisplayRole: "initalitem"
    for (int  r = 0; r < m_model->rowCount(); ++r) {
        for (int c = 0; c < m_model->columnCount(); ++c) {
            m_model->setData(m_model->index(r, c), "initialitem", Qt::DisplayRole);
        }
    }

    // check that inserts changes rowCount
    QCOMPARE(m_model->rowCount(), defaultSize);
    m_model->insertRow(insertRow);
    if (insertRow >= 0 && insertRow <= defaultSize) {
        QCOMPARE(m_model->rowCount(), defaultSize + 1);

        // check that signals were emitted with correct info
        QCOMPARE(rcFirst[RowsAboutToBeInserted], expectedRow);
        QCOMPARE(rcLast[RowsAboutToBeInserted], expectedRow);
        QCOMPARE(rcFirst[RowsInserted], expectedRow);
        QCOMPARE(rcLast[RowsInserted], expectedRow);

        //check that the inserted item has different DisplayRole than initial items
        QVERIFY(m_model->data(m_model->index(expectedRow, 0), Qt::DisplayRole).toString() != QLatin1String("initialitem"));
    } else {
        // We inserted something outside the bounds, do nothing
        QCOMPARE(m_model->rowCount(), defaultSize);
        QCOMPARE(rcFirst[RowsAboutToBeInserted], -1);
        QCOMPARE(rcLast[RowsAboutToBeInserted], -1);
        QCOMPARE(rcFirst[RowsInserted], -1);
        QCOMPARE(rcLast[RowsInserted], -1);
    }
}

void tst_QStandardItemModel::insertRows()
{
    int rowCount = m_model->rowCount();
    QCOMPARE(rowCount, defaultSize);

    // insert custom header label
    QString headerLabel = "custom";
    m_model->setHeaderData(0, Qt::Vertical, headerLabel);

    // insert one row
    m_model->insertRows(0, 1);
    QCOMPARE(m_model->rowCount(), rowCount + 1);
    rowCount = m_model->rowCount();

    // check header data has moved
    QCOMPARE(m_model->headerData(1, Qt::Vertical).toString(), headerLabel);

    // insert two rows
    m_model->insertRows(0, 2);
    QCOMPARE(m_model->rowCount(), rowCount + 2);

    // check header data has moved
    QCOMPARE(m_model->headerData(3, Qt::Vertical).toString(), headerLabel);

    // do not assert on empty list
    QStandardItem *si = m_model->invisibleRootItem();
    si->insertRow(0, QList<QStandardItem*>());
    si->insertRows(0, 0);
    si->insertRows(0, QList<QStandardItem*>());
}

void tst_QStandardItemModel::insertRowsItems()
{
    int rowCount = m_model->rowCount();

    QList<QStandardItem *> items;
    QStandardItemModel *m = m_model;
    QStandardItem *hiddenRoot = m->invisibleRootItem();
    for (int i = 0; i < 3; ++i)
        items.append(new QStandardItem(QString::number(i + 10)));
    hiddenRoot->appendRows(items);
    QCOMPARE(m_model->rowCount(), rowCount + 3);
    QCOMPARE(m_model->index(rowCount + 0, 0).data().toInt(), 10);
    QCOMPARE(m_model->index(rowCount + 1, 0).data().toInt(), 11);
    QCOMPARE(m_model->index(rowCount + 2, 0).data().toInt(), 12);
    for (int i = rowCount; i < rowCount + 3; ++i) {
        QVERIFY(m->item(i));
        QCOMPARE(m->item(i)->model(), m_model);
    }
}

void tst_QStandardItemModel::insertRowInHierarcy()
{
    QVERIFY(m_model->insertRows(0, 1, QModelIndex()));
    QVERIFY(m_model->insertColumns(0, 1, QModelIndex()));
    QVERIFY(m_model->hasIndex(0, 0, QModelIndex()));

    QModelIndex parent = m_model->index(0, 0, QModelIndex());
    QVERIFY(parent.isValid());

    QVERIFY(m_model->insertRows(0, 1, parent));
    QVERIFY(m_model->insertColumns(0, 1, parent));
    QVERIFY(m_model->hasIndex(0, 0, parent));

    QModelIndex child = m_model->index(0, 0, parent);
    QVERIFY(child.isValid());
}

void tst_QStandardItemModel::insertColumn_data()
{
    QTest::addColumn<int>("insertColumn");
    QTest::addColumn<int>("expectedColumn");

    QTest::newRow("Insert less then 0") << -1 << 0;
    QTest::newRow("Insert at 0")  << 0 << 0;
    QTest::newRow("Insert beyond count")  << defaultSize+1 << defaultSize;
    QTest::newRow("Insert at count") << defaultSize << defaultSize;
    QTest::newRow("Insert in the middle") << 1 << 1;
}

void tst_QStandardItemModel::insertColumn()
{
    QFETCH(int, insertColumn);
    QFETCH(int, expectedColumn);

    // default all initial items to DisplayRole: "initalitem"
    for (int r = 0; r < m_model->rowCount(); ++r) {
        for (int c = 0; c < m_model->columnCount(); ++c) {
            m_model->setData(m_model->index(r, c), "initialitem", Qt::DisplayRole);
        }
    }

    // check that inserts changes columnCount
    QCOMPARE(m_model->columnCount(), defaultSize);
    m_model->insertColumn(insertColumn);
    if (insertColumn >= 0 && insertColumn <= defaultSize) {
        QCOMPARE(m_model->columnCount(), defaultSize +  1);
        // check that signals were emitted with correct info
        QCOMPARE(rcFirst[ColumnsAboutToBeInserted], expectedColumn);
        QCOMPARE(rcLast[ColumnsAboutToBeInserted], expectedColumn);
        QCOMPARE(rcFirst[ColumnsInserted], expectedColumn);
        QCOMPARE(rcLast[ColumnsInserted], expectedColumn);

        //check that the inserted item has different DisplayRole than initial items
        QVERIFY(m_model->data(m_model->index(0, expectedColumn), Qt::DisplayRole).toString() != QLatin1String("initialitem"));
    } else {
        // We inserted something outside the bounds, do nothing
        QCOMPARE(m_model->columnCount(), defaultSize);
        QCOMPARE(rcFirst[ColumnsAboutToBeInserted], -1);
        QCOMPARE(rcLast[ColumnsAboutToBeInserted], -1);
        QCOMPARE(rcFirst[ColumnsInserted], -1);
        QCOMPARE(rcLast[ColumnsInserted], -1);
    }

}

void tst_QStandardItemModel::insertColumns()
{
    int columnCount = m_model->columnCount();
    QCOMPARE(columnCount, defaultSize);

    // insert custom header label
    QString headerLabel = "custom";
    m_model->setHeaderData(0, Qt::Horizontal, headerLabel);

    // insert one column
    m_model->insertColumns(0, 1);
    QCOMPARE(m_model->columnCount(), columnCount + 1);
    columnCount = m_model->columnCount();

    // check header data has moved
    QCOMPARE(m_model->headerData(1, Qt::Horizontal).toString(), headerLabel);

    // insert two columns
    m_model->insertColumns(0, 2);
    QCOMPARE(m_model->columnCount(), columnCount + 2);

    // check header data has moved
    QCOMPARE(m_model->headerData(3, Qt::Horizontal).toString(), headerLabel);

    // do not assert on empty list
    QStandardItem *si = m_model->invisibleRootItem();
    si->insertColumn(0, QList<QStandardItem*>());
    si->insertColumns(0, 0);
}

void tst_QStandardItemModel::removeRows()
{
    int rowCount = m_model->rowCount();
    QCOMPARE(rowCount, defaultSize);

    // insert custom header label
    QString headerLabel = "custom";
    m_model->setHeaderData(rowCount - 1, Qt::Vertical, headerLabel);

    // remove one row
    m_model->removeRows(0, 1);
    QCOMPARE(m_model->rowCount(), rowCount - 1);
    rowCount = m_model->rowCount();

    // check header data has moved
    QCOMPARE(m_model->headerData(rowCount - 1, Qt::Vertical).toString(), headerLabel);

    // remove two rows
    m_model->removeRows(0, 2);
    QCOMPARE(m_model->rowCount(), rowCount - 2);
}

void tst_QStandardItemModel::removeColumns()
{
    int columnCount = m_model->columnCount();
    QCOMPARE(columnCount, defaultSize);

    // insert custom header label
    QString headerLabel = "custom";
    m_model->setHeaderData(columnCount - 1, Qt::Horizontal, headerLabel);

    // remove one column
    m_model->removeColumns(0, 1);
    QCOMPARE(m_model->columnCount(), columnCount - 1);
    columnCount = m_model->columnCount();

    // check header data has moved
    QCOMPARE(m_model->headerData(columnCount - 1, Qt::Horizontal).toString(), headerLabel);

    // remove two columns
    m_model->removeColumns(0, 2);
    QCOMPARE(m_model->columnCount(), columnCount - 2);
}


void tst_QStandardItemModel::setHeaderData()
{
    for (int x = 0; x < 2; ++x) {
        bool vertical = (x == 0);
        int count = vertical ? m_model->rowCount() : m_model->columnCount();
        QCOMPARE(count, defaultSize);
        Qt::Orientation orient = vertical ? Qt::Vertical : Qt::Horizontal;

        // check default values are ok
        for (int i = 0; i < count; ++i)
            QCOMPARE(m_model->headerData(i, orient).toString(), QString::number(i + 1));

        QSignalSpy headerDataChangedSpy(
            m_model, &QAbstractItemModel::headerDataChanged);
        QSignalSpy dataChangedSpy(
            m_model, &QAbstractItemModel::dataChanged);
        // insert custom values and check
        for (int i = 0; i < count; ++i) {
            QString customString = QString("custom") + QString::number(i);
            QCOMPARE(m_model->setHeaderData(i, orient, customString), true);
            QCOMPARE(headerDataChangedSpy.size(), 1);
            QCOMPARE(dataChangedSpy.size(), 0);
            QVariantList args = headerDataChangedSpy.takeFirst();
            QCOMPARE(qvariant_cast<Qt::Orientation>(args.at(0)), orient);
            QCOMPARE(args.at(1).toInt(), i);
            QCOMPARE(args.at(2).toInt(), i);
            QCOMPARE(m_model->headerData(i, orient).toString(), customString);
            QCOMPARE(m_model->setHeaderData(i, orient, customString), true);
            QCOMPARE(headerDataChangedSpy.size(), 0);
            QCOMPARE(dataChangedSpy.size(), 0);
        }

        //check read from invalid sections
        QVERIFY(!m_model->headerData(count, orient).isValid());
        QVERIFY(!m_model->headerData(-1, orient).isValid());
        //check write to invalid section
        QCOMPARE(m_model->setHeaderData(count, orient, "foo"), false);
        QCOMPARE(m_model->setHeaderData(-1, orient, "foo"), false);
        QVERIFY(!m_model->headerData(count, orient).isValid());
        QVERIFY(!m_model->headerData(-1, orient).isValid());
    }
}

void tst_QStandardItemModel::persistentIndexes()
{
    QCOMPARE(m_model->rowCount(), defaultSize);
    QCOMPARE(m_model->columnCount(), defaultSize);

    // create a persisten index at 0,0
    QPersistentModelIndex persistentIndex(m_model->index(0, 0));

    // verify it is ok and at the correct spot
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 0);
    QCOMPARE(persistentIndex.column(), 0);

    // insert row and check that the persisten index has moved
    QVERIFY(m_model->insertRow(0));
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 1);
    QCOMPARE(persistentIndex.column(), 0);

    // insert row after the persisten index and see that it stays the same
    QVERIFY(m_model->insertRow(m_model->rowCount()));
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 1);
    QCOMPARE(persistentIndex.column(), 0);

    // insert column and check that the persisten index has moved
    QVERIFY(m_model->insertColumn(0));
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 1);
    QCOMPARE(persistentIndex.column(), 1);

    // insert column after the persisten index and see that it stays the same
    QVERIFY(m_model->insertColumn(m_model->columnCount()));
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 1);
    QCOMPARE(persistentIndex.column(), 1);

    // removes a row beyond the persistent index and see it stays the same
    QVERIFY(m_model->removeRow(m_model->rowCount() - 1));
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 1);
    QCOMPARE(persistentIndex.column(), 1);

    // removes a column beyond the persistent index and see it stays the same
    QVERIFY(m_model->removeColumn(m_model->columnCount() - 1));
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 1);
    QCOMPARE(persistentIndex.column(), 1);

    // removes a row before the persistent index and see it moves the same
    QVERIFY(m_model->removeRow(0));
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 0);
    QCOMPARE(persistentIndex.column(), 1);

    // removes a column before the persistent index and see it moves the same
    QVERIFY(m_model->removeColumn(0));
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 0);
    QCOMPARE(persistentIndex.column(), 0);

    // remove the row where the persistent index is, and see that it becomes invalid
    QVERIFY(m_model->removeRow(0));
    QVERIFY(!persistentIndex.isValid());

    // remove the row where the persistent index is, and see that it becomes invalid
    persistentIndex = m_model->index(0, 0);
    QVERIFY(persistentIndex.isValid());
    QVERIFY(m_model->removeColumn(0));
    QVERIFY(!persistentIndex.isValid());
}

void tst_QStandardItemModel::checkAboutToBeRemoved()
{
    QVERIFY(persistent.isValid());
}

void tst_QStandardItemModel::checkRemoved()
{
    QVERIFY(!persistent.isValid());
}

void tst_QStandardItemModel::removingPersistentIndexes()
{
    // add 10 rows and columns to model to make it big enough
    QVERIFY(m_model->insertRows(0, 10));
    QVERIFY(m_model->insertColumns(0, 10));

    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &tst_QStandardItemModel::checkAboutToBeRemoved);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, &tst_QStandardItemModel::checkRemoved);
    connect(m_model, &QAbstractItemModel::columnsAboutToBeRemoved,
            this, &tst_QStandardItemModel::checkAboutToBeRemoved);
    connect(m_model, &QAbstractItemModel::columnsRemoved,
            this, &tst_QStandardItemModel::checkRemoved);


    // test removeRow
    // add child table 3x3 to parent index(0, 0)
    QVERIFY(m_model->insertRows(0, 3, m_model->index(0, 0)));
    QVERIFY(m_model->insertColumns(0, 3, m_model->index(0, 0)));

    // set child to persistent and delete parent row
    persistent = m_model->index(0, 0, m_model->index(0, 0));
    QVERIFY(persistent.isValid());
    QVERIFY(m_model->removeRow(0));

    // set persistent to index(0, 0) and remove that row
    persistent = m_model->index(0, 0);
    QVERIFY(persistent.isValid());
    QVERIFY(m_model->removeRow(0));


    // test removeColumn
    // add child table 3x3 to parent index (0, 0)
    QVERIFY(m_model->insertRows(0, 3, m_model->index(0, 0)));
    QVERIFY(m_model->insertColumns(0, 3, m_model->index(0, 0)));

    // set child to persistent and delete parent column
    persistent = m_model->index(0, 0, m_model->index(0, 0));
    QVERIFY(persistent.isValid());
    QVERIFY(m_model->removeColumn(0));

    // set persistent to index(0, 0) and remove that column
    persistent = m_model->index(0, 0);
    QVERIFY(persistent.isValid());
    QVERIFY(m_model->removeColumn(0));


    disconnect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
               this, &tst_QStandardItemModel::checkAboutToBeRemoved);
    disconnect(m_model, &QAbstractItemModel::rowsRemoved,
               this, &tst_QStandardItemModel::checkRemoved);
    disconnect(m_model, &QAbstractItemModel::columnsAboutToBeRemoved,
               this, &tst_QStandardItemModel::checkAboutToBeRemoved);
    disconnect(m_model, &QAbstractItemModel::columnsRemoved,
               this, &tst_QStandardItemModel::checkRemoved);
}

void tst_QStandardItemModel::updateRowAboutToBeRemoved()
{
    QModelIndex idx = m_model->index(0, 0);
    QVERIFY(idx.isValid());
    persistent = idx;
}

void tst_QStandardItemModel::updatingPersistentIndexes()
{
    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &tst_QStandardItemModel::updateRowAboutToBeRemoved);

    persistent = m_model->index(1, 0);
    QVERIFY(persistent.isValid());
    QVERIFY(m_model->removeRow(1));
    QVERIFY(persistent.isValid());
    QPersistentModelIndex tmp = m_model->index(0, 0);
    QCOMPARE(persistent, tmp);

    disconnect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
               this, &tst_QStandardItemModel::updateRowAboutToBeRemoved);
}

void tst_QStandardItemModel::modelChanged(ModelChanged change, const QModelIndex &parent,
                                          int first, int last)
{
    rcParent[change] = parent;
    rcFirst[change] = first;
    rcLast[change] = last;
}


void tst_QStandardItemModel::checkChildren()
{
    QStandardItemModel model(0, 0);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 0);
    QVERIFY(!model.hasChildren());

    QVERIFY(model.insertRows(0, 1));
    QVERIFY(!model.hasChildren());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.columnCount(), 0);

    QVERIFY(model.insertColumns(0, 1));
    QVERIFY(model.hasChildren());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.columnCount(), 1);

    QModelIndex idx = model.index(0, 0);
    QVERIFY(!model.hasChildren(idx));
    QCOMPARE(model.rowCount(idx), 0);
    QCOMPARE(model.columnCount(idx), 0);

    QVERIFY(model.insertRows(0, 1, idx));
    QVERIFY(!model.hasChildren(idx));
    QCOMPARE(model.rowCount(idx), 1);
    QCOMPARE(model.columnCount(idx), 0);

    QVERIFY(model.insertColumns(0, 1, idx));
    QVERIFY(model.hasChildren(idx));
    QCOMPARE(model.rowCount(idx), 1);
    QCOMPARE(model.columnCount(idx), 1);

    QModelIndex idx2 = model.index(0, 0, idx);
    QVERIFY(!model.hasChildren(idx2));
    QCOMPARE(model.rowCount(idx2), 0);
    QCOMPARE(model.columnCount(idx2), 0);

    QVERIFY(model.removeRows(0, 1, idx));
    QVERIFY(model.hasChildren());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.columnCount(), 1);
    QVERIFY(!model.hasChildren(idx));
    QCOMPARE(model.rowCount(idx), 0);
    QCOMPARE(model.columnCount(idx), 1);

    QVERIFY(model.removeRows(0, 1));
    QVERIFY(!model.hasChildren());
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 1);

    QVERIFY(!model.index(0,0).sibling(100,100).isValid());
}

void tst_QStandardItemModel::data()
{
    currentRoles.clear();
    // bad args
    m_model->setData(QModelIndex(), "bla", Qt::DisplayRole);
    QVERIFY(currentRoles.isEmpty());

    QIcon icon;
    for (int r = 0; r < m_model->rowCount(); ++r) {
        for (int c = 0; c < m_model->columnCount(); ++c) {
            m_model->setData(m_model->index(r,c), "initialitem", Qt::DisplayRole);
            QCOMPARE(currentRoles, QList<int>({ Qt::DisplayRole, Qt::EditRole }));
            m_model->setData(m_model->index(r,c), "tooltip", Qt::ToolTipRole);
            QCOMPARE(currentRoles, QList<int> { Qt::ToolTipRole });
            m_model->setData(m_model->index(r,c), icon, Qt::DecorationRole);
            QCOMPARE(currentRoles, QList<int> { Qt::DecorationRole });
        }
    }

    QCOMPARE(m_model->data(m_model->index(0, 0), Qt::DisplayRole).toString(), QLatin1String("initialitem"));
    QCOMPARE(m_model->data(m_model->index(0, 0), Qt::ToolTipRole).toString(), QLatin1String("tooltip"));
    const QMap<int, QVariant> itmData = m_model->itemData(m_model->index(0, 0));
    QCOMPARE(itmData.value(Qt::DisplayRole), QLatin1String("initialitem"));
    QCOMPARE(itmData.value(Qt::ToolTipRole), QLatin1String("tooltip"));
    QVERIFY(!itmData.contains(Qt::UserRole - 1));
    QVERIFY(m_model->itemData(QModelIndex()).isEmpty());
}

void tst_QStandardItemModel::clearItemData()
{
    currentRoles.clear();
    QVERIFY(!m_model->clearItemData(QModelIndex()));
    QVERIFY(currentRoles.isEmpty());
    const QModelIndex idx = m_model->index(0, 0);
    const QMap<int, QVariant> oldData = m_model->itemData(idx);
    m_model->setData(idx, QLatin1String("initialitem"), Qt::DisplayRole);
    m_model->setData(idx, QLatin1String("tooltip"), Qt::ToolTipRole);
    m_model->setData(idx, 5, Qt::UserRole);
    currentRoles.clear();
    QVERIFY(m_model->clearItemData(idx));
    QCOMPARE(idx.data(Qt::UserRole), QVariant());
    QCOMPARE(idx.data(Qt::ToolTipRole), QVariant());
    QCOMPARE(idx.data(Qt::DisplayRole), QVariant());
    QCOMPARE(idx.data(Qt::EditRole), QVariant());
    QVERIFY(currentRoles.isEmpty());
    m_model->setItemData(idx, oldData);
    currentRoles.clear();
}

void tst_QStandardItemModel::clear()
{
    QStandardItemModel model;
    model.insertColumns(0, 10);
    model.insertRows(0, 10);
    QCOMPARE(model.columnCount(), 10);
    QCOMPARE(model.rowCount(), 10);

    QSignalSpy modelResetSpy(&model, &QStandardItemModel::modelReset);
    QSignalSpy layoutChangedSpy(&model, &QStandardItemModel::layoutChanged);
    QSignalSpy rowsRemovedSpy(&model, &QStandardItemModel::rowsRemoved);

    QAbstractItemModelTester mt(&model);

    model.clear();

    QCOMPARE(modelResetSpy.size(), 1);
    QCOMPARE(layoutChangedSpy.size(), 0);
    QCOMPARE(rowsRemovedSpy.size(), 0);
    QCOMPARE(model.index(0, 0), QModelIndex());
    QCOMPARE(model.columnCount(), 0);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.hasChildren(), false);
}

void tst_QStandardItemModel::sort_data()
{
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QStringList>("expected");

    const QStringList unsorted(
        {"delta", "yankee", "bravo", "lima", "charlie", "juliet",
         "tango", "hotel", "uniform", "alpha", "echo", "golf",
         "quebec", "foxtrot", "india", "romeo", "november",
         "oskar", "zulu", "kilo", "whiskey", "mike", "papa",
         "sierra", "xray" , "viktor"});
    QStringList sorted = unsorted;

    std::sort(sorted.begin(), sorted.end());
    QTest::newRow("flat ascending") <<  Qt::AscendingOrder
                                 << unsorted
                                 << sorted;
    std::reverse(sorted.begin(), sorted.end());
    QTest::newRow("flat descending") << Qt::DescendingOrder
                                  << unsorted
                                  << sorted;
    QStringList list;
    for (int i = 1000; i < 2000; ++i)
        list.append(QStringLiteral("Number: ") + QString::number(i));
    QTest::newRow("large set ascending") <<  Qt::AscendingOrder << list << list;
}

void tst_QStandardItemModel::sort()
{
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(QStringList, initial);
    QFETCH(QStringList, expected);
    // prepare model
    QStandardItemModel model;
    QVERIFY(model.insertRows(0, initial.size(), QModelIndex()));
    QCOMPARE(model.rowCount(QModelIndex()), initial.size());
    model.insertColumns(0, 1, QModelIndex());
    QCOMPARE(model.columnCount(QModelIndex()), 1);
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row) {
        QModelIndex index = model.index(row, 0, QModelIndex());
        model.setData(index, initial.at(row), Qt::DisplayRole);
    }

    QSignalSpy layoutAboutToBeChangedSpy(
        &model, &QStandardItemModel::layoutAboutToBeChanged);
    QSignalSpy layoutChangedSpy(
        &model, &QStandardItemModel::layoutChanged);

    // sort
    model.sort(0, sortOrder);

    QCOMPARE(layoutAboutToBeChangedSpy.size(), 1);
    QCOMPARE(layoutChangedSpy.size(), 1);

    // make sure the model is sorted
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row) {
        QModelIndex index = model.index(row, 0, QModelIndex());
        QCOMPARE(model.data(index, Qt::DisplayRole).toString(), expected.at(row));
    }
}

void tst_QStandardItemModel::sortRole_data()
{
    QTest::addColumn<QStringList>("initialText");
    QTest::addColumn<QVariantList>("initialData");
    QTest::addColumn<Qt::ItemDataRole>("sortRole");
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<QStringList>("expectedText");
    QTest::addColumn<QVariantList>("expectedData");

    QTest::newRow("sort ascending with Qt::DisplayRole")
        << (QStringList() << "b" << "a" << "c")
        << (QVariantList() << 2 << 3 << 1)
        << Qt::DisplayRole
        << Qt::AscendingOrder
        << (QStringList() << "a" << "b" << "c")
        << (QVariantList() << 3 << 2 << 1);
    QTest::newRow("sort ascending with Qt::UserRole")
        << (QStringList() << "a" << "b" << "c")
        << (QVariantList() << 3 << 2 << 1)
        << Qt::UserRole
        << Qt::AscendingOrder
        << (QStringList() << "c" << "b" << "a")
        << (QVariantList() << 1 << 2 << 3);
}

void tst_QStandardItemModel::sortRole()
{
    QFETCH(QStringList, initialText);
    QFETCH(QVariantList, initialData);
    QFETCH(Qt::ItemDataRole, sortRole);
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(QStringList, expectedText);
    QFETCH(QVariantList, expectedData);

    QStandardItemModel model;
    for (int i = 0; i < initialText.size(); ++i) {
        QStandardItem *item = new QStandardItem;
        item->setText(initialText.at(i));
        item->setData(initialData.at(i), Qt::UserRole);
        model.appendRow(item);
    }
    model.setSortRole(sortRole);
    model.sort(0, sortOrder);
    for (int i = 0; i < expectedText.size(); ++i) {
        QStandardItem *item = model.item(i);
        QCOMPARE(item->text(), expectedText.at(i));
        QCOMPARE(item->data(Qt::UserRole), expectedData.at(i));
    }
}

void tst_QStandardItemModel::sortRoleBindings()
{
    QStandardItemModel model;
    QCOMPARE(model.sortRole(), Qt::DisplayRole);

    QProperty<int> sortRole;
    model.bindableSortRole().setBinding(Qt::makePropertyBinding(sortRole));
    sortRole = Qt::UserRole;
    QCOMPARE(model.sortRole(), Qt::UserRole);

    QProperty<int> sortRoleObserver;
    sortRoleObserver.setBinding([&] { return model.sortRole(); });
    model.setSortRole(Qt::EditRole);
    QCOMPARE(sortRoleObserver, Qt::EditRole);

    QTestPrivate::testReadWritePropertyBasics(model, static_cast<int>(Qt::DisplayRole),
                                              static_cast<int>(Qt::EditRole), "sortRole");
}

void tst_QStandardItemModel::findItems()
{
    QStandardItemModel model;
    model.appendRow(new QStandardItem(QLatin1String("foo")));
    model.appendRow(new QStandardItem(QLatin1String("bar")));
    model.item(1)->appendRow(new QStandardItem(QLatin1String("foo")));
    QList<QStandardItem*> matches;
    matches = model.findItems(QLatin1String("foo"), Qt::MatchExactly|Qt::MatchRecursive, 0);
    QCOMPARE(matches.size(), 2);
    matches = model.findItems(QLatin1String("foo"), Qt::MatchExactly, 0);
    QCOMPARE(matches.size(), 1);
    matches = model.findItems(QLatin1String("food"), Qt::MatchExactly|Qt::MatchRecursive, 0);
    QCOMPARE(matches.size(), 0);
    matches = model.findItems(QLatin1String("foo"), Qt::MatchExactly|Qt::MatchRecursive, -1);
    QCOMPARE(matches.size(), 0);
    matches = model.findItems(QLatin1String("foo"), Qt::MatchExactly|Qt::MatchRecursive, 1);
    QCOMPARE(matches.size(), 0);
}

void tst_QStandardItemModel::getSetHeaderItem()
{
    QStandardItemModel model;

    QCOMPARE(model.horizontalHeaderItem(0), nullptr);
    QStandardItem *hheader = new QStandardItem();
    model.setHorizontalHeaderItem(0, hheader);
    QCOMPARE(model.columnCount(), 1);
    QCOMPARE(model.horizontalHeaderItem(0), hheader);
    QCOMPARE(hheader->model(), &model);
    model.setHorizontalHeaderItem(0, nullptr);
    QCOMPARE(model.horizontalHeaderItem(0), nullptr);

    QCOMPARE(model.verticalHeaderItem(0), nullptr);
    QStandardItem *vheader = new QStandardItem();
    model.setVerticalHeaderItem(0, vheader);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.verticalHeaderItem(0), vheader);
    QCOMPARE(vheader->model(), &model);
    model.setVerticalHeaderItem(0, nullptr);
    QCOMPARE(model.verticalHeaderItem(0), nullptr);
}

void tst_QStandardItemModel::indexFromItem()
{
    QStandardItemModel model;

    QCOMPARE(model.indexFromItem(model.invisibleRootItem()), QModelIndex());

    QStandardItem *item = new QStandardItem;
    model.setItem(10, 20, item);
    QCOMPARE(item->model(), &model);
    QModelIndex itemIndex = model.indexFromItem(item);
    QVERIFY(itemIndex.isValid());
    QCOMPARE(itemIndex.row(), 10);
    QCOMPARE(itemIndex.column(), 20);
    QCOMPARE(itemIndex.parent(), QModelIndex());
    QCOMPARE(itemIndex.model(), &model);

    QStandardItem *child = new QStandardItem;
    item->setChild(4, 2, child);
    QModelIndex childIndex = model.indexFromItem(child);
    QVERIFY(childIndex.isValid());
    QCOMPARE(childIndex.row(), 4);
    QCOMPARE(childIndex.column(), 2);
    QCOMPARE(childIndex.parent(), itemIndex);

    QStandardItem *dummy = new QStandardItem;
    QModelIndex noSuchIndex = model.indexFromItem(dummy);
    QVERIFY(!noSuchIndex.isValid());
    delete dummy;

    noSuchIndex = model.indexFromItem(nullptr);
    QVERIFY(!noSuchIndex.isValid());
}

void tst_QStandardItemModel::itemFromIndex()
{
    QStandardItemModel model;

    QCOMPARE(model.itemFromIndex(QModelIndex()), nullptr);

    QStandardItem *item = new QStandardItem;
    model.setItem(10, 20, item);
    QModelIndex itemIndex = model.index(10, 20, QModelIndex());
    QVERIFY(itemIndex.isValid());
    QCOMPARE(model.itemFromIndex(itemIndex), item);

    QStandardItem *child = new QStandardItem;
    item->setChild(4, 2, child);
    QModelIndex childIndex = model.index(4, 2, itemIndex);
    QVERIFY(childIndex.isValid());
    QCOMPARE(model.itemFromIndex(childIndex), child);

    QModelIndex noSuchIndex = model.index(99, 99, itemIndex);
    QVERIFY(!noSuchIndex.isValid());
}

class CustomItem : public QStandardItem
{
public:
    using QStandardItem::QStandardItem;

    int type() const override { return UserType; }
    QStandardItem *clone() const override { return new CustomItem; }
};

void tst_QStandardItemModel::getSetItemPrototype()
{
    QStandardItemModel model;
    QCOMPARE(model.itemPrototype(), nullptr);

    const CustomItem *proto = new CustomItem;
    model.setItemPrototype(proto);
    QCOMPARE(model.itemPrototype(), proto);

    model.setRowCount(1);
    model.setColumnCount(1);
    QModelIndex index = model.index(0, 0, QModelIndex());
    model.setData(index, "foo");
    QStandardItem *item = model.itemFromIndex(index);
    QVERIFY(item != nullptr);
    QCOMPARE(item->type(), static_cast<int>(QStandardItem::UserType));

    model.setItemPrototype(nullptr);
    QCOMPARE(model.itemPrototype(), nullptr);
}

using RoleMap = QMap<int, QVariant>;
using RoleList = QList<int>;

static RoleMap getSetItemDataRoleMap(int textRole)
{
    return {{textRole, "text"_L1},
            {Qt::StatusTipRole, "statusTip"_L1},
            {Qt::ToolTipRole, "toolTip"_L1},
            {Qt::WhatsThisRole, "whatsThis"_L1},
            {Qt::SizeHintRole, QSize{64, 48}},
            {Qt::FontRole, QFont{}},
            {Qt::TextAlignmentRole, int(Qt::AlignLeft|Qt::AlignVCenter)},
            {Qt::BackgroundRole, QColor(Qt::blue)},
            {Qt::ForegroundRole, QColor(Qt::green)},
            {Qt::CheckStateRole, int(Qt::PartiallyChecked)},
            {Qt::AccessibleTextRole, "accessibleText"_L1},
            {Qt::AccessibleDescriptionRole, "accessibleDescription"_L1}};
}

void tst_QStandardItemModel::getSetItemData_data()
{
    QTest::addColumn<RoleMap>("itemData");
    QTest::addColumn<RoleMap>("expectedItemData");
    QTest::addColumn<RoleList>("expectedRoles");

    // QTBUG-112326: verify that text data set using Qt::EditRole is mapped to
    // Qt::DisplayRole and both roles are in the changed signal
    const RoleMap expectedItemData =  getSetItemDataRoleMap(Qt::DisplayRole);
    RoleList expectedRoles = expectedItemData.keys() << Qt::EditRole;
    std::sort(expectedRoles.begin(), expectedRoles.end());

    QTest::newRow("DisplayRole") << expectedItemData
                                 << expectedItemData << expectedRoles;

    QTest::newRow("EditRole") << getSetItemDataRoleMap(Qt::EditRole)
                              << expectedItemData << expectedRoles;
}

void tst_QStandardItemModel::getSetItemData()
{
    QFETCH(RoleMap, itemData);
    QFETCH(RoleMap, expectedItemData);
    QFETCH(RoleList, expectedRoles);

    QStandardItemModel model;
    model.insertRows(0, 1);
    model.insertColumns(0, 1);
    QModelIndex idx = model.index(0, 0, QModelIndex());

    QSignalSpy modelDataChangedSpy(
         &model, &QStandardItemModel::dataChanged);
    QVERIFY(model.setItemData(idx, itemData));
    QCOMPARE(modelDataChangedSpy.size(), 1);
    const QVariantList &args = modelDataChangedSpy.constFirst();
    QCOMPARE(args.size(), 3);
    auto roleList = args.at(2).value<QList<int> >();
    std::sort(roleList.begin(), roleList.end());
    QCOMPARE(roleList, expectedRoles);

    QVERIFY(model.setItemData(idx, itemData));
    QCOMPARE(modelDataChangedSpy.size(), 1); //it was already changed once
    QCOMPARE(model.itemData(idx), expectedItemData);
}

void tst_QStandardItemModel::setHeaderLabels_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<Qt::Orientation>("orientation");
    QTest::addColumn<QStringList>("labels");
    QTest::addColumn<QStringList>("expectedLabels");

    QTest::newRow("horizontal labels")
        << 1
        << 4
        << Qt::Horizontal
        << (QStringList() << "a" << "b" << "c" << "d")
        << (QStringList() << "a" << "b" << "c" << "d");
    QTest::newRow("vertical labels")
        << 4
        << 1
        << Qt::Vertical
        << (QStringList() << "a" << "b" << "c" << "d")
        << (QStringList() << "a" << "b" << "c" << "d");
    QTest::newRow("too few (horizontal)")
        << 1
        << 4
        << Qt::Horizontal
        << (QStringList() << "a" << "b")
        << (QStringList() << "a" << "b" << "3" << "4");
    QTest::newRow("too few (vertical)")
        << 4
        << 1
        << Qt::Vertical
        << (QStringList() << "a" << "b")
        << (QStringList() << "a" << "b" << "3" << "4");
    QTest::newRow("too many (horizontal)")
        << 1
        << 2
        << Qt::Horizontal
        << (QStringList() << "a" << "b" << "c" << "d")
        << (QStringList() << "a" << "b" << "c" << "d");
    QTest::newRow("too many (vertical)")
        << 2
        << 1
        << Qt::Vertical
        << (QStringList() << "a" << "b" << "c" << "d")
        << (QStringList() << "a" << "b" << "c" << "d");
}

void tst_QStandardItemModel::setHeaderLabels()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(Qt::Orientation, orientation);
    QFETCH(QStringList, labels);
    QFETCH(QStringList, expectedLabels);
    QStandardItemModel model(rows, columns);
    QSignalSpy columnsInsertedSpy(&model, &QAbstractItemModel::columnsInserted);
    QSignalSpy rowsInsertedSpy(&model, &QAbstractItemModel::rowsInserted);
    if (orientation == Qt::Horizontal)
        model.setHorizontalHeaderLabels(labels);
    else
        model.setVerticalHeaderLabels(labels);
    for (int i = 0; i < expectedLabels.size(); ++i)
        QCOMPARE(model.headerData(i, orientation).toString(), expectedLabels.at(i));
    QCOMPARE(columnsInsertedSpy.size(),
             (orientation == Qt::Vertical) ? 0 : labels.size() > columns);
    QCOMPARE(rowsInsertedSpy.size(),
             (orientation == Qt::Horizontal) ? 0 : labels.size() > rows);
}

void tst_QStandardItemModel::itemDataChanged()
{
    QStandardItemModel model(6, 4);
    QStandardItem item;
    QSignalSpy dataChangedSpy(&model, &QStandardItemModel::dataChanged);
    QSignalSpy itemChangedSpy(&model, &QStandardItemModel::itemChanged);

    model.setItem(0, &item);
    QCOMPARE(dataChangedSpy.size(), 1);
    QCOMPARE(itemChangedSpy.size(), 1);
    QModelIndex index = model.indexFromItem(&item);
    QList<QVariant> args;
    args = dataChangedSpy.takeFirst();
    QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), index);
    QCOMPARE(qvariant_cast<QModelIndex>(args.at(1)), index);
    args = itemChangedSpy.takeFirst();
    QCOMPARE(qvariant_cast<QStandardItem*>(args.at(0)), &item);

    item.setData(QLatin1String("foo"), Qt::DisplayRole);
    QCOMPARE(dataChangedSpy.size(), 1);
    QCOMPARE(itemChangedSpy.size(), 1);
    args = dataChangedSpy.takeFirst();
    QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), index);
    QCOMPARE(qvariant_cast<QModelIndex>(args.at(1)), index);
    args = itemChangedSpy.takeFirst();
    QCOMPARE(qvariant_cast<QStandardItem*>(args.at(0)), &item);

    item.setData(item.data(Qt::DisplayRole), Qt::DisplayRole);
    QCOMPARE(dataChangedSpy.size(), 0);
    QCOMPARE(itemChangedSpy.size(), 0);

    item.setFlags(Qt::ItemIsEnabled);
    QCOMPARE(dataChangedSpy.size(), 1);
    QCOMPARE(itemChangedSpy.size(), 1);
    args = dataChangedSpy.takeFirst();
    QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), index);
    QCOMPARE(qvariant_cast<QModelIndex>(args.at(1)), index);
    args = itemChangedSpy.takeFirst();
    QCOMPARE(qvariant_cast<QStandardItem*>(args.at(0)), &item);

    item.setFlags(item.flags());
    QCOMPARE(dataChangedSpy.size(), 0);
    QCOMPARE(itemChangedSpy.size(), 0);
}

void tst_QStandardItemModel::takeHeaderItem()
{
    QStandardItemModel model;
    // set header items
    QScopedPointer<QStandardItem> hheader(new QStandardItem());
    model.setHorizontalHeaderItem(0, hheader.get());
    QScopedPointer<QStandardItem> vheader(new QStandardItem());
    model.setVerticalHeaderItem(0, vheader.get());
    // take header items
    QCOMPARE(model.takeHorizontalHeaderItem(0), hheader.get());
    QCOMPARE(model.takeVerticalHeaderItem(0), vheader.get());
    QCOMPARE(hheader->model(), nullptr);
    QCOMPARE(vheader->model(), nullptr);
    QCOMPARE(model.takeHorizontalHeaderItem(0), nullptr);
    QCOMPARE(model.takeVerticalHeaderItem(0), nullptr);
}

void tst_QStandardItemModel::useCase1()
{
    const int rows = 5;
    const int columns = 8;
    QStandardItemModel model(rows, columns);
    for (int i = 0; i < model.rowCount(); ++i) {
        for (int j = 0; j < model.columnCount(); ++j) {
            QCOMPARE(model.item(i, j), nullptr);

            QStandardItem *item = new QStandardItem();
            model.setItem(i, j, item);
            QCOMPARE(item->row(), i);
            QCOMPARE(item->column(), j);
            QCOMPARE(item->model(), &model);

            QModelIndex index = model.indexFromItem(item);
            QCOMPARE(index, model.index(i, j, QModelIndex()));
            QStandardItem *sameItem = model.itemFromIndex(index);
            QCOMPARE(sameItem, item);
        }
    }
}

static void createChildren(QStandardItemModel *model, QStandardItem *parent, int level)
{
    if (level > 4)
        return;
    for (int i = 0; i < 4; ++i) {
        QCOMPARE(parent->rowCount(), i);
        parent->appendRow(QList<QStandardItem*>());
        for (int j = 0; j < parent->columnCount(); ++j) {
            QStandardItem *item = new QStandardItem();
            parent->setChild(i, j, item);
            QCOMPARE(item->row(), i);
            QCOMPARE(item->column(), j);

            QModelIndex parentIndex = model->indexFromItem(parent);
            QModelIndex index = model->indexFromItem(item);
            QCOMPARE(index, model->index(i, j, parentIndex));
            QStandardItem *theItem = model->itemFromIndex(index);
            QCOMPARE(theItem, item);
            QStandardItem *theParent = model->itemFromIndex(parentIndex);
            QCOMPARE(theParent, (level == 0) ? static_cast<QStandardItem *>(nullptr) : parent);
        }

        {
            QStandardItem *item = parent->child(i);
            item->setColumnCount(parent->columnCount());
            createChildren(model, item, level + 1);
        }
    }
}

void tst_QStandardItemModel::useCase2()
{
    QStandardItemModel model;
    model.setColumnCount(2);
    createChildren(&model, model.invisibleRootItem(), 0);
}

void tst_QStandardItemModel::useCase3()
{
    // create the tree structure first
    QStandardItem *childItem = nullptr;
    for (int i = 0; i < 100; ++i) {
        QStandardItem *item = new QStandardItem(QStringLiteral("item ") + QString::number(i));
        if (childItem)
            item->appendRow(childItem);
        childItem = item;
    }

    // add to model as last step
    QStandardItemModel model;
    model.appendRow(childItem);

    // make sure each item has the correct model and parent
    QStandardItem *parentItem = nullptr;
    while (childItem) {
        QCOMPARE(childItem->model(), &model);
        QCOMPARE(childItem->parent(), parentItem);
        parentItem = childItem;
        childItem = childItem->child(0);
    }

    // take the item, make sure model is set to 0, but that parents are the same
    childItem = model.takeItem(0);
    {
        parentItem = nullptr;
        QStandardItem *item = childItem;
        while (item) {
            QCOMPARE(item->model(), nullptr);
            QCOMPARE(item->parent(), parentItem);
            parentItem = item;
            item = item->child(0);
        }
    }
    delete childItem;
}

void tst_QStandardItemModel::setNullChild()
{
    QStandardItemModel model;
    model.setColumnCount(2);
    createChildren(&model, model.invisibleRootItem(), 0);
    QStandardItem *item = model.item(0);
    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    item->setChild(0, nullptr);
    QCOMPARE(item->child(0), nullptr);
    QCOMPARE(spy.size(), 1);
}

void tst_QStandardItemModel::deleteChild()
{
    QStandardItemModel model;
    model.setColumnCount(2);
    createChildren(&model, model.invisibleRootItem(), 0);
    QStandardItem *item = model.item(0);
    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    delete item->child(0);
    QCOMPARE(item->child(0), nullptr);
    QCOMPARE(spy.size(), 1);
}

void tst_QStandardItemModel::rootItemFlags()
{
    QStandardItemModel model(6, 4);
    QCOMPARE(model.invisibleRootItem()->flags() , model.flags(QModelIndex()));
    QCOMPARE(model.invisibleRootItem()->flags() , Qt::ItemIsDropEnabled);

    Qt::ItemFlags f = Qt::ItemIsDropEnabled | Qt::ItemIsEnabled;
    model.invisibleRootItem()->setFlags(f);
    QCOMPARE(model.invisibleRootItem()->flags() , f);
    QCOMPARE(model.invisibleRootItem()->flags() , model.flags(QModelIndex()));

#if QT_CONFIG(draganddrop)
    model.invisibleRootItem()->setDropEnabled(false);
#endif
    QCOMPARE(model.invisibleRootItem()->flags() , Qt::ItemIsEnabled);
    QCOMPARE(model.invisibleRootItem()->flags() , model.flags(QModelIndex()));
}

bool tst_QStandardItemModel::compareModels(QStandardItemModel *model1, QStandardItemModel *model2)
{
    return compareItems(model1->invisibleRootItem(), model2->invisibleRootItem());
}

bool tst_QStandardItemModel::compareItems(QStandardItem *item1, QStandardItem *item2)
{
    if (!item1 && !item2)
        return true;
    if (!item1 || !item2)
        return false;
    if (item1->text() != item2->text()) {
        qDebug() << item1->text() << item2->text();
        return false;
    }
    if (item1->rowCount() != item2->rowCount()) {
  //      qDebug() << "RowCount" << item1->text() << item1->rowCount() << item2->rowCount();
        return false;
    }
    if (item1->columnCount() != item2->columnCount()) {
  //     qDebug() << "ColumnCount" << item1->text() << item1->columnCount() << item2->columnCount();
        return false;
    }
    for (int row = 0; row < item1->columnCount(); row++) {
        for (int col = 0; col < item1->columnCount(); col++) {
            if (!compareItems(item1->child(row, col), item2->child(row, col)))
                return false;
        }
    }
    return true;
}

static QStandardItem *itemFromText(QStandardItem *parent, const QString &text)
{
    QStandardItem *item = nullptr;
    for (int i = 0; i < parent->columnCount(); i++) {
        for (int j = 0; j < parent->rowCount(); j++) {
            QStandardItem *child = parent->child(j, i);
            if (!child)
                continue;

            if (child->text() == text) {
                if (item)
                    return nullptr;
                item = child;
            }

            QStandardItem *candidate = itemFromText(child, text);
            if (candidate) {
                if (item)
                    return nullptr;
                item = candidate;
            }
        }
    }
    return item;
}

#ifdef QT_BUILD_INTERNAL
static QModelIndex indexFromText(QStandardItemModel *model, const QString &text)
{
    QStandardItem *item = itemFromText(model->invisibleRootItem(), text);
    /*QVERIFY(item);*/
    return model->indexFromItem(item);
}


struct FriendlyTreeView : public QTreeView
{
    friend class tst_QStandardItemModel;
    Q_DECLARE_PRIVATE(QTreeView)
};
#endif

#ifdef QT_BUILD_INTERNAL

static void populateDragAndDropModel(QStandardItemModel &model, int nRow, int nCol)
{
    const QString item = QStringLiteral("item ");
    const QString dash = QStringLiteral(" - ");
    for (int i = 0; i < nRow; ++i) {
        const QString iS = QString::number(i);
        QList<QStandardItem *> colItems1;
        for (int c = 0 ; c < nCol; c ++)
            colItems1 << new QStandardItem(item + iS + dash + QString::number(c));
        model.appendRow(colItems1);

        for (int j = 0; j < nRow; ++j) {
            const QString jS = QString::number(j);
            QList<QStandardItem *> colItems2;
            for (int c = 0 ; c < nCol; c ++)
                colItems2 << new QStandardItem(item + iS + QLatin1Char('/') + jS + dash + QString::number(c));
            colItems1.at(0)->appendRow(colItems2);

            for (int k = 0; k < nRow; ++k) {
                QList<QStandardItem *> colItems3;
                const QString kS = QString::number(k);
                for (int c = 0 ; c < nCol; c ++)
                    colItems3 << new QStandardItem(item + iS + QLatin1Char('/') + jS
                                                   + QLatin1Char('/') + kS
                                                   + dash + QString::number(c));
                colItems2.at(0)->appendRow(colItems3);
            }
        }
    }
}

void tst_QStandardItemModel::treeDragAndDrop()
{
    const int nRow = 5;
    const int nCol = 3;

    QStandardItemModel model;
    QStandardItemModel checkModel;

    populateDragAndDropModel(model, nRow, nCol);
    populateDragAndDropModel(checkModel, nRow, nCol);

    QVERIFY(compareModels(&model, &checkModel));

    FriendlyTreeView  view;
    view.setModel(&model);
    view.expandAll();
    view.show();
#if QT_CONFIG(draganddrop)
    view.setDragDropMode(QAbstractItemView::InternalMove);
#endif
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);

    QItemSelectionModel *selection = view.selectionModel();

    //
    // step1  drag  "item 1" and "item 2"   into "item 4"
    //
    {
        selection->clear();
        selection->select(QItemSelection(indexFromText(&model, QString("item 1 - 0")),
                                        indexFromText(&model, QString("item 1 - %0").arg(nCol-1))), QItemSelectionModel::Select);

        selection->select(QItemSelection(indexFromText(&model, QString("item 2 - 0")),
                                        indexFromText(&model, QString("item 2 - %0").arg(nCol-1))), QItemSelectionModel::Select);

        //code based from QAbstractItemView::startDrag and QAbstractItemView::dropEvent
        QModelIndexList indexes = view.selectedIndexes();
        QMimeData *data = model.mimeData(indexes);
        if(model.dropMimeData(data, Qt::MoveAction, 0, 0, indexFromText(&model, "item 4 - 0")))
            view.d_func()->clearOrRemove();
        delete data;

        QVERIFY(!compareModels(&model, &checkModel)); //the model must be different at this point
        QStandardItem *item4 = itemFromText(checkModel.invisibleRootItem(), "item 4 - 0");
        item4->insertRow(0, checkModel.takeRow(1));
        item4->insertRow(1, checkModel.takeRow(1));
        QVERIFY(compareModels(&model, &checkModel));
    }

    //
    // step2  drag  "item 3" and "item 3/0"   into "item 4"
    //
    {
        selection->clear();
        selection->select(QItemSelection(indexFromText(&model, QString("item 3 - 0")),
                                          indexFromText(&model, QString("item 3 - %0").arg(nCol-1))), QItemSelectionModel::Select);

        selection->select(QItemSelection(indexFromText(&model, QString("item 3/0 - 0")),
                                        indexFromText(&model, QString("item 3/0 - %0").arg(nCol-1))), QItemSelectionModel::Select);

        //code based from QAbstractItemView::startDrag and QAbstractItemView::dropEvent
        QModelIndexList indexes = view.selectedIndexes();
        QMimeData *data = model.mimeData(indexes);
        if(model.dropMimeData(data, Qt::MoveAction, 0, 0, indexFromText(&model, "item 4 - 0")))
        view.d_func()->clearOrRemove();
        delete data;

        QVERIFY(!compareModels(&model, &checkModel)); //the model must be different at this point
        QStandardItem *item4 = itemFromText(checkModel.invisibleRootItem(), "item 4 - 0");
        item4->insertRow(0, checkModel.takeRow(1));

        QVERIFY(compareModels(&model, &checkModel));
    }

    //
    // step2  drag  "item 3" and "item 3/0/2"   into "item 0/2"
    // ( remember "item 3" is now the first child of "item 4")
    //
    {
        selection->clear();
        selection->select(QItemSelection(indexFromText(&model, QString("item 3 - 0")),
                                         indexFromText(&model, QString("item 3 - %0").arg(nCol-1))), QItemSelectionModel::Select);

        selection->select(QItemSelection(indexFromText(&model, QString("item 3/0/2 - 0")),
                                        indexFromText(&model, QString("item 3/0/2 - %0").arg(nCol-1))), QItemSelectionModel::Select);

        //code based from QAbstractItemView::startDrag and QAbstractItemView::dropEvent
        QModelIndexList indexes = view.selectedIndexes();
        QMimeData *data = model.mimeData(indexes);
        if(model.dropMimeData(data, Qt::MoveAction, 0, 0, indexFromText(&model, "item 0/2 - 0")))
        view.d_func()->clearOrRemove();
        delete data;

        QVERIFY(!compareModels(&model, &checkModel)); //the model must be different at this point
        QStandardItem *item02 = itemFromText(checkModel.invisibleRootItem(), "item 0/2 - 0");
        QStandardItem *item4 = itemFromText(checkModel.invisibleRootItem(), "item 4 - 0");
        item02->insertRow(0, item4->takeRow(0));

        QVERIFY(compareModels(&model, &checkModel));
    }
}
#endif

void tst_QStandardItemModel::removeRowsAndColumns()
{
#define VERIFY_MODEL \
    for (int c = 0; c < col_list.count(); c++) \
        for (int r = 0; r < row_list.count(); r++) \
            QCOMPARE(model.item(r,c)->text() , row_list[r] + QLatin1Char('x') + col_list[c]);

    QStringList row_list = QString("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20").split(',');
    QStringList col_list = row_list;
    QStandardItemModel model;
    for (int c = 0; c < col_list.size(); c++)
        for (int r = 0; r < row_list.size(); r++)
            model.setItem(r, c, new QStandardItem(row_list[r] + QLatin1Char('x') + col_list[c]));
    VERIFY_MODEL

    row_list.remove(3);
    model.removeRow(3);
    VERIFY_MODEL

    col_list.remove(5);
    model.removeColumn(5);
    VERIFY_MODEL

    row_list.remove(2, 5);
    model.removeRows(2, 5);
    VERIFY_MODEL

    col_list.remove(1, 6);
    model.removeColumns(1, 6);
    VERIFY_MODEL

    QList<QStandardItem *> row_taken = model.takeRow(6);
    QCOMPARE(row_taken.size(), col_list.size());
    for (qsizetype c = 0; c < row_taken.size(); c++) {
        auto item = row_taken.at(c);
        QCOMPARE(item->text() , row_list[6] + QLatin1Char('x') + col_list[c]);
        delete item;
    }
    row_list.remove(6);
    VERIFY_MODEL

    QList<QStandardItem *> col_taken = model.takeColumn(10);
    QCOMPARE(col_taken.size(), row_list.size());
    for (qsizetype r = 0; r < col_taken.size(); r++) {
        auto item = col_taken.at(r);
        QCOMPARE(item->text() , row_list[r] + QLatin1Char('x') + col_list[10]);
        delete item;
    }
    col_list.remove(10);
    VERIFY_MODEL
}

void tst_QStandardItemModel::defaultItemRoles()
{
    QStandardItemModel model;
    QCOMPARE(model.roleNames(), QAbstractItemModelPrivate::defaultRoleNames());
}

void tst_QStandardItemModel::itemRoleNames()
{
    QStringList row_list = QString("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20").split(',');
    QStringList col_list = row_list;
    QStandardItemModel model;
    for (int c = 0; c < col_list.size(); c++)
        for (int r = 0; r < row_list.size(); r++)
            model.setItem(r, c, new QStandardItem(row_list[r] + QLatin1Char('x') + col_list[c]));
    VERIFY_MODEL

    QHash<int, QByteArray> newRoleNames;
    newRoleNames.insert(Qt::DisplayRole, "Name");
    newRoleNames.insert(Qt::DecorationRole, "Avatar");
    model.setItemRoleNames(newRoleNames);
    QCOMPARE(model.roleNames(), newRoleNames);
    VERIFY_MODEL
}

void tst_QStandardItemModel::getMimeDataWithInvalidModelIndex()
{
    QStandardItemModel model;
    QTest::ignoreMessage(QtWarningMsg, "QStandardItemModel::mimeData: No item associated with invalid index");
    QMimeData *data = model.mimeData(QModelIndexList() << QModelIndex());
    QVERIFY(!data);
}

void tst_QStandardItemModel::supportedDragDropActions()
{
    QStandardItemModel model;
    QCOMPARE(model.supportedDragActions(), Qt::CopyAction | Qt::MoveAction);
    QCOMPARE(model.supportedDropActions(), Qt::CopyAction | Qt::MoveAction);
}

void tst_QStandardItemModel::taskQTBUG_45114_setItemData()
{
    QStandardItemModel model;
    QSignalSpy spy(&model, &QStandardItemModel::itemChanged);

    QStandardItem *item = new QStandardItem("item");
    item->setData(1, Qt::UserRole + 1);
    item->setData(2, Qt::UserRole + 2);
    model.appendRow(item);

    QModelIndex index = item->index();
    QCOMPARE(model.itemData(index).size(), 3);

    QCOMPARE(spy.size(), 0);

    QMap<int, QVariant> roles;

    roles.insert(Qt::UserRole + 1, 1);
    roles.insert(Qt::UserRole + 2, 2);
    model.setItemData(index, roles);

    QCOMPARE(spy.size(), 0);

    roles.insert(Qt::UserRole + 1, 1);
    roles.insert(Qt::UserRole + 2, 2);
    roles.insert(Qt::UserRole + 3, QVariant());
    model.setItemData(index, roles);

    QCOMPARE(spy.size(), 0);

    roles.clear();
    roles.insert(Qt::UserRole + 1, 10);
    roles.insert(Qt::UserRole + 3, 12);
    model.setItemData(index, roles);

    QCOMPARE(spy.size(), 1);
    QMap<int, QVariant> itemRoles = model.itemData(index);

    QCOMPARE(itemRoles.size(), 4);
    QCOMPARE(itemRoles[Qt::UserRole + 1].toInt(), 10);
    QCOMPARE(itemRoles[Qt::UserRole + 2].toInt(), 2);
    QCOMPARE(itemRoles[Qt::UserRole + 3].toInt(), 12);

    roles.clear();
    roles.insert(Qt::UserRole + 3, 1);
    model.setItemData(index, roles);

    QCOMPARE(spy.size(), 2);

    roles.clear();
    roles.insert(Qt::UserRole + 3, QVariant());
    model.setItemData(index, roles);

    QCOMPARE(spy.size(), 3);

    itemRoles = model.itemData(index);
    QCOMPARE(itemRoles.size(), 3);
    QVERIFY(!itemRoles.keys().contains(Qt::UserRole + 3));
}

void tst_QStandardItemModel::setItemPersistentIndex()
{
    QPersistentModelIndex persistentIndex;
    // setItem on an already existing item should not destroy the persistent index
    QStandardItemModel m;
    persistentIndex = m.index(0, 0);
    QVERIFY(!persistentIndex.isValid());

    m.setItem(0, 0, new QStandardItem);
    persistentIndex = m.index(0, 0);
    QVERIFY(persistentIndex.isValid());
    QCOMPARE(persistentIndex.row(), 0);
    QCOMPARE(persistentIndex.column(), 0);

    m.setItem(0, 0, new QStandardItem);
    QVERIFY(persistentIndex.isValid());

    m.setItem(0, 0, nullptr);
    QVERIFY(!persistentIndex.isValid());
}

void tst_QStandardItemModel::signalsOnTakeItem() // QTBUG-89145
{
    QStandardItemModel m;
    m.insertColumns(0, 2);
    m.insertRows(0, 5);
    for (int i = 0; i < m.rowCount(); ++i) {
        for (int j = 0; j < m.columnCount(); ++j)
            m.setData(m.index(i, j), i + j);
    }
    const QModelIndex parentIndex = m.index(1, 0);
    m.insertColumns(0, 2, parentIndex);
    m.insertRows(0, 2, parentIndex);
    for (int i = 0; i < m.rowCount(parentIndex); ++i) {
        for (int j = 0; j < m.columnCount(parentIndex); ++j)
            m.setData(m.index(i, j, parentIndex), i + j + 100);
    }
    QAbstractItemModelTester mTester(&m, nullptr);
    QSignalSpy columnsRemovedSpy(&m, &QAbstractItemModel::columnsRemoved);
    QSignalSpy columnsAboutToBeRemovedSpy(&m, &QAbstractItemModel::columnsAboutToBeRemoved);
    QSignalSpy rowsRemovedSpy(&m, &QAbstractItemModel::rowsRemoved);
    QSignalSpy rowsAboutToBeRemovedSpy(&m, &QAbstractItemModel::rowsAboutToBeRemoved);
    QSignalSpy *const removeSpies[] = {&columnsRemovedSpy, &columnsAboutToBeRemovedSpy, &rowsRemovedSpy, &rowsAboutToBeRemovedSpy};
    QSignalSpy dataChangedSpy(&m, &QAbstractItemModel::dataChanged);
    QStandardItem *const takenItem = m.takeItem(1, 0);
    for (auto &&spy : removeSpies) {
        QCOMPARE(spy->size(), 1);
        const auto spyArgs = spy->takeFirst();
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), parentIndex);
        QCOMPARE(spyArgs.at(1).toInt(), 0);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    QCOMPARE(dataChangedSpy.size(), 1);
    const auto dataChangedSpyArgs = dataChangedSpy.takeFirst();
    QCOMPARE(dataChangedSpyArgs.at(0).value<QModelIndex>(), m.index(1, 0));
    QCOMPARE(dataChangedSpyArgs.at(1).value<QModelIndex>(), m.index(1, 0));
    QCOMPARE(takenItem->data(Qt::EditRole).toInt(), 1);
    QCOMPARE(takenItem->rowCount(), 2);
    QCOMPARE(takenItem->columnCount(), 2);
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j)
            QCOMPARE(takenItem->child(i, j)->data(Qt::EditRole).toInt(), i + j + 100);
    }
    QCOMPARE(takenItem->model(), nullptr);
    QCOMPARE(takenItem->child(0, 0)->model(), nullptr);
    QCOMPARE(m.index(1, 0).data(), QVariant());
    delete takenItem;
}

void tst_QStandardItemModel::createPersistentOnLayoutAboutToBeChanged() // QTBUG-93466
{
    QStandardItemModel model;
    QAbstractItemModelTester mTester(&model, nullptr);
    model.insertColumn(0);
    QCOMPARE(model.columnCount(), 1);
    model.insertRows(0, 3);
    for (int row = 0; row < 3; ++row)
        model.setData(model.index(row, 0), row);
    QList<QPersistentModelIndex> idxList;
    QSignalSpy layoutAboutToBeChangedSpy(&model, &QAbstractItemModel::layoutAboutToBeChanged);
    QSignalSpy layoutChangedSpy(&model, &QAbstractItemModel::layoutChanged);
    connect(&model, &QAbstractItemModel::layoutAboutToBeChanged, this, [&idxList, &model](){
        idxList.clear();
        for (int row = 0; row < 3; ++row)
            idxList << QPersistentModelIndex(model.index(row, 0));
    });
    connect(&model, &QAbstractItemModel::layoutChanged, this, [&idxList](){
        QCOMPARE(idxList.size(), 3);
        QCOMPARE(idxList.at(0).row(), 1);
        QCOMPARE(idxList.at(0).column(), 0);
        QCOMPARE(idxList.at(0).data().toInt(), 0);
        QCOMPARE(idxList.at(1).row(), 0);
        QCOMPARE(idxList.at(1).column(), 0);
        QCOMPARE(idxList.at(1).data().toInt(), -1);
        QCOMPARE(idxList.at(2).row(), 2);
        QCOMPARE(idxList.at(2).column(), 0);
        QCOMPARE(idxList.at(2).data().toInt(), 2);
    });
    model.setData(model.index(1, 0), -1);
    model.sort(0);
    QCOMPARE(layoutAboutToBeChangedSpy.size(), 1);
    QCOMPARE(layoutChangedSpy.size(), 1);
}

void tst_QStandardItemModel::takeChild()  // QTBUG-117900
{
  {
      // with model
      QStandardItemModel model1;
      QStandardItemModel model2;
      QStandardItem base1("base1");
      model1.setItem(0, 0, &base1);
      QStandardItem base2("base2");
      model2.setItem(0, 0, &base2);
      auto item = new QStandardItem("item1");
      item->appendRow({new QStandardItem("child")});
      base1.appendRow({item});
      base2.appendRow({base1.takeChild(0, 0)});
      QCOMPARE(base1.child(0, 0), nullptr);
      QCOMPARE(base2.child(0, 0), item);
  }
  {
      // without model
      QStandardItem base1("base1");
      QStandardItem base2("base2");
      auto item = new QStandardItem("item1");
      item->appendRow({new QStandardItem("child")});
      base1.appendRow({item});
      base2.appendRow({base1.takeChild(0, 0)});
      QCOMPARE(base1.child(0, 0), nullptr);
      QCOMPARE(base2.child(0, 0), item);
  }
}


QTEST_MAIN(tst_QStandardItemModel)
#include "tst_qstandarditemmodel.moc"
