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

#include <QHeaderView>
#include <QLineEdit>
#include <QMimeData>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTest>

class QObjectTableItem : public QObject, public QTableWidgetItem
{
    Q_OBJECT
};

class tst_QTableWidget : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

private slots:
    void initTestCase();
    void init();
    void getSetCheck();
    void clear();
    void clearContents();
    void rowCount();
    void columnCount();
    void itemAssignment();
    void item_data();
    void item();
    void takeItem_data();
    void takeItem();
    void selectedItems_data();
    void selectedItems();
    void removeRow_data();
    void removeRow();
    void removeColumn_data();
    void removeColumn();
    void insertRow_data();
    void insertRow();
    void insertColumn_data();
    void insertColumn();
    void itemStreaming_data();
    void itemStreaming();
    void itemOwnership();
    void sortItems_data();
    void sortItems();
    void setItemWithSorting_data();
    void setItemWithSorting();
    void itemData();
    void setItemData();
    void cellWidget();
    void cellWidgetGeometry();
    void sizeHint_data();
    void sizeHint();
    void task231094();
    void task219380_removeLastRow();
    void task262056_sortDuplicate();
    void itemWithHeaderItems();
    void mimeData();
    void selectedRowAfterSorting();
    void search();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void clearItemData();
#endif

private:
    std::unique_ptr<QTableWidget> testWidget;
};

using IntPair = QPair<int, int>;
using IntList = QVector<int>;
using IntIntList = QVector<IntPair>;

Q_DECLARE_METATYPE(QTableWidgetSelectionRange)


// Testing get/set functions
void tst_QTableWidget::getSetCheck()
{
    QTableWidget obj1;
    // int QTableWidget::rowCount()
    // void QTableWidget::setRowCount(int)
    obj1.setRowCount(0);
    QCOMPARE(0, obj1.rowCount());
    obj1.setRowCount(INT_MIN);
    QCOMPARE(0, obj1.rowCount()); // Row count can never be negative
//    obj1.setRowCount(INT_MAX);
//    QCOMPARE(INT_MAX, obj1.rowCount());
    obj1.setRowCount(100);
    QCOMPARE(100, obj1.rowCount());


    // int QTableWidget::columnCount()
    // void QTableWidget::setColumnCount(int)
    obj1.setColumnCount(0);
    QCOMPARE(0, obj1.columnCount());
    obj1.setColumnCount(INT_MIN);
    QCOMPARE(0, obj1.columnCount()); // Column count can never be negative
    obj1.setColumnCount(1000);
    QCOMPARE(1000, obj1.columnCount());
//    obj1.setColumnCount(INT_MAX);
//    QCOMPARE(INT_MAX, obj1.columnCount());

    // QTableWidgetItem * QTableWidget::currentItem()
    // void QTableWidget::setCurrentItem(QTableWidgetItem *)
    QTableWidgetItem *var3 = new QTableWidgetItem("0,0");
    obj1.setItem(0, 0, var3);
    obj1.setItem(1, 1, new QTableWidgetItem("1,1"));
    obj1.setItem(2, 2, new QTableWidgetItem("2,2"));
    obj1.setItem(3, 3, new QTableWidgetItem("3,3"));
    obj1.setCurrentItem(var3);
    QCOMPARE(var3, obj1.currentItem());
    obj1.setCurrentItem(nullptr);
    QCOMPARE(obj1.currentItem(), nullptr);
    obj1.setItem(0, 0, nullptr);
    QCOMPARE(obj1.item(0, 0), nullptr);

    // const QTableWidgetItem * QTableWidget::itemPrototype()
    // void QTableWidget::setItemPrototype(const QTableWidgetItem *)
    const QTableWidgetItem *var4 = new QTableWidgetItem;
    obj1.setItemPrototype(var4);
    QCOMPARE(var4, obj1.itemPrototype());
    obj1.setItemPrototype(nullptr);
    QCOMPARE(obj1.itemPrototype(), nullptr);
}

void tst_QTableWidget::initTestCase()
{
    testWidget.reset(new QTableWidget);
    testWidget->show();
    QApplication::setKeyboardInputInterval(100);
}

void tst_QTableWidget::init()
{
    testWidget->clear();
    testWidget->setRowCount(5);
    testWidget->setColumnCount(5);

    for (int row = 0; row < testWidget->rowCount(); ++row)
        testWidget->showRow(row);
    for (int column = 0; column < testWidget->columnCount(); ++column)
        testWidget->showColumn(column);
}

void tst_QTableWidget::clearContents()
{
    QTableWidgetItem *item = new QTableWidgetItem("test");
    testWidget->setHorizontalHeaderItem(0, item);
    QCOMPARE(testWidget->horizontalHeaderItem(0), item);
    testWidget->clearContents();
    QCOMPARE(testWidget->horizontalHeaderItem(0), item);
}

void tst_QTableWidget::clear()
{
    QTableWidgetItem *item = new QTableWidgetItem("foo");
    testWidget->setItem(0, 0, item);
    item->setSelected(true);

    QVERIFY(testWidget->item(0, 0) == item);
    QVERIFY(item->isSelected());


    QPointer<QObjectTableItem> bla = new QObjectTableItem();
    testWidget->setItem(1, 1, bla);

    testWidget->clear();

    QVERIFY(bla.isNull());

    QVERIFY(!testWidget->item(0,0));
    QVERIFY(!testWidget->selectedRanges().count());
    QVERIFY(!testWidget->selectedItems().count());
}

void tst_QTableWidget::rowCount()
{
    int rowCountBefore = 5;
    int rowCountAfter = 10;

    int rowCount = testWidget->rowCount();
    QCOMPARE(rowCount, rowCountBefore);

    testWidget->setRowCount(rowCountAfter);
    rowCount = testWidget->rowCount();
    QCOMPARE(rowCount, rowCountAfter);

    QPersistentModelIndex index(testWidget->model()->index(rowCountAfter - 1, 0,
                                                           testWidget->rootIndex()));
    QCOMPARE(index.row(), rowCountAfter - 1);
    QCOMPARE(index.column(), 0);
    QVERIFY(index.isValid());
    testWidget->setRowCount(rowCountBefore);
    QCOMPARE(index.row(), -1);
    QCOMPARE(index.column(), -1);
    QVERIFY(!index.isValid());

    rowCountBefore = testWidget->rowCount();
    testWidget->setRowCount(-1);
    QCOMPARE(testWidget->rowCount(), rowCountBefore);
}

void tst_QTableWidget::columnCount()
{
    int columnCountBefore = 5;
    int columnCountAfter = 10;

    int columnCount = testWidget->columnCount();
    QCOMPARE(columnCount, columnCountBefore);

    testWidget->setColumnCount(columnCountAfter);
    columnCount = testWidget->columnCount();
    QCOMPARE(columnCount, columnCountAfter);

    QPersistentModelIndex index(testWidget->model()->index(0, columnCountAfter - 1,
                                                           testWidget->rootIndex()));
    QCOMPARE(index.row(), 0);
    QCOMPARE(index.column(), columnCountAfter - 1);
    QVERIFY(index.isValid());
    testWidget->setColumnCount(columnCountBefore);
    QCOMPARE(index.row(), -1);
    QCOMPARE(index.column(), -1);
    QVERIFY(!index.isValid());

    columnCountBefore = testWidget->columnCount();
    testWidget->setColumnCount(-1);
    QCOMPARE(testWidget->columnCount(), columnCountBefore);
}

