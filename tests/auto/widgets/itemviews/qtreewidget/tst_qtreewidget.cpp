/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qtreewidget.h>
#include <qtreewidgetitemiterator.h>
#include <qapplication.h>
#include <qeventloop.h>
#include <qdebug.h>
#include <qheaderview.h>
#include <qlineedit.h>
#include <QScrollBar>
#include <QStyledItemDelegate>

class CustomTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    QModelIndex indexFromItem(QTreeWidgetItem *item, int column = 0) const
    { return QTreeWidget::indexFromItem(item, column); }
};

class tst_QTreeWidget : public QObject
{
    Q_OBJECT

public:
    tst_QTreeWidget();
    ~tst_QTreeWidget();


public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void getSetCheck();
    void addTopLevelItem();
    void currentItem_data();
    void currentItem();
    void editItem_data();
    void editItem();
    void takeItem_data();
    void takeItem();
    void removeChild_data();
    void removeChild();
    void setItemHidden();
    void setItemHidden2();
    void selectedItems_data();
    void selectedItems();
    void itemAssignment();
    void clone_data();
    void clone();
    void expand_data();
    void expand();
    void checkState_data();
    void checkState();
    void findItems_data();
    void findItems();
    void findItemsInColumn();
    void sortItems_data();
    void sortItems();
    void deleteItems_data();
    void deleteItems();
    void itemAboveOrBelow();
    void itemStreaming_data();
    void itemStreaming();
    void insertTopLevelItems_data();
    void insertTopLevelItems();
    void keyboardNavigation();
    void scrollToItem();
    void setSortingEnabled();
    void match();
    void columnCount();
    void setHeaderLabels();
    void setHeaderItem();
    void itemWidget_data();
    void itemWidget();
    void insertItemsWithSorting_data();
    void insertItemsWithSorting();
    void insertExpandedItemsWithSorting_data();
    void insertExpandedItemsWithSorting();
    void changeDataWithSorting_data();
    void changeDataWithSorting();
    void changeDataWithStableSorting_data();
    void changeDataWithStableSorting();

    void sortedIndexOfChild_data();
    void sortedIndexOfChild();
    void defaultRowSizes();

    void task191552_rtl();
    void task203673_selection();
    void rootItemFlags();
    void task218661_setHeaderData();
    void task245280_sortChildren();
    void task253109_itemHeight();

    // QTreeWidgetItem
    void itemOperatorLessThan();
    void addChild();
    void setData();
    void enableDisable();

    void expandAndCallapse();
    void itemData();
    void setDisabled();
    void removeSelectedItem();
    void removeCurrentItem();
    void removeCurrentItem_task186451();
    void randomExpand();
    void crashTest();
    void sortAndSelect();

    void task206367_duplication();
    void selectionOrder();

    void setSelectionModel();
    void task217309();
    void setCurrentItemExpandsParent();
    void task239150_editorWidth();
    void setTextUpdate();
    void taskQTBUG2844_visualItemRect();
    void setChildIndicatorPolicy();

    void task20345_sortChildren();

public slots:
    void itemSelectionChanged();
    void emitDataChanged();

private:
    CustomTreeWidget *testWidget;
};

// Testing get/set functions
void tst_QTreeWidget::getSetCheck()
{
    QTreeWidget obj1;
    // int QTreeWidget::columnCount()
    // void QTreeWidget::setColumnCount(int)
    obj1.setColumnCount(0);
    QCOMPARE(obj1.columnCount(), 0);

    obj1.setColumnCount(INT_MIN);
    QCOMPARE(obj1.columnCount(), 0);

    //obj1.setColumnCount(INT_MAX);
    //QCOMPARE(obj1.columnCount(), INT_MAX);
    // Since setColumnCount allocates memory, there is no way this will succeed

    obj1.setColumnCount(100);
    QCOMPARE(obj1.columnCount(), 100);

    // QTreeWidgetItem * QTreeWidget::headerItem()
    // void QTreeWidget::setHeaderItem(QTreeWidgetItem *)
    QTreeWidgetItem *var2 = new QTreeWidgetItem();
    obj1.setHeaderItem(var2);
    QCOMPARE(obj1.headerItem(), var2);

    obj1.setHeaderItem((QTreeWidgetItem *)0);
//    QCOMPARE(obj1.headerItem(), (QTreeWidgetItem *)0);

    // QTreeWidgetItem * QTreeWidget::currentItem()
    // void QTreeWidget::setCurrentItem(QTreeWidgetItem *)
    QTreeWidgetItem *var3 = new QTreeWidgetItem(&obj1);
    obj1.setCurrentItem(var3);
    QCOMPARE(obj1.currentItem(), var3);

    obj1.setCurrentItem((QTreeWidgetItem *)0);
    QCOMPARE(obj1.currentItem(), (QTreeWidgetItem *)0);
}

typedef QList<int> IntList;
typedef QList<IntList> ListIntList;

Q_DECLARE_METATYPE(Qt::Orientation)

typedef QTreeWidgetItem TreeItem;
typedef QList<TreeItem*> TreeItemList;

Q_DECLARE_METATYPE(QTreeWidgetItem*)
Q_DECLARE_METATYPE(TreeItemList)

tst_QTreeWidget::tst_QTreeWidget(): testWidget(0)
{
}

tst_QTreeWidget::~tst_QTreeWidget()
{
}

void tst_QTreeWidget::initTestCase()
{
    qMetaTypeId<QModelIndex>();
    qMetaTypeId<Qt::Orientation>();
    qRegisterMetaType<QTreeWidgetItem*>("QTreeWidgetItem*");

    testWidget = new CustomTreeWidget();
    testWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(testWidget));
}

void tst_QTreeWidget::cleanupTestCase()
{
    testWidget->hide();
    delete testWidget;
}

void tst_QTreeWidget::init()
{
    testWidget->clear();
    testWidget->setColumnCount(2);
}

void tst_QTreeWidget::cleanup()
{
}

TreeItem *operator<<(TreeItem *parent, const TreeItemList &children) {
    for (int i = 0; i < children.count(); ++i)
        parent->addChild(children.at(i));
    return parent;
}

static void populate(QTreeWidget *widget, const TreeItemList &topLevelItems,
                     TreeItem *headerItem = 0)
{
    widget->clear();
    widget->setHeaderItem(headerItem);
    foreach (TreeItem *item, topLevelItems)
        widget->addTopLevelItem(item);
}

void tst_QTreeWidget::addTopLevelItem()
{
    QTreeWidget tree;
    QCOMPARE(tree.topLevelItemCount(), 0);

    // try to add 0
    tree.addTopLevelItem(0);
    QCOMPARE(tree.topLevelItemCount(), 0);
    QCOMPARE(tree.indexOfTopLevelItem(0), -1);

    // add one at a time
    QList<TreeItem*> tops;
    for (int i = 0; i < 10; ++i) {
        TreeItem *ti = new TreeItem();
        QCOMPARE(tree.indexOfTopLevelItem(ti), -1);
        tree.addTopLevelItem(ti);
        QCOMPARE(tree.topLevelItemCount(), i+1);
        QCOMPARE(tree.topLevelItem(i), ti);
        QCOMPARE(tree.topLevelItem(-1), static_cast<TreeItem*>(0));
        QCOMPARE(tree.indexOfTopLevelItem(ti), i);
        QCOMPARE(ti->parent(), static_cast<TreeItem*>(0));
        tree.addTopLevelItem(ti);
        QCOMPARE(tree.topLevelItemCount(), i+1);
        tops.append(ti);
    }

    // delete one at a time
    while (!tops.isEmpty()) {
        TreeItem *ti = tops.takeFirst();
        delete ti;
        QCOMPARE(tree.topLevelItemCount(), tops.count());
        for (int i = 0; i < tops.count(); ++i)
            QCOMPARE(tree.topLevelItem(i), tops.at(i));
    }

    // add none
    {
        int count = tree.topLevelItemCount();
        tree.addTopLevelItems(tops);
        QCOMPARE(tree.topLevelItemCount(), count);
    }

    // add many at a time
    {
        const int count = 10;
        for (int i = 0; i < 100; i += count) {
            tops.clear();
            for (int j = 0; j < count; ++j)
                tops << new TreeItem(QStringList() << QString("%0").arg(j));
            tree.addTopLevelItems(tops);
            QCOMPARE(tree.topLevelItemCount(), count + i);
            for (int j = 0; j < count; ++j)
                QCOMPARE(tree.topLevelItem(i+j), tops.at(j));

            tree.addTopLevelItems(tops);
            QCOMPARE(tree.topLevelItemCount(), count + i);
        }
    }

    // insert
    {
        tops.clear();
        for (int i = 0; i < 10; ++i)
            tops << new TreeItem();
        int count = tree.topLevelItemCount();
        tree.insertTopLevelItems(100000, tops);
        // ### fixme
        QCOMPARE(tree.topLevelItemCount(), count + 10);
    }
}

void tst_QTreeWidget::currentItem_data()
{
    QTest::addColumn<TreeItemList>("topLevelItems");

    QTest::newRow("only top-level items, 2 columns")
        << (TreeItemList()
            << new TreeItem(QStringList() << "a" << "b")
            << new TreeItem(QStringList() << "c" << "d"));
    TreeItemList lst;
    lst << (new TreeItem(QStringList() << "a" << "b")
        << (TreeItemList()
            << new TreeItem(QStringList() << "c" << "d")
            << new TreeItem(QStringList() << "c" << "d")
            )
            )
        << (new TreeItem(QStringList() << "e" << "f")
            << (TreeItemList()
                << new TreeItem(QStringList() << "g" << "h")
                << new TreeItem(QStringList() << "g" << "h")
                )
            );
    QTest::newRow("hierarchy, 2 columns") << lst;
}

void tst_QTreeWidget::currentItem()
{
    QFETCH(TreeItemList, topLevelItems);

    QTreeWidget tree;
    tree.show();
    populate(&tree, topLevelItems, new TreeItem(QStringList() << "1" << "2"));
    QTreeWidgetItem *previous = 0;
    for (int x = 0; x < 2; ++x) {
        tree.setSelectionBehavior(x ? QAbstractItemView::SelectItems
                                  : QAbstractItemView::SelectRows);
        QSignalSpy currentItemChangedSpy(
            &tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
        QSignalSpy itemSelectionChangedSpy(
            &tree, SIGNAL(itemSelectionChanged()));

        QTreeWidgetItemIterator it(&tree);
        // do all items
        while (QTreeWidgetItem *item = (*it++)) {
            tree.setCurrentItem(item);
            QCOMPARE(tree.currentItem(), item);

            QCOMPARE(currentItemChangedSpy.count(), 1);
            QVariantList args = currentItemChangedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
            QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(1)), previous);

            QCOMPARE(itemSelectionChangedSpy.count(), 1);
            itemSelectionChangedSpy.clear();

            previous = item;
            // do all columns
            for (int col = 0; col < item->columnCount(); ++col) {
                tree.setCurrentItem(item, col);
                QCOMPARE(tree.currentItem(), item);
                QCOMPARE(tree.currentColumn(), col);

                if (!currentItemChangedSpy.isEmpty()) {
                    // ### we get a currentItemChanged() when what really
                    // changed was just currentColumn(). Should it be like this?
                    QCOMPARE(currentItemChangedSpy.count(), 1);
                    QVariantList args = currentItemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(1)), item);
                    if (tree.selectionBehavior() == QAbstractItemView::SelectItems) {
                        QCOMPARE(itemSelectionChangedSpy.count(), 1);
                        itemSelectionChangedSpy.clear();
                    } else {
                        QCOMPARE(itemSelectionChangedSpy.count(), 0);
                    }
                }
            }
        }
    }

    // can't set the headerItem to be the current item
    tree.setCurrentItem(tree.headerItem());
    QCOMPARE(tree.currentItem(), static_cast<TreeItem*>(0));
}

void tst_QTreeWidget::editItem_data()
{
    QTest::addColumn<TreeItemList>("topLevelItems");

    {
        TreeItemList list;
        for (int i = 0; i < 10; i++) {
            TreeItem *item = new TreeItem(QStringList() << "col1" << "col2");
            if ((i & 1) == 0)
                item->setFlags(item->flags() | Qt::ItemIsEditable);
            else
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            list << item;
        }
        QTest::newRow("2 columns, only even items editable")
            << list;
    }
}

void tst_QTreeWidget::editItem()
{
    QFETCH(TreeItemList, topLevelItems);

    QTreeWidget tree;
    populate(&tree, topLevelItems, new TreeItem(QStringList() << "1" << "2"));
    tree.show();
    QVERIFY(QTest::qWaitForWindowActive(&tree));

    QSignalSpy itemChangedSpy(
        &tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)));

    QTreeWidgetItemIterator it(&tree);
    while (QTreeWidgetItem *item = (*it++)) {
        for (int col = 0; col < item->columnCount(); ++col) {
            if (!(item->flags() & Qt::ItemIsEditable))
                QTest::ignoreMessage(QtWarningMsg, "edit: editing failed");
            tree.editItem(item, col);
            QApplication::instance()->processEvents();
            QApplication::instance()->processEvents();
            QLineEdit *editor = tree.findChild<QLineEdit*>();
            if (editor) {
                QVERIFY(item->flags() & Qt::ItemIsEditable);
                QCOMPARE(editor->selectedText(), editor->text());
                QTest::keyClick(editor, Qt::Key_A);
                QTest::keyClick(editor, Qt::Key_Enter);
                QApplication::instance()->processEvents();
                QCOMPARE(itemChangedSpy.count(), 1);
                QVariantList args = itemChangedSpy.takeFirst();
                QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                QCOMPARE(qvariant_cast<int>(args.at(1)), col);
            } else {
                QVERIFY(!(item->flags() & Qt::ItemIsEditable));
            }
        }
    }
}

void tst_QTreeWidget::takeItem_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<bool>("topLevel");
    QTest::addColumn<bool>("outOfBounds");

    QTest::newRow("First, topLevel") << 0 << true << false;
    QTest::newRow("Last, topLevel") << 2 << true << false;
    QTest::newRow("Middle, topLevel") << 1 << true << false;
    QTest::newRow("Out of bounds, toplevel, (index: -1)") << -1 << true << true;
    QTest::newRow("Out of bounds, toplevel, (index: 3)") << 3 << true << true;

    QTest::newRow("First, child of topLevel") << 0 << false << false;
    QTest::newRow("Last, child of topLevel") << 2 << false << false;
    QTest::newRow("Middle, child of topLevel") << 1 << false << false;
    QTest::newRow("Out of bounds, child of toplevel, (index: -1)") << -1 << false << true;
    QTest::newRow("Out of bounds, child of toplevel, (index: 3)") << 3 << false << true;
}

