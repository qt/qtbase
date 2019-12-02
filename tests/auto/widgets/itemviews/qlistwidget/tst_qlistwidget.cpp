/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QCompleter>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QSignalSpy>
#include <QStyledItemDelegate>
#include <QTest>
#include <private/qlistwidget_p.h>

using IntList = QVector<int>;

class tst_QListWidget : public QObject
{
    Q_OBJECT

public:
    tst_QListWidget() = default;

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

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void addItem();
    void addItem2();
    void addItems();
    void openPersistentEditor();
    void closePersistentEditor();
    void count();
    void currentItem();
    void setCurrentItem_data();
    void setCurrentItem();
    void currentRow();
    void setCurrentRow_data();
    void setCurrentRow();
    void editItem_data();
    void editItem();
    void findItems();
    void insertItem_data();
    void insertItem();
    void insertItems_data();
    void insertItems();
    void moveItemsPriv_data();
    void moveItemsPriv();

    void itemAssignment();
    void item_data();
    void item();
    void takeItem_data();
    void takeItem();
    void setItemHidden();
    void selectedItems_data();
    void selectedItems();
    void removeItems_data();
    void removeItems();
    void itemStreaming_data();
    void itemStreaming();
    void sortItems_data();
    void sortItems();
    void sortHiddenItems();
    void sortHiddenItems_data();
    void closeEditor();
    void setData_data();
    void setData();
    void insertItemsWithSorting_data();
    void insertItemsWithSorting();
    void changeDataWithSorting_data();
    void changeDataWithSorting();
    void itemData();
    void itemWidget();
#ifndef Q_OS_MAC
    void fastScroll();
#endif
    void insertUnchanged();
    void setSortingEnabled();
    void task199503_crashWhenCleared();
    void task217070_scrollbarsAdjusted();
    void task258949_keypressHangup();
    void QTBUG8086_currentItemChangedOnClick();
    void QTBUG14363_completerWithAnyKeyPressedEditTriggers();
    void mimeData();
    void QTBUG50891_ensureSelectionModelSignalConnectionsAreSet();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void clearItemData();
#endif

    void moveRows_data();
    void moveRows();
    void moveRowsInvalid_data();
    void moveRowsInvalid();

protected slots:
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

    void modelChanged(ModelChanged change, const QModelIndex &parent, int first, int last)
    {
        rcParent[change] = parent;
        rcFirst[change] = first;
        rcLast[change] = last;
    }

private:
    QListWidget *testWidget = nullptr;
    QVector<QModelIndex> rcParent{8};
    QVector<int> rcFirst = QVector<int>(8, 0);
    QVector<int> rcLast = QVector<int>(8, 0);

    void populate();
    void checkDefaultValues();
};


void tst_QListWidget::moveRowsInvalid_data()
{
    QTest::addColumn<QListWidget*>("baseWidget");
    QTest::addColumn<QModelIndex>("startParent");
    QTest::addColumn<int>("startRow");
    QTest::addColumn<int>("count");
    QTest::addColumn<QModelIndex>("destinationParent");
    QTest::addColumn<int>("destination");

    const auto createWidget = []() -> QListWidget* {
        QListWidget* result = new QListWidget;
        result->addItems({"A", "B", "C", "D", "E", "F"});
        return result;
    };
    constexpr int rowCount = 6;

    QTest::addRow("destination_equal_source") << createWidget() << QModelIndex() << 0 << 1 << QModelIndex() << 0;
    QTest::addRow("count_equal_0") << createWidget() << QModelIndex() << 0 << 0 << QModelIndex() << 2;
    QListWidget* tempWidget = createWidget();
    QTest::addRow("move_child") << tempWidget << tempWidget->model()->index(0, 0) << 0 << 1 << QModelIndex() << 2;
    tempWidget = createWidget();
    QTest::addRow("move_to_child") << tempWidget << QModelIndex() << 0 << 1 << tempWidget->model()->index(0, 0) << 2;
    QTest::addRow("negative_count") << createWidget() << QModelIndex() << 0 << -1 << QModelIndex() << 2;
    QTest::addRow("negative_source_row") << createWidget() << QModelIndex() << -1 << 1 << QModelIndex() << 2;
    QTest::addRow("negative_destination_row") << createWidget() << QModelIndex() << 0 << 1 << QModelIndex() << -1;
    QTest::addRow("source_row_equal_rowCount") << createWidget() << QModelIndex() << rowCount << 1 << QModelIndex() << 1;
    QTest::addRow("source_row_equal_destination_row") << createWidget() << QModelIndex() << 2 << 1 << QModelIndex() << 2;
    QTest::addRow("source_row_equal_destination_row_plus1") << createWidget() << QModelIndex() << 2 << 1 << QModelIndex() << 3;
    QTest::addRow("destination_row_greater_rowCount") << createWidget() << QModelIndex() << 0 << 1 << QModelIndex() << rowCount + 1;
    QTest::addRow("move_row_within_source_range") << createWidget() << QModelIndex() << 0 << 3 << QModelIndex() << 2;
}

void tst_QListWidget::moveRowsInvalid()
{
    QFETCH(QListWidget* const, baseWidget);
    QFETCH(const QModelIndex, startParent);
    QFETCH(const int, startRow);
    QFETCH(const int, count);
    QFETCH(const QModelIndex, destinationParent);
    QFETCH(const int, destination);
    QAbstractItemModel *baseModel = baseWidget->model();
    QSignalSpy rowMovedSpy(baseModel, &QAbstractItemModel::rowsMoved);
    QSignalSpy rowAboutMovedSpy(baseModel, &QAbstractItemModel::rowsAboutToBeMoved);
    QVERIFY(rowMovedSpy.isValid());
    QVERIFY(rowAboutMovedSpy.isValid());
    QVERIFY(!baseModel->moveRows(startParent, startRow, count, destinationParent, destination));
    QCOMPARE(rowMovedSpy.size(), 0);
    QCOMPARE(rowAboutMovedSpy.size(), 0);
    delete baseWidget;
}

void tst_QListWidget::moveRows_data()
{
    QTest::addColumn<int>("startRow");
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("destination");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("1_Item_from_top_to_middle") << 0 << 1 << 3 << QStringList{"B", "C", "A", "D", "E", "F"};
    QTest::newRow("1_Item_from_top_to_bottom") << 0 << 1 << 6 << QStringList{"B", "C", "D", "E", "F", "A"};
    QTest::newRow("1_Item_from_middle_to_top") << 2 << 1 << 0 << QStringList{"C", "A", "B", "D", "E", "F"};
    QTest::newRow("1_Item_from_bottom_to_middle") << 5 << 1 << 2 << QStringList{"A", "B", "F", "C", "D", "E"};
    QTest::newRow("1_Item_from_bottom to_top") << 5 << 1 << 0 << QStringList{"F", "A", "B", "C", "D", "E"};
    QTest::newRow("1_Item_from_middle_to_bottom") << 2 << 1 << 6 << QStringList{"A", "B", "D", "E", "F", "C"};
    QTest::newRow("1_Item_from_middle_to_middle_before") << 2 << 1 << 1 << QStringList{"A", "C", "B", "D", "E", "F"};
    QTest::newRow("1_Item_from_middle_to_middle_after") << 2 << 1 << 4 << QStringList{"A", "B", "D", "C", "E", "F"};

    QTest::newRow("2_Items_from_top_to_middle") << 0 << 2 << 3 << QStringList{"C", "A", "B", "D", "E", "F"};
    QTest::newRow("2_Items_from_top_to_bottom") << 0 << 2 << 6 << QStringList{"C", "D", "E", "F", "A", "B"};
    QTest::newRow("2_Items_from_middle_to_top") << 2 << 2 << 0 << QStringList{"C", "D", "A", "B", "E", "F"};
    QTest::newRow("2_Items_from_bottom_to_middle") << 4 << 2 << 2 << QStringList{"A", "B", "E", "F", "C", "D"};
    QTest::newRow("2_Items_from_bottom_to_top") << 4 << 2 << 0 << QStringList{"E", "F", "A", "B", "C", "D"};
    QTest::newRow("2_Items_from_middle_to_bottom") << 2 << 2 << 6 << QStringList{"A", "B", "E", "F", "C", "D"};
    QTest::newRow("2_Items_from_middle_to_middle_before") << 3 << 2 << 1 << QStringList{"A", "D", "E", "B", "C", "F"};
    QTest::newRow("2_Items_from_middle_to_middle_after") << 1 << 2 << 5 << QStringList{"A", "D", "E", "B", "C", "F"};
}