void tst_QTableWidget::itemAssignment()
{
    QTableWidgetItem itemInWidget("inWidget");
    testWidget->setItem(0, 0, &itemInWidget);
    itemInWidget.setFlags(itemInWidget.flags() | Qt::ItemIsUserTristate);
    QTableWidgetItem itemOutsideWidget("outsideWidget");

    QVERIFY(itemInWidget.tableWidget());
    QCOMPARE(itemInWidget.text(), QString("inWidget"));
    QVERIFY(itemInWidget.flags() & Qt::ItemIsUserTristate);

    QVERIFY(!itemOutsideWidget.tableWidget());
    QCOMPARE(itemOutsideWidget.text(), QString("outsideWidget"));
    QVERIFY(!(itemOutsideWidget.flags() & Qt::ItemIsUserTristate));

    itemOutsideWidget = itemInWidget;
    QVERIFY(!itemOutsideWidget.tableWidget());
    QCOMPARE(itemOutsideWidget.text(), QString("inWidget"));
    QVERIFY(itemOutsideWidget.flags() & Qt::ItemIsUserTristate);
}

void tst_QTableWidget::item_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");
    QTest::addColumn<bool>("expectItem");

    QTest::newRow("0x0 take [0,0]") << 0 << 0 << 0 << 0 << false;
    QTest::newRow("0x0 take [4,4]") << 0 << 0 << 4 << 4 << false;
    QTest::newRow("4x4 take [0,0]") << 4 << 4 << 0 << 0 << true;
    QTest::newRow("4x4 take [4,4]") << 4 << 4 << 4 << 4 << false;
    QTest::newRow("4x4 take [2,2]") << 4 << 4 << 2 << 2 << true;
}

void tst_QTableWidget::item()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, row);
    QFETCH(int, column);
    QFETCH(bool, expectItem);

    testWidget->setRowCount(rowCount);
    testWidget->setColumnCount(columnCount);
    QCOMPARE(testWidget->rowCount(), rowCount);
    QCOMPARE(testWidget->columnCount(), columnCount);

    for (int r = 0; r < testWidget->rowCount(); ++r) {
        for (int c = 0; c < testWidget->columnCount(); ++c)
            testWidget->setItem(r, c, new QTableWidgetItem(QString::number(r * c + c)));
    }

    for (int r = 0; r < testWidget->rowCount(); ++r) {
        for (int c = 0; c < testWidget->columnCount(); ++c)
            QCOMPARE(testWidget->item(r, c)->text(), QString::number(r * c + c));
    }

    QTableWidgetItem *item = testWidget->item(row, column);
    QCOMPARE(!!item, expectItem);
    if (expectItem)
        QCOMPARE(item->text(), QString::number(row * column + column));
}

void tst_QTableWidget::takeItem_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");
    QTest::addColumn<bool>("expectItem");

    QTest::newRow("0x0 take [0,0]") << 0 << 0 << 0 << 0 << false;
    QTest::newRow("0x0 take [4,4]") << 0 << 0 << 4 << 4 << false;
    QTest::newRow("4x4 take [0,0]") << 4 << 4 << 0 << 0 << true;
    QTest::newRow("4x4 take [4,4]") << 4 << 4 << 4 << 4 << false;
    QTest::newRow("4x4 take [2,2]") << 4 << 4 << 2 << 2 << true;
}

void tst_QTableWidget::takeItem()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, row);
    QFETCH(int, column);
    QFETCH(bool, expectItem);

    testWidget->setRowCount(rowCount);
    testWidget->setColumnCount(columnCount);
    QCOMPARE(testWidget->rowCount(), rowCount);
    QCOMPARE(testWidget->columnCount(), columnCount);

    for (int r = 0; r < testWidget->rowCount(); ++r) {
        for (int c = 0; c < testWidget->columnCount(); ++c)
            testWidget->setItem(r, c, new QTableWidgetItem(QString::number(r * c + c)));
    }

    for (int r = 0; r < testWidget->rowCount(); ++r) {
        for (int c = 0; c < testWidget->columnCount(); ++c)
            QCOMPARE(testWidget->item(r, c)->text(), QString::number(r * c + c));
    }

    QSignalSpy spy(testWidget.get(), &QTableWidget::cellChanged);
    QTableWidgetItem *item = testWidget->takeItem(row, column);
    QCOMPARE(!!item, expectItem);
    if (expectItem) {
        QCOMPARE(item->text(), QString::number(row * column + column));
        delete item;

        QTRY_COMPARE(spy.count(), 1);
        const QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(arguments.at(0).toInt(), row);
        QCOMPARE(arguments.at(1).toInt(), column);
    }
    QVERIFY(!testWidget->takeItem(row, column));
}

void tst_QTableWidget::selectedItems_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<IntIntList>("createItems");
    QTest::addColumn<IntList>("hiddenRows");
    QTest::addColumn<IntList>("hiddenColumns");
    QTest::addColumn<QTableWidgetSelectionRange>("selectionRange");
    QTest::addColumn<IntIntList>("expectedItems");

    QTest::newRow("3x3 empty cells, no hidden rows/columns, none selected")
        << 3 << 3
        << IntIntList()
        << IntList()
        << IntList()
        << QTableWidgetSelectionRange()
        << IntIntList();

    QTest::newRow("3x3 empty cells,no hidden rows/columnms, all selected")
        << 3 << 3
        << IntIntList()
        << IntList()
        << IntList()
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << IntIntList();

    QTest::newRow("3x3 (1,1) exists, no hidden rows/columnms, all selected")
        << 3 << 3
        << (IntIntList() << IntPair(1,1))
        << IntList()
        << IntList()
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << (IntIntList() << IntPair(1,1));

    QTest::newRow("3x3 (1,1) exists, row 1 hidden, all selected")
        << 3 << 3
        << (IntIntList() << IntPair(1,1))
        << (IntList() << 1)
        << IntList()
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << IntIntList();

    QTest::newRow("3x3 (1,1) exists, column 1 hidden, all selected")
        << 3 << 3
        << (IntIntList() << IntPair(1,1))
        << IntList()
        << (IntList() << 1)
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << IntIntList();

    QTest::newRow("3x3 all exists, no hidden rows/columns, all selected")
        << 3 << 3
        << (IntIntList()
            << IntPair(0,0) << IntPair(0,1) << IntPair(0,2)
            << IntPair(1,0) << IntPair(1,1) << IntPair(1,2)
            << IntPair(2,0) << IntPair(2,1) << IntPair(2,2))
        << IntList()
        << IntList()
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << (IntIntList()
            << IntPair(0,0) << IntPair(0,1) << IntPair(0,2)
            << IntPair(1,0) << IntPair(1,1) << IntPair(1,2)
            << IntPair(2,0) << IntPair(2,1) << IntPair(2,2));

    QTest::newRow("3x3 all exists, row 1 hidden, all selected")
        << 3 << 3
        << (IntIntList()
            << IntPair(0,0) << IntPair(0,1) << IntPair(0,2)
            << IntPair(1,0) << IntPair(1,1) << IntPair(1,2)
            << IntPair(2,0) << IntPair(2,1) << IntPair(2,2))
        << (IntList() << 1)
        << IntList()
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << (IntIntList()
            << IntPair(0,0) << IntPair(0,1) << IntPair(0,2)
            << IntPair(2,0) << IntPair(2,1) << IntPair(2,2));

    QTest::newRow("3x3 all exists, column 1 hidden, all selected")
        << 3 << 3
        << (IntIntList()
            << IntPair(0,0) << IntPair(0,1) << IntPair(0,2)
            << IntPair(1,0) << IntPair(1,1) << IntPair(1,2)
            << IntPair(2,0) << IntPair(2,1) << IntPair(2,2))
        << IntList()
        << (IntList() << 1)
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << (IntIntList()
            << IntPair(0,0) << IntPair(0,2)
            << IntPair(1,0) << IntPair(1,2)
            << IntPair(2,0) << IntPair(2,2));

    QTest::newRow("3x3 none exists, no hidden rows/columns, all selected")
        << 3 << 3
        << IntIntList()
        << IntList()
        << IntList()
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << IntIntList();
//         << (IntIntList()
//             << IntPair(0,0) << IntPair(0,1) << IntPair(0,2)
//             << IntPair(1,0) << IntPair(1,1) << IntPair(1,2)
//             << IntPair(2,0) << IntPair(2,1) << IntPair(2,2));

    QTest::newRow("3x3 none exists,  row 1 hidden, all selected, filling empty cells")
        << 3 << 3
        << IntIntList()
        << (IntList() << 1)
        << IntList()
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << IntIntList();
//         << (IntIntList()
//             << IntPair(0,0) << IntPair(0,1) << IntPair(0,2)
//             << IntPair(2,0) << IntPair(2,1) << IntPair(2,2));

    QTest::newRow("3x3 none exists,  column 1 hidden, all selected")
        << 3 << 3
        << IntIntList()
        << IntList()
        << (IntList() << 1)
        << QTableWidgetSelectionRange(0, 0, 2, 2)
        << IntIntList();