void tst_QTreeWidget::takeItem()
{
    QFETCH(int, index);
    QFETCH(bool, topLevel);
    QFETCH(bool, outOfBounds);

    for (int i=0; i<3; ++i) {
        QTreeWidgetItem *top = new QTreeWidgetItem(testWidget);
        top->setText(0, QString("top%1").arg(i));
        for (int j=0; j<3; ++j) {
            QTreeWidgetItem *child = new QTreeWidgetItem(top);
            child->setText(0, QString("child%1").arg(j));
        }
    }

    QCOMPARE(testWidget->topLevelItemCount(), 3);
    QCOMPARE(testWidget->topLevelItem(0)->childCount(), 3);

    if (topLevel) {
        int count = testWidget->topLevelItemCount();
        QTreeWidgetItem *item = testWidget->takeTopLevelItem(index);
        if (outOfBounds) {
            QCOMPARE(item, (QTreeWidgetItem *)0);
            QCOMPARE(count, testWidget->topLevelItemCount());
        } else {
            QCOMPARE(item->text(0), QString("top%1").arg(index));
            QCOMPARE(count-1, testWidget->topLevelItemCount());
            delete item;
        }
    } else {
        int count = testWidget->topLevelItem(0)->childCount();
        QTreeWidgetItem *item = testWidget->topLevelItem(0)->takeChild(index);
        if (outOfBounds) {
            QCOMPARE(item, (QTreeWidgetItem *)0);
            QCOMPARE(count, testWidget->topLevelItem(0)->childCount());
        } else {
            QCOMPARE(item->text(0), QString("child%1").arg(index));
            QCOMPARE(count-1, testWidget->topLevelItem(0)->childCount());
            delete item;
        }
    }
}

void tst_QTreeWidget::removeChild_data()
{
    QTest::addColumn<int>("childCount");
    QTest::addColumn<int>("removeAt");

    QTest::newRow("10 remove 3") << 10 << 3;
}

void tst_QTreeWidget::removeChild()
{
    QFETCH(int, childCount);
    QFETCH(int, removeAt);

    QTreeWidgetItem *root = new QTreeWidgetItem;
    for (int i = 0; i < childCount; ++i)
        new QTreeWidgetItem(root, QStringList(QString::number(i)));

    QCOMPARE(root->childCount(), childCount);
    for (int j = 0; j < childCount; ++j)
        QCOMPARE(root->child(j)->text(0), QString::number(j));

    QTreeWidgetItem *remove = root->child(removeAt);
    root->removeChild(remove);

    QCOMPARE(root->childCount(), childCount - 1);
    for (int k = 0; k < childCount; ++k) {
        if (k == removeAt)
            QCOMPARE(remove->text(0), QString::number(k));
        else if (k < removeAt)
            QCOMPARE(root->child(k)->text(0), QString::number(k));
        else if (k > removeAt)
            QCOMPARE(root->child(k - 1)->text(0), QString::number(k));
    }
    delete root;
}

void tst_QTreeWidget::setItemHidden()
{
    QTreeWidgetItem *parent = new QTreeWidgetItem(testWidget);
    parent->setText(0, "parent");
    QTreeWidgetItem *child = new QTreeWidgetItem(parent);
    child->setText(0, "child");
    QVERIFY(child->parent());

    testWidget->expandItem(parent);
    testWidget->scrollToItem(child);

    QVERIFY(testWidget->visualItemRect(parent).isValid()
           && testWidget->viewport()->rect().intersects(testWidget->visualItemRect(parent)));
    QVERIFY(testWidget->visualItemRect(child).isValid()
           && testWidget->viewport()->rect().intersects(testWidget->visualItemRect(child)));

    QVERIFY(!testWidget->isItemHidden(parent));
    QVERIFY(!testWidget->isItemHidden(child));

    testWidget->setItemHidden(parent, true);

    QVERIFY(!(testWidget->visualItemRect(parent).isValid()
             && testWidget->viewport()->rect().intersects(testWidget->visualItemRect(parent))));
    QVERIFY(!(testWidget->visualItemRect(child).isValid()
             && testWidget->viewport()->rect().intersects(testWidget->visualItemRect(child))));

    QVERIFY(testWidget->isItemHidden(parent));
    QVERIFY(!testWidget->isItemHidden(child));

    // From task 78670 (This caused an core dump)
    // Check if we can set an item visible if it already is visible.
    testWidget->setItemHidden(parent, false);
    testWidget->setItemHidden(parent, false);
    QVERIFY(!testWidget->isItemHidden(parent));


    // hide, hide and then unhide.
    testWidget->setItemHidden(parent, true);
    testWidget->setItemHidden(parent, true);
    testWidget->setItemHidden(parent, false);
    QVERIFY(!testWidget->isItemHidden(parent));


}


void tst_QTreeWidget::setItemHidden2()
{
    // From Task 78587
    QStringList hl;
    hl << "ID" << "Desc";
    testWidget->setColumnCount(hl.count());
    testWidget->setHeaderLabels(hl);
    testWidget->setSortingEnabled(true);

    QTreeWidgetItem *top = new QTreeWidgetItem(testWidget);
    QTreeWidgetItem *leaf = 0;
    top->setText(0, "ItemList");
    for (int i = 1; i <= 4; i++) {
        leaf = new QTreeWidgetItem(top);
        leaf->setText(0, QString().sprintf("%d", i));
        leaf->setText(1, QString().sprintf("Item %d", i));
    }

    if (testWidget->topLevelItemCount() > 0) {
        top = testWidget->topLevelItem(0);
        testWidget->setItemExpanded(top, true);
    }

    if (testWidget->topLevelItemCount() > 0) {
        top = testWidget->topLevelItem(0);
        for (int i = 0; i < top->childCount(); i++) {
            leaf = top->child(i);
            if (leaf->text(0).toInt() % 2 == 0) {
                if (!testWidget->isItemHidden(leaf)) {
                    testWidget->setItemHidden(leaf, true);
                }
            }
        }
    }
}


void tst_QTreeWidget::selectedItems_data()
{
    QTest::addColumn<int>("topLevel");
    QTest::addColumn<int>("children");
    QTest::addColumn<bool>("closeTopLevel");
    QTest::addColumn<ListIntList>("selectedItems");
    QTest::addColumn<ListIntList>("hiddenItems");
    QTest::addColumn<ListIntList>("expectedItems");

    ListIntList selectedItems;
    ListIntList hiddenItems;
    ListIntList expectedItems;

    selectedItems.clear(); hiddenItems.clear(); expectedItems.clear();
    selectedItems
        << (IntList()
            << 0);
    expectedItems
        << (IntList() << 0);
    QTest::newRow("2 top with 2 children, closed, top0 selected, no hidden")
        << 2 << 2 << true << selectedItems << hiddenItems << expectedItems;

    selectedItems.clear(); hiddenItems.clear(); expectedItems.clear();
    selectedItems
        << (IntList()
            << 0 << 0);
    expectedItems
        << (IntList() << 0 << 0);
    QTest::newRow("2 top with 2 children, closed, top0child0 selected, no hidden")
        << 2 << 2 << true << selectedItems << hiddenItems << expectedItems;

    selectedItems.clear(); hiddenItems.clear(); expectedItems.clear();
    selectedItems
        << (IntList()
            << 0 << 0);
    expectedItems
        << (IntList()
            << 0 << 0);
    QTest::newRow("2 top with 2 children, open, top0child0 selected, no hidden")
        << 2 << 2 << false << selectedItems << hiddenItems << expectedItems;

    selectedItems.clear(); hiddenItems.clear(); expectedItems.clear();
    selectedItems << (IntList() << 0);
    hiddenItems << (IntList() << 0);
    QTest::newRow("2 top with 2 children, closed, top0 selected, top0 hidden")
        << 2 << 2 << true << selectedItems << hiddenItems << expectedItems;

    selectedItems.clear(); hiddenItems.clear(); expectedItems.clear();
    selectedItems << (IntList() << 0 << 0);
    hiddenItems << (IntList() << 0);
    expectedItems << (IntList() << 0 << 0);
    QTest::newRow("2 top with 2 children, closed, top0child0 selected, top0 hidden")
        << 2 << 2 << true << selectedItems << hiddenItems << expectedItems;

    selectedItems.clear(); hiddenItems.clear(); expectedItems.clear();
    selectedItems
        << (IntList() << 0)
        << (IntList() << 0 << 0)
        << (IntList() << 0 << 1)
        << (IntList() << 1)
        << (IntList() << 1 << 0)
        << (IntList() << 1 << 1);
    expectedItems
        << (IntList() << 0)
        << (IntList() << 0 << 0)
        << (IntList() << 0 << 1)
        << (IntList() << 1)
        << (IntList() << 1 << 0)
        << (IntList() << 1 << 1);
    QTest::newRow("2 top with 2 children, closed, all selected, no hidden")
        << 2 << 2 << true << selectedItems << hiddenItems << expectedItems;


    selectedItems.clear(); hiddenItems.clear(); expectedItems.clear();
    selectedItems
        << (IntList() << 0)
        << (IntList() << 0 << 0)
        << (IntList() << 0 << 1)
        << (IntList() << 1)
        << (IntList() << 1 << 0)
        << (IntList() << 1 << 1);
    hiddenItems
        << (IntList() << 0);
    expectedItems
        //<< (IntList() << 0)
        << (IntList() << 0 << 0)
        << (IntList() << 0 << 1)
        << (IntList() << 1)
        << (IntList() << 1 << 0)
        << (IntList() << 1 << 1);
    QTest::newRow("2 top with 2 children, closed, all selected, top0 hidden")
        << 2 << 2 << true << selectedItems << hiddenItems << expectedItems;

    selectedItems.clear(); hiddenItems.clear(); expectedItems.clear();
    selectedItems
        << (IntList() << 0)
        << (IntList() << 0 << 0)
        << (IntList() << 0 << 1)
        << (IntList() << 1)
        << (IntList() << 1 << 0)
        << (IntList() << 1 << 1);
    hiddenItems
        << (IntList() << 0 << 1)
        << (IntList() << 1);
    expectedItems
        << (IntList() << 0)
        << (IntList() << 0 << 0)
        //<< (IntList() << 0 << 1)
        //<< (IntList() << 1)
        << (IntList() << 1 << 0)
        << (IntList() << 1 << 1);

    QTest::newRow("2 top with 2 children, closed, all selected, top0child1 and top1")
        << 2 << 2 << true << selectedItems << hiddenItems << expectedItems;

}

void tst_QTreeWidget::selectedItems()
{
    QFETCH(int, topLevel);
    QFETCH(int, children);
    QFETCH(bool, closeTopLevel);
    QFETCH(ListIntList, selectedItems);
    QFETCH(ListIntList, hiddenItems);
    QFETCH(ListIntList, expectedItems);

    // create items
    for (int t=0; t<topLevel; ++t) {
        QTreeWidgetItem *top = new QTreeWidgetItem(testWidget);
        top->setText(0, QString("top%1").arg(t));
        for (int c=0; c<children; ++c) {
            QTreeWidgetItem *child = new QTreeWidgetItem(top);
            child->setText(0, QString("top%1child%2").arg(t).arg(c));
        }
    }

    // set selected
    foreach (IntList itemPath, selectedItems) {
        QTreeWidgetItem *item = 0;
        foreach(int index, itemPath) {
            if (!item)
                item = testWidget->topLevelItem(index);
            else
                item = item->child(index);
        }
        testWidget->setItemSelected(item, true);
    }

    // hide rows
    foreach (IntList itemPath, hiddenItems) {
        QTreeWidgetItem *item = 0;
        foreach(int index, itemPath) {
            if (!item)
                item = testWidget->topLevelItem(index);
            else
                item = item->child(index);
        }
        testWidget->setItemHidden(item, true);
    }

    // open/close toplevel
    for (int i=0; i<testWidget->topLevelItemCount(); ++i) {
        if (closeTopLevel)
            testWidget->collapseItem(testWidget->topLevelItem(i));
        else
            testWidget->expandItem(testWidget->topLevelItem(i));
    }

    // check selectedItems
    QList<QTreeWidgetItem*> sel = testWidget->selectedItems();
    QCOMPARE(sel.count(), expectedItems.count());
    foreach (IntList itemPath, expectedItems) {
        QTreeWidgetItem *item = 0;
        foreach(int index, itemPath) {
            if (!item)
                item = testWidget->topLevelItem(index);
            else
                item = item->child(index);
        }
        if (item)
        QVERIFY(sel.contains(item));
    }

    // compare isSelected
    for (int t=0; t<testWidget->topLevelItemCount(); ++t) {
        QTreeWidgetItem *top = testWidget->topLevelItem(t);
        if (testWidget->isItemSelected(top) && !testWidget->isItemHidden(top))
            QVERIFY(sel.contains(top));
        for (int c=0; c<top->childCount(); ++c) {
            QTreeWidgetItem *child = top->child(c);
            if (testWidget->isItemSelected(child) && !testWidget->isItemHidden(child))
                QVERIFY(sel.contains(child));
        }
    }

    // Possible to select null without crashing?
    testWidget->setItemSelected(0, true);
    QVERIFY(!testWidget->isItemSelected(0));

    // unselect
    foreach (IntList itemPath, selectedItems) {
        QTreeWidgetItem *item = 0;
        foreach(int index, itemPath) {
            if (!item)
                item = testWidget->topLevelItem(index);
            else
                item = item->child(index);
        }
        testWidget->setItemSelected(item, false);
    }
    QCOMPARE(testWidget->selectedItems().count(), 0);
}

void tst_QTreeWidget::itemAssignment()
{
    // create item with children and parent but not insert in the view
    QTreeWidgetItem grandParent;
    QTreeWidgetItem *parent = new QTreeWidgetItem(&grandParent);
    parent->setText(0, "foo");
    parent->setText(1, "bar");
    for (int i=0; i<5; ++i) {
        QTreeWidgetItem *child = new QTreeWidgetItem(parent);
        child->setText(0, "bingo");
        child->setText(1, "bango");
    }
    QCOMPARE(parent->parent(), &grandParent);
    QVERIFY(!parent->treeWidget());
    QCOMPARE(parent->columnCount(), 2);
    QCOMPARE(parent->text(0), QString("foo"));
    QCOMPARE(parent->childCount(), 5);
    QCOMPARE(parent->child(0)->parent(), parent);

    // create item which is inserted in the widget
    QTreeWidgetItem item(testWidget);
    item.setText(0, "baz");
    QVERIFY(!item.parent());
    QCOMPARE(item.treeWidget(), static_cast<QTreeWidget *>(testWidget));
    QCOMPARE(item.columnCount(), 1);
    QCOMPARE(item.text(0), QString("baz"));
    QCOMPARE(item.childCount(), 0);

    // assign and test
    *parent = item;
    QCOMPARE(parent->parent(), &grandParent);
    QVERIFY(!parent->treeWidget());
    QCOMPARE(parent->columnCount(), 1);
    QCOMPARE(parent->text(0), QString("baz"));
    QCOMPARE(parent->childCount(), 5);
    QCOMPARE(parent->child(0)->parent(), parent);
}

void tst_QTreeWidget::clone_data()
{
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("topLevelIndex");
    QTest::addColumn<int>("childIndex");
    QTest::addColumn<QStringList>("topLevelText");
    QTest::addColumn<QStringList>("childText");
    QTest::addColumn<bool>("cloneChild");

    QTest::newRow("clone parent with child") << 0 << 0 << 0
                                          << (QStringList() << "some text")
                                          << (QStringList() << "more text")
                                          << false;

    QTest::newRow("clone child") << 0 << 0 << 0
                              << (QStringList() << "some text")
                              << (QStringList() << "more text")
                              << true;
}