void tst_QListWidget::moveRows()
{
    QFETCH(const int, startRow);
    QFETCH(const int, count);
    QFETCH(const int, destination);
    QFETCH(const QStringList, expected);
    QListWidget baseWidget;
    baseWidget.addItems(QStringList{"A", "B", "C", "D", "E", "F"});
    QAbstractItemModel *baseModel = baseWidget.model();
    QSignalSpy rowMovedSpy(baseModel, &QAbstractItemModel::rowsMoved);
    QSignalSpy rowAboutMovedSpy(baseModel, &QAbstractItemModel::rowsAboutToBeMoved);
    QVERIFY(baseModel->moveRows(QModelIndex(), startRow, count, QModelIndex(), destination));
    QCOMPARE(baseModel->rowCount(), expected.size());
    for (int i = 0; i < expected.size(); ++i)
        QCOMPARE(baseModel->index(i, 0).data().toString(), expected.at(i));
    QCOMPARE(rowMovedSpy.size(), 1);
    QCOMPARE(rowAboutMovedSpy.size(), 1);
    for (const QList<QVariant> &signalArgs : {rowMovedSpy.first(), rowAboutMovedSpy.first()}){
        QVERIFY(!signalArgs.at(0).value<QModelIndex>().isValid());
        QCOMPARE(signalArgs.at(1).toInt(), startRow);
        QCOMPARE(signalArgs.at(2).toInt(), startRow + count - 1);
        QVERIFY(!signalArgs.at(3).value<QModelIndex>().isValid());
        QCOMPARE(signalArgs.at(4).toInt(), destination);
    }
}


void tst_QListWidget::initTestCase()
{
    qRegisterMetaType<QListWidgetItem*>("QListWidgetItem*");
    qRegisterMetaType<QList<QPersistentModelIndex>>("QList<QPersistentModelIndex>");
    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint");

    testWidget = new QListWidget;
    testWidget->show();

    connect(testWidget->model(), &QAbstractItemModel::rowsAboutToBeInserted,
            this, &tst_QListWidget::rowsAboutToBeInserted);
    connect(testWidget->model(), &QAbstractItemModel::rowsInserted,
            this, &tst_QListWidget::rowsInserted);
    connect(testWidget->model(), &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &tst_QListWidget::rowsAboutToBeRemoved);
    connect(testWidget->model(), &QAbstractItemModel::rowsRemoved,
            this, &tst_QListWidget::rowsRemoved);

    connect(testWidget->model(), &QAbstractItemModel::columnsAboutToBeInserted,
            this, &tst_QListWidget::columnsAboutToBeInserted);
    connect(testWidget->model(), &QAbstractItemModel::columnsInserted,
            this, &tst_QListWidget::columnsInserted);
    connect(testWidget->model(), &QAbstractItemModel::columnsAboutToBeRemoved,
            this, &tst_QListWidget::columnsAboutToBeRemoved);
    connect(testWidget->model(), &QAbstractItemModel::columnsRemoved,
            this, &tst_QListWidget::columnsRemoved);

    checkDefaultValues();
}

void tst_QListWidget::cleanupTestCase()
{
    delete testWidget;
}