//         << (IntIntList()
//             << IntPair(0,0) << IntPair(0,2)
//             << IntPair(1,0) << IntPair(1,2)
//             << IntPair(2,0) << IntPair(2,2));
}

void tst_QTableWidget::selectedItems()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(const IntIntList, createItems);
    QFETCH(const IntList, hiddenRows);
    QFETCH(const IntList, hiddenColumns);
    QFETCH(const QTableWidgetSelectionRange, selectionRange);
    QFETCH(const IntIntList, expectedItems);

    // set dimensions and test they are ok
    testWidget->setRowCount(rowCount);
    testWidget->setColumnCount(columnCount);
    QCOMPARE(testWidget->rowCount(), rowCount);
    QCOMPARE(testWidget->columnCount(), columnCount);

    // create and set items
    for (const auto &intPair : createItems) {
        testWidget->setItem(intPair.first, intPair.second,
                            new QTableWidgetItem(QString("Item %1 %2")
                                                 .arg(intPair.first).arg(intPair.second)));
    }
    // hide rows/columns
    for (int row : hiddenRows)
        testWidget->setRowHidden(row, true);
    for (int column : hiddenColumns)
        testWidget->setColumnHidden(column, true);

    // make sure we don't have any previous selections hanging around
    QVERIFY(!testWidget->selectedRanges().count());
    QVERIFY(!testWidget->selectedItems().count());

    // select range and check that it is set correctly
    testWidget->setRangeSelected(selectionRange, true);
    if (selectionRange.topRow() >= 0) {
        QCOMPARE(testWidget->selectedRanges().count(), 1);
        QCOMPARE(testWidget->selectedRanges().at(0).topRow(), selectionRange.topRow());
        QCOMPARE(testWidget->selectedRanges().at(0).bottomRow(), selectionRange.bottomRow());
        QCOMPARE(testWidget->selectedRanges().at(0).leftColumn(), selectionRange.leftColumn());
        QCOMPARE(testWidget->selectedRanges().at(0).rightColumn(), selectionRange.rightColumn());
    } else {
        QCOMPARE(testWidget->selectedRanges().count(), 0);
    }

    // check that the correct number of items and the expected items are there
    const QList<QTableWidgetItem *> selectedItems = testWidget->selectedItems();
    QCOMPARE(selectedItems.count(), expectedItems.count());
    for (const auto &intPair : expectedItems)
        QVERIFY(selectedItems.contains(testWidget->item(intPair.first, intPair.second)));

    // check that setItemSelected agrees with selectedItems
    for (int row = 0; row < testWidget->rowCount(); ++row) {
        if (hiddenRows.contains(row))
            continue;

        for (int column = 0; column < testWidget->columnCount(); ++column) {
            if (hiddenColumns.contains(column))
                continue;

            QTableWidgetItem *item = testWidget->item(row, column);
            if (item && item->isSelected())
                QVERIFY(selectedItems.contains(item));
        }
    }
}

void tst_QTableWidget::removeRow_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("expectedRowCount");
    QTest::addColumn<int>("expectedColumnCount");

    QTest::newRow("Empty") << 0 << 0 << 0 << 0 << 0;
    QTest::newRow("1x1:0") << 1 << 1 << 0 << 0 << 1;
    QTest::newRow("3x3:0") << 3 << 3 << 0 << 2 << 3;
    QTest::newRow("3x3:1") << 3 << 3 << 1 << 2 << 3;
    QTest::newRow("3x3:2") << 3 << 3 << 2 << 2 << 3;
}

void tst_QTableWidget::removeRow()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, row);
    QFETCH(int, expectedRowCount);
    QFETCH(int, expectedColumnCount);

    // set dimensions and test they are ok
    testWidget->setRowCount(rowCount);
    testWidget->setColumnCount(columnCount);
    QCOMPARE(testWidget->rowCount(), rowCount);
    QCOMPARE(testWidget->columnCount(), columnCount);

    // fill table with items
    for (int r = 0; r < rowCount; ++r) {
        const QString rRow = QString::number(r) + QLatin1Char(':');
        for (int c = 0; c < columnCount; ++c) {
            testWidget->setItem(r, c,
                                new QTableWidgetItem(rRow + QString::number(c)));
        }
    }

    // remove and compare the results
    testWidget->removeRow(row);
    QCOMPARE(testWidget->rowCount(), expectedRowCount);
    QCOMPARE(testWidget->columnCount(), expectedColumnCount);

    // check if the correct items were removed
    for (int r = 0; r < expectedRowCount; ++r) {
        const QString rRow = QString::number(r < row ? r : r + 1) +
            QLatin1Char(':');
        for (int c = 0; c < expectedColumnCount; ++c) {
            if (r < row)
                QCOMPARE(testWidget->item(r, c)->text(),
                         rRow + QString::number(c));
            else
                QCOMPARE(testWidget->item(r, c)->text(),
                         rRow + QString::number(c));
        }
    }
}

void tst_QTableWidget::removeColumn_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("expectedRowCount");
    QTest::addColumn<int>("expectedColumnCount");

    QTest::newRow("Empty") << 0 << 0 << 0 << 0 << 0;
    QTest::newRow("1x1:0") << 1 << 1 << 0 << 1 << 0;
    QTest::newRow("3x3:0") << 3 << 3 << 0 << 3 << 2;
    QTest::newRow("3x3:1") << 3 << 3 << 1 << 3 << 2;
    QTest::newRow("3x3:2") << 3 << 3 << 2 << 3 << 2;
}

void tst_QTableWidget::removeColumn()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, column);
    QFETCH(int, expectedRowCount);
    QFETCH(int, expectedColumnCount);

    // set dimensions and test they are ok
    testWidget->setRowCount(rowCount);
    testWidget->setColumnCount(columnCount);
    QCOMPARE(testWidget->rowCount(), rowCount);
    QCOMPARE(testWidget->columnCount(), columnCount);

    // fill table with items
    for (int r = 0; r < rowCount; ++r) {
        const QString rStr = QString::number(r) + QLatin1Char(':');
        for (int c = 0; c < columnCount; ++c) {
            testWidget->setItem(r, c,
                                new QTableWidgetItem(
                                    rStr + QString::number(c)));
        }
    }

    // remove and compare the results
    testWidget->removeColumn(column);
    QCOMPARE(testWidget->rowCount(), expectedRowCount);
    QCOMPARE(testWidget->columnCount(), expectedColumnCount);


    // check if the correct items were removed
    for (int r = 0; r < expectedRowCount; ++r) {
        const QString rStr = QString::number(r) + QLatin1Char(':');
        for (int c = 0; c < expectedColumnCount; ++c) {
            if (c < column)
                QCOMPARE(testWidget->item(r, c)->text(),
                         rStr + QString::number(c));
            else
                QCOMPARE(testWidget->item(r, c)->text(),
                         rStr + QString::number(c + 1));
        }
    }
}