void tst_QTreeWidget::clone()
{
    QFETCH(int, column);
    QFETCH(int, topLevelIndex);
    QFETCH(int, childIndex);
    QFETCH(QStringList, topLevelText);
    QFETCH(QStringList, childText);
    QFETCH(bool, cloneChild);

    for (int i = 0; i < topLevelText.count(); ++i) {
        QTreeWidgetItem *item = new QTreeWidgetItem(testWidget);
        item->setText(column, topLevelText.at(i));
        for (int j = 0; j < childText.count(); ++j) {
            QTreeWidgetItem *child = new QTreeWidgetItem(item);
            child->setText(column, childText.at(j));
        }
    }

    QTreeWidgetItem *original = testWidget->topLevelItem(topLevelIndex);
    QTreeWidgetItem *copy = original->clone();
    QCOMPARE(copy->text(column), original->text(column));
    QCOMPARE(copy->childCount(), original->childCount());
    QVERIFY(!copy->parent());
    QVERIFY(!copy->treeWidget());

    QTreeWidgetItem *originalChild = original->child(childIndex);
    QTreeWidgetItem *copiedChild = cloneChild ? originalChild->clone() : copy->child(childIndex);
    QVERIFY(copiedChild != originalChild);
    QCOMPARE(copiedChild->text(column), originalChild->text(column));
    QCOMPARE(copiedChild->childCount(), originalChild->childCount());
    QCOMPARE(copiedChild->parent(), cloneChild ? 0 : copy);
    QVERIFY(!copiedChild->treeWidget());
    if (cloneChild)
        delete copiedChild;
    delete copy;
}

void tst_QTreeWidget::expand_data()
{
    QTest::addColumn<int>("topLevelIndex");
    QTest::addColumn<int>("topLevelCount");
    QTest::addColumn<int>("childIndex");
    QTest::addColumn<int>("childCount");

    QTest::newRow("the only test data for now") << 0 << 1 << 0 << 1;
}

void tst_QTreeWidget::expand()
{
    QFETCH(int, topLevelIndex);
    QFETCH(int, topLevelCount);
    QFETCH(int, childIndex);
    QFETCH(int, childCount);

    for (int i = 0; i < topLevelCount; ++i) {
        QTreeWidgetItem *item = new QTreeWidgetItem(testWidget);
        for (int j = 0; j < childCount; ++j)
            new QTreeWidgetItem(item);
    }

    QTreeWidgetItem *topLevelItem = testWidget->topLevelItem(topLevelIndex);
    QTreeWidgetItem *childItem = topLevelItem->child(childIndex);

    QVERIFY(!testWidget->isItemExpanded(topLevelItem));
    testWidget->setItemExpanded(topLevelItem, true);
    QVERIFY(testWidget->isItemExpanded(topLevelItem));

    QVERIFY(!testWidget->isItemExpanded(childItem));
    testWidget->setItemExpanded(childItem, true);
    QVERIFY(testWidget->isItemExpanded(childItem));

    QVERIFY(testWidget->isItemExpanded(topLevelItem));
    testWidget->setItemExpanded(topLevelItem, false);
    QVERIFY(!testWidget->isItemExpanded(topLevelItem));

    QVERIFY(testWidget->isItemExpanded(childItem));
    testWidget->setItemExpanded(childItem, false);
    QVERIFY(!testWidget->isItemExpanded(childItem));
}

void tst_QTreeWidget::checkState_data()
{
}

void tst_QTreeWidget::checkState()
{
    QTreeWidgetItem *item = new QTreeWidgetItem(testWidget);
    item->setCheckState(0, Qt::Unchecked);
    QTreeWidgetItem *firstChild = new QTreeWidgetItem(item);
    firstChild->setCheckState(0, Qt::Unchecked);
    QTreeWidgetItem *seccondChild = new QTreeWidgetItem(item);
    seccondChild->setCheckState(0, Qt::Unchecked);

    QCOMPARE(item->checkState(0), Qt::Unchecked);
    QCOMPARE(firstChild->checkState(0), Qt::Unchecked);
    QCOMPARE(seccondChild->checkState(0), Qt::Unchecked);

    firstChild->setCheckState(0, Qt::Checked);
    QCOMPARE(item->checkState(0), Qt::Unchecked);
    QCOMPARE(firstChild->checkState(0), Qt::Checked);
    QCOMPARE(seccondChild->checkState(0), Qt::Unchecked);

    item->setFlags(item->flags()|Qt::ItemIsTristate);
    QCOMPARE(item->checkState(0), Qt::PartiallyChecked);
    QCOMPARE(firstChild->checkState(0), Qt::Checked);
    QCOMPARE(seccondChild->checkState(0), Qt::Unchecked);

    seccondChild->setCheckState(0, Qt::Checked);
    QCOMPARE(item->checkState(0), Qt::Checked);
    QCOMPARE(firstChild->checkState(0), Qt::Checked);
    QCOMPARE(seccondChild->checkState(0), Qt::Checked);

    firstChild->setCheckState(0, Qt::Unchecked);
    seccondChild->setCheckState(0, Qt::Unchecked);
    QCOMPARE(item->checkState(0), Qt::Unchecked);
    QCOMPARE(firstChild->checkState(0), Qt::Unchecked);
    QCOMPARE(seccondChild->checkState(0), Qt::Unchecked);
}

void tst_QTreeWidget::findItems_data()
{
    QTest::addColumn<int>("column");
    QTest::addColumn<QStringList>("topLevelText");
    QTest::addColumn<QStringList>("childText");
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<int>("resultCount");
    QTest::addColumn<QStringList>("resultText");

    QTest::newRow("find in toplevel")
        << 0
        << (QStringList() << "This is a text" << "This is another" << "This is the one")
        << (QStringList() << "A child" << "This is not the one" << "And yet another child")
        << "This is the one"
        << 1
        << (QStringList() << "This is the one");

    QTest::newRow("find child")
        << 0
        << (QStringList() << "This is a text" << "This is another" << "This is the one")
        << (QStringList() << "A child" << "This is not the one" << "And yet another child")
        << "A child"
        << 3 // once for each branch
        << (QStringList() << "A child");

}

void tst_QTreeWidget::findItems()
{
    QFETCH(int, column);
    QFETCH(QStringList, topLevelText);
    QFETCH(QStringList, childText);
    QFETCH(QString, pattern);
    QFETCH(int, resultCount);
    QFETCH(QStringList, resultText);

    for (int i = 0; i < topLevelText.count(); ++i) {
        QTreeWidgetItem *item = new QTreeWidgetItem(testWidget);
        item->setText(column, topLevelText.at(i));
        for (int j = 0; j < childText.count(); ++j) {
            QTreeWidgetItem *child = new QTreeWidgetItem(item);
            child->setText(column, childText.at(j));
        }
    }

    QList<QTreeWidgetItem*> result = testWidget->findItems(pattern,
                                                           Qt::MatchExactly|Qt::MatchRecursive);
    QCOMPARE(result.count(), resultCount);

    for (int k = 0; k < result.count() && k < resultText.count(); ++k)
        QCOMPARE(result.at(k)->text(column), resultText.at(k));
}

void tst_QTreeWidget::findItemsInColumn()
{
    // Create 5 root items.
    for (int i = 0; i < 5; i++)
        new QTreeWidgetItem(testWidget, QStringList() << QString::number(i));

    // Create a child with two columns for each root item.
    for (int i = 0; i < 5; i++) {
        QTreeWidgetItem * const  parent = testWidget->topLevelItem(i);
        new QTreeWidgetItem(parent, QStringList() << QString::number(i * 10) << QString::number(i * 100));
    }

    // Recursively search column one for 400.
    QList<QTreeWidgetItem*> items = testWidget->findItems("400", Qt::MatchExactly|Qt::MatchRecursive, 1);
    QCOMPARE(items.count(), 1);
}

void tst_QTreeWidget::sortItems_data()
{
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("order");
    QTest::addColumn<QStringList>("topLevelText");
    QTest::addColumn<QStringList>("childText");
    QTest::addColumn<QStringList>("topLevelResult");
    QTest::addColumn<QStringList>("childResult");
    QTest::addColumn<IntList>("expectedTopRows");
    QTest::addColumn<IntList>("expectedChildRows");

    QTest::newRow("ascending order")
        << 0
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "c" << "d" << "a" << "b")
        << (QStringList() << "e" << "h" << "g" << "f")
        << (QStringList() << "a" << "b" << "c" << "d")
        << (QStringList() << "e" << "f" << "g" << "h")
        << (IntList() << 2 << 3 << 0 << 1)
        << (IntList() << 0 << 3 << 2 << 1);

    QTest::newRow("descending order")
        << 0
        << static_cast<int>(Qt::DescendingOrder)
        << (QStringList() << "c" << "d" << "a" << "b")
        << (QStringList() << "e" << "h" << "g" << "f")
        << (QStringList() << "d" << "c" << "b" << "a")
        << (QStringList() << "h" << "g" << "f" << "e")
        << (IntList() << 1 << 0 << 3 << 2)
        << (IntList() << 3 << 0 << 1 << 2);
}

void tst_QTreeWidget::sortItems()
{
    QFETCH(int, column);
    QFETCH(int, order);
    QFETCH(QStringList, topLevelText);
    QFETCH(QStringList, childText);
    QFETCH(QStringList, topLevelResult);
    QFETCH(QStringList, childResult);
    QFETCH(IntList, expectedTopRows);
    QFETCH(IntList, expectedChildRows);
    testWidget->setSortingEnabled(false);

    for (int i = 0; i < topLevelText.count(); ++i) {
        QTreeWidgetItem *item = new QTreeWidgetItem(testWidget);
        item->setText(column, topLevelText.at(i));
        for (int j = 0; j < childText.count(); ++j) {
            QTreeWidgetItem *child = new QTreeWidgetItem(item);
            child->setText(column, childText.at(j));
        }
    }

    QAbstractItemModel *model = testWidget->model();
    QList<QPersistentModelIndex> tops;
    for (int r = 0; r < model->rowCount(QModelIndex()); ++r) {
        QPersistentModelIndex p = model->index(r, 0, QModelIndex());
        tops << p;
    }
    QList<QPersistentModelIndex> children;
    for (int s = 0; s < model->rowCount(tops.first()); ++s) {
        QPersistentModelIndex c = model->index(s, 0, tops.first());
        children << c;
    }

    testWidget->sortItems(column, static_cast<Qt::SortOrder>(order));
    QCOMPARE(testWidget->sortColumn(), column);

    for (int k = 0; k < topLevelResult.count(); ++k) {
        QTreeWidgetItem *item = testWidget->topLevelItem(k);
        QCOMPARE(item->text(column), topLevelResult.at(k));
        for (int l = 0; l < childResult.count(); ++l)
            QCOMPARE(item->child(l)->text(column), childResult.at(l));
    }

    for (int m = 0; m < tops.count(); ++m)
        QCOMPARE(tops.at(m).row(), expectedTopRows.at(m));
    for (int n = 0; n < children.count(); ++n)
        QCOMPARE(children.at(n).row(), expectedChildRows.at(n));
}

void tst_QTreeWidget::deleteItems_data()
{
    QTest::addColumn<int>("topLevelCount");
    QTest::addColumn<int>("childCount");
    QTest::addColumn<int>("grandChildCount");

    QTest::addColumn<int>("deleteTopLevelCount");
    QTest::addColumn<int>("deleteChildCount");
    QTest::addColumn<int>("deleteGrandChildCount");

    QTest::addColumn<int>("expectedTopLevelCount");
    QTest::addColumn<int>("expectedChildCount");
    QTest::addColumn<int>("expectedGrandChildCount");

    QTest::addColumn<int>("persistentRow");
    QTest::addColumn<int>("persistentColumn");
    QTest::addColumn<bool>("persistentIsValid");

    QTest::newRow("start with 10, delete 1")
        << 10 << 10 << 10
        << 1 << 1 << 1
        << 9 << 9 << 9
        << 0 << 0 << false;
    QTest::newRow("start with 10, delete 5")
        << 10 << 10 << 10
        << 5 << 5 << 5
        << 5 << 5 << 5
        << 0 << 0 << false;
    QTest::newRow("mixed")
        << 10 << 13 << 7
        << 3 << 7 << 4
        << 7 << 6 << 3
        << 0 << 0 << false;
    QTest::newRow("all")
        << 10 << 10 << 10
        << 10 << 10 << 10
        << 0 << 0 << 0
        << 0 << 0 << false;
}

void tst_QTreeWidget::deleteItems()
{
    QFETCH(int, topLevelCount);
    QFETCH(int, childCount);
    QFETCH(int, grandChildCount);

    QFETCH(int, deleteTopLevelCount);
    QFETCH(int, deleteChildCount);
    QFETCH(int, deleteGrandChildCount);

    QFETCH(int, expectedTopLevelCount);
    QFETCH(int, expectedChildCount);
    QFETCH(int, expectedGrandChildCount);

    QFETCH(int, persistentRow);
    QFETCH(int, persistentColumn);
    QFETCH(bool, persistentIsValid);

    for (int i = 0; i < topLevelCount; ++i) {
        QTreeWidgetItem *top = new QTreeWidgetItem(testWidget);
        for (int j = 0; j < childCount; ++j) {
            QTreeWidgetItem *child = new QTreeWidgetItem(top);
            for (int k = 0; k < grandChildCount; ++k) {
                new QTreeWidgetItem(child);
            }
        }
    }

    QPersistentModelIndex persistent = testWidget->model()->index(persistentRow,
                                                                  persistentColumn);
    QVERIFY(persistent.isValid());

    QTreeWidgetItem *top = testWidget->topLevelItem(0);
    QTreeWidgetItem *child = top->child(0);

    for (int n = 0; n < deleteGrandChildCount; ++n)
        delete child->child(0);
    QCOMPARE(child->childCount(), expectedGrandChildCount);

    for (int m = 0; m < deleteChildCount; ++m)
        delete top->child(0);
    QCOMPARE(top->childCount(), expectedChildCount);

    for (int l = 0; l < deleteTopLevelCount; ++l)
        delete testWidget->topLevelItem(0);
    QCOMPARE(testWidget->topLevelItemCount(), expectedTopLevelCount);

    QCOMPARE(persistent.isValid(), persistentIsValid);
}


void tst_QTreeWidget::itemAboveOrBelow()
{
    QTreeWidget tw;
    tw.setColumnCount(1);
    QTreeWidgetItem *twi = new QTreeWidgetItem(&tw, QStringList() << "Test");
    QTreeWidgetItem *twi2 = new QTreeWidgetItem(&tw, QStringList() << "Test 2");
    QTreeWidgetItem *twi3 = new QTreeWidgetItem(&tw, QStringList() << "Test 3");
    tw.show();
    QCOMPARE(tw.itemAbove(twi2), twi);
    QCOMPARE(tw.itemBelow(twi2), twi3);
}

void tst_QTreeWidget::itemStreaming_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("toolTip");
    QTest::addColumn<int>("column");

    QTest::newRow("Data") << "item text" << "tool tip text" << 0;
}