void tst_QListWidget::init()
{
    testWidget->clear();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void tst_QListWidget::checkDefaultValues()
{
    QCOMPARE(testWidget->currentItem(), nullptr);
    QCOMPARE(testWidget->currentRow(), -1);
    QCOMPARE(testWidget->count(), 0);
}

void tst_QListWidget::populate()
{
    addItem();
    addItem2();
    addItems();
    setItemHidden();

    testWidget->setCurrentIndex(testWidget->model()->index(0, 0));

    // setCurrentItem();
    // setCurrentRow();
}

void tst_QListWidget::addItem()
{
    int count = testWidget->count();
    const QString label = QString::number(count);
    testWidget->addItem(label);
    QCOMPARE(testWidget->count(), ++count);
    QCOMPARE(testWidget->item(testWidget->count() - 1)->text(), label);
}

void tst_QListWidget::addItem2()
{
    int count = testWidget->count();

    // Boundary Checking
    testWidget->addItem(nullptr);
    QCOMPARE(testWidget->count(), count);

    QListWidgetItem *item = new QListWidgetItem(QString::number(count));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    testWidget->addItem(item);
    QCOMPARE(testWidget->count(), ++count);
    QCOMPARE(testWidget->item(testWidget->count()-1), item);
    QCOMPARE(item->isHidden(), false);
}

void tst_QListWidget::addItems()
{
    int count = testWidget->count();

    // Boundary Checking
    testWidget->addItems(QStringList());
    QCOMPARE(testWidget->count(), count);

    QString label = QString::number(count);
    const QStringList stringList{QString::number(testWidget->count() + 1),
                                 QString::number(testWidget->count() + 2),
                                 QString::number(testWidget->count() + 3),
                                 label};
    testWidget->addItems(stringList);
    QCOMPARE(testWidget->count(), count + stringList.count());
    QCOMPARE(testWidget->item(testWidget->count()-1)->text(), label);
}


void tst_QListWidget::openPersistentEditor()
{
    // Boundary checking
    testWidget->openPersistentEditor(nullptr);
    QListWidgetItem *item = new QListWidgetItem(QString::number(testWidget->count()));
    testWidget->openPersistentEditor(item);

    int childCount = testWidget->viewport()->children().count();
    testWidget->addItem(item);
    testWidget->openPersistentEditor(item);
    QCOMPARE(childCount + 1, testWidget->viewport()->children().count());
}

void tst_QListWidget::closePersistentEditor()
{
    // Boundary checking
    int childCount = testWidget->viewport()->children().count();
    testWidget->closePersistentEditor(nullptr);
    QListWidgetItem *item = new QListWidgetItem(QString::number(testWidget->count()));
    testWidget->closePersistentEditor(item);
    QCOMPARE(childCount, testWidget->viewport()->children().count());

    // Create something
    testWidget->addItem(item);
    testWidget->openPersistentEditor(item);

    // actual test
    childCount = testWidget->viewport()->children().count();
    testWidget->closePersistentEditor(item);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(testWidget->viewport()->children().count(), childCount - 1);
}

void tst_QListWidget::setItemHidden()
{
#if QT_DEPRECATED_SINCE(5, 13)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    // Boundary checking
    testWidget->setItemHidden(nullptr, true);
    testWidget->setItemHidden(nullptr, false);
QT_WARNING_POP
#endif

    auto countHidden = [](QListWidget *testWidget)
    {
        int totalHidden = 0;
        for (int i = 0; i < testWidget->model()->rowCount(); ++i) {
            if (testWidget->item(i)->isHidden())
                totalHidden++;
        }
        return totalHidden;
    };
    const int totalHidden = countHidden(testWidget);
    QListWidgetItem *item = new QListWidgetItem(QString::number(testWidget->count()));
    testWidget->addItem(item);

    // Check that nothing else changed
    QCOMPARE(countHidden(testWidget), totalHidden);

    item->setHidden(true);
    QCOMPARE(item->isHidden(), true);

    // Check that nothing else changed
    QCOMPARE(countHidden(testWidget), totalHidden + 1);

    item->setHidden(false);
    QCOMPARE(item->isHidden(), false);

    // Check that nothing else changed
    QCOMPARE(countHidden(testWidget), totalHidden);

    item->setHidden(true);
}

void tst_QListWidget::setCurrentItem_data()
{
    QTest::addColumn<int>("fill");
    QTest::newRow("HasItems: 0") << 0;
    QTest::newRow("HasItems: 1") << 1;
    QTest::newRow("HasItems: 2") << 2;
    QTest::newRow("HasItems: 3") << 3;
}

void tst_QListWidget::setCurrentItem()
{
    QFETCH(int, fill);
    for (int i = 0; i < fill; ++i)
        testWidget->addItem(QString::number(i));

    // Boundary checking
    testWidget->setCurrentItem(nullptr);
    QVERIFY(!testWidget->currentItem());
    QListWidgetItem item;
    testWidget->setCurrentItem(&item);
    QVERIFY(!testWidget->currentItem());

    // Make sure that currentItem changes to what is passed into setCurrentItem
    for (int i = 0; i < testWidget->count(); ++i) {
        testWidget->setCurrentItem(testWidget->item(i));
        for (int j = 0; j < testWidget->count(); ++j) {
            testWidget->setCurrentItem(testWidget->item(j));
            QCOMPARE(testWidget->item(j), testWidget->currentItem());
        }
    }
}

void tst_QListWidget::setCurrentRow_data()
{
    QTest::addColumn<int>("fill");
    QTest::newRow("HasItems: 0") << 0;
    QTest::newRow("HasItems: 1") << 1;
    QTest::newRow("HasItems: 2") << 2;
    QTest::newRow("HasItems: 3") << 3;
}

void tst_QListWidget::setCurrentRow()
{
    QFETCH(int, fill);
    for (int i = 0; i < fill; ++i)
        testWidget->addItem(QString::number(i));

    // Boundary checking
    testWidget->setCurrentRow(-1);
    QCOMPARE(-1, testWidget->currentRow());
    testWidget->setCurrentRow(testWidget->count());
    QCOMPARE(-1, testWidget->currentRow());

    // Make sure that currentRow changes to what is passed into setCurrentRow
    for (int i = 0; i < testWidget->count(); ++i) {
        testWidget->setCurrentRow(i);
        for (int j = 0; j < testWidget->count(); ++j) {
            testWidget->setCurrentRow(j);
            QCOMPARE(j, testWidget->currentRow());
        }
    }
}

void tst_QListWidget::count()
{
    populate();

    // actual test
    QCOMPARE(testWidget->model()->rowCount(), testWidget->count());
}

void tst_QListWidget::currentItem()
{
    populate();

    // actual test
    QModelIndex currentIndex = testWidget->selectionModel()->currentIndex();
    if (currentIndex.isValid())
        QCOMPARE(testWidget->currentItem(), testWidget->item(currentIndex.row()));
    else
        QCOMPARE(testWidget->currentItem(), nullptr);
}

void tst_QListWidget::currentRow()
{
    populate();

    // actual test
    QModelIndex currentIndex = testWidget->selectionModel()->currentIndex();
    if (currentIndex.isValid())
        QCOMPARE(testWidget->currentRow(), currentIndex.row());
    else
        QCOMPARE(testWidget->currentRow(), -1);
}

void tst_QListWidget::editItem_data()
{
    QTest::addColumn<bool>("editable");
    QTest::newRow("editable") << true;
    QTest::newRow("not editable") << false;
}

void tst_QListWidget::editItem()
{
    // Boundary checking
    testWidget->editItem(nullptr);
    QListWidgetItem *item = new QListWidgetItem(QString::number(testWidget->count()));
    testWidget->editItem(item);

    QFETCH(bool, editable);
    if (editable)
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    testWidget->addItem(item);

    int childCount = testWidget->viewport()->children().count();
    QWidget *existsAlready = testWidget->indexWidget(testWidget->model()->index(testWidget->row(item), 0));
    testWidget->editItem(item);
    Qt::ItemFlags flags = item->flags();

    // There doesn't seem to be a way to detect if the item has already been edited...
    if (!existsAlready && flags & Qt::ItemIsEditable && flags & Qt::ItemIsEnabled) {
        QList<QObject *> children = testWidget->viewport()->children();
        QVERIFY(children.count() > childCount);
        bool found = false;
        for (int i = 0; i < children.size(); ++i) {
            if (children.at(i)->inherits("QExpandingLineEdit"))
                found = true;
        }
        QVERIFY(found);
    } else {
        QCOMPARE(testWidget->viewport()->children().count(), childCount);
    }
}

void tst_QListWidget::findItems()
{
    // This really just tests that the items that are returned are converted from index's to items correctly.

    // Boundary checking
    QCOMPARE(testWidget->findItems("GirlsCanWearJeansAndCutTheirHairShort", Qt::MatchExactly).count(), 0);

    populate();

    for (int i = 0; i < testWidget->count(); ++i)
        QCOMPARE(testWidget->findItems(testWidget->item(i)->text(), Qt::MatchExactly).count(), 1);
}


void tst_QListWidget::insertItem_data()
{
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<int>("insertIndex");
    QTest::addColumn<QString>("itemLabel");
    QTest::addColumn<int>("expectedIndex");

    const QStringList initialItems{"foo", "bar"};

    QTest::newRow("Insert less then 0") << initialItems << -1 << "inserted" << 0;
    QTest::newRow("Insert at 0") << initialItems << 0 << "inserted" << 0;
    QTest::newRow("Insert beyond count") << initialItems << initialItems.count()+1 << "inserted" << initialItems.count();
    QTest::newRow("Insert at count") << initialItems << initialItems.count() << "inserted" << initialItems.count();
    QTest::newRow("Insert in the middle") << initialItems << 1 << "inserted" << 1;
}

void tst_QListWidget::insertItem()
{
    QFETCH(QStringList, initialItems);
    QFETCH(int, insertIndex);
    QFETCH(QString, itemLabel);
    QFETCH(int, expectedIndex);

    testWidget->insertItems(0, initialItems);
    QCOMPARE(testWidget->count(), initialItems.count());

    testWidget->insertItem(insertIndex, itemLabel);

    QCOMPARE(rcFirst[RowsAboutToBeInserted], expectedIndex);
    QCOMPARE(rcLast[RowsAboutToBeInserted], expectedIndex);
    QCOMPARE(rcFirst[RowsInserted], expectedIndex);
    QCOMPARE(rcLast[RowsInserted], expectedIndex);

    QCOMPARE(testWidget->count(), initialItems.count() + 1);
    QCOMPARE(testWidget->item(expectedIndex)->text(), itemLabel);
}

void tst_QListWidget::insertItems_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("insertType");

    QTest::newRow("Insert 1 item using constructor") << 1 << 0;
    QTest::newRow("Insert 10 items using constructor") << 10 << 0;
    QTest::newRow("Insert 100 items using constructor") << 100 << 0;

    QTest::newRow("Insert 1 item with insertItem") << 1 << 1;
    QTest::newRow("Insert 10 items with insertItem") << 10 << 1;
    QTest::newRow("Insert 100 items with insertItem") << 100 << 1;

    QTest::newRow("Insert/Create 1 item using insertItem") << 1 << 2;
    QTest::newRow("Insert/Create 10 items using insertItem") << 10 << 2;
    QTest::newRow("Insert/Create 100 items using insertItem") << 100 << 2;

    QTest::newRow("Insert 0 items with insertItems") << 0 << 3;
    QTest::newRow("Insert 1 item with insertItems") << 1 << 3;
    QTest::newRow("Insert 10 items with insertItems") << 10 << 3;
    QTest::newRow("Insert 100 items with insertItems") << 100 << 3;
}

void tst_QListWidget::insertItems()
{
    QFETCH(int, rowCount);
    QFETCH(int, insertType);

    QSignalSpy itemChangedSpy(testWidget, &QListWidget::itemChanged);
    QSignalSpy dataChangedSpy(testWidget->model(), &QAbstractItemModel::dataChanged);

    if (insertType == 3) {
        QStringList strings;
        for (int i = 0; i < rowCount; ++i)
            strings << QString::number(i);
        testWidget->insertItems(0, strings);
    } else {
        for (int r = 0; r < rowCount; ++r) {
            if (insertType == 0) {
                // insert with QListWidgetItem constructor
                new QListWidgetItem(QString::number(r), testWidget);
            } else if (insertType == 1) {
                // insert actual item
                testWidget->insertItem(r, new QListWidgetItem(QString::number(r)));
            } else if (insertType == 2) {
                // insert/creating with string
                testWidget->insertItem(r, QString::number(r));
            } else if (insertType == 3) {
                QStringList strings;
                for (int i = 0; i < rowCount; ++i)
                    strings << QString::number(i);
                testWidget->insertItems(0, strings);
                break;
            } else {
                QVERIFY(0);
            }
        }
    }
    // compare the results
    QCOMPARE(testWidget->count(), rowCount);

    // check if the text
    for (int r = 0; r < rowCount; ++r)
        QCOMPARE(testWidget->item(r)->text(), QString::number(r));

    // make sure all items have view set correctly
    for (int i = 0; i < testWidget->count(); ++i)
        QCOMPARE(testWidget->item(i)->listWidget(), testWidget);

    QCOMPARE(itemChangedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 0);
}