void tst_QTableWidget::insertRow_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("expectedRowCount");
    QTest::addColumn<int>("expectedColumnCount");

    QTest::newRow("Empty")  << 0 << 0 << 0  << 1 << 0;
    QTest::newRow("1x1:0")  << 1 << 1 << 0  << 2 << 1;
    QTest::newRow("3x3:-1") << 3 << 3 << -1 << 3 << 3;
    QTest::newRow("3x3:0")  << 3 << 3 << 0  << 4 << 3;
    QTest::newRow("3x3:1")  << 3 << 3 << 1  << 4 << 3;
    QTest::newRow("3x3:2")  << 3 << 3 << 2  << 4 << 3;
    QTest::newRow("3x3:3")  << 3 << 3 << 3  << 4 << 3;
    QTest::newRow("3x3:4")  << 3 << 3 << 4  << 3 << 3;
}

void tst_QTableWidget::insertRow()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, row);
    QFETCH(int, expectedRowCount);
    QFETCH(int, expectedColumnCount);

    // set dimensions and test they are ok
    testWidget->setRowCount(rowCount);
    testWidget->setColumnCount(columnCount);
    QCOMPARE(testWidget->rowCount(), rowCount);
    QCOMPARE(testWidget->columnCount(), columnCount);

    // insert and compare the results
    testWidget->insertRow(row);
    QCOMPARE(testWidget->rowCount(), expectedRowCount);
    QCOMPARE(testWidget->columnCount(), expectedColumnCount);
}

void tst_QTableWidget::insertColumn_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("expectedRowCount");
    QTest::addColumn<int>("expectedColumnCount");

    QTest::newRow("Empty")  << 0 << 0 << 0  << 0 << 1;
    QTest::newRow("1x1:0")  << 1 << 1 << 0  << 1 << 2;
    QTest::newRow("3x3:-1") << 3 << 3 << -1 << 3 << 3;
    QTest::newRow("3x3:0")  << 3 << 3 << 0  << 3 << 4;
    QTest::newRow("3x3:1")  << 3 << 3 << 1  << 3 << 4;
    QTest::newRow("3x3:2")  << 3 << 3 << 2  << 3 << 4;
    QTest::newRow("3x3:3")  << 3 << 3 << 3  << 3 << 4;
    QTest::newRow("3x3:4")  << 3 << 3 << 4  << 3 << 3;
}

void tst_QTableWidget::insertColumn()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, column);
    QFETCH(int, expectedRowCount);
    QFETCH(int, expectedColumnCount);

    // set dimensions and test they are ok
    testWidget->setRowCount(rowCount);
    testWidget->setColumnCount(columnCount);
    QCOMPARE(testWidget->rowCount(), rowCount);
    QCOMPARE(testWidget->columnCount(), columnCount);

    // insert and compare the results
    testWidget->insertColumn(column);
    QCOMPARE(testWidget->rowCount(), expectedRowCount);
    QCOMPARE(testWidget->columnCount(), expectedColumnCount);
}

void tst_QTableWidget::itemStreaming_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("toolTip");

    QTest::newRow("Data") << "item text" << "tool tip text";
}

void tst_QTableWidget::itemStreaming()
{
    QFETCH(QString, text);
    QFETCH(QString, toolTip);

    QTableWidgetItem item;
    QCOMPARE(item.text(), QString());
    QCOMPARE(item.toolTip(), QString());

    item.setText(text);
    item.setToolTip(toolTip);
    QCOMPARE(item.text(), text);
    QCOMPARE(item.toolTip(), toolTip);

    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << item;

    QTableWidgetItem item2;
    QCOMPARE(item2.text(), QString());
    QCOMPARE(item2.toolTip(), QString());

    QVERIFY(!buffer.isEmpty());

    QDataStream in(&buffer, QIODevice::ReadOnly);
    in >> item2;
    QCOMPARE(item2.text(), text);
    QCOMPARE(item2.toolTip(), toolTip);
}

void tst_QTableWidget::itemOwnership()
{
    QPointer<QObjectTableItem> item;
    QPointer<QObjectTableItem> headerItem;

    //delete from outside
    item = new QObjectTableItem();
    testWidget->setItem(0, 0, item);
    delete item;
    QCOMPARE(testWidget->item(0, 0), nullptr);

    //delete vertical headeritem from outside
    headerItem = new QObjectTableItem();
    testWidget->setVerticalHeaderItem(0, headerItem);
    delete headerItem;
    QCOMPARE(testWidget->verticalHeaderItem(0), nullptr);

    //delete horizontal headeritem from outside
    headerItem = new QObjectTableItem();
    testWidget->setHorizontalHeaderItem(0, headerItem);
    delete headerItem;
    QCOMPARE(testWidget->horizontalHeaderItem(0), nullptr);

    //setItem
    item = new QObjectTableItem();
    testWidget->setItem(0, 0, item);
    testWidget->setItem(0, 0, new QTableWidgetItem());
    QVERIFY(item.isNull());

    //setHorizontalHeaderItem
    headerItem = new QObjectTableItem();
    testWidget->setHorizontalHeaderItem(0, headerItem);
    testWidget->setHorizontalHeaderItem(0, new QTableWidgetItem());
    QVERIFY(headerItem.isNull());

    //setVerticalHeaderItem
    headerItem = new QObjectTableItem();
    testWidget->setVerticalHeaderItem(0, headerItem);
    testWidget->setVerticalHeaderItem(0, new QTableWidgetItem());
    QVERIFY(headerItem.isNull());

    //takeItem
    item = new QObjectTableItem();
    testWidget->setItem(0, 0, item);
    testWidget->takeItem(0, 0);
    QVERIFY(!item.isNull());
    delete item;

    // removeRow
    item = new QObjectTableItem();
    headerItem = new QObjectTableItem();
    testWidget->setItem(0, 0, item);
    testWidget->setVerticalHeaderItem(0, headerItem);
    testWidget->removeRow(0);
    QVERIFY(item.isNull());
    QVERIFY(headerItem.isNull());

    // removeColumn
    item = new QObjectTableItem();
    headerItem = new QObjectTableItem();
    testWidget->setItem(0, 0, item);
    testWidget->setHorizontalHeaderItem(0, headerItem);
    testWidget->removeColumn(0);
    QVERIFY(item.isNull());
    QVERIFY(headerItem.isNull());

    // clear
    item = new QObjectTableItem();
    testWidget->setItem(0, 0, item);
    testWidget->clear();
    QVERIFY(item.isNull());
}