void tst_QTreeWidget::itemStreaming()
{
    QFETCH(QString, text);
    QFETCH(QString, toolTip);
    QFETCH(int, column);

    QTreeWidgetItem item(testWidget);
    QCOMPARE(item.text(column), QString());
    QCOMPARE(item.toolTip(column), QString());

    item.setText(column, text);
    item.setToolTip(column, toolTip);
    QCOMPARE(item.text(column), text);
    QCOMPARE(item.toolTip(column), toolTip);

    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << item;

    QTreeWidgetItem item2(testWidget);
    QCOMPARE(item2.text(column), QString());
    QCOMPARE(item2.toolTip(column), QString());

    QVERIFY(!buffer.isEmpty());

    QDataStream in(&buffer, QIODevice::ReadOnly);
    in >> item2;
    QCOMPARE(item2.text(column), text);
    QCOMPARE(item2.toolTip(column), toolTip);
}

void tst_QTreeWidget::insertTopLevelItems_data()
{
    QTest::addColumn<QStringList>("initialText");
    QTest::addColumn<QStringList>("insertText");
    QTest::addColumn<int>("insertTopLevelIndex");
    QTest::addColumn<int>("expectedTopLevelIndex");
    QTest::addColumn<int>("insertChildIndex");
    QTest::addColumn<int>("expectedChildIndex");

    QStringList initial = (QStringList() << "foo" << "bar");
    QStringList insert = (QStringList() << "baz");

    QTest::newRow("Insert at count") << initial << insert
                                     << initial.count() << initial.count()
                                     << initial.count() << initial.count();
    QTest::newRow("Insert in the middle") << initial << insert
                                          << (initial.count() / 2) << (initial.count() / 2)
                                          << (initial.count() / 2) << (initial.count() / 2);
    QTest::newRow("Insert less than 0") << initial << insert
                                        << -1 << -1
                                        << -1 << -1;
    QTest::newRow("Insert beyond count") << initial << insert
                                         << initial.count() + 1 << -1
                                         << initial.count() + 1 << -1;
}

void tst_QTreeWidget::insertTopLevelItems()
{
    QFETCH(QStringList, initialText);
    QFETCH(QStringList, insertText);
    QFETCH(int, insertTopLevelIndex);
    QFETCH(int, expectedTopLevelIndex);
    QFETCH(int, insertChildIndex);
    QFETCH(int, expectedChildIndex);
    testWidget->setSortingEnabled(false);

    { // insert the initial items
        QCOMPARE(testWidget->topLevelItemCount(), 0);
        for (int i = 0; i < initialText.count(); ++i) {
            QTreeWidgetItem *top = new QTreeWidgetItem(QStringList(initialText.at(i)));
            testWidget->addTopLevelItem(top);
            QCOMPARE(testWidget->indexOfTopLevelItem(top), i);
        }
        QCOMPARE(testWidget->topLevelItemCount(), initialText.count());
    }

    { // test adding children
        QTreeWidgetItem *topLevel = testWidget->topLevelItem(0);
        for (int i = 0; i < initialText.count(); ++i)
            topLevel->addChild(new QTreeWidgetItem(QStringList(initialText.at(i))));
        QCOMPARE(topLevel->childCount(), initialText.count());
    }

    { // test adding more top level items
        QTreeWidgetItem *topsy = new QTreeWidgetItem(QStringList(insertText.at(0)));
        testWidget->insertTopLevelItem(insertTopLevelIndex, topsy);
        if (expectedTopLevelIndex == -1) {
            QCOMPARE(testWidget->topLevelItemCount(), initialText.count());
            delete topsy;
        } else {
            QTreeWidgetItem *item = testWidget->topLevelItem(expectedTopLevelIndex);
            QVERIFY(item != 0);
            QCOMPARE(item->text(0), insertText.at(0));
            QCOMPARE(testWidget->indexOfTopLevelItem(item), expectedTopLevelIndex);
        }
    }

    { // test adding more children
        QTreeWidgetItem *topLevel = testWidget->topLevelItem(0);
        QVERIFY(topLevel != 0);
        QTreeWidgetItem *child = new QTreeWidgetItem(QStringList(insertText.at(0)));
        topLevel->insertChild(insertChildIndex, child);
        if (expectedChildIndex == -1) {
            QCOMPARE(topLevel->childCount(), initialText.count());
            delete child;
        } else {
            QTreeWidgetItem *item = topLevel->child(expectedChildIndex);
            QVERIFY(item != 0);
            QCOMPARE(item->text(0), insertText.at(0));
        }
    }
}

static void fillTreeWidget(QTreeWidgetItem *parent, int rows)
{
    const int columns = parent->treeWidget()->columnCount();
    for (int r = 0; r < rows; ++r) {
        QTreeWidgetItem *w = new QTreeWidgetItem(parent);
        for ( int c = 0; c < columns; ++c ) {
            QString s = QString("[r:%1,c:%2]").arg(r).arg(c);
            w->setText(c, s);
        }
        fillTreeWidget(w, rows - r - 1);
    }
}

static void fillTreeWidget(QTreeWidget *tree, int rows)
{
    for (int r = 0; r < rows; ++r) {
        QTreeWidgetItem *w = new QTreeWidgetItem();
        for ( int c = 0; c < tree->columnCount(); ++c ) {
            QString s = QString("[r:%1,c:%2]").arg(r).arg(c);
            w->setText(c, s);
        }
        tree->insertTopLevelItem(r, w);
        fillTreeWidget(w, rows - r - 1);
    }
}

void tst_QTreeWidget::keyboardNavigation()
{
    int rows = 8;

    fillTreeWidget(testWidget, rows);

    QVector<Qt::Key> keymoves;
    keymoves << Qt::Key_Down << Qt::Key_Right << Qt::Key_Left
	     << Qt::Key_Down << Qt::Key_Down << Qt::Key_Down << Qt::Key_Down
	     << Qt::Key_Right
	     << Qt::Key_Up << Qt::Key_Left << Qt::Key_Left
	     << Qt::Key_Up << Qt::Key_Down << Qt::Key_Up << Qt::Key_Up
	     << Qt::Key_Up << Qt::Key_Up << Qt::Key_Up << Qt::Key_Up
             << Qt::Key_Down << Qt::Key_Right << Qt::Key_Down << Qt::Key_Down
             << Qt::Key_Down << Qt::Key_Right << Qt::Key_Down << Qt::Key_Down
	     << Qt::Key_Left << Qt::Key_Left << Qt::Key_Up << Qt::Key_Down
             << Qt::Key_Up << Qt::Key_Up << Qt::Key_Up << Qt::Key_Left
             << Qt::Key_Down << Qt::Key_Right << Qt::Key_Right << Qt::Key_Right
             << Qt::Key_Left << Qt::Key_Left << Qt::Key_Right << Qt::Key_Left;

    int row    = 0;
    QTreeWidgetItem *item = testWidget->topLevelItem(0);
    testWidget->setCurrentItem(item);
    QCOMPARE(testWidget->currentItem(), item);
    QApplication::instance()->processEvents();

    QScrollBar *scrollBar = testWidget->horizontalScrollBar();
    bool checkScroll = false;
    for (int i = 0; i < keymoves.size(); ++i) {
        Qt::Key key = keymoves.at(i);
        int valueBeforeClick = scrollBar->value();
        if (valueBeforeClick >= scrollBar->singleStep())
            checkScroll = true;
        else
            checkScroll = false;
        QTest::keyClick(testWidget, key);
        QApplication::instance()->processEvents();

        switch (key) {
        case Qt::Key_Up:
	    if (row > 0) {
                if (item->parent())
                    item = item->parent()->child(row - 1);
                else
                    item = testWidget->topLevelItem(row - 1);
		row -= 1;
	    } else if (item->parent()) {
		item = item->parent();
		row = item->parent() ? item->parent()->indexOfChild(item) : testWidget->indexOfTopLevelItem(item);
	    }
            break;
        case Qt::Key_Down:
            if (testWidget->isItemExpanded(item)) {
                row = 0;
                item = item->child(row);
            } else {
                row = qMin(rows - 1, row + 1);
                if (item->parent())
                    item = item->parent()->child(row);
                else
                    item = testWidget->topLevelItem(row);
            }
            break;
        case Qt::Key_Left:
            if (checkScroll) {
                QVERIFY(testWidget->isItemExpanded(item));
                QCOMPARE(scrollBar->value(), valueBeforeClick - scrollBar->singleStep());
            }
            // windows style right will walk to the parent
            if (testWidget->currentItem() != item) {
                QCOMPARE(testWidget->currentItem(), item->parent());
                item = testWidget->currentItem();
                row = item->parent() ? item->parent()->indexOfChild(item) : testWidget->indexOfTopLevelItem(item);;
            }
            break;
        case Qt::Key_Right:
            if (checkScroll)
                QCOMPARE(scrollBar->value(), valueBeforeClick + scrollBar->singleStep());
	    // windows style right will walk to the first child
            if (testWidget->currentItem() != item) {
                QCOMPARE(testWidget->currentItem()->parent(), item);
                row = item->indexOfChild(testWidget->currentItem());
                item = testWidget->currentItem();
                QCOMPARE(row, 0);
            }
            break;
        default:
            QVERIFY(false);
        }

        QTreeWidgetItem *current = testWidget->currentItem();
        QCOMPARE(current->text(0), QString("[r:%1,c:0]").arg(row));
        if (current->parent())
            QCOMPARE(current->parent()->indexOfChild(current), row);
        else
            QCOMPARE(testWidget->indexOfTopLevelItem(current), row);
    }
}

void tst_QTreeWidget::scrollToItem()
{
    // Check if all parent nodes of the item found are expanded.
    // Reported in task #78761
    QTreeWidgetItem *bar;
    QTreeWidgetItem *search;
    for (int i=0; i<2; ++i) {
        bar = new QTreeWidgetItem(testWidget);
        bar->setText(0, QString::number(i));

        for (int j=0; j<2; ++j) {
            QTreeWidgetItem *foo = new QTreeWidgetItem(bar);
            foo->setText(0, bar->text(0) + QString::number(j));

            for (int k=0; k<2; ++k) {
                QTreeWidgetItem *yo = new QTreeWidgetItem(foo);
                yo->setText(0, foo->text(0) + QString::number(k));
                search = yo;
            }
        }
    }

    testWidget->setHeaderLabels(QStringList() << "foo");
    testWidget->scrollToItem(search);
    QVERIFY(search->text(0) == "111");

    bar = search->parent();
    QVERIFY(testWidget->isItemExpanded(bar));
    bar = bar->parent();
    QVERIFY(testWidget->isItemExpanded(bar));
}

// From task #85413
void tst_QTreeWidget::setSortingEnabled()
{
    QStringList hl;
    hl << "ID";
    testWidget->setColumnCount(hl.count());
    testWidget->setHeaderLabels(hl);

    QTreeWidgetItem *item1 = new QTreeWidgetItem(testWidget);
    QTreeWidgetItem *item2 = new QTreeWidgetItem(testWidget);

    testWidget->setSortingEnabled(true);
    QCOMPARE(testWidget->isSortingEnabled(), true);
    QCOMPARE(testWidget->isSortingEnabled(), testWidget->header()->isSortIndicatorShown());
    QCOMPARE(testWidget->topLevelItem(0), item1);
    QCOMPARE(testWidget->topLevelItem(1), item2);

    // Make sure we do it twice
    testWidget->setSortingEnabled(true);
    QCOMPARE(testWidget->isSortingEnabled(), true);
    QCOMPARE(testWidget->isSortingEnabled(), testWidget->header()->isSortIndicatorShown());

    testWidget->setSortingEnabled(false);
    QCOMPARE(testWidget->isSortingEnabled(), false);
    QCOMPARE(testWidget->isSortingEnabled(), testWidget->header()->isSortIndicatorShown());

    testWidget->setSortingEnabled(false);
    QCOMPARE(testWidget->isSortingEnabled(), false);
    QCOMPARE(testWidget->isSortingEnabled(), testWidget->header()->isSortIndicatorShown());

    // And back again so that we make sure that we test the transition from false to true
    testWidget->setSortingEnabled(true);
    QCOMPARE(testWidget->isSortingEnabled(), true);
    QCOMPARE(testWidget->isSortingEnabled(), testWidget->header()->isSortIndicatorShown());

    testWidget->setSortingEnabled(true);
    QCOMPARE(testWidget->isSortingEnabled(), true);
    QCOMPARE(testWidget->isSortingEnabled(), testWidget->header()->isSortIndicatorShown());

    testWidget->setSortingEnabled(false);
}

void tst_QTreeWidget::addChild()
{
    QTreeWidget tree;
    for (int x = 0; x < 2; ++x) {
        QTreeWidget *view = x ? &tree : static_cast<QTreeWidget*>(0);
        QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)view);
        QCOMPARE(item->childCount(), 0);

        // try to add 0
        item->addChild(0);
        QCOMPARE(item->childCount(), 0);
        QCOMPARE(item->indexOfChild(0), -1);

        // add one at a time
        QList<QTreeWidgetItem*> children;
        for (int i = 0; i < 10; ++i) {
            QTreeWidgetItem *child = new QTreeWidgetItem();
            item->addChild(child);
            QCOMPARE(item->childCount(), i+1);
            QCOMPARE(item->child(i), child);
            QCOMPARE(item->indexOfChild(child), i);
            QCOMPARE(child->parent(), item);
            QCOMPARE(child->treeWidget(), view);
            item->addChild(child);
            QCOMPARE(item->childCount(), i+1);
            children.append(child);
        }

        // take them all
        QList<QTreeWidgetItem*> taken = item->takeChildren();
        QCOMPARE(taken, children);
        QCOMPARE(item->childCount(), 0);
        for (int i = 0; i < taken.count(); ++i) {
            QCOMPARE(taken.at(i)->parent(), static_cast<QTreeWidgetItem*>(0));
            QCOMPARE(taken.at(i)->treeWidget(), static_cast<QTreeWidget*>(0));
            item->addChild(taken.at(i)); // re-add
        }

        // delete one at a time
        while (!children.isEmpty()) {
            QTreeWidgetItem *ti = children.takeFirst();
            delete ti;
            QCOMPARE(item->childCount(), children.count());
            for (int i = 0; i < children.count(); ++i)
                QCOMPARE(item->child(i), children.at(i));
        }

        // add none
        {
            int count = item->childCount();
            item->addChildren(QList<QTreeWidgetItem*>());
            QCOMPARE(item->childCount(), count);
        }

        // add many at a time
        const int count = 10;
        for (int i = 0; i < 100; i += count) {
            QList<QTreeWidgetItem*> list;
            for (int j = 0; j < count; ++j)
                list << new QTreeWidgetItem(QStringList() << QString("%0").arg(j));
            item->addChildren(list);
            QCOMPARE(item->childCount(), count + i);
            for (int j = 0; j < count; ++j) {
                QCOMPARE(item->child(i+j), list.at(j));
                QCOMPARE(item->child(i+j)->parent(), item);
            }

            item->addChildren(list);
            QCOMPARE(item->childCount(), count + i);
        }

        if (!view)
            delete item;
    }
}