void tst_QListWidget::itemAssignment()
{
    QListWidgetItem itemInWidget("inWidget", testWidget);
    itemInWidget.setFlags(itemInWidget.flags() | Qt::ItemIsUserTristate);
    QListWidgetItem itemOutsideWidget("outsideWidget");

    QVERIFY(itemInWidget.listWidget());
    QCOMPARE(itemInWidget.text(), QString("inWidget"));
    QVERIFY(itemInWidget.flags() & Qt::ItemIsUserTristate);

    QVERIFY(!itemOutsideWidget.listWidget());
    QCOMPARE(itemOutsideWidget.text(), QString("outsideWidget"));
    QVERIFY(!(itemOutsideWidget.flags() & Qt::ItemIsUserTristate));

    itemOutsideWidget = itemInWidget;
    QVERIFY(!itemOutsideWidget.listWidget());
    QCOMPARE(itemOutsideWidget.text(), QString("inWidget"));
    QVERIFY(itemOutsideWidget.flags() & Qt::ItemIsUserTristate);
}

void tst_QListWidget::item_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<bool>("outOfBounds");

    QTest::newRow("First item, row: 0") << 0 << false;
    QTest::newRow("Middle item, row: 1") << 1 << false;
    QTest::newRow("Last item, row: 2") << 2 << false;
    QTest::newRow("Out of bounds, row: -1") << -1 << true;
    QTest::newRow("Out of bounds, row: 3") << 3 << true;
}

void tst_QListWidget::item()
{
    QFETCH(int, row);
    QFETCH(bool, outOfBounds);

    (new QListWidgetItem(testWidget))->setText("item0");
    (new QListWidgetItem(testWidget))->setText("item1");
    (new QListWidgetItem(testWidget))->setText("item2");

    QCOMPARE(testWidget->count(), 3);

    QListWidgetItem *item = testWidget->item(row);
    if (outOfBounds) {
        QCOMPARE(item, nullptr);
        QCOMPARE(testWidget->count(), 3);
    } else {
        QCOMPARE(item->text(), QStringLiteral("item") + QString::number(row));
        QCOMPARE(testWidget->count(), 3);
    }
}

void tst_QListWidget::takeItem_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<bool>("outOfBounds");

    QTest::newRow("First item, row: 0") << 0 << false;
    QTest::newRow("Middle item, row: 1") << 1 << false;
    QTest::newRow("Last item, row: 2") << 2 << false;
    QTest::newRow("Out of bounds, row: -1") << -1 << true;
    QTest::newRow("Out of bounds, row: 3") << 3 << true;
}

void tst_QListWidget::takeItem()
{
    QFETCH(int, row);
    QFETCH(bool, outOfBounds);

    (new QListWidgetItem(testWidget))->setText("item0");
    (new QListWidgetItem(testWidget))->setText("item1");
    (new QListWidgetItem(testWidget))->setText("item2");

    QCOMPARE(testWidget->count(), 3);

    QListWidgetItem *item = testWidget->takeItem(row);
    if (outOfBounds) {
        QCOMPARE(item, nullptr);
        QCOMPARE(testWidget->count(), 3);
    } else {
        QCOMPARE(item->text(), QStringLiteral("item") + QString::number(row));
        QCOMPARE(testWidget->count(), 2);
    }

    delete item;
}

void tst_QListWidget::selectedItems_data()
{
    QTest::addColumn<int>("itemCount");
    QTest::addColumn<IntList>("hiddenRows");
    QTest::addColumn<IntList>("selectedRows");
    QTest::addColumn<IntList>("expectedRows");


    QTest::newRow("none hidden, none selected")
        << 3
        << IntList()
        << IntList()
        << IntList();

    QTest::newRow("none hidden, all selected")
        << 3
        << IntList()
        << (IntList() << 0 << 1 << 2)
        << (IntList() << 0 << 1 << 2);

    QTest::newRow("first hidden, all selected")
        << 3
        << (IntList() << 0)
        << (IntList() << 0 << 1 << 2)
        << (IntList() << 0 << 1 << 2);

    QTest::newRow("last hidden, all selected")
        << 3
        << (IntList() << 2)
        << (IntList() << 0 << 1 << 2)
        << (IntList() << 0 << 1 << 2);

    QTest::newRow("middle hidden, all selected")
        << 3
        << (IntList() << 1)
        << (IntList() << 0 << 1 << 2)
        << (IntList() << 0 << 1 << 2);

    QTest::newRow("all hidden, all selected")
        << 3
        << (IntList() << 0 << 1 << 2)
        << (IntList() << 0 << 1 << 2)
        << (IntList() << 0 << 1 << 2);
}

void tst_QListWidget::selectedItems()
{
    QFETCH(int, itemCount);
    QFETCH(const IntList, hiddenRows);
    QFETCH(const IntList, selectedRows);
    QFETCH(const IntList, expectedRows);

    QCOMPARE(testWidget->count(), 0);

    //insert items
    for (int i = 0; i < itemCount; ++i)
        new QListWidgetItem(QStringLiteral("Item") + QString::number(i), testWidget);

    //test the selection
    testWidget->setSelectionMode(QListWidget::SingleSelection);
    for (int i = 0; i < itemCount; ++i) {
        QListWidgetItem *item = testWidget->item(i);
        item->setSelected(true);
        QVERIFY(item->isSelected());
        QCOMPARE(testWidget->selectedItems().count(), 1);
    }
    //let's clear the selection
    testWidget->clearSelection();
    //... and set the selection mode to allow more than 1 item to be selected
    testWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    //verify items are inserted
    QCOMPARE(testWidget->count(), itemCount);
    // hide items
    for (int row : hiddenRows)
        testWidget->item(row)->setHidden(true);
    // select items
    for (int row : selectedRows)
        testWidget->item(row)->setSelected(true);

    // check that the correct number of items and the expected items are there
    QList<QListWidgetItem *> selectedItems = testWidget->selectedItems();
    QCOMPARE(selectedItems.count(), expectedRows.count());
    for (int row : expectedRows)
        QVERIFY(selectedItems.contains(testWidget->item(row)));

    //check that isSelected agrees with selectedItems
    for (int i = 0; i < itemCount; ++i) {
        QListWidgetItem *item = testWidget->item(i);
        if (item->isSelected())
            QVERIFY(selectedItems.contains(item));
    }
}

void tst_QListWidget::removeItems_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("removeRows");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("expectedRowCount");

    QTest::newRow("Empty") << 0 << 1 << 0 << 0;
    QTest::newRow("1:1") << 1 << 1 << 0 << 0;
    QTest::newRow("3:1") << 3 << 1 << 0 << 2;
    QTest::newRow("3:2") << 3 << 2 << 0 << 1;
    QTest::newRow("100:10") << 100 << 10 << 0 << 90;
}

void tst_QListWidget::removeItems()
{
    QFETCH(int, rowCount);
    QFETCH(int, removeRows);
    QFETCH(int, row);
    QFETCH(int, expectedRowCount);

    //insert items
    for (int r = 0; r < rowCount; ++r)
        new QListWidgetItem(QString::number(r), testWidget);

    // remove and compare the results
    for (int r = 0; r < removeRows; ++r)
        delete testWidget->item(row);
    QCOMPARE(testWidget->count(), expectedRowCount);

    // check if the correct items were removed
    for (int r = 0; r < expectedRowCount; ++r)
        if (r < row)
            QCOMPARE(testWidget->item(r)->text(), QString::number(r));
        else
            QCOMPARE(testWidget->item(r)->text(), QString::number(r + removeRows));
}

void tst_QListWidget::moveItemsPriv_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("srcRow");
    QTest::addColumn<int>("dstRow");
    QTest::addColumn<bool>("shouldHaveSignaled");

    QTest::newRow("Empty") << 0 << 0 << 0 << false;
    QTest::newRow("Overflow src") << 5 << 5 << 2 << false;
    QTest::newRow("Underflow src") << 5 << -1 << 2 << false;
    QTest::newRow("Overflow dst") << 5 << 2 << 6 << false;
    QTest::newRow("Underflow dst") << 5 << 2 << -1 << false;
    QTest::newRow("Same place") << 5 << 2 << 2 << false;
    QTest::newRow("Up") << 5 << 4 << 2 << true;
    QTest::newRow("Down") << 5 << 2 << 4 << true;
    QTest::newRow("QTBUG-6532 assert") << 5 << 0 << 1 << false;
    QTest::newRow("QTBUG-6565 to the end") << 5 << 3 << 5 << true;
    QTest::newRow("Same place 2") << 2 << 0 << 1 << false;
    QTest::newRow("swap") << 2 << 0 << 2 << true;
    QTest::newRow("swap2") << 4 << 1 << 3 << true;
    QTest::newRow("swap3") << 4 << 3 << 2 << true;
    QTest::newRow("swap4") << 2 << 1 << 0 << true;
}