void tst_QTableWidget::sortItems_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<int>("sortColumn");
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QStringList>("expected");
    QTest::addColumn<IntList>("rows");
    QTest::addColumn<IntList>("initialHidden");
    QTest::addColumn<IntList>("expectedHidden");

    QTest::newRow("ascending")
        << 4 << 5
        << Qt::AscendingOrder
        << 0
        << (QStringList()
            << "0" << "a" << "o" << "8" << "k"
            << "3" << "d" << "k" << "o" << "6"
            << "2" << "c" << "9" << "y" << "8"
            << "1" << "b" << "7" << "3" << "u")
        << (QStringList()
            << "0" << "a" << "o" << "8" << "k"
            << "1" << "b" << "7" << "3" << "u"
            << "2" << "c" << "9" << "y" << "8"
            << "3" << "d" << "k" << "o" << "6")
        << (IntList() << 0 << 3 << 2 << 1)
        << IntList()
        << IntList();

    QTest::newRow("descending")
        << 4 << 5
        << Qt::DescendingOrder
        << 0
        << (QStringList()
            << "0" << "a" << "o" << "8" << "k"
            << "3" << "d" << "k" << "o" << "6"
            << "2" << "c" << "9" << "y" << "8"
            << "1" << "b" << "7" << "3" << "u")
        << (QStringList()
            << "3" << "d" << "k" << "o" << "6"
            << "2" << "c" << "9" << "y" << "8"
            << "1" << "b" << "7" << "3" << "u"
            << "0" << "a" << "o" << "8" << "k")
        << (IntList() << 3 << 0 << 1 << 2)
        << IntList()
        << IntList();

    QTest::newRow("empty table")
        << 4 << 5
        << Qt::AscendingOrder
        << 0
        << (QStringList()
            << QString() << QString() << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString())
        << (QStringList()
            << QString() << QString() << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString())
        << IntList()
        << IntList()
        << IntList();

    QTest::newRow("half-empty table")
        << 4 << 5
        << Qt::AscendingOrder
        << 0
        << (QStringList()
            << "0"       << QString() << QString() << QString() << QString()
            << "3"       << "d"       << QString() << QString() << QString()
            << "2"       << "c"       << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString())
        << (QStringList()
            << "0"       << QString() << QString() << QString() << QString()
            << "2"       << "c"       << QString() << QString() << QString()
            << "3"       << "d"       << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString())
        << (IntList() << 0 << 2 << 1)
        << IntList()
        << IntList();

    QTest::newRow("empty column, should not sort.")
        << 4 << 5
        << Qt::AscendingOrder
        << 3
        << (QStringList()
            << "0"       << QString() << QString() << QString() << QString()
            << "3"       << "d"       << QString() << QString() << QString()
            << "2"       << "c"       << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString())
        << (QStringList()
            << "0"       << QString() << QString() << QString() << QString()
            << "3"       << "d"       << QString() << QString() << QString()
            << "2"       << "c"       << QString() << QString() << QString()
            << QString() << QString() << QString() << QString() << QString())
        << IntList()
        << IntList()
        << IntList();

    QTest::newRow("descending with null cell, the null cell should be placed at the bottom")
        << 4 << 5
        << Qt::DescendingOrder
        << 0
        << (QStringList()
            << "0"       << "a" << "o" << "8" << "k"
            << "3"       << "d" << "k" << "o" << "6"
            << "2"       << "c" << "9" << "y" << "8"
            << QString() << "b" << "7" << "3" << "u")
        << (QStringList()
            << "3"       << "d" << "k" << "o" << "6"
            << "2"       << "c" << "9" << "y" << "8"
            << "0"       << "a" << "o" << "8" << "k"
            << QString() << "b" << "7" << "3" << "u")
        << (IntList() << 2 << 0 << 1)
        << IntList()
        << IntList();

    QTest::newRow("ascending with null cell, the null cell should be placed at the bottom")
        << 4 << 5
        << Qt::AscendingOrder
        << 0
        << (QStringList()
            << "0"       << "a" << "o" << "8" << "k"
            << "3"       << "d" << "k" << "o" << "6"
            << "2"       << "c" << "9" << "y" << "8"
            << QString() << "b" << "7" << "3" << "u")
        << (QStringList()
            << "0"       << "a" << "o" << "8" << "k"
            << "2"       << "c" << "9" << "y" << "8"
            << "3"       << "d" << "k" << "o" << "6"
            << QString() << "b" << "7" << "3" << "u")
        << (IntList() << 0 << 2 << 1)
        << IntList()
        << IntList();

    QTest::newRow("ascending with null cells, the null cells should be placed at the bottom")
        << 4 << 5
        << Qt::AscendingOrder
        << 0
        << (QStringList()
            << "3"       << "d" << "k" << "o" << "6"
            << "0"       << "a" << "o" << "8" << "k"
            << QString() << "c" << "9" << "y" << "8"
            << QString() << "b" << "7" << "3" << "u")
        << (QStringList()
            << "0"       << "a" << "o" << "8" << "k"
            << "3"       << "d" << "k" << "o" << "6"
            << QString() << "c" << "9" << "y" << "8"
            << QString() << "b" << "7" << "3" << "u")
        << (IntList() << 1 << 0)
        << IntList()
        << IntList();

    QTest::newRow("ascending... Check a bug in PersistentIndexes")
        << 4 << 5
        << Qt::AscendingOrder
        << 0
        << (QStringList()
            << "3" << "c" << "9" << "y" << "8"
            << "2" << "b" << "7" << "3" << "u"
            << "4" << "d" << "k" << "o" << "6"
            << "1" << "a" << "o" << "8" << "k")
        << (QStringList()
            << "1" << "a" << "o" << "8" << "k"
            << "2" << "b" << "7" << "3" << "u"
            << "3" << "c" << "9" << "y" << "8"
            << "4" << "d" << "k" << "o" << "6")
        << (IntList() << 2 << 1 << 3 << 0)
        << IntList()
        << IntList();

    QTest::newRow("ascending with some null cells inbetween")
        << 4 << 5
        << Qt::AscendingOrder
        << 0
        << (QStringList()
            << QString() << "a" << "o" << "8" << "k"
            << "2"       << "c" << "9" << "y" << "8"
            << QString() << "d" << "k" << "o" << "6"
            << "1"       << "b" << "7" << "3" << "u")
        << (QStringList()
            << "1"       << "b" << "7" << "3" << "u"
            << "2"       << "c" << "9" << "y" << "8"
            << QString() << "a" << "o" << "8" << "k"
            << QString() << "d" << "k" << "o" << "6")
        << (IntList() << 1 << 0)
        << IntList()
        << IntList();

    QTest::newRow("ascending hidden")
        << 4 << 5
        << Qt::AscendingOrder
        << 0
        << (QStringList()
            << "0" << "a" << "o" << "8" << "k"
            << "3" << "d" << "k" << "o" << "6"
            << "2" << "c" << "9" << "y" << "8"
            << "1" << "b" << "7" << "3" << "u")
        << (QStringList()
            << "0" << "a" << "o" << "8" << "k"
            << "1" << "b" << "7" << "3" << "u"
            << "2" << "c" << "9" << "y" << "8"
            << "3" << "d" << "k" << "o" << "6")
        << (IntList() << 0 << 3 << 2 << 1)
        << (IntList() << 0 << 2)
        << (IntList() << 0 << 2);

    QTest::newRow("descending hidden")
        << 4 << 5
        << Qt::DescendingOrder
        << 0
        << (QStringList()
            << "0" << "a" << "o" << "8" << "k"
            << "3" << "d" << "k" << "o" << "6"
            << "2" << "c" << "9" << "y" << "8"
            << "1" << "b" << "7" << "3" << "u")
        << (QStringList()
            << "3" << "d" << "k" << "o" << "6"
            << "2" << "c" << "9" << "y" << "8"
            << "1" << "b" << "7" << "3" << "u"
            << "0" << "a" << "o" << "8" << "k")
        << (IntList() << 3 << 0 << 1 << 2)
        << (IntList() << 0 << 2)
        << (IntList() << 3 << 1);
}

void tst_QTableWidget::sortItems()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(int, sortColumn);
    QFETCH(QStringList, initial);
    QFETCH(QStringList, expected);
    QFETCH(IntList, rows);
    QFETCH(IntList, initialHidden);
    QFETCH(IntList, expectedHidden);

    testWidget->setRowCount(rowCount);
    testWidget->setColumnCount(columnCount);

    QAbstractItemModel *model = testWidget->model();
    QVector<QPersistentModelIndex> persistent;

    int ti = 0;
    for (int r = 0; r < rowCount; ++r) {
        for (int c = 0; c < columnCount; ++c) {
            QString str = initial.at(ti++);
            if (!str.isEmpty()) {
                testWidget->setItem(r, c, new QTableWidgetItem(str));
            }
        }
        if (testWidget->item(r, sortColumn))
            persistent << model->index(r, sortColumn, QModelIndex());
    }

    for (int h = 0; h < initialHidden.count(); ++h)
        testWidget->hideRow(initialHidden.at(h));

    QCOMPARE(testWidget->verticalHeader()->hiddenSectionCount(), initialHidden.count());

    testWidget->sortItems(sortColumn, sortOrder);

    int te = 0;
    for (int i = 0; i < rows.count(); ++i) {
        for (int j = 0; j < columnCount; ++j) {
            QString value;
            QTableWidgetItem *itm = testWidget->item(i, j);
            if (itm) {
                value = itm->text();
            }
            QCOMPARE(value, expected.at(te++));
        }
        QCOMPARE(persistent.at(i).row(), rows.at(i));
        //qDebug() << "persistent" << persistent.at(i).row()
        //         << "expected" << rows.at(i);
    }

    for (int k = 0; k < expectedHidden.count(); ++k)
        QVERIFY(testWidget->isRowHidden(expectedHidden.at(k)));
}