void tst_QTreeWidget::setData()
{
    {
        QTreeWidgetItem *headerItem = new QTreeWidgetItem();
        headerItem->setText(0, "Item1");
        testWidget->setHeaderItem(headerItem);

        QSignalSpy headerDataChangedSpy(
            testWidget->model(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)));
        QSignalSpy dataChangedSpy(
            testWidget->model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)));
        QSignalSpy itemChangedSpy(
            testWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)));
        headerItem->setText(0, "test");
        QCOMPARE(dataChangedSpy.count(), 0);
        QCOMPARE(headerDataChangedSpy.count(), 1);
        QCOMPARE(itemChangedSpy.count(), 0); // no itemChanged() signal for header item

        headerItem->setData(-1, -1, QVariant());
    }

    {
        QSignalSpy itemChangedSpy(
            testWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)));
        QTreeWidgetItem *item = new QTreeWidgetItem();
        testWidget->addTopLevelItem(item);
        for (int x = 0; x < 2; ++x) {
            for (int i = 1; i <= 2; ++i) {
                for (int j = 0; j < 5; ++j) {
                    QVariantList args;
                    QString text = QString("text %0").arg(i);
                    item->setText(j, text);
                    QCOMPARE(item->text(j), text);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setText(j, text);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    QPixmap pixmap(32, 32);
                    pixmap.fill((i == 1) ? Qt::red : Qt::green);
                    QIcon icon(pixmap);
                    item->setIcon(j, icon);
                    QCOMPARE(item->icon(j), icon);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setIcon(j, icon);
                    // #### shouldn't cause dataChanged()
                    QCOMPARE(itemChangedSpy.count(), 1);
                    itemChangedSpy.clear();

                    QString toolTip = QString("toolTip %0").arg(i);
                    item->setToolTip(j, toolTip);
                    QCOMPARE(item->toolTip(j), toolTip);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setToolTip(j, toolTip);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    QString statusTip = QString("statusTip %0").arg(i);
                    item->setStatusTip(j, statusTip);
                    QCOMPARE(item->statusTip(j), statusTip);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setStatusTip(j, statusTip);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    QString whatsThis = QString("whatsThis %0").arg(i);
                    item->setWhatsThis(j, whatsThis);
                    QCOMPARE(item->whatsThis(j), whatsThis);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setWhatsThis(j, whatsThis);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    QSize sizeHint(64*i, 48*i);
                    item->setSizeHint(j, sizeHint);
                    QCOMPARE(item->sizeHint(j), sizeHint);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setSizeHint(j, sizeHint);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    QFont font;
                    item->setFont(j, font);
                    QCOMPARE(item->font(j), font);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setFont(j, font);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    Qt::Alignment textAlignment((i == 1)
                                                ? Qt::AlignLeft|Qt::AlignVCenter
                                                : Qt::AlignRight);
                    item->setTextAlignment(j, textAlignment);
                    QCOMPARE(item->textAlignment(j), int(textAlignment));
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setTextAlignment(j, textAlignment);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    QColor backgroundColor((i == 1) ? Qt::blue : Qt::yellow);
                    item->setBackground(j, backgroundColor);
                    QCOMPARE(item->background(j).color(), backgroundColor);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setBackground(j, backgroundColor);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    QColor textColor((i == 1) ? Qt::green : Qt::cyan);
                    item->setTextColor(j, textColor);
                    QCOMPARE(item->textColor(j), textColor);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setTextColor(j, textColor);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    Qt::CheckState checkState((i == 1) ? Qt::PartiallyChecked : Qt::Checked);
                    item->setCheckState(j, checkState);
                    QCOMPARE(item->checkState(j), checkState);
                    QCOMPARE(itemChangedSpy.count(), 1);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setCheckState(j, checkState);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    QCOMPARE(item->text(j), text);
                    QCOMPARE(item->icon(j), icon);
                    QCOMPARE(item->toolTip(j), toolTip);
                    QCOMPARE(item->statusTip(j), statusTip);
                    QCOMPARE(item->whatsThis(j), whatsThis);
                    QCOMPARE(item->sizeHint(j), sizeHint);
                    QCOMPARE(item->font(j), font);
                    QCOMPARE(item->textAlignment(j), int(textAlignment));
                    QCOMPARE(item->background(j).color(), backgroundColor);
                    QCOMPARE(item->textColor(j), textColor);
                    QCOMPARE(item->checkState(j), checkState);

                    QCOMPARE(qvariant_cast<QString>(item->data(j, Qt::DisplayRole)), text);
                    QCOMPARE(qvariant_cast<QIcon>(item->data(j, Qt::DecorationRole)), icon);
                    QCOMPARE(qvariant_cast<QString>(item->data(j, Qt::ToolTipRole)), toolTip);
                    QCOMPARE(qvariant_cast<QString>(item->data(j, Qt::StatusTipRole)), statusTip);
                    QCOMPARE(qvariant_cast<QString>(item->data(j, Qt::WhatsThisRole)), whatsThis);
                    QCOMPARE(qvariant_cast<QSize>(item->data(j, Qt::SizeHintRole)), sizeHint);
                    QCOMPARE(qvariant_cast<QFont>(item->data(j, Qt::FontRole)), font);
                    QCOMPARE(qvariant_cast<int>(item->data(j, Qt::TextAlignmentRole)), int(textAlignment));
                    QCOMPARE(qvariant_cast<QBrush>(item->data(j, Qt::BackgroundColorRole)), QBrush(backgroundColor));
                    QCOMPARE(qvariant_cast<QBrush>(item->data(j, Qt::BackgroundRole)), QBrush(backgroundColor));
                    QCOMPARE(qvariant_cast<QColor>(item->data(j, Qt::TextColorRole)), textColor);
                    QCOMPARE(qvariant_cast<int>(item->data(j, Qt::CheckStateRole)), int(checkState));

                    item->setBackground(j, pixmap);
                    QCOMPARE(item->background(j).texture(), pixmap);
                    QCOMPARE(qvariant_cast<QBrush>(item->data(j, Qt::BackgroundRole)).texture(), pixmap);
                    args = itemChangedSpy.takeFirst();
                    QCOMPARE(qvariant_cast<QTreeWidgetItem*>(args.at(0)), item);
                    QCOMPARE(qvariant_cast<int>(args.at(1)), j);
                    item->setBackground(j, pixmap);
                    QCOMPARE(itemChangedSpy.count(), 0);

                    item->setData(j, Qt::DisplayRole, QVariant());
                    item->setData(j, Qt::DecorationRole, QVariant());
                    item->setData(j, Qt::ToolTipRole, QVariant());
                    item->setData(j, Qt::StatusTipRole, QVariant());
                    item->setData(j, Qt::WhatsThisRole, QVariant());
                    item->setData(j, Qt::SizeHintRole, QVariant());
                    item->setData(j, Qt::FontRole, QVariant());
                    item->setData(j, Qt::TextAlignmentRole, QVariant());
                    item->setData(j, Qt::BackgroundColorRole, QVariant());
                    item->setData(j, Qt::TextColorRole, QVariant());
                    item->setData(j, Qt::CheckStateRole, QVariant());
                    QCOMPARE(itemChangedSpy.count(), 11);
                    itemChangedSpy.clear();

                    QCOMPARE(item->data(j, Qt::DisplayRole).toString(), QString());
                    QCOMPARE(item->data(j, Qt::DecorationRole), QVariant());
                    QCOMPARE(item->data(j, Qt::ToolTipRole), QVariant());
                    QCOMPARE(item->data(j, Qt::StatusTipRole), QVariant());
                    QCOMPARE(item->data(j, Qt::WhatsThisRole), QVariant());
                    QCOMPARE(item->data(j, Qt::SizeHintRole), QVariant());
                    QCOMPARE(item->data(j, Qt::FontRole), QVariant());
                    QCOMPARE(item->data(j, Qt::TextAlignmentRole), QVariant());
                    QCOMPARE(item->data(j, Qt::BackgroundColorRole), QVariant());
                    QCOMPARE(item->data(j, Qt::BackgroundRole), QVariant());
                    QCOMPARE(item->data(j, Qt::TextColorRole), QVariant());
                    QCOMPARE(item->data(j, Qt::CheckStateRole), QVariant());
                }
            }
        }

        // ### add more data types here

        item->setData(0, Qt::DisplayRole, 5);
        QCOMPARE(item->data(0, Qt::DisplayRole).type(), QVariant::Int);

        item->setData(0, Qt::DisplayRole, "test");
        QCOMPARE(item->data(0, Qt::DisplayRole).type(), QVariant::String);

        item->setData(0, Qt::DisplayRole, 0.4);
        QCOMPARE(item->data(0, Qt::DisplayRole).type(), QVariant::Double);

        delete item;
    }
}

void tst_QTreeWidget::itemData()
{
    QTreeWidget widget;
    QTreeWidgetItem item(&widget);
    widget.setColumnCount(2);
    item.setFlags(item.flags() | Qt::ItemIsEditable);
    item.setData(0, Qt::DisplayRole,  QString("0"));
    item.setData(0, Qt::CheckStateRole, Qt::PartiallyChecked);
    item.setData(0, Qt::UserRole + 0, QString("1"));
    item.setData(0, Qt::UserRole + 1, QString("2"));
    item.setData(0, Qt::UserRole + 2, QString("3"));
    item.setData(0, Qt::UserRole + 3, QString("4"));

    QMap<int, QVariant> flags = widget.model()->itemData(widget.model()->index(0, 0));
    QCOMPARE(flags.count(), 6);
    QCOMPARE(flags[Qt::UserRole + 0].toString(), QString("1"));

    flags = widget.model()->itemData(widget.model()->index(0, 1));
    QCOMPARE(flags.count(), 0);
}

void tst_QTreeWidget::enableDisable()
{
    QTreeWidgetItem *itm = new QTreeWidgetItem();
    for (int i = 0; i < 10; ++i)
        new QTreeWidgetItem(itm);

    // make sure all items are enabled
    QVERIFY(itm->flags() & Qt::ItemIsEnabled);
    for (int j = 0; j < itm->childCount(); ++j)
        QVERIFY(itm->child(j)->flags() & Qt::ItemIsEnabled);

    // disable root and make sure they are all disabled
    itm->setFlags(itm->flags() & ~Qt::ItemIsEnabled);
    QVERIFY(!(itm->flags() & Qt::ItemIsEnabled));
    for (int k = 0; k < itm->childCount(); ++k)
        QVERIFY(!(itm->child(k)->flags() & Qt::ItemIsEnabled));

    // disable a child and make sure they are all still disabled
    itm->child(5)->setFlags(itm->child(5)->flags() & ~Qt::ItemIsEnabled);
    QVERIFY(!(itm->flags() & Qt::ItemIsEnabled));
    for (int l = 0; l < itm->childCount(); ++l)
        QVERIFY(!(itm->child(l)->flags() & Qt::ItemIsEnabled));

    // enable root and make sure all items except one are enabled
    itm->setFlags(itm->flags() | Qt::ItemIsEnabled);
    QVERIFY(itm->flags() & Qt::ItemIsEnabled);
    for (int m = 0; m < itm->childCount(); ++m)
        if (m == 5)
            QVERIFY(!(itm->child(m)->flags() & Qt::ItemIsEnabled));
        else
            QVERIFY(itm->child(m)->flags() & Qt::ItemIsEnabled);
}

void tst_QTreeWidget::match()
{
    QTreeWidget tree;
    QModelIndexList list = tree.model()->match(QModelIndex(), Qt::DisplayRole, QString());

    QVERIFY(list.isEmpty());
}

void tst_QTreeWidget::columnCount()
{
    int columnCountBefore = testWidget->columnCount();
    testWidget->setColumnCount(-1);
    QCOMPARE(testWidget->columnCount(), columnCountBefore);
}

void tst_QTreeWidget::setHeaderLabels()
{
    QStringList list = QString("a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z").split(",");
    testWidget->setHeaderLabels(list);
    QCOMPARE(testWidget->header()->count(), list.count());
}

void tst_QTreeWidget::setHeaderItem()
{
    testWidget->setHeaderItem(0);
    QTreeWidgetItem *headerItem = new QTreeWidgetItem();

    testWidget->setColumnCount(0);
    QCOMPARE(testWidget->header()->count(), 0);
    QCOMPARE(testWidget->columnCount(), 0);

    headerItem->setText(0, "0");
    headerItem->setText(1, "1");
    testWidget->setHeaderItem(headerItem);
    QTest::qWait(100);
    QCOMPARE(testWidget->headerItem(), headerItem);
    QCOMPARE(headerItem->treeWidget(), static_cast<QTreeWidget *>(testWidget));

    QCOMPARE(testWidget->header()->count(), 2);
    QCOMPARE(testWidget->columnCount(), 2);

    headerItem->setText(2, "2");
    QCOMPARE(testWidget->header()->count(), 3);
    QCOMPARE(testWidget->columnCount(), 3);

    delete headerItem;
    testWidget->setColumnCount(3);
    testWidget->setColumnCount(5);
    QCOMPARE(testWidget->model()->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(), QString("1"));
    QCOMPARE(testWidget->model()->headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(), QString("2"));
    QCOMPARE(testWidget->model()->headerData(2, Qt::Horizontal, Qt::DisplayRole).toString(), QString("3"));
    QCOMPARE(testWidget->model()->headerData(3, Qt::Horizontal, Qt::DisplayRole).toString(), QString("4"));
    QCOMPARE(testWidget->model()->headerData(4, Qt::Horizontal, Qt::DisplayRole).toString(), QString("5"));

    headerItem = new QTreeWidgetItem();
    testWidget->setHeaderItem(headerItem);
    testWidget->model()->insertColumns(0, 5, QModelIndex());
    QCOMPARE(testWidget->model()->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(), QString("1"));
    QCOMPARE(testWidget->model()->headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(), QString("2"));
    QCOMPARE(testWidget->model()->headerData(2, Qt::Horizontal, Qt::DisplayRole).toString(), QString("3"));
    QCOMPARE(testWidget->model()->headerData(3, Qt::Horizontal, Qt::DisplayRole).toString(), QString("4"));
    QCOMPARE(testWidget->model()->headerData(4, Qt::Horizontal, Qt::DisplayRole).toString(), QString("5"));
}

void tst_QTreeWidget::itemWidget_data()
{
    editItem_data();
}

void tst_QTreeWidget::itemWidget()
{
    QFETCH(TreeItemList, topLevelItems);

    QTreeWidget tree;
    populate(&tree, topLevelItems, new TreeItem(QStringList() << "1" << "2"));
    tree.show();

    for (int x = 0; x < 2; ++x) {
        QTreeWidgetItemIterator it(&tree);
        while (QTreeWidgetItem *item = (*it++)) {
            for (int col = 0; col < item->columnCount(); ++col) {
                if (x == 0) {
                    QCOMPARE(tree.itemWidget(item, col), static_cast<QWidget*>(0));
                    QWidget *editor = new QLineEdit();
                    tree.setItemWidget(item, col, editor);
                    QCOMPARE(tree.itemWidget(item, col), editor);
                    tree.removeItemWidget(item, col);
                    QCOMPARE(tree.itemWidget(item, col), static_cast<QWidget*>(0));
                } else {
                    // ### should you really be able to open a persistent
                    //     editor for an item that isn't editable??
                    tree.openPersistentEditor(item, col);
                    QWidget *editor = tree.findChild<QLineEdit*>();
                    QVERIFY(editor != 0);
                    tree.closePersistentEditor(item, col);
                }
            }
        }
    }
}