void tst_QListWidget::moveItemsPriv()
{
    QFETCH(int, rowCount);
    QFETCH(int, srcRow);
    QFETCH(int, dstRow);
    QFETCH(bool, shouldHaveSignaled);

    for (int r = 0; r < rowCount; ++r)
        new QListWidgetItem(QString::number(r), testWidget);

    QListModel *model = qobject_cast<QListModel *>(testWidget->model());
    QVERIFY(model);
    QSignalSpy beginMoveSpy(model, &QAbstractItemModel::rowsAboutToBeMoved);
    QSignalSpy movedSpy(model, &QAbstractItemModel::rowsMoved);
    model->move(srcRow, dstRow);

    if (shouldHaveSignaled) {
        if (srcRow < dstRow)
            QCOMPARE(testWidget->item(dstRow - 1)->text(), QString::number(srcRow));
        else
            QCOMPARE(testWidget->item(dstRow)->text(), QString::number(srcRow));

        QCOMPARE(beginMoveSpy.count(), 1);
        const QList<QVariant> &beginMoveArgs = beginMoveSpy.takeFirst();
        QCOMPARE(beginMoveArgs.at(1).toInt(), srcRow);
        QCOMPARE(beginMoveArgs.at(2).toInt(), srcRow);
        QCOMPARE(beginMoveArgs.at(4).toInt(), dstRow);

        QCOMPARE(movedSpy.count(), 1);
        const QList<QVariant> &movedArgs = movedSpy.takeFirst();
        QCOMPARE(movedArgs.at(1).toInt(), srcRow);
        QCOMPARE(movedArgs.at(2).toInt(), srcRow);
        QCOMPARE(movedArgs.at(4).toInt(), dstRow);
    } else {
        QCOMPARE(beginMoveSpy.count(), 0);
        QCOMPARE(movedSpy.count(), 0);
    }
}

void tst_QListWidget::itemStreaming_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("toolTip");

    QTest::newRow("Data") << "item text" << "tool tip text";
}

void tst_QListWidget::itemStreaming()
{
    QFETCH(QString, text);
    QFETCH(QString, toolTip);

    QListWidgetItem item;
    QCOMPARE(item.text(), QString());
    QCOMPARE(item.toolTip(), QString());

    item.setText(text);
    item.setToolTip(toolTip);
    QCOMPARE(item.text(), text);
    QCOMPARE(item.toolTip(), toolTip);

    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << item;

    QListWidgetItem item2;
    QCOMPARE(item2.text(), QString());
    QCOMPARE(item2.toolTip(), QString());

    QVERIFY(!buffer.isEmpty());

    QDataStream in(&buffer, QIODevice::ReadOnly);
    in >> item2;
    QCOMPARE(item2.text(), text);
    QCOMPARE(item2.toolTip(), toolTip);
}

void tst_QListWidget::sortItems_data()
{
    QTest::addColumn<Qt::SortOrder>("order");
    QTest::addColumn<QVariantList>("initialList");
    QTest::addColumn<QVariantList>("expectedList");
    QTest::addColumn<IntList>("expectedRows");

    QTest::newRow("ascending strings")
        << Qt::AscendingOrder
        << (QVariantList() << QString("c") << QString("d") << QString("a") << QString("b"))
        << (QVariantList() << QString("a") << QString("b") << QString("c") << QString("d"))
        << (IntList() << 2 << 3 << 0 << 1);

    QTest::newRow("descending strings")
        << Qt::DescendingOrder
        << (QVariantList() << QString("c") << QString("d") << QString("a") << QString("b"))
        << (QVariantList() << QString("d") << QString("c") << QString("b") << QString("a"))
        << (IntList() << 1 << 0 << 3 << 2);

    QTest::newRow("ascending numbers")
        << Qt::AscendingOrder
        << (QVariantList() << 1 << 11 << 2 << 22)
        << (QVariantList() << 1 << 2 << 11 << 22)
        << (IntList() << 0 << 2 << 1 << 3);

    QTest::newRow("descending numbers")
        << Qt::DescendingOrder
        << (QVariantList() << 1 << 11 << 2 << 22)
        << (QVariantList() << 22 << 11 << 2 << 1)
        << (IntList() << 3 << 1 << 2 << 0);
}

void tst_QListWidget::sortItems()
{
    QFETCH(Qt::SortOrder, order);
    QFETCH(const QVariantList, initialList);
    QFETCH(const QVariantList, expectedList);
    QFETCH(const IntList, expectedRows);

    for (const QVariant &data : initialList) {
        QListWidgetItem *item = new QListWidgetItem(testWidget);
        item->setData(Qt::DisplayRole, data);
    }

    QAbstractItemModel *model = testWidget->model();
    QVector<QPersistentModelIndex> persistent;
    for (int j = 0; j < model->rowCount(QModelIndex()); ++j)
        persistent << model->index(j, 0, QModelIndex());

    testWidget->sortItems(order);

    QCOMPARE(testWidget->count(), expectedList.count());
    for (int i = 0; i < testWidget->count(); ++i)
        QCOMPARE(testWidget->item(i)->text(), expectedList.at(i).toString());

    for (int k = 0; k < testWidget->count(); ++k)
        QCOMPARE(persistent.at(k).row(), expectedRows.at(k));
}

void tst_QListWidget::sortHiddenItems_data()
{
    QTest::addColumn<Qt::SortOrder>("order");
    QTest::addColumn<QStringList>("initialList");
    QTest::addColumn<QStringList>("expectedList");
    QTest::addColumn<IntList>("expectedRows");
    QTest::addColumn<IntList>("expectedVisibility");

    QStringList initial, expected;
    IntList rowOrder;
    IntList visible;
    for (int i = 0; i < 20; ++i) {
        initial << QString(QChar(0x41 + i));
        expected << QString(QChar(0x54 - i));
        rowOrder << 19 - i;
        visible << (i % 2);

    }
    QTest::newRow("descending order, 20 items")
        << Qt::DescendingOrder
        << initial
        << expected
        << rowOrder
        << visible;

    QTest::newRow("ascending order")
        << Qt::AscendingOrder
        << (QStringList() << "c" << "d" << "a" << "b")
        << (QStringList() << "a" << "b" << "c" << "d")
        << (IntList() << 2 << 3 << 0 << 1)
        << (IntList() << 1 << 0 << 1 << 0);

    QTest::newRow("descending order")
        << Qt::DescendingOrder
        << (QStringList() << "c" << "d" << "a" << "b")
        << (QStringList() << "d" << "c" << "b" << "a")
        << (IntList() << 1 << 0 << 3 << 2)
        << (IntList() << 0 << 1 << 0 << 1);
}

void tst_QListWidget::sortHiddenItems()
{
    QFETCH(Qt::SortOrder, order);
    QFETCH(QStringList, initialList);
    QFETCH(QStringList, expectedList);
    QFETCH(IntList, expectedRows);
    QFETCH(IntList, expectedVisibility);

    // init() won't clear hidden items...
    QListWidget *tw = new QListWidget();
    tw->addItems(initialList);

    QAbstractItemModel *model = tw->model();
    QVector<QPersistentModelIndex> persistent;
    for (int j = 0; j < model->rowCount(QModelIndex()); ++j) {
        persistent << model->index(j, 0, QModelIndex());
        tw->setRowHidden(j, j & 1); // every odd is hidden
    }

    tw->setSortingEnabled(true);
    tw->sortItems(order);

    QCOMPARE(tw->count(), expectedList.count());
    for (int i = 0; i < tw->count(); ++i) {
        QCOMPARE(tw->item(i)->text(), expectedList.at(i));
        QCOMPARE(tw->item(i)->isHidden(), !expectedVisibility.at(i));
    }

    for (int k = 0; k < tw->count(); ++k)
        QCOMPARE(persistent.at(k).row(), expectedRows.at(k));

    delete tw;
}