void tst_QTableWidget::setItemWithSorting_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<int>("sortColumn");
    QTest::addColumn<QStringList>("initialValues");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");
    QTest::addColumn<QString>("newValue");
    QTest::addColumn<QStringList>("expectedValues");
    QTest::addColumn<IntList>("expectedRows");
    QTest::addColumn<bool>("reorderingExpected");

    QTest::newRow("2x1 no change (ascending)")
        << 2 << 1
        << Qt::AscendingOrder << 0
        << (QStringList() << "0" << "1")
        << 1 << 0 << "2"
        << (QStringList() << "0" << "2")
        << (IntList() << 0 << 1)
        << false;
    QTest::newRow("2x1 no change (descending)")
        << 2 << 1
        << Qt::DescendingOrder << 0
        << (QStringList() << "1" << "0")
        << 0 << 0 << "2"
        << (QStringList() << "2" << "0")
        << (IntList() << 0 << 1)
        << false;
    QTest::newRow("2x1 reorder (ascending)")
        << 2 << 1
        << Qt::AscendingOrder << 0
        << (QStringList() << "0" << "1")
        << 0 << 0 << "2"
        << (QStringList() << "1" << "2")
        << (IntList() << 1 << 0)
        << true;
    QTest::newRow("2x1 reorder (descending)")
        << 2 << 1
        << Qt::DescendingOrder << 0
        << (QStringList() << "1" << "0")
        << 1 << 0 << "2"
        << (QStringList() << "2" << "1")
        << (IntList() << 1 << 0)
        << true;
    QTest::newRow("2x2 no change (ascending)")
        << 2 << 2
        << Qt::AscendingOrder << 0
        << (QStringList()
            << "0" << "00"
            << "1" << "11")
        << 1 << 0 << "2"
        << (QStringList()
            << "0" << "00"
            << "2" << "11")
        << (IntList() << 0 << 1)
        << false;
    QTest::newRow("2x2 reorder (ascending)")
        << 2 << 2
        << Qt::AscendingOrder << 0
        << (QStringList()
            << "0" << "00"
            << "1" << "11")
        << 0 << 0 << "2"
        << (QStringList()
            << "1" << "11"
            << "2" << "00")
        << (IntList() << 1 << 0)
        << true;
    QTest::newRow("2x2 reorder (ascending, sortColumn = 1)")
        << 2 << 2
        << Qt::AscendingOrder << 1
        << (QStringList()
            << "00" << "0"
            << "11" << "1")
        << 0 << 1 << "2"
        << (QStringList()
            << "11" << "1"
            << "00" << "2")
        << (IntList() << 1 << 0)
        << true;
    QTest::newRow("2x2 no change (column != sortColumn)")
        << 2 << 2
        << Qt::AscendingOrder << 1
        << (QStringList()
            << "00" << "0"
            << "11" << "1")
        << 0 << 0 << "22"
        << (QStringList()
            << "22" << "0"
            << "11" << "1")
        << (IntList() << 0 << 1)
        << false;
    QTest::newRow("8x4 reorder (ascending, sortColumn = 3)")
        << 8 << 4
        << Qt::AscendingOrder << 3
        << (QStringList()
            << "q" << "v" << "u" << "0"
            << "e" << "j" << "i" << "10"
            << "h" << "d" << "c" << "12"
            << "k" << "g" << "f" << "14"
            << "w" << "y" << "x" << "2"
            << "t" << "s" << "o" << "4"
            << "z" << "p" << "r" << "6"
            << "n" << "m" << "l" << "8")
        << 2 << 3 << "5"
        << (QStringList()
            << "q" << "v" << "u" << "0"
            << "e" << "j" << "i" << "10"
            << "k" << "g" << "f" << "14"
            << "w" << "y" << "x" << "2"
            << "t" << "s" << "o" << "4"
            << "h" << "d" << "c" << "5"
            << "z" << "p" << "r" << "6"
            << "n" << "m" << "l" << "8")
        << (IntList() << 0 << 1 << 5 << 2 << 3 << 4 << 6 << 7)
        << true;
}

void tst_QTableWidget::setItemWithSorting()
{
    static int dummy1 = qRegisterMetaType<QList<QPersistentModelIndex>>();
    static int dummy2 = qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>();
    Q_UNUSED(dummy1);
    Q_UNUSED(dummy2);
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(int, sortColumn);
    QFETCH(QStringList, initialValues);
    QFETCH(int, row);
    QFETCH(int, column);
    QFETCH(QString, newValue);
    QFETCH(QStringList, expectedValues);
    QFETCH(IntList, expectedRows);
    QFETCH(bool, reorderingExpected);

    for (int i = 0; i < 2; ++i) {
        QTableWidget w(rowCount, columnCount);

        QAbstractItemModel *model = w.model();
        QVector<QPersistentModelIndex> persistent;

        int ti = 0;
        for (int r = 0; r < rowCount; ++r) {
            for (int c = 0; c < columnCount; ++c) {
                QString str = initialValues.at(ti++);
                w.setItem(r, c, new QTableWidgetItem(str));
            }
            persistent << model->index(r, sortColumn);
        }

        w.sortItems(sortColumn, sortOrder);
        w.setSortingEnabled(true);

        QSignalSpy dataChangedSpy(model, &QAbstractItemModel::dataChanged);
        QSignalSpy layoutChangedSpy(model, &QAbstractItemModel::layoutChanged);

        if (i == 0) {
            // set a new item
            QTableWidgetItem *item = new QTableWidgetItem(newValue);
            w.setItem(row, column, item);
        } else {
            // change the data of existing item
            QTableWidgetItem *item = w.item(row, column);
            item->setText(newValue);
        }

        ti = 0;
        for (int r = 0; r < rowCount; ++r) {
            for (int c = 0; c < columnCount; ++c) {
                QString str = expectedValues.at(ti++);
                QCOMPARE(w.item(r, c)->text(), str);
            }
        }

        for (int k = 0; k < persistent.count(); ++k) {
            QCOMPARE(persistent.at(k).row(), expectedRows.at(k));
            int i = (persistent.at(k).row() * columnCount) + sortColumn;
            QCOMPARE(persistent.at(k).data().toString(), expectedValues.at(i));
        }

        if (i == 0)
            QCOMPARE(dataChangedSpy.count(), reorderingExpected ? 0 : 1);
        else
            QCOMPARE(dataChangedSpy.count(), 1);

        QCOMPARE(layoutChangedSpy.count(), reorderingExpected ? 1 : 0);
    }
}

class QTableWidgetDataChanged : public QTableWidget
{
    Q_OBJECT
public:
    using QTableWidget::QTableWidget;

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) override
    {
        QTableWidget::dataChanged(topLeft, bottomRight, roles);
        currentRoles = roles;
    }
    QVector<int> currentRoles;
};

void tst_QTableWidget::itemData()
{
    QTableWidgetDataChanged widget(2, 2);
    widget.setItem(0, 0, new QTableWidgetItem);
    QTableWidgetItem *item = widget.item(0, 0);
    QVERIFY(item);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setData(Qt::DisplayRole,  QString("0"));
    QCOMPARE(widget.currentRoles, QVector<int>({Qt::DisplayRole, Qt::EditRole}));
    item->setData(Qt::CheckStateRole, Qt::PartiallyChecked);
    QCOMPARE(widget.currentRoles, QVector<int>{Qt::CheckStateRole});
    for (int i = 0; i < 4; ++i)
    {
        item->setData(Qt::UserRole + i, QString::number(i + 1));
        QCOMPARE(widget.currentRoles, QVector<int>{Qt::UserRole + i});
    }
    QMap<int, QVariant> flags = widget.model()->itemData(widget.model()->index(0, 0));
    QCOMPARE(flags.count(), 6);
    for (int i = 0; i < 4; ++i)
        QCOMPARE(flags[Qt::UserRole + i].toString(), QString::number(i + 1));
}