void tst_QTreeWidget::insertItemsWithSorting_data()
{
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<QStringList>("insertItems");
    QTest::addColumn<QStringList>("expectedItems");
    QTest::addColumn<IntList>("expectedRows");

    QTest::newRow("() + (a) = (a)")
        << static_cast<int>(Qt::AscendingOrder)
        << QStringList()
        << (QStringList() << "a")
        << (QStringList() << "a")
        << IntList();
    QTest::newRow("() + (c, b, a) = (a, b, c)")
        << static_cast<int>(Qt::AscendingOrder)
        << QStringList()
        << (QStringList() << "c" << "b" << "a")
        << (QStringList() << "a" << "b" << "c")
        << IntList();
    QTest::newRow("() + (a, b, c) = (c, b, a)")
        << static_cast<int>(Qt::DescendingOrder)
        << QStringList()
        << (QStringList() << "a" << "b" << "c")
        << (QStringList() << "c" << "b" << "a")
        << IntList();
    QTest::newRow("(a) + (b) = (a, b)")
        << static_cast<int>(Qt::AscendingOrder)
        << QStringList("a")
        << (QStringList() << "b")
        << (QStringList() << "a" << "b")
        << (IntList() << 0);
    QTest::newRow("(a) + (b) = (b, a)")
        << static_cast<int>(Qt::DescendingOrder)
        << QStringList("a")
        << (QStringList() << "b")
        << (QStringList() << "b" << "a")
        << (IntList() << 1);
    QTest::newRow("(a, c, b) + (d) = (a, b, c, d)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "b")
        << (QStringList() << "d")
        << (QStringList() << "a" << "b" << "c" << "d")
        << (IntList() << 0 << 1 << 2);
    QTest::newRow("(b, c, a) + (d) = (d, c, b, a)")
        << static_cast<int>(Qt::DescendingOrder)
        << (QStringList() << "b" << "c" << "a")
        << (QStringList() << "d")
        << (QStringList() << "d" << "c" << "b" << "a")
        << (IntList() << 1 << 2 << 3);
    {
        IntList ascendingRows;
        IntList reverseRows;
        QStringList ascendingItems;
        QStringList reverseItems;
        for (int i = 'a'; i <= 'z'; ++i) {
            ascendingItems << QString("%0").arg(QLatin1Char(i));
            reverseItems << QString("%0").arg(QLatin1Char('z' - i + 'a'));
            ascendingRows << i - 'a';
            reverseRows << 'z' - i + 'a';
        }
        QTest::newRow("() + (sorted items) = (sorted items)")
            << static_cast<int>(Qt::AscendingOrder)
            << QStringList()
            << ascendingItems
            << ascendingItems
            << IntList();
        QTest::newRow("(sorted items) + () = (sorted items)")
            << static_cast<int>(Qt::AscendingOrder)
            << ascendingItems
            << QStringList()
            << ascendingItems
            << ascendingRows;
        QTest::newRow("() + (ascending items) = (reverse items)")
            << static_cast<int>(Qt::DescendingOrder)
            << QStringList()
            << ascendingItems
            << reverseItems
            << IntList();
        QTest::newRow("(reverse items) + () = (ascending items)")
            << static_cast<int>(Qt::AscendingOrder)
            << reverseItems
            << QStringList()
            << ascendingItems
            << ascendingRows;
        QTest::newRow("(reverse items) + () = (reverse items)")
            << static_cast<int>(Qt::DescendingOrder)
            << reverseItems
            << QStringList()
            << reverseItems
            << ascendingRows;
    }
}

void tst_QTreeWidget::insertItemsWithSorting()
{
    QFETCH(int, sortOrder);
    QFETCH(QStringList, initialItems);
    QFETCH(QStringList, insertItems);
    QFETCH(QStringList, expectedItems);
    QFETCH(IntList, expectedRows);

    for (int method = 0; method < 5; ++method) {
        QTreeWidget w;
        w.setSortingEnabled(true);
        w.sortItems(0, static_cast<Qt::SortOrder>(sortOrder));
        for (int i = 0; i < initialItems.count(); ++i)
            w.addTopLevelItem(new QTreeWidgetItem(QStringList() << initialItems.at(i)));

        QAbstractItemModel *model = w.model();
        QList<QPersistentModelIndex> persistent;
        for (int j = 0; j < model->rowCount(QModelIndex()); ++j)
            persistent << model->index(j, 0, QModelIndex());

        switch (method) {
            case 0:
                // insert using item constructor
                for (int i = 0; i < insertItems.size(); ++i)
                    new QTreeWidgetItem(&w, QStringList() << insertItems.at(i));
                break;
            case 1:
            {
                // insert using insertTopLevelItems()
                QList<QTreeWidgetItem*> lst;
                for (int i = 0; i < insertItems.size(); ++i)
                    lst << new QTreeWidgetItem(QStringList() << insertItems.at(i));
                w.insertTopLevelItems(0, lst);
                break;
            }
            case 2:
                // insert using insertTopLevelItem()
                for (int i = 0; i < insertItems.size(); ++i)
                    w.insertTopLevelItem(0, new QTreeWidgetItem(QStringList() << insertItems.at(i)));
                break;
            case 3:
            {
                // insert using addTopLevelItems()
                QList<QTreeWidgetItem*> lst;
                for (int i = 0; i < insertItems.size(); ++i)
                    lst << new QTreeWidgetItem(QStringList() << insertItems.at(i));
                w.addTopLevelItems(lst);
                break;
            }
            case 4:
                // insert using addTopLevelItem()
                for (int i = 0; i < insertItems.size(); ++i)
                    w.addTopLevelItem(new QTreeWidgetItem(QStringList() << insertItems.at(i)));
                break;
        }
        QCOMPARE(w.topLevelItemCount(), expectedItems.count());
        for (int i = 0; i < w.topLevelItemCount(); ++i)
            QCOMPARE(w.topLevelItem(i)->text(0), expectedItems.at(i));

        for (int k = 0; k < persistent.count(); ++k)
            QCOMPARE(persistent.at(k).row(), expectedRows.at(k));
    }
}

void tst_QTreeWidget::insertExpandedItemsWithSorting_data()
{
    QTest::addColumn<QStringList>("parentText");
    QTest::addColumn<QStringList>("childText");
    QTest::addColumn<QStringList>("parentResult");
    QTest::addColumn<QStringList>("childResult");
    QTest::newRow("test 1")
        << (QStringList() << "c" << "d" << "a" << "b")
        << (QStringList() << "e" << "h" << "g" << "f")
        << (QStringList() << "d" << "c" << "b" << "a")
        << (QStringList() << "h" << "g" << "f" << "e");
}

// From Task 134978
void tst_QTreeWidget::insertExpandedItemsWithSorting()
{
    QFETCH(QStringList, parentText);
    QFETCH(QStringList, childText);
    QFETCH(QStringList, parentResult);
    QFETCH(QStringList, childResult);

    // create a tree with autosorting enabled
    CustomTreeWidget tree;
    tree.setSortingEnabled(true);

    // insert expanded items in unsorted order
    QList<QTreeWidgetItem *> items;
    for (int i = 0; i < parentText.count(); ++i) {
        QTreeWidgetItem *parent = new QTreeWidgetItem(&tree, QStringList(parentText.at(i)));
        parent->setExpanded(true);
        QVERIFY(parent->isExpanded());
        items << parent;
        for (int j = 0; j < childText.count(); ++j) {
            QTreeWidgetItem *child = new QTreeWidgetItem(parent, QStringList(childText.at(j)));
            items << child;
        }
        QCOMPARE(parent->childCount(), childText.count());
        QVERIFY(parent->isExpanded());
    }
    QVERIFY(tree.model()->rowCount() == parentText.count());

    // verify that the items are still expanded
    foreach (QTreeWidgetItem *item, items) {
        if (item->childCount() > 0)
            QVERIFY(item->isExpanded());
        QModelIndex idx = tree.indexFromItem(const_cast<QTreeWidgetItem *>(item));
        QVERIFY(idx.isValid());
        //QRect rect = tree.visualRect(idx);
        //QVERIFY(rect.isValid());
        // ### it is not guarantied that the index is in the viewport
    }

    // verify that the tree is sorted
    QAbstractItemModel *model = tree.model();
    QList<QPersistentModelIndex> parents;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QPersistentModelIndex parent = model->index(i, 0, QModelIndex());
        parents << parent;
    }
    QList<QPersistentModelIndex> children;
    for (int i = 0; i < model->rowCount(parents.first()); ++i) {
        QPersistentModelIndex child = model->index(i, 0, parents.first());
        children << child;
    }
    for (int i = 0; i < parentResult.count(); ++i) {
        QTreeWidgetItem *item = tree.topLevelItem(i);
        QCOMPARE(item->text(0), parentResult.at(i));
        for (int j = 0; j < childResult.count(); ++j)
            QCOMPARE(item->child(j)->text(0), childResult.at(j));
    }
}