class TestListWidget : public QListWidget
{
    Q_OBJECT
public:
    using QListWidget::QListWidget;
    using QListWidget::state;
    using QListWidget::closeEditor;
    using QListWidget::mimeData;
    using QListWidget::indexFromItem;

    bool isEditingState() const {
        return QListWidget::state() == QListWidget::EditingState;
    }
};

void tst_QListWidget::closeEditor()
{
    TestListWidget w;
    w.addItems({"a", "b", "c", "d"});
    QListWidgetItem *item = w.item(0);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    QVERIFY(item);
    w.editItem(item);

    QVERIFY(w.isEditingState());

    w.reset();

    QVERIFY(!w.isEditingState());
}

void tst_QListWidget::setData_data()
{
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<int>("itemIndex");
    QTest::addColumn<IntList>("roles");
    QTest::addColumn<QVariantList>("values");
    QTest::addColumn<int>("expectedSignalCount");

    QStringList initialItems;
    IntList roles;
    QVariantList values;

    {
        initialItems.clear(); roles.clear(); values.clear();
        initialItems << "foo";
        roles << Qt::DisplayRole;
        values << "xxx";
        QTest::newRow("changing a role should emit")
            << initialItems << 0 << roles << values << 1;
    }
    {
        initialItems.clear(); roles.clear(); values.clear();
        initialItems << "foo";
        roles << Qt::DisplayRole;
        values << "foo";
        QTest::newRow("setting the same value should not emit")
            << initialItems << 0 << roles << values << 0;
    }
    {
        initialItems.clear(); roles.clear(); values.clear();
        initialItems << "foo";
        roles << Qt::DisplayRole << Qt::DisplayRole;
        values << "bar" << "bar";
        QTest::newRow("setting the same value twice should only emit once")
            << initialItems << 0 << roles << values << 1;
    }
    {
        initialItems.clear(); roles.clear(); values.clear();
        initialItems << "foo";
        roles << Qt::DisplayRole << Qt::ToolTipRole << Qt::WhatsThisRole;
        values << "bar" << "bartooltip" << "barwhatsthis";
        QTest::newRow("changing three roles should emit three times")
            << initialItems << 0 << roles << values << 3;
    }
}

void tst_QListWidget::setData()
{
    QFETCH(QStringList, initialItems);
    QFETCH(int, itemIndex);
    QFETCH(IntList, roles);
    QFETCH(QVariantList, values);
    QFETCH(int, expectedSignalCount);

    QCOMPARE(roles.count(), values.count());

    for (int manipulateModel = 0; manipulateModel < 2; ++manipulateModel) {
        testWidget->clear();
        testWidget->insertItems(0, initialItems);
        QCOMPARE(testWidget->count(), initialItems.count());

        QSignalSpy itemChanged(testWidget, &QListWidget::itemChanged);
        QSignalSpy dataChanged(testWidget->model(), &QAbstractItemModel::dataChanged);

        for (int i = 0; i < roles.count(); ++i) {
            if (manipulateModel)
                testWidget->model()->setData(
                    testWidget->model()->index(itemIndex, 0, testWidget->rootIndex()),
                    values.at(i),
                    roles.at(i));
            else
                testWidget->item(itemIndex)->setData(roles.at(i), values.at(i));
        }

        // make sure the data is actually set
        for (int i = 0; i < roles.count(); ++i)
            QCOMPARE(testWidget->item(itemIndex)->data(roles.at(i)), values.at(i));

        // make sure we get the right number of emits
        QCOMPARE(itemChanged.count(), expectedSignalCount);
        QCOMPARE(dataChanged.count(), expectedSignalCount);
    }
}

void tst_QListWidget::insertItemsWithSorting_data()
{
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<QStringList>("insertItems");
    QTest::addColumn<QStringList>("expectedItems");
    QTest::addColumn<IntList>("expectedRows");

    QTest::newRow("() + (a) = (a)")
        << Qt::AscendingOrder
        << QStringList()
        << (QStringList() << "a")
        << (QStringList() << "a")
        << IntList();
    QTest::newRow("() + (c, b, a) = (a, b, c)")
        << Qt::AscendingOrder
        << QStringList()
        << (QStringList() << "c" << "b" << "a")
        << (QStringList() << "a" << "b" << "c")
        << IntList();
    QTest::newRow("() + (a, b, c) = (c, b, a)")
        << Qt::DescendingOrder
        << QStringList()
        << (QStringList() << "a" << "b" << "c")
        << (QStringList() << "c" << "b" << "a")
        << IntList();
    QTest::newRow("(a) + (b) = (a, b)")
        << Qt::AscendingOrder
        << QStringList("a")
        << (QStringList() << "b")
        << (QStringList() << "a" << "b")
        << (IntList() << 0);
    QTest::newRow("(a) + (b) = (b, a)")
        << Qt::DescendingOrder
        << QStringList("a")
        << (QStringList() << "b")
        << (QStringList() << "b" << "a")
        << (IntList() << 1);
    QTest::newRow("(a, c, b) + (d) = (a, b, c, d)")
        << Qt::AscendingOrder
        << (QStringList() << "a" << "c" << "b")
        << (QStringList() << "d")
        << (QStringList() << "a" << "b" << "c" << "d")
        << (IntList() << 0 << 1 << 2);
    QTest::newRow("(b, c, a) + (d) = (d, c, b, a)")
        << Qt::DescendingOrder
        << (QStringList() << "b" << "c" << "a")
        << (QStringList() << "d")
        << (QStringList() << "d" << "c" << "b" << "a")
        << (IntList() << 1 << 2 << 3);
    {
        IntList ascendingRows;
        IntList reverseRows;
        QStringList ascendingItems;
        QStringList reverseItems;
        for (char i = 'a'; i <= 'z'; ++i) {
            ascendingItems << QString(1, QLatin1Char(i));
            reverseItems << QString(1, QLatin1Char('z' - i + 'a'));
            ascendingRows << i - 'a';
            reverseRows << 'z' - i + 'a';
        }
        QTest::newRow("() + (sorted items) = (sorted items)")
            << Qt::AscendingOrder
            << QStringList()
            << ascendingItems
            << ascendingItems
            << IntList();
        QTest::newRow("(sorted items) + () = (sorted items)")
            << Qt::AscendingOrder
            << ascendingItems
            << QStringList()
            << ascendingItems
            << ascendingRows;
        QTest::newRow("() + (ascending items) = (reverse items)")
            << Qt::DescendingOrder
            << QStringList()
            << ascendingItems
            << reverseItems
            << IntList();
        QTest::newRow("(reverse items) + () = (ascending items)")
            << Qt::AscendingOrder
            << reverseItems
            << QStringList()
            << ascendingItems
            << ascendingRows;
        QTest::newRow("(reverse items) + () = (reverse items)")
            << Qt::DescendingOrder
            << reverseItems
            << QStringList()
            << reverseItems
            << ascendingRows;
    }
}

void tst_QListWidget::insertItemsWithSorting()
{
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(const QStringList, initialItems);
    QFETCH(const QStringList, insertItems);
    QFETCH(const QStringList, expectedItems);
    QFETCH(const IntList, expectedRows);

    for (int method = 0; method < 5; ++method) {
        QListWidget w;
        w.setSortingEnabled(true);
        w.sortItems(sortOrder);
        w.addItems(initialItems);

        QAbstractItemModel *model = w.model();
        QList<QPersistentModelIndex> persistent;
        for (int j = 0; j < model->rowCount(QModelIndex()); ++j)
            persistent << model->index(j, 0, QModelIndex());

        switch (method) {
            case 0:
                // insert using item constructor
                for (const QString &str : insertItems)
                    new QListWidgetItem(str, &w);
                break;
            case 1:
                // insert using insertItems()
                w.insertItems(0, insertItems);
                break;
            case 2:
                // insert using insertItem()
                for (const QString &str : insertItems)
                    w.insertItem(0, str);
                break;
            case 3:
                // insert using addItems()
                w.addItems(insertItems);
                break;
            case 4:
                // insert using addItem()
                for (const QString &str : insertItems)
                    w.addItem(str);
                break;
        }
        QCOMPARE(w.count(), expectedItems.count());
        for (int i = 0; i < w.count(); ++i)
            QCOMPARE(w.item(i)->text(), expectedItems.at(i));

        for (int k = 0; k < persistent.count(); ++k)
            QCOMPARE(persistent.at(k).row(), expectedRows.at(k));
    }
}