void tst_QTableWidget::setItemData()
{
    QTableWidgetDataChanged table(10, 10);
    table.setSortingEnabled(false);
    QSignalSpy dataChangedSpy(table.model(), &QAbstractItemModel::dataChanged);

    QTableWidgetItem *item = new QTableWidgetItem;
    table.setItem(0, 0, item);
    QCOMPARE(dataChangedSpy.count(), 1);
    QModelIndex idx = qvariant_cast<QModelIndex>(dataChangedSpy.takeFirst().at(0));

    QMap<int, QVariant> data;
    data.insert(Qt::DisplayRole, QLatin1String("Display"));
    data.insert(Qt::ToolTipRole, QLatin1String("ToolTip"));
    table.model()->setItemData(idx, data);
    QCOMPARE(table.currentRoles, QVector<int>({Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole}));

    QCOMPARE(table.model()->data(idx, Qt::DisplayRole).toString(), QLatin1String("Display"));
    QCOMPARE(table.model()->data(idx, Qt::EditRole).toString(), QLatin1String("Display"));
    QCOMPARE(table.model()->data(idx, Qt::ToolTipRole).toString(), QLatin1String("ToolTip"));
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(idx, qvariant_cast<QModelIndex>(dataChangedSpy.first().at(0)));
    QCOMPARE(idx, qvariant_cast<QModelIndex>(dataChangedSpy.first().at(1)));
    const auto roles = qvariant_cast<QVector<int>>(dataChangedSpy.first().at(2));
    QCOMPARE(roles.size(), 3);
    QVERIFY(roles.contains(Qt::DisplayRole));
    QVERIFY(roles.contains(Qt::EditRole));
    QVERIFY(roles.contains(Qt::ToolTipRole));
    dataChangedSpy.clear();

    table.model()->setItemData(idx, data);
    QCOMPARE(dataChangedSpy.count(), 0);

    data.clear();
    data.insert(Qt::DisplayRole, QLatin1String("dizplaye"));
    table.model()->setItemData(idx, data);
    QCOMPARE(table.model()->data(idx, Qt::DisplayRole).toString(), QLatin1String("dizplaye"));
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(QVector<int>({Qt::DisplayRole, Qt::EditRole}), qvariant_cast<QVector<int>>(dataChangedSpy.first().at(2)));

    item->setBackground(QBrush(Qt::red));
    item->setForeground(QBrush(Qt::green));
    item->setSizeHint(QSize(10, 10));
    QCOMPARE(item->data(Qt::BackgroundRole), QVariant(QBrush(Qt::red)));
    QCOMPARE(item->data(Qt::ForegroundRole), QVariant(QBrush(Qt::green)));
    QCOMPARE(item->data(Qt::SizeHintRole), QVariant(QSize(10, 10)));
    // an empty brush should result in a QVariant()
    item->setBackground(QBrush());
    item->setForeground(QBrush());
    item->setSizeHint(QSize());
    QCOMPARE(item->data(Qt::BackgroundRole), QVariant());
    QCOMPARE(item->data(Qt::ForegroundRole), QVariant());
    QCOMPARE(item->data(Qt::SizeHintRole), QVariant());
}

void tst_QTableWidget::cellWidget()
{
    QTableWidget table(10, 10);
    QWidget widget;

    QCOMPARE(table.cellWidget(5, 5), nullptr);
    table.setCellWidget(5, 5, &widget);
    QCOMPARE(table.cellWidget(5, 5), &widget);
    table.removeCellWidget(5, 5);
    QCOMPARE(table.cellWidget(5, 5), nullptr);
}

void tst_QTableWidget::cellWidgetGeometry()
{
    QTableWidget tw(3,2);
    tw.show();
    // make sure the next row added is not completely visibile
    tw.resize(300, tw.rowHeight(0) * 3 + tw.rowHeight(0) / 2);
    QVERIFY(QTest::qWaitForWindowExposed(&tw));

    tw.scrollToBottom();
    tw.setRowCount(tw.rowCount() + 1);
    auto item = new QTableWidgetItem("Hello");
    tw.setItem(0,0,item);
    auto le = new QLineEdit("world");
    tw.setCellWidget(0,1,le);
    // process delayedPendingLayout triggered by setting the row count
    tw.doItemsLayout();
    // so y pos is 0 for the first row
    tw.scrollToTop();
    // check if updateEditorGeometries has set the correct y pos for the editors
    QCOMPARE(tw.visualItemRect(item).top(), le->geometry().top());
}

void tst_QTableWidget::sizeHint_data()
{
    QTest::addColumn<Qt::ScrollBarPolicy>("scrollBarPolicy");
    QTest::addColumn<QSize>("viewSize");
    QTest::newRow("ScrollBarAlwaysOn") << Qt::ScrollBarAlwaysOn << QSize();
    QTest::newRow("ScrollBarAlwaysOff") << Qt::ScrollBarAlwaysOff << QSize();
    // make sure the scrollbars are shown by resizing the view to 40x40
    QTest::newRow("ScrollBarAsNeeded (40x40)") << Qt::ScrollBarAsNeeded << QSize(40, 40);
    QTest::newRow("ScrollBarAsNeeded (1000x1000)") << Qt::ScrollBarAsNeeded << QSize(1000, 1000);
}

void tst_QTableWidget::sizeHint()
{
    QFETCH(Qt::ScrollBarPolicy, scrollBarPolicy);
    QFETCH(QSize, viewSize);

    QTableWidget view(2, 2);
    view.setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    view.setVerticalScrollBarPolicy(scrollBarPolicy);
    view.setHorizontalScrollBarPolicy(scrollBarPolicy);
    for (int r = 0 ; r < view.rowCount(); ++r) {
        const QString rStr = QString::number(r) + QLatin1Char('/');
        for (int c = 0 ; c < view.columnCount(); ++c)
            view.setItem(r, c, new QTableWidgetItem(rStr + QString::number(c)));
    }

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    if (viewSize.isValid()) {
        view.resize(viewSize);
        view.setColumnWidth(0, 100);
        view.setRowHeight(0, 100);
        QTRY_COMPARE(view.size(), viewSize);
    }

    auto sizeHint = view.sizeHint();
    view.hide();
    QCOMPARE(view.sizeHint(), sizeHint);

    view.horizontalHeader()->hide();
    view.show();
    sizeHint = view.sizeHint();
    view.hide();
    QCOMPARE(view.sizeHint(), sizeHint);

    view.verticalHeader()->hide();
    view.show();
    sizeHint = view.sizeHint();
    view.hide();
    QCOMPARE(view.sizeHint(), sizeHint);
}

void tst_QTableWidget::task231094()
{
    QTableWidget tw(5, 3);
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 5; y++) {
            QTableWidgetItem *twi = new QTableWidgetItem(QLatin1String("1"));
            if (y == 1)
                twi->setFlags(Qt::ItemIsEnabled);
            else
                twi->setFlags({});
            tw.setItem(y, x, twi);
        }
    }

    tw.setCurrentCell(1, 1);
    QCOMPARE(tw.currentRow(), 1);
    QCOMPARE(tw.currentColumn(), 1);

    //this would provoke a end-less loop
    QTest::keyClick(&tw, '1');

    //all the items are disabled: the current item shouldn't have changed
    QCOMPARE(tw.currentRow(), 1);
    QCOMPARE(tw.currentColumn(), 1);
}