void tst_QTreeWidget::changeDataWithSorting_data()
{
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<int>("itemIndex");
    QTest::addColumn<QString>("newValue");
    QTest::addColumn<QStringList>("expectedItems");
    QTest::addColumn<IntList>("expectedRows");
    QTest::addColumn<bool>("reorderingExpected");

    QTest::newRow("change a to b in (a)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a")
        << 0 << "b"
        << (QStringList() << "b")
        << (IntList() << 0)
        << false;
    QTest::newRow("change a to b in (a, c)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c")
        << 0 << "b"
        << (QStringList() << "b" << "c")
        << (IntList() << 0 << 1)
        << false;
    QTest::newRow("change a to c in (a, b)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "b")
        << 0 << "c"
        << (QStringList() << "b" << "c")
        << (IntList() << 1 << 0)
        << true;
    QTest::newRow("change c to a in (c, b)")
        << static_cast<int>(Qt::DescendingOrder)
        << (QStringList() << "c" << "b")
        << 0 << "a"
        << (QStringList() << "b" << "a")
        << (IntList() << 1 << 0)
        << true;
    QTest::newRow("change e to i in (a, c, e, g)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "e" << "g")
        << 2 << "i"
        << (QStringList() << "a" << "c" << "g" << "i")
        << (IntList() << 0 << 1 << 3 << 2)
        << true;
    QTest::newRow("change e to a in (c, e, g, i)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "c" << "e" << "g" << "i")
        << 1 << "a"
        << (QStringList() << "a" << "c" << "g" << "i")
        << (IntList() << 1 << 0 << 2 << 3)
        << true;
    QTest::newRow("change e to f in (c, e, g, i)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "c" << "e" << "g" << "i")
        << 1 << "f"
        << (QStringList() << "c" << "f" << "g" << "i")
        << (IntList() << 0 << 1 << 2 << 3)
        << false;
}

void tst_QTreeWidget::changeDataWithSorting()
{
    QFETCH(int, sortOrder);
    QFETCH(QStringList, initialItems);
    QFETCH(int, itemIndex);
    QFETCH(QString, newValue);
    QFETCH(QStringList, expectedItems);
    QFETCH(IntList, expectedRows);
    QFETCH(bool, reorderingExpected);

    QTreeWidget w;
    w.setSortingEnabled(true);
    w.sortItems(0, static_cast<Qt::SortOrder>(sortOrder));
    for (int i = 0; i < initialItems.count(); ++i)
        w.addTopLevelItem(new QTreeWidgetItem(QStringList() << initialItems.at(i)));

    QAbstractItemModel *model = w.model();
    QList<QPersistentModelIndex> persistent;
    for (int j = 0; j < model->rowCount(QModelIndex()); ++j)
        persistent << model->index(j, 0, QModelIndex());

    QSignalSpy dataChangedSpy(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)));
    QSignalSpy layoutChangedSpy(model, SIGNAL(layoutChanged()));

    QTreeWidgetItem *item = w.topLevelItem(itemIndex);
    item->setText(0, newValue);
    for (int i = 0; i < expectedItems.count(); ++i) {
        QCOMPARE(w.topLevelItem(i)->text(0), expectedItems.at(i));
        for (int j = 0; j < persistent.count(); ++j) {
            if (persistent.at(j).row() == i) // the same toplevel row
                QCOMPARE(persistent.at(j).internalPointer(), (void *)w.topLevelItem(i));
        }
    }

    for (int k = 0; k < persistent.count(); ++k)
        QCOMPARE(persistent.at(k).row(), expectedRows.at(k));

    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(layoutChangedSpy.count(), reorderingExpected ? 1 : 0);
}

void tst_QTreeWidget::changeDataWithStableSorting_data()
{
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QStringList>("initialItems");
    QTest::addColumn<int>("itemIndex");
    QTest::addColumn<QString>("newValue");
    QTest::addColumn<QStringList>("expectedItems");
    QTest::addColumn<IntList>("expectedRows");
    QTest::addColumn<bool>("reorderingExpected");
    QTest::addColumn<bool>("forceChange");

    QTest::newRow("change a to c in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 0 << "c"
        << (QStringList() << "c" << "c" << "c" << "c" << "e")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << false;
    QTest::newRow("change e to c in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 4 << "c"
        << (QStringList() << "a" << "c" << "c" << "c" << "c")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << false;
    QTest::newRow("change 1st c to c in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 1 << "c"
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << true;
    QTest::newRow("change 2nd c to c in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 2 << "c"
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << true;
    QTest::newRow("change 3rd c to c in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 3 << "c"
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << true;
    QTest::newRow("change 1st c to c in (e, c, c, c, a)")
        << static_cast<int>(Qt::DescendingOrder)
        << (QStringList() << "e" << "c" << "c" << "c" << "a")
        << 1 << "c"
        << (QStringList() << "e" << "c" << "c" << "c" << "a")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << true;
    QTest::newRow("change 2nd c to c in (e, c, c, c, a)")
        << static_cast<int>(Qt::DescendingOrder)
        << (QStringList() << "e" << "c" << "c" << "c" << "a")
        << 2 << "c"
        << (QStringList() << "e" << "c" << "c" << "c" << "a")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << true;
    QTest::newRow("change 3rd c to c in (e, c, c, c, a)")
        << static_cast<int>(Qt::DescendingOrder)
        << (QStringList() << "e" << "c" << "c" << "c" << "a")
        << 3 << "c"
        << (QStringList() << "e" << "c" << "c" << "c" << "a")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << true;
    QTest::newRow("change 1st c to b in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 1 << "b"
        << (QStringList() << "a" << "b" << "c" << "c" << "e")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << false;
    QTest::newRow("change 2nd c to b in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 2 << "b"
        << (QStringList() << "a" << "b" << "c" << "c" << "e")
        << (IntList() << 0 << 2 << 1 << 3 << 4)
        << true
        << false;
    QTest::newRow("change 3rd c to b in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 3 << "b"
        << (QStringList() << "a" << "b" << "c" << "c" << "e")
        << (IntList() << 0 << 2 << 3 << 1 << 4)
        << true
        << false;
    QTest::newRow("change 1st c to d in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 1 << "d"
        << (QStringList() << "a" << "c" << "c" << "d" << "e")
        << (IntList() << 0 << 3 << 1 << 2 << 4)
        << true
        << false;
    QTest::newRow("change 2nd c to d in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 2 << "d"
        << (QStringList() << "a" << "c" << "c" << "d" << "e")
        << (IntList() << 0 << 1 << 3 << 2 << 4)
        << true
        << false;
    QTest::newRow("change 3rd c to d in (a, c, c, c, e)")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "c" << "c" << "c" << "e")
        << 3 << "d"
        << (QStringList() << "a" << "c" << "c" << "d" << "e")
        << (IntList() << 0 << 1 << 2 << 3 << 4)
        << false
        << false;
}

void tst_QTreeWidget::changeDataWithStableSorting()
{
    QFETCH(int, sortOrder);
    QFETCH(QStringList, initialItems);
    QFETCH(int, itemIndex);
    QFETCH(QString, newValue);
    QFETCH(QStringList, expectedItems);
    QFETCH(IntList, expectedRows);
    QFETCH(bool, reorderingExpected);
    QFETCH(bool, forceChange);

    class StableItem : public QTreeWidgetItem
    {
    public:
        StableItem(const QStringList &strings) : QTreeWidgetItem(strings, QTreeWidgetItem::UserType) {}
        void forceChangeData() {
            emitDataChanged();
        }
    };

    QTreeWidget w;
    w.setSortingEnabled(true);
    w.sortItems(0, static_cast<Qt::SortOrder>(sortOrder));
    for (int i = 0; i < initialItems.count(); ++i)
        w.addTopLevelItem(new StableItem(QStringList() << initialItems.at(i)));

    QAbstractItemModel *model = w.model();
    QList<QPersistentModelIndex> persistent;
    for (int j = 0; j < model->rowCount(QModelIndex()); ++j)
        persistent << model->index(j, 0, QModelIndex());

    QSignalSpy dataChangedSpy(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)));
    QSignalSpy layoutChangedSpy(model, SIGNAL(layoutChanged()));

    StableItem *item = static_cast<StableItem *>(w.topLevelItem(itemIndex));
    item->setText(0, newValue);
    if (forceChange)
        item->forceChangeData();
    for (int i = 0; i < expectedItems.count(); ++i) {
        QCOMPARE(w.topLevelItem(i)->text(0), expectedItems.at(i));
        for (int j = 0; j < persistent.count(); ++j) {
            if (persistent.at(j).row() == i) // the same toplevel row
                QCOMPARE(persistent.at(j).internalPointer(), (void *)w.topLevelItem(i));
        }
    }

    for (int k = 0; k < persistent.count(); ++k)
        QCOMPARE(persistent.at(k).row(), expectedRows.at(k));

    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(layoutChangedSpy.count(), reorderingExpected ? 1 : 0);
}

void tst_QTreeWidget::itemOperatorLessThan()
{
    QTreeWidget tw;
    tw.setColumnCount(2);
    {
        QTreeWidgetItem item1(&tw);
        QTreeWidgetItem item2(&tw);
        QCOMPARE(item1 < item2, false);
        item1.setText(1, "a");
        item2.setText(1, "b");
        QCOMPARE(item1 < item2, false);
        item1.setText(0, "a");
        item2.setText(0, "b");
        QCOMPARE(item1 < item2, true);
        tw.sortItems(1, Qt::AscendingOrder);
        item1.setText(0, "b");
        item2.setText(0, "a");
        QCOMPARE(item1 < item2, true);
        tw.sortItems(0, Qt::AscendingOrder);
        item1.setData(0, Qt::DisplayRole, 11);
        item2.setData(0, Qt::DisplayRole, 2);
        QCOMPARE(item1 < item2, false);
    }
}

void tst_QTreeWidget::sortedIndexOfChild_data()
{
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QStringList>("itemTexts");
    QTest::addColumn<QList<int> >("expectedIndexes");

    QTest::newRow("three ascending")
        << int(Qt::AscendingOrder)
        << (QStringList() << "A" << "B" << "C")
        << (QList<int>() << 0 << 1 << 2);


    QTest::newRow("three descending")
        << int(Qt::DescendingOrder)
        << (QStringList() << "A" << "B" << "C")
        << (QList<int>() << 2 << 1 << 0);
}

void tst_QTreeWidget::sortedIndexOfChild()
{
    QFETCH(int, sortOrder);
    QFETCH(QStringList, itemTexts);
    QFETCH(QList<int>, expectedIndexes);

    QTreeWidget tw;
    QList<QTreeWidgetItem*> itms;
    QTreeWidgetItem *top = new QTreeWidgetItem(&tw, QStringList() << "top");

    for (int i = 0; i < itemTexts.count(); ++i)
        itms << new QTreeWidgetItem(top, QStringList() << itemTexts.at(i));

    tw.sortItems(0, (Qt::SortOrder)sortOrder);
    tw.expandAll();

    QVERIFY(itms.count() == expectedIndexes.count());
    for (int j = 0; j < expectedIndexes.count(); ++j)
        QCOMPARE(top->indexOfChild(itms.at(j)), expectedIndexes.at(j));
}

void tst_QTreeWidget::expandAndCallapse()
{
    QTreeWidget tw;
    QTreeWidgetItem *top = new QTreeWidgetItem(&tw, QStringList() << "top");
    QTreeWidgetItem *p;
    for (int i = 0; i < 10; ++i) {
        p = new QTreeWidgetItem(top, QStringList() << QString("%1").arg(i));
        for (int j = 0; j < 10; ++j)
            new QTreeWidgetItem(p, QStringList() << QString("%1").arg(j));
    }
    QSignalSpy spy0(&tw, SIGNAL(itemExpanded(QTreeWidgetItem*)));
    QSignalSpy spy1(&tw, SIGNAL(itemCollapsed(QTreeWidgetItem*)));


    tw.expandItem(p);
    tw.collapseItem(p);
    tw.expandItem(p);
    tw.expandItem(top);
    tw.collapseItem(top);
    tw.collapseItem(top);

    QCOMPARE(spy0.count(), 3);
    QCOMPARE(spy1.count(), 2);
}

void tst_QTreeWidget::setDisabled()
{
    QTreeWidget w;
    QTreeWidgetItem *i1 = new QTreeWidgetItem();
    QTreeWidgetItem *i2 = new QTreeWidgetItem(i1);
    QTreeWidgetItem *i3 = new QTreeWidgetItem(i1);

    QTreeWidgetItem *top = new QTreeWidgetItem(&w);
    top->setDisabled(true);
    top->addChild(i1);
    QCOMPARE(i1->isDisabled(), true);
    QCOMPARE(i2->isDisabled(), true);
    QCOMPARE(i3->isDisabled(), true);

    i1 = top->takeChild(0);
    QCOMPARE(i1->isDisabled(), false);
    QCOMPARE(i2->isDisabled(), false);
    QCOMPARE(i3->isDisabled(), false);

    top->addChild(i1);
    QCOMPARE(i1->isDisabled(), true);
    QCOMPARE(i2->isDisabled(), true);
    QCOMPARE(i3->isDisabled(), true);

    top->setDisabled(false);
    QCOMPARE(i1->isDisabled(), false);
    QCOMPARE(i2->isDisabled(), false);
    QCOMPARE(i3->isDisabled(), false);



    QList<QTreeWidgetItem*> children;
    children.append(new QTreeWidgetItem());
    children.append(new QTreeWidgetItem());
    children.append(new QTreeWidgetItem());
    i1 = top->takeChild(0);

    top->addChildren(children);
    QCOMPARE(top->child(0)->isDisabled(), false);
    QCOMPARE(top->child(1)->isDisabled(), false);
    QCOMPARE(top->child(1)->isDisabled(), false);

    top->setDisabled(true);
    QCOMPARE(top->child(0)->isDisabled(), true);
    QCOMPARE(top->child(1)->isDisabled(), true);
    QCOMPARE(top->child(1)->isDisabled(), true);

    children = top->takeChildren();
    QCOMPARE(children.at(0)->isDisabled(), false);
    QCOMPARE(children.at(1)->isDisabled(), false);
    QCOMPARE(children.at(1)->isDisabled(), false);

}

void tst_QTreeWidget::removeSelectedItem()
{
    QTreeWidget *w = new QTreeWidget();
    w->setSortingEnabled(true);

    QTreeWidgetItem *first = new QTreeWidgetItem();
    first->setText(0, QLatin1String("D"));
    w->addTopLevelItem(first);

    QTreeWidgetItem *itm = new QTreeWidgetItem();
    itm->setText(0, QLatin1String("D"));
    w->addTopLevelItem(itm);

    itm = new QTreeWidgetItem();
    itm->setText(0, QLatin1String("C"));
    w->addTopLevelItem(itm);
    itm->setSelected(true);

    itm = new QTreeWidgetItem();
    itm->setText(0, QLatin1String("A"));
    w->addTopLevelItem(itm);

    //w->show();

    QItemSelectionModel *selModel = w->selectionModel();
    QCOMPARE(selModel->hasSelection(), true);
    QCOMPARE(selModel->selectedRows().count(), 1);

    QTreeWidgetItem *taken = w->takeTopLevelItem(2);
    QCOMPARE(taken->text(0), QLatin1String("C"));

    QCOMPARE(selModel->hasSelection(), false);
    QCOMPARE(selModel->selectedRows().count(), 0);
    QItemSelection sel = selModel->selection();
    QCOMPARE(selModel->isSelected(w->model()->index(0,0)), false);

    delete w;
}

class AnotherTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    AnotherTreeWidget(QWidget *parent = 0) : QTreeWidget(parent) {}
    void deleteCurrent() { if (currentItem()) delete currentItem(); }
};

void tst_QTreeWidget::removeCurrentItem()
{
    AnotherTreeWidget widget;
    QObject::connect(widget.selectionModel(),
                     SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                     &widget, SLOT(clear()));
    QTreeWidgetItem *item = new QTreeWidgetItem(&widget);
    widget.setCurrentItem(item);
    widget.deleteCurrent();
}

void tst_QTreeWidget::removeCurrentItem_task186451()
{
    AnotherTreeWidget widget;
    QTreeWidgetItem *item = new QTreeWidgetItem(&widget, QStringList() << "1");
    QTreeWidgetItem *item2 = new QTreeWidgetItem(&widget, QStringList() << "2");
    widget.setCurrentItem(item);
    widget.deleteCurrent();

    QVERIFY(item2->isSelected());
    QCOMPARE(item2, widget.currentItem());
}


class TreeWidget : QTreeWidget {

public:
    QModelIndex indexFromItem(QTreeWidgetItem *item, int column = 0) const {
        return QTreeWidget::indexFromItem(item, column);
    }
    QTreeWidgetItem *itemFromIndex(const QModelIndex &index) const {
        return QTreeWidget::itemFromIndex(index);
    }
};


void tst_QTreeWidget::randomExpand()
{
    QTreeWidget tree;
    QTreeWidgetItem *item1 = new QTreeWidgetItem(&tree);
    QTreeWidgetItem *item3 = new QTreeWidgetItem(&tree, item1);
    new QTreeWidgetItem(item1);
    new QTreeWidgetItem(item3);

    tree.expandAll();

    /*
        item1
         \- item2
        item3
         \- item4
    */

    QTreeWidgetItem *newItem1 = 0;
    for (int i = 0; i < 100; i++) {
        newItem1 = new QTreeWidgetItem(&tree, item1);
        tree.setItemExpanded(newItem1, true);
        QCOMPARE(tree.isItemExpanded(newItem1), true);

        QTreeWidgetItem *x = new QTreeWidgetItem();
        QCOMPARE(tree.isItemExpanded(newItem1), true);
        newItem1->addChild(x);

        QCOMPARE(tree.isItemExpanded(newItem1), true);
    }

}

void tst_QTreeWidget::crashTest()
{
    QTreeWidget *tree = new QTreeWidget();
    tree->setColumnCount(1);
    tree->show();

    QTreeWidgetItem *item1 = new QTreeWidgetItem(tree);
    item1->setText(0, "item1");
    tree->setItemExpanded(item1, true);
    QTreeWidgetItem *item2 = new QTreeWidgetItem(item1);
    item2->setText(0, "item2");

    QTreeWidgetItem *item3 = new QTreeWidgetItem(tree, item1);
    item3->setText(0, "item3");
    tree->setItemExpanded(item3, true);
    QTreeWidgetItem *item4 = new QTreeWidgetItem(item3);
    item4->setText(0, "item4");

    QTreeWidgetItem *item5 = new QTreeWidgetItem(tree, item3);
    item5->setText(0, "item5");
    tree->setItemExpanded(item5, true);
    QTreeWidgetItem *item6 = new QTreeWidgetItem(item5);
    item6->setText(0, "item6");

    for (int i = 0; i < 1000; i++) {
        QTreeWidgetItem *newItem1 = new QTreeWidgetItem(tree, item1);
        newItem1->setText(0, "newItem");
        QTreeWidgetItem *newItem2 = new QTreeWidgetItem(newItem1);
        newItem2->setText(0, "subItem1");
        QTreeWidgetItem *newItem3 = new QTreeWidgetItem(newItem1, newItem2);
        newItem3->setText(0, "subItem2");
        delete item3;
        item3 = newItem1;
    }
    QApplication::instance()->processEvents();

    delete tree;
}

class CrashWidget : public QTreeWidget
{
public:
    CrashWidget(QWidget *parent = 0) : QTreeWidget(parent), i(0) {
        setSortingEnabled(true);
        timerId = startTimer(10);
    }
    int i;
protected:
    void timerEvent(QTimerEvent * event) {
        if (event->timerId() == timerId) {
            QTreeWidgetItem *newItem = new QTreeWidgetItem((QStringList() << QString::number(i++)));
            m_list.append(newItem);
            insertTopLevelItem(0, newItem);
            while (m_list.count() > 10)
                delete m_list.takeFirst();
        }
        QTreeWidget::timerEvent(event);
    }
private:
    int timerId;
    QList<QTreeWidgetItem*> m_list;
};

void tst_QTreeWidget::sortAndSelect()
{
    CrashWidget w;
    w.resize(1, 1);
    w.show();
    while (w.i < 100) {
        QApplication::processEvents();
        if (w.i & 16) {
            QPoint pt = w.viewport()->rect().center();
            QTest::mouseClick(w.viewport(), Qt::LeftButton, Qt::NoModifier, pt);
        }
    }
    QVERIFY(true);
}

void tst_QTreeWidget::defaultRowSizes()
{
    QTreeWidget *tw = new QTreeWidget();
    tw->setIconSize(QSize(50, 50));
    tw->setColumnCount(6);
    for (int i=0; i<10; ++i) {
        QTreeWidgetItem *it = new QTreeWidgetItem(tw);
        for (int j=0; j<tw->columnCount() - 1; ++j) {
            it->setText(j, "This is a test");
        }
        QPixmap icon = tw->style()->standardPixmap((QStyle::StandardPixmap)(i + QStyle::SP_TitleBarMenuButton));
        if (icon.isNull())
            QSKIP("No pixmap found on current style, skipping this test.");
        it->setIcon(tw->columnCount() - 1,
                    icon.scaled(tw->iconSize()));
    }
    tw->resize(100,100);
    tw->show();
    QApplication::processEvents();

    QRect visualRect = tw->visualItemRect(tw->topLevelItem(0));
    QVERIFY(visualRect.height() >= 50);
}

void tst_QTreeWidget::task191552_rtl()
{
    Qt::LayoutDirection oldDir = qApp->layoutDirection();
    qApp->setLayoutDirection(Qt::RightToLeft);

    QTreeWidget tw;
    tw.setColumnCount(1);
    QTreeWidgetItem *item = new QTreeWidgetItem(&tw);
    item->setText(0, "item 1");
    item->setCheckState(0, Qt::Checked);
    QCOMPARE(item->checkState(0), Qt::Checked);
    tw.show();
    QTest::qWait(50);
    QStyleOptionViewItem opt;
    opt.initFrom(&tw);
    opt.rect = tw.visualItemRect(item);
    // mimic QStyledItemDelegate::initStyleOption logic
    opt.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::HasCheckIndicator;
    opt.checkState = Qt::Checked;
    opt.widget = &tw;
    const QRect checkRect = tw.style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &opt, &tw);
    QTest::mouseClick(tw.viewport(), Qt::LeftButton, Qt::NoModifier, checkRect.center());
    QTest::qWait(200);
    QCOMPARE(item->checkState(0), Qt::Unchecked);

    qApp->setLayoutDirection(oldDir);
}

void tst_QTreeWidget::task203673_selection()
{
    //we try to change the selection by rightclick + ctrl
    //it should do anything when using ExtendedSelection

    QTreeWidget tw;
    tw.setColumnCount(1);
    QTreeWidgetItem *item1 = new QTreeWidgetItem(&tw);
    item1->setText(0, "item 1");
    tw.setSelectionMode(QTreeView::ExtendedSelection);

    QPoint center = tw.visualItemRect(item1).center();
    QCOMPARE(item1->isSelected(), false);

    QTest::mouseClick(tw.viewport(), Qt::RightButton, Qt::ControlModifier, center);
    QCOMPARE(item1->isSelected(), false);

    QTest::mouseClick(tw.viewport(), Qt::LeftButton, Qt::ControlModifier, center);
    QCOMPARE(item1->isSelected(), true);

    QTest::mouseClick(tw.viewport(), Qt::RightButton, Qt::ControlModifier, center);
    QCOMPARE(item1->isSelected(), true); //it shouldn't change

    QTest::mouseClick(tw.viewport(), Qt::LeftButton, Qt::ControlModifier, center);
    QCOMPARE(item1->isSelected(), false);
}


void tst_QTreeWidget::rootItemFlags()
{
    QTreeWidget tw;
    tw.setColumnCount(1);
    QTreeWidgetItem *item = new QTreeWidgetItem(&tw);
    item->setText(0, "item 1");

    QVERIFY(tw.invisibleRootItem()->flags() & Qt::ItemIsDropEnabled);

    tw.invisibleRootItem()->setFlags(tw.invisibleRootItem()->flags() & ~Qt::ItemIsDropEnabled);

    QVERIFY(!(tw.invisibleRootItem()->flags() & Qt::ItemIsDropEnabled));
}

void tst_QTreeWidget::task218661_setHeaderData()
{
    //We check that setting header data out of bounds returns false
    //and doesn't increase the size of the model
    QTreeWidget tw;
    tw.setColumnCount(1);
    QCOMPARE(tw.columnCount(), 1);

    QCOMPARE(tw.model()->setHeaderData(99999, Qt::Horizontal, QVariant()), false);

    QCOMPARE(tw.columnCount(), 1);
}

void tst_QTreeWidget::task245280_sortChildren()
{
    QTreeWidget tw;
    tw.setColumnCount(2);

    QTreeWidgetItem top(&tw);
    top.setText(0,"Col 0");
    top.setText(1,"Col 1");
    QTreeWidgetItem item1(&top);
    item1.setText(0,"X");
    item1.setText(1,"0");
    QTreeWidgetItem item2(&top);
    item2.setText(0,"A");
    item2.setText(1,"4");
    QTreeWidgetItem item3(&top);
    item3.setText(0,"E");
    item3.setText(1,"1");
    QTreeWidgetItem item4(&top);
    item4.setText(0,"Z");
    item4.setText(1,"3");
    QTreeWidgetItem item5(&top);
    item5.setText(0,"U");
    item5.setText(1,"2");
    tw.expandAll();
    tw.show();
    top.sortChildren(1,Qt::AscendingOrder);

    for (int i = 0; i < top.childCount(); ++i)
        QCOMPARE(top.child(i)->text(1), QString::number(i));
}

void tst_QTreeWidget::task253109_itemHeight()
{
    QTreeWidget treeWidget;
    treeWidget.setColumnCount(1);
    treeWidget.show();
    QTest::qWait(200);

    QTreeWidgetItem item(&treeWidget);
    class MyWidget : public QWidget
    {
        virtual QSize sizeHint() const { return QSize(200,100); }
    } w;
    treeWidget.setItemWidget(&item, 0, &w);

    QTest::qWait(200);
    QCOMPARE(w.geometry(), treeWidget.visualItemRect(&item));

}

void tst_QTreeWidget::task206367_duplication()
{
    QWidget topLevel;
    QTreeWidget treeWidget(&topLevel);
    topLevel.show();
    treeWidget.resize(200, 200);

    treeWidget.setSortingEnabled(true);
    QTreeWidgetItem* rootItem = new QTreeWidgetItem( &treeWidget, QStringList("root") );
    for (int nFile = 0; nFile < 2; nFile++ )  {
        QTreeWidgetItem* itemFile = new QTreeWidgetItem(rootItem, QStringList(QString::number(nFile)));
        for (int nRecord = 0; nRecord < 2; nRecord++)
            new QTreeWidgetItem(itemFile ,  QStringList(QString::number(nRecord)));
        itemFile->setExpanded(true);
    }
    rootItem->setExpanded(true);
    QTest::qWait(2000);

    //there should be enough room for 2x2 items.  If there is a scrollbar, it means the items are duplicated
    QVERIFY(!treeWidget.verticalScrollBar()->isVisible());

}

void tst_QTreeWidget::itemSelectionChanged()
{
    QVERIFY(testWidget);
    if(testWidget->topLevelItem(0))
        QVERIFY(testWidget->topLevelItem(0)->isSelected());
}

void tst_QTreeWidget::selectionOrder()
{
    testWidget->setColumnCount(1);
    QList<QTreeWidgetItem *> items;
    for (int i = 0; i < 10; ++i)
        items.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("item: %1").arg(i))));
    testWidget->insertTopLevelItems(0, items);

    QModelIndex idx = testWidget->indexFromItem(items[0]);
    connect(testWidget, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    testWidget->selectionModel()->select(idx, QItemSelectionModel::SelectCurrent);
    disconnect(testWidget, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
}

void tst_QTreeWidget::setSelectionModel()
{
    QTreeWidget tree;
    for(int i = 0; i < 3; ++i)
        new QTreeWidgetItem(&tree, QStringList(QString::number(i)));
    QItemSelectionModel selection(tree.model());
    selection.select(tree.model()->index(1,0), QItemSelectionModel::Select);
    tree.setSelectionModel(&selection);
    QCOMPARE(tree.topLevelItem(1)->isSelected(), true);
}

void tst_QTreeWidget::task217309()
{
    QTreeWidgetItem item;
    item.setFlags(item.flags() | Qt::ItemIsTristate);
    QTreeWidgetItem subitem1;
    subitem1.setFlags(subitem1.flags() | Qt::ItemIsTristate);
    QTreeWidgetItem subitem2;
    subitem2.setFlags(subitem2.flags() | Qt::ItemIsTristate);
    item.addChild(&subitem1);
    item.addChild(&subitem2);
    subitem1.setCheckState(0, Qt::Checked);
    subitem2.setCheckState(0, Qt::Unchecked);

    QVERIFY(item.data(0, Qt::CheckStateRole) == Qt::PartiallyChecked);

    subitem2.setCheckState(0, Qt::PartiallyChecked);
    QVERIFY(item.data(0, Qt::CheckStateRole) == Qt::PartiallyChecked);

    subitem2.setCheckState(0, Qt::Checked);
    QVERIFY(item.data(0, Qt::CheckStateRole) == Qt::Checked);
}

class TreeWidgetItem : public QTreeWidgetItem
{

public:
    void _emitDataChanged() { emitDataChanged(); }

};

void tst_QTreeWidget::emitDataChanged()
{

    QTreeWidget *tree = new QTreeWidget;
    QSignalSpy spy(tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)));
    TreeWidgetItem *item = new TreeWidgetItem();
    tree->insertTopLevelItem(0, item);
    item->_emitDataChanged();
    QCOMPARE(spy.count(), 1);

}

void tst_QTreeWidget::setCurrentItemExpandsParent()
{
    QTreeWidget w;
    w.setColumnCount(1);
    QTreeWidgetItem *i1 = new QTreeWidgetItem(&w, QStringList() << "parent");
    QTreeWidgetItem *i2 = new QTreeWidgetItem(i1, QStringList() << "child");
    QVERIFY(!i2->isExpanded());
    QVERIFY(w.currentItem() == 0);
    w.setCurrentItem(i2);
    QVERIFY(!i2->isExpanded());
    QCOMPARE(w.currentItem(), i2);
}

void tst_QTreeWidget::task239150_editorWidth()
{
    //we check that an item with no text will get an editor with a correct size
    QTreeWidget tree;

    QStyleOptionFrameV2 opt;
    opt.init(&tree);
    const int minWidth = tree.style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(0, 0).
        expandedTo(QApplication::globalStrut()), 0).width();

    {
        QTreeWidgetItem item;
        item.setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        tree.addTopLevelItem(&item);
        QVERIFY(tree.itemWidget(&item, 0) == 0);
        tree.editItem(&item);
        QVERIFY(tree.itemWidget(&item, 0));
        QVERIFY(tree.itemWidget(&item, 0)->width() >= minWidth);
    }

    //now let's test it with an item with a lot of text
    {
        QTreeWidgetItem item;
        item.setText(0, "foooooooooooooooooooooooo");
        item.setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        tree.addTopLevelItem(&item);
        QVERIFY(tree.itemWidget(&item, 0) == 0);
        tree.editItem(&item);
        QVERIFY(tree.itemWidget(&item, 0));
        QVERIFY(tree.itemWidget(&item, 0)->width() >= minWidth + tree.fontMetrics().width(item.text(0)));
    }
}



void tst_QTreeWidget::setTextUpdate()
{
    QTreeWidget treeWidget;
    treeWidget.setColumnCount(2);

    class MyItemDelegate : public QStyledItemDelegate
    {
    public:
        MyItemDelegate() : numPaints(0) { }
        void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const
        {
            numPaints++;
            QStyledItemDelegate::paint(painter, option, index);
        }

        mutable int numPaints;
    } delegate;

    treeWidget.setItemDelegate(&delegate);
    treeWidget.show();
    QStringList strList;
    strList << "variable1" << "0";
    QTreeWidgetItem *item = new QTreeWidgetItem(strList);
    treeWidget.insertTopLevelItem(0, item);
    QTest::qWait(50);
    QTRY_VERIFY(delegate.numPaints > 0);
    delegate.numPaints = 0;

    item->setText(1, "42");
    QApplication::processEvents();
    QTRY_VERIFY(delegate.numPaints > 0);
}

void tst_QTreeWidget::taskQTBUG2844_visualItemRect()
{
    CustomTreeWidget tree;
    tree.resize(150, 100);
    tree.setColumnCount(3);
    QTreeWidgetItem item(&tree);

    QRect rectCol0 = tree.visualRect(tree.indexFromItem(&item, 0));
    QRect rectCol1 = tree.visualRect(tree.indexFromItem(&item, 1));
    QRect rectCol2 = tree.visualRect(tree.indexFromItem(&item, 2));

    QCOMPARE(tree.visualItemRect(&item), rectCol0 | rectCol2);
    tree.setColumnHidden(2, true);
    QCOMPARE(tree.visualItemRect(&item), rectCol0 | rectCol1);
}

void tst_QTreeWidget::setChildIndicatorPolicy()
{
    QTreeWidget treeWidget;
    treeWidget.setColumnCount(1);

    class MyItemDelegate : public QStyledItemDelegate
    {
    public:
        MyItemDelegate() : numPaints(0), expectChildren(false)  { }
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
        {
            numPaints++;
            QCOMPARE(!(option.state & QStyle::State_Children), !expectChildren);
            QStyledItemDelegate::paint(painter, option, index);
        }
        mutable int numPaints;
        bool expectChildren;
    } delegate;

    treeWidget.setItemDelegate(&delegate);
    treeWidget.show();

    QTreeWidgetItem *item = new QTreeWidgetItem(QStringList("Hello"));
    treeWidget.insertTopLevelItem(0, item);
    QTest::qWait(50);
    QTRY_VERIFY(delegate.numPaints > 0);

    delegate.numPaints = 0;
    delegate.expectChildren = true;
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    QApplication::processEvents();
    QTRY_COMPARE(delegate.numPaints, 1);

    delegate.numPaints = 0;
    delegate.expectChildren = false;
    item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    QApplication::processEvents();
    QTRY_COMPARE(delegate.numPaints, 1);

    delegate.numPaints = 0;
    delegate.expectChildren = true;
    new QTreeWidgetItem(item);
    QApplication::processEvents();
    QTRY_COMPARE(delegate.numPaints, 1);

    delegate.numPaints = 0;
    delegate.expectChildren = false;
    item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
    QApplication::processEvents();
    QTRY_COMPARE(delegate.numPaints, 1);
}

void tst_QTreeWidget::task20345_sortChildren()
{
    // This test case is considered successful if it is executed (no crash in sorting)
    QTreeWidget tw;
    tw.setColumnCount(3);
    tw.headerItem()->setText(0, "Col 0");
    tw.headerItem()->setText(1, "Col 1");
    tw.header()->setSortIndicator(0, Qt::AscendingOrder);
    tw.setSortingEnabled(true);
    tw.show();

    QTreeWidgetItem *rootItem = 0;
    QTreeWidgetItem *childItem = 0;

    rootItem = new QTreeWidgetItem(&tw, QStringList("a"));
    childItem = new QTreeWidgetItem(rootItem);
    childItem->setText(1, "3");
    childItem = new QTreeWidgetItem(rootItem);
    childItem->setText(1, "1");
    childItem = new QTreeWidgetItem(rootItem);
    childItem->setText(1, "2");

    tw.setCurrentItem(tw.topLevelItem(0));

    QTreeWidgetItem * curItem = tw.currentItem();
    int childCount = curItem->childCount() + 1;

    QTreeWidgetItem * newItem = new QTreeWidgetItem(curItem);
    newItem->setText(1, QString::number(childCount));
    rootItem->sortChildren(1, Qt::AscendingOrder);
    QVERIFY(1);
}


QTEST_MAIN(tst_QTreeWidget)
#include "tst_qtreewidget.moc"