void tst_QListWidget::changeDataWithSorting_data()
{
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<int>("itemIndex");
    QTest::addColumn<QString>("newValue");
    QTest::addColumn<QStringList>("expectedItems");
    QTest::addColumn<IntList>("expectedRows");
    QTest::addColumn<bool>("reorderingExpected");

    QTest::newRow("change a to b in (a)")
        << Qt::AscendingOrder
        << (QStringList() << "a")
        << 0 << "b"
        << (QStringList() << "b")
        << (IntList() << 0)
        << false;
    QTest::newRow("change a to b in (a, c)")
        << Qt::AscendingOrder
        << (QStringList() << "a" << "c")
        << 0 << "b"
        << (QStringList() << "b" << "c")
        << (IntList() << 0 << 1)
        << false;
    QTest::newRow("change a to c in (a, b)")
        << Qt::AscendingOrder
        << (QStringList() << "a" << "b")
        << 0 << "c"
        << (QStringList() << "b" << "c")
        << (IntList() << 1 << 0)
        << true;
    QTest::newRow("change c to a in (c, b)")
        << Qt::DescendingOrder
        << (QStringList() << "c" << "b")
        << 0 << "a"
        << (QStringList() << "b" << "a")
        << (IntList() << 1 << 0)
        << true;
    QTest::newRow("change e to i in (a, c, e, g)")
        << Qt::AscendingOrder
        << (QStringList() << "a" << "c" << "e" << "g")
        << 2 << "i"
        << (QStringList() << "a" << "c" << "g" << "i")
        << (IntList() << 0 << 1 << 3 << 2)
        << true;
    QTest::newRow("change e to a in (c, e, g, i)")
        << Qt::AscendingOrder
        << (QStringList() << "c" << "e" << "g" << "i")
        << 1 << "a"
        << (QStringList() << "a" << "c" << "g" << "i")
        << (IntList() << 1 << 0 << 2 << 3)
        << true;
    QTest::newRow("change e to f in (c, e, g, i)")
        << Qt::AscendingOrder
        << (QStringList() << "c" << "e" << "g" << "i")
        << 1 << "f"
        << (QStringList() << "c" << "f" << "g" << "i")
        << (IntList() << 0 << 1 << 2 << 3)
        << false;
}

class QListWidgetDataChanged : public QListWidget
{
    Q_OBJECT
public:
    using QListWidget::QListWidget;

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) override
    {
        QListWidget::dataChanged(topLeft, bottomRight, roles);
        currentRoles = roles;
    }
    QVector<int> currentRoles;
};

void tst_QListWidget::itemData()
{
    QListWidgetDataChanged widget;
    QListWidgetItem item(&widget);
    item.setFlags(item.flags() | Qt::ItemIsEditable);
    item.setData(Qt::DisplayRole,  QString("0"));
    QCOMPARE(widget.currentRoles, QVector<int>({Qt::DisplayRole, Qt::EditRole}));
    item.setData(Qt::CheckStateRole, Qt::PartiallyChecked);
    QCOMPARE(widget.currentRoles, QVector<int>{Qt::CheckStateRole});
    for (int i = 0; i < 4; ++i)
    {
        item.setData(Qt::UserRole + i, QString::number(i + 1));
        QCOMPARE(widget.currentRoles, QVector<int>{Qt::UserRole + i});
    }
    QMap<int, QVariant> flags = widget.model()->itemData(widget.model()->index(0, 0));
    QCOMPARE(flags.count(), 6);
    for (int i = 0; i < 4; ++i)
        QCOMPARE(flags[Qt::UserRole + i].toString(), QString::number(i + 1));

    item.setBackground(QBrush(Qt::red));
    item.setForeground(QBrush(Qt::red));
    item.setSizeHint(QSize(10, 10));
    QCOMPARE(item.data(Qt::BackgroundRole), QVariant(QBrush(Qt::red)));
    QCOMPARE(item.data(Qt::ForegroundRole), QVariant(QBrush(Qt::red)));
    QCOMPARE(item.data(Qt::SizeHintRole), QVariant(QSize(10, 10)));
    // an empty brush should result in a QVariant()
    item.setBackground(QBrush());
    item.setForeground(QBrush());
    item.setSizeHint(QSize());
    QCOMPARE(item.data(Qt::BackgroundRole), QVariant());
    QCOMPARE(item.data(Qt::ForegroundRole), QVariant());
    QCOMPARE(item.data(Qt::SizeHintRole), QVariant());
}

void tst_QListWidget::changeDataWithSorting()
{
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(QStringList, initialItems);
    QFETCH(int, itemIndex);
    QFETCH(QString, newValue);
    QFETCH(QStringList, expectedItems);
    QFETCH(IntList, expectedRows);
    QFETCH(bool, reorderingExpected);

    QListWidget w;
    w.setSortingEnabled(true);
    w.sortItems(sortOrder);
    w.addItems(initialItems);

    QAbstractItemModel *model = w.model();
    QVector<QPersistentModelIndex> persistent;
    for (int j = 0; j < model->rowCount(QModelIndex()); ++j)
        persistent << model->index(j, 0, QModelIndex());

    QSignalSpy dataChangedSpy(model, &QAbstractItemModel::dataChanged);
    QSignalSpy layoutChangedSpy(model, &QAbstractItemModel::layoutChanged);

    QListWidgetItem *item = w.item(itemIndex);
    item->setText(newValue);
    for (int i = 0; i < expectedItems.count(); ++i) {
        QCOMPARE(w.item(i)->text(), expectedItems.at(i));
        for (int j = 0; j < persistent.count(); ++j) {
            if (persistent.at(j).row() == i) // the same toplevel row
                QCOMPARE(persistent.at(j).internalPointer(), static_cast<void *>(w.item(i)));
        }
    }

    for (int k = 0; k < persistent.count(); ++k)
        QCOMPARE(persistent.at(k).row(), expectedRows.at(k));

    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(layoutChangedSpy.count(), reorderingExpected ? 1 : 0);
}

void tst_QListWidget::itemWidget()
{
    QListWidget list;
    QWidget widget;

    QListWidgetItem *item = new QListWidgetItem(&list);


    QCOMPARE(list.itemWidget(item), nullptr);
    list.setItemWidget(item, &widget);
    QCOMPARE(list.itemWidget(item), &widget);
    list.removeItemWidget(item);
    QCOMPARE(list.itemWidget(item), nullptr);
}

#ifndef Q_OS_MAC
class MyListWidget : public QListWidget
{
    Q_OBJECT
public:
    using QListWidget::QListWidget;

    void paintEvent(QPaintEvent *e) override
    {
        painted += e->region();
        QListWidget::paintEvent(e);
    }

    QRegion painted;
};

void tst_QListWidget::fastScroll()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QWidget topLevel;
    MyListWidget widget(&topLevel);
    for (int i = 0; i < 50; ++i)
        widget.addItem(QStringLiteral("Item ") + QString::number(i));

    topLevel.resize(300, 300); // toplevel needs to be wide enough for the item
    topLevel.show();

    // Force the mouse cursor off the widget as it causes item it is over to highlight,
    // which causes unexpected paint region.
    QTest::mouseMove(&widget, QPoint(-10, -10));

    // Make sure the widget gets the first full repaint. On
    // some WMs, we'll get two (first inactive exposure, then
    // active exposure.
    QVERIFY(QTest::qWaitForWindowActive(&topLevel));

    QSize itemSize = widget.visualItemRect(widget.item(0)).size();
    QVERIFY(!itemSize.isEmpty());

    QScrollBar *sbar = widget.verticalScrollBar();
    widget.setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    widget.painted = QRegion();
    sbar->setValue(sbar->value() + sbar->singleStep());
    QApplication::processEvents();

    const QSize actualItemSize = widget.painted.boundingRect().size();
    if (actualItemSize != itemSize)
        QEXPECT_FAIL("", "QTBUG-21098", Continue);

    // only one item should be repainted, the rest should be scrolled in memory
    QCOMPARE(actualItemSize, itemSize);
}
#endif // Q_OS_MAC

void tst_QListWidget::insertUnchanged()
{
    QListWidget w;
    QSignalSpy itemChangedSpy(&w, &QListWidget::itemChanged);
    QListWidgetItem item("foo", &w);
    QCOMPARE(itemChangedSpy.count(), 0);
}