void tst_QTableWidget::task219380_removeLastRow()
{
    testWidget->setColumnCount(1);
    testWidget->setRowCount(20);
    QTableWidgetItem item;
    testWidget->setItem(18, 0, &item); //we put the item in the second last row
    testWidget->openPersistentEditor(&item);

    testWidget->scrollToBottom();

    testWidget->removeRow(19); //we remove the last row

    //we make sure the editor is at the cell position
    QTRY_COMPARE(testWidget->cellWidget(18, 0)->geometry(), testWidget->visualItemRect(&item));
}

void tst_QTableWidget::task262056_sortDuplicate()
{
    testWidget->setColumnCount(2);
    testWidget->setRowCount(8);
    testWidget->setSortingEnabled(true);
    const QStringList items({"AAA", "BBB", "CCC", "CCC", "DDD",
                             "EEE", "FFF", "GGG"});
    for (int i = 0; i < 8; i++ ) {
        QTableWidgetItem *twi = new QTableWidgetItem(items.at(i));
        testWidget->setItem(i, 0, twi);
        testWidget->setItem(i, 1, new QTableWidgetItem(QLatin1String("item ") + QString::number(i)));
    }
    testWidget->sortItems(0, Qt::AscendingOrder);
    QSignalSpy layoutChangedSpy(testWidget->model(), &QAbstractItemModel::layoutChanged);
    testWidget->item(3,0)->setBackground(Qt::red);

    QCOMPARE(layoutChangedSpy.count(),0);

}

void tst_QTableWidget::itemWithHeaderItems()
{
    // Need a separate testcase for this because the tst_QTableWidget::item testcase
    // does creates QTableWidgetItems for each available cell in the table. We're testing
    // the case of not all available cells having a QTableWidgetItem set.
    QTableWidget table(2, 1);

    QTableWidgetItem *item0_0 = new QTableWidgetItem(QTableWidgetItem::UserType);
    table.setItem(0, 0, item0_0);

    QTableWidgetItem *item1_0 = new QTableWidgetItem(QTableWidgetItem::UserType);
    table.setItem(1, 0, item1_0);

    QCOMPARE(table.item(0, 1), nullptr);
}

class TestTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    using QTableWidget::QTableWidget;
    using QTableWidget::mimeData;
    using QTableWidget::indexFromItem;
    using QTableWidget::keyPressEvent;
};

void tst_QTableWidget::mimeData()
{
    TestTableWidget table(10, 10);

    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10; ++y) {
            QTableWidgetItem *item = new QTableWidgetItem(QStringLiteral("123"));
            table.setItem(y, x, item);
        }
    }

    QList<QTableWidgetItem *> tableWidgetItemList;
    QModelIndexList modelIndexList;

    // do these checks more than once to ensure that the "cached indexes" work as expected
    QVERIFY(!table.mimeData(tableWidgetItemList));
    QVERIFY(!table.model()->mimeData(modelIndexList));
    QVERIFY(!table.model()->mimeData(modelIndexList));
    QVERIFY(!table.mimeData(tableWidgetItemList));

    tableWidgetItemList << table.item(1, 1);
    modelIndexList << table.indexFromItem(table.item(1, 1));

    QMimeData *data;

    QVERIFY((data = table.mimeData(tableWidgetItemList)));
    delete data;

    QVERIFY((data = table.model()->mimeData(modelIndexList)));
    delete data;

    QVERIFY((data = table.model()->mimeData(modelIndexList)));
    delete data;

    QVERIFY((data = table.mimeData(tableWidgetItemList)));
    delete data;

    // check the saved data is actually the same

    QMimeData *data2;

    data = table.mimeData(tableWidgetItemList);
    data2 = table.model()->mimeData(modelIndexList);

    const QString format = QStringLiteral("application/x-qabstractitemmodeldatalist");

    QVERIFY(data->hasFormat(format));
    QVERIFY(data2->hasFormat(format));
    QCOMPARE(data->data(format), data2->data(format));

    delete data;
    delete data2;
}

void tst_QTableWidget::selectedRowAfterSorting()
{
    TestTableWidget table(3,3);
    table.setSelectionBehavior(QAbstractItemView::SelectRows);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++)
            table.setItem(r, c, new QTableWidgetItem(QStringLiteral("0")));
    }
    QHeaderView *localHorizontalHeader = table.horizontalHeader();
    localHorizontalHeader->setSortIndicator(1,Qt::DescendingOrder);
    table.setProperty("sortingEnabled",true);
    table.selectRow(1);
    table.item(1,1)->setText("9");
    QCOMPARE(table.selectedItems().count(),3);
    const auto selectedItems = table.selectedItems();
    for (QTableWidgetItem *item : selectedItems)
        QCOMPARE(item->row(), 0);
}

void tst_QTableWidget::search()
{
    auto createItem = [](const QString &txt)
    {
        auto item = new QTableWidgetItem(txt);
        item->setFlags(item->flags().setFlag(Qt::ItemIsEditable, false));
        return item;
    };

    auto checkSeries = [](TestTableWidget &tw, const QVector<QPair<QKeyEvent, int>> &series)
    {
        for (const auto &p : series) {
            QKeyEvent e = p.first;
            tw.keyPressEvent(&e);
            QVERIFY(tw.selectionModel()->isSelected(tw.model()->index(p.second, 0)));
        }
    };
    TestTableWidget tw(5, 1);
    tw.setItem(0, 0, createItem("12"));
    tw.setItem(1, 0, createItem("123"));
    tw.setItem(2, 0, createItem("123 4"));
    tw.setItem(3, 0, createItem("123 5"));
    tw.setItem(4, 0, createItem(" "));
    tw.show();

    QKeyEvent evSpace(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier, " ");
    QKeyEvent ev1(QEvent::KeyPress, Qt::Key_1, Qt::NoModifier, "1");
    QKeyEvent ev2(QEvent::KeyPress, Qt::Key_2, Qt::NoModifier, "2");
    QKeyEvent ev3(QEvent::KeyPress, Qt::Key_3, Qt::NoModifier, "3");
    QKeyEvent ev4(QEvent::KeyPress, Qt::Key_4, Qt::NoModifier, "4");
    QKeyEvent ev5(QEvent::KeyPress, Qt::Key_5, Qt::NoModifier, "5");

    checkSeries(tw, {{evSpace, 4}, {ev1, 4}});
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    checkSeries(tw, {{ev1, 0}, {ev2, 0}, {ev3, 1}, {evSpace, 2}, {ev5, 3}});
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    checkSeries(tw, {{ev1, 0}, {ev2, 0}, {ev3, 1}, {evSpace, 2}, {ev4, 2}});
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void tst_QTableWidget::clearItemData()
{
    QTableWidget table(3, 3);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++)
            table.setItem(r, c, new QTableWidgetItem(QStringLiteral("0")));
    }
    QSignalSpy dataChangeSpy(table.model(), &QAbstractItemModel::dataChanged);
    QVERIFY(dataChangeSpy.isValid());
    QVERIFY(!table.model()->clearItemData(QModelIndex()));
    QCOMPARE(dataChangeSpy.size(), 0);
    QVERIFY(table.model()->clearItemData(table.model()->index(0, 0)));
    QVERIFY(!table.model()->index(0, 0).data().isValid());
    QCOMPARE(dataChangeSpy.size(), 1);
    const QList<QVariant> dataChangeArgs = dataChangeSpy.takeFirst();
    QCOMPARE(dataChangeArgs.at(0).value<QModelIndex>(), table.model()->index(0, 0));
    QCOMPARE(dataChangeArgs.at(1).value<QModelIndex>(), table.model()->index(0, 0));
    QVERIFY(dataChangeArgs.at(2).value<QVector<int>>().isEmpty());
    QVERIFY(table.model()->clearItemData(table.model()->index(0, 0)));
    QCOMPARE(dataChangeSpy.size(), 0);
}
#endif

QTEST_MAIN(tst_QTableWidget)
#include "tst_qtablewidget.moc"