void tst_QListWidget::setSortingEnabled()
{
    QListWidget w;
    QListWidgetItem *item1 = new QListWidgetItem(&w);
    QListWidgetItem *item2 = new QListWidgetItem(&w);

    w.setSortingEnabled(true);
    QCOMPARE(w.isSortingEnabled(), true);
    QCOMPARE(w.item(0), item1);
    QCOMPARE(w.item(1), item2);
}

void tst_QListWidget::task199503_crashWhenCleared()
{
    //we test here for a crash that would occur if you clear the items in the currentItemChanged signal
    QListWidget w;
    w.addItems({"item1", "item2", "item3"});
    w.setCurrentRow(0);
    w.connect(&w, &QListWidget::currentItemChanged, &w, &QListWidget::clear);
    w.setCurrentRow(1);
}

void tst_QListWidget::task217070_scrollbarsAdjusted()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    //This task was mailing for style using SH_ScrollView_FrameOnlyAroundContents such as QMotifStyle
    QListWidget v;
    for (int i = 0; i < 200;i++)
        v.addItem(QString::number(i));
    v.show();
    v.setViewMode(QListView::IconMode);
    v.setResizeMode(QListView::Adjust);
    v.setUniformItemSizes(true);
    v.resize(160, 100);
    QVERIFY(QTest::qWaitForWindowActive(&v));
    QScrollBar *hbar = v.horizontalScrollBar();
    QScrollBar *vbar = v.verticalScrollBar();
    QVERIFY(hbar && vbar);
    const auto style = vbar->style();
    for (int f = 150; f > 90 ; f--) {
        v.resize(f, 100);
        QTRY_VERIFY(style->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, vbar) ||
                    vbar->isVisible());
        //the horizontal scrollbar must not be visible.
        QVERIFY(!hbar->isVisible());
    }
}

void tst_QListWidget::task258949_keypressHangup()
{
    QListWidget lw;
    for (int y = 0; y < 5; y++) {
        QListWidgetItem *lwi = new QListWidgetItem(&lw);
        lwi->setText(y ? "1" : "0");
        if (y)
            lwi->setFlags(Qt::ItemIsSelectable);
    }

    lw.show();
    lw.setCurrentIndex(lw.model()->index(0, 0));
    QCOMPARE(lw.currentIndex(), lw.model()->index(0, 0));
    QTest::qWait(30);
    QTest::keyPress(&lw, '1'); //this used to freeze
    QTRY_COMPARE(lw.currentIndex(), lw.model()->index(0, 0));
}

void tst_QListWidget::QTBUG8086_currentItemChangedOnClick()
{
    QWidget win;
    QHBoxLayout layout(&win);
    QListWidget list;
    for (int i = 0 ; i < 4; ++i)
        new QListWidgetItem(QString::number(i), &list);

    layout.addWidget(&list);

    QLineEdit edit;
    layout.addWidget(&edit);

    edit.setFocus();
    win.show();

    QSignalSpy spy(&list, &QListWidget::currentItemChanged);

    QVERIFY(QTest::qWaitForWindowExposed(&win));

    QCOMPARE(spy.count(), 0);

    QTest::mouseClick(list.viewport(), Qt::LeftButton, {},
                      list.visualItemRect(list.item(2)).center());

    QCOMPARE(spy.count(), 1);
}


class ItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                          const QModelIndex &) const override
    {
        QLineEdit *lineEdit = new QLineEdit(parent);
        lineEdit->setFrame(false);
        QCompleter *completer = new QCompleter(QStringList() << "completer", lineEdit);
        completer->setCompletionMode(QCompleter::InlineCompletion);
        lineEdit->setCompleter(completer);
        return lineEdit;
    }
};

void tst_QListWidget::QTBUG14363_completerWithAnyKeyPressedEditTriggers()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QListWidget listWidget;
    listWidget.setEditTriggers(QAbstractItemView::AnyKeyPressed);
    listWidget.setItemDelegate(new ItemDelegate(&listWidget));
    QListWidgetItem *item = new QListWidgetItem(QLatin1String("select an item (don't start editing)"), &listWidget);
    item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable);
    new QListWidgetItem(QLatin1String("try to type the letter 'c'"), &listWidget);
    new QListWidgetItem(QLatin1String("completer"), &listWidget);
    listWidget.show();
    listWidget.setCurrentItem(item);
    QApplication::setActiveWindow(&listWidget);
    QVERIFY(QTest::qWaitForWindowActive(&listWidget));
    listWidget.setFocus();
    QCOMPARE(QApplication::focusWidget(), &listWidget);

    QTest::keyClick(listWidget.viewport(), Qt::Key_C);

    QLineEdit *le = qobject_cast<QLineEdit*>(listWidget.itemWidget(item));
    QVERIFY(le);
    QCOMPARE(le->text(), QString("completer"));
    QCOMPARE(le->completer()->currentCompletion(), QString("completer"));
}

void tst_QListWidget::mimeData()
{
    TestListWidget list;

    for (int x = 0; x < 10; ++x)
        list.addItem(new QListWidgetItem(QStringLiteral("123")));

    const QList<QListWidgetItem *> tableWidgetItemList{list.item(1)};
    const QModelIndexList modelIndexList{list.indexFromItem(list.item(1))};

    // do these checks more than once to ensure that the "cached indexes" work as expected
    QMimeData *data;
    for (int i = 0; i < 2; ++i) {
      QVERIFY(!list.mimeData({}));
      QVERIFY(!list.model()->mimeData({}));

      QVERIFY((data = list.mimeData(tableWidgetItemList)));
      delete data;

      QVERIFY((data = list.model()->mimeData(modelIndexList)));
      delete data;
    }

    // check the saved data is actually the same
    QMimeData *data2;
    data = list.mimeData(tableWidgetItemList);
    data2 = list.model()->mimeData(modelIndexList);

    const QString format = QStringLiteral("application/x-qabstractitemmodeldatalist");

    QVERIFY(data->hasFormat(format));
    QVERIFY(data2->hasFormat(format));
    QCOMPARE(data->data(format), data2->data(format));

    delete data;
    delete data2;
}

void tst_QListWidget::QTBUG50891_ensureSelectionModelSignalConnectionsAreSet()
{
    QListWidget list;
    for (int i = 0 ; i < 4; ++i)
        new QListWidgetItem(QString::number(i), &list);

    list.setSelectionModel(new QItemSelectionModel(list.model()));
    list.show();
    QVERIFY(QTest::qWaitForWindowExposed(&list));

    QSignalSpy currentItemChangedSpy(&list, &QListWidget::currentItemChanged);
    QSignalSpy itemSelectionChangedSpy(&list, &QListWidget::itemSelectionChanged);

    QCOMPARE(currentItemChangedSpy.count(), 0);
    QCOMPARE(itemSelectionChangedSpy.count(), 0);

    QTest::mouseClick(list.viewport(), Qt::LeftButton, {},
                      list.visualItemRect(list.item(2)).center());

    QCOMPARE(currentItemChangedSpy.count(), 1);
    QCOMPARE(itemSelectionChangedSpy.count(), 1);

}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void tst_QListWidget::clearItemData()
{
    QListWidget list;
    for (int i = 0 ; i < 4; ++i)
        new QListWidgetItem(QString::number(i), &list);
    QSignalSpy dataChangeSpy(list.model(), &QAbstractItemModel::dataChanged);
    QVERIFY(dataChangeSpy.isValid());
    QVERIFY(!list.model()->clearItemData(QModelIndex()));
    QCOMPARE(dataChangeSpy.size(), 0);
    QVERIFY(list.model()->clearItemData(list.model()->index(0, 0)));
    QVERIFY(!list.model()->index(0, 0).data().isValid());
    QCOMPARE(dataChangeSpy.size(), 1);
    const QList<QVariant> dataChangeArgs = dataChangeSpy.takeFirst();
    QCOMPARE(dataChangeArgs.at(0).value<QModelIndex>(), list.model()->index(0, 0));
    QCOMPARE(dataChangeArgs.at(1).value<QModelIndex>(), list.model()->index(0, 0));
    QVERIFY(dataChangeArgs.at(2).value<QVector<int>>().isEmpty());
    QVERIFY(list.model()->clearItemData(list.model()->index(0, 0)));
    QCOMPARE(dataChangeSpy.size(), 0);
}
#endif

QTEST_MAIN(tst_QListWidget)
#include "tst_qlistwidget.moc"
