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

#include <qstandarditemmodel.h>

class tst_QStandardItem : public QObject
{
    Q_OBJECT

public:
    tst_QStandardItem();
    virtual ~tst_QStandardItem();

public slots:
    void init();
    void cleanup();

private slots:
    void ctor();
    void textCtor();
    void iconTextCtor();
    void rowsColumnsCtor();
    void getSetData();
    void getSetFlags();
    void getSetRowAndColumnCount();
    void getSetChild_data();
    void getSetChild();
    void parent();
    void insertColumn_data();
    void insertColumn();
    void insertColumns_data();
    void insertColumns();
    void insertRow_data();
    void insertRow();
    void insertRows_data();
    void insertRows();
    void appendColumn_data();
    void appendColumn();
    void appendRow_data();
    void appendRow();
    void takeChild();
    void takeColumn_data();
    void takeColumn();
    void takeRow_data();
    void takeRow();
    void streamItem();
    void deleteItem();
    void clone();
    void sortChildren();
    void subclassing();
};

tst_QStandardItem::tst_QStandardItem()
{
}

tst_QStandardItem::~tst_QStandardItem()
{
}

void tst_QStandardItem::init()
{
}

void tst_QStandardItem::cleanup()
{
}

void tst_QStandardItem::ctor()
{
    QStandardItem item;
    QVERIFY(!item.hasChildren());
}

void tst_QStandardItem::textCtor()
{
    QLatin1String text("text");
    QStandardItem item(text);
    QCOMPARE(item.text(), text);
    QVERIFY(!item.hasChildren());
}

void tst_QStandardItem::iconTextCtor()
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::red);
    QIcon icon(pixmap);
    QLatin1String text("text");
    QStandardItem item(icon, text);
    QCOMPARE(item.icon(), icon);
    QCOMPARE(item.text(), text);
    QVERIFY(!item.hasChildren());
}

void tst_QStandardItem::rowsColumnsCtor()
{
    const int rows = 5;
    const int columns = 12;
    QStandardItem item(rows, columns);
    QCOMPARE(item.rowCount(), rows);
    QCOMPARE(item.columnCount(), columns);
}

void tst_QStandardItem::getSetData()
{
    QStandardItem item;
    for (int x = 0; x < 2; ++x) {
        for (int i = 1; i <= 2; ++i) {
            QString text = QString("text %0").arg(i);
            item.setText(text);
            QCOMPARE(item.text(), text);

            QPixmap pixmap(32, 32);
            pixmap.fill((i == 1) ? Qt::red : Qt::green);
            QIcon icon(pixmap);
            item.setIcon(icon);
            QCOMPARE(item.icon(), icon);

            QString toolTip = QString("toolTip %0").arg(i);
            item.setToolTip(toolTip);
            QCOMPARE(item.toolTip(), toolTip);

            QString statusTip = QString("statusTip %0").arg(i);
            item.setStatusTip(statusTip);
            QCOMPARE(item.statusTip(), statusTip);

            QString whatsThis = QString("whatsThis %0").arg(i);
            item.setWhatsThis(whatsThis);
            QCOMPARE(item.whatsThis(), whatsThis);

            QSize sizeHint(64*i, 48*i);
            item.setSizeHint(sizeHint);
            QCOMPARE(item.sizeHint(), sizeHint);

            QFont font;
            item.setFont(font);
            QCOMPARE(item.font(), font);

            Qt::Alignment textAlignment((i == 1)
                                        ? Qt::AlignLeft|Qt::AlignVCenter
                                        : Qt::AlignRight);
            item.setTextAlignment(textAlignment);
            QCOMPARE(item.textAlignment(), textAlignment);

            QColor backgroundColor((i == 1) ? Qt::blue : Qt::yellow);
            item.setBackground(backgroundColor);
            QCOMPARE(item.background().color(), backgroundColor);

            QColor textColor((i == 1) ? Qt::green : Qt::cyan);
            item.setForeground(textColor);
            QCOMPARE(item.foreground().color(), textColor);

            Qt::CheckState checkState((i == 1) ? Qt::PartiallyChecked : Qt::Checked);
            item.setCheckState(checkState);
            QCOMPARE(item.checkState(), checkState);

            QString accessibleText = QString("accessibleText %0").arg(i);
            item.setAccessibleText(accessibleText);
            QCOMPARE(item.accessibleText(), accessibleText);

            QString accessibleDescription = QString("accessibleDescription %0").arg(i);
            item.setAccessibleDescription(accessibleDescription);
            QCOMPARE(item.accessibleDescription(), accessibleDescription);

            QCOMPARE(item.text(), text);
            QCOMPARE(item.icon(), icon);
            QCOMPARE(item.toolTip(), toolTip);
            QCOMPARE(item.statusTip(), statusTip);
            QCOMPARE(item.whatsThis(), whatsThis);
            QCOMPARE(item.sizeHint(), sizeHint);
            QCOMPARE(item.font(), font);
            QCOMPARE(item.textAlignment(), textAlignment);
            QCOMPARE(item.background().color(), backgroundColor);
            QCOMPARE(item.foreground().color(), textColor);
            QCOMPARE(item.checkState(), checkState);
            QCOMPARE(item.accessibleText(), accessibleText);
            QCOMPARE(item.accessibleDescription(), accessibleDescription);

            QCOMPARE(qvariant_cast<QString>(item.data(Qt::DisplayRole)), text);
            QCOMPARE(qvariant_cast<QIcon>(item.data(Qt::DecorationRole)), icon);
            QCOMPARE(qvariant_cast<QString>(item.data(Qt::ToolTipRole)), toolTip);
            QCOMPARE(qvariant_cast<QString>(item.data(Qt::StatusTipRole)), statusTip);
            QCOMPARE(qvariant_cast<QString>(item.data(Qt::WhatsThisRole)), whatsThis);
            QCOMPARE(qvariant_cast<QSize>(item.data(Qt::SizeHintRole)), sizeHint);
            QCOMPARE(qvariant_cast<QFont>(item.data(Qt::FontRole)), font);
            QCOMPARE(qvariant_cast<int>(item.data(Qt::TextAlignmentRole)), int(textAlignment));
            QCOMPARE(qvariant_cast<QBrush>(item.data(Qt::BackgroundColorRole)), QBrush(backgroundColor));
            QCOMPARE(qvariant_cast<QBrush>(item.data(Qt::BackgroundRole)), QBrush(backgroundColor));
            QCOMPARE(qvariant_cast<QBrush>(item.data(Qt::TextColorRole)), QBrush(textColor));
            QCOMPARE(qvariant_cast<QBrush>(item.data(Qt::ForegroundRole)), QBrush(textColor));
            QCOMPARE(qvariant_cast<int>(item.data(Qt::CheckStateRole)), int(checkState));
            QCOMPARE(qvariant_cast<QString>(item.data(Qt::AccessibleTextRole)), accessibleText);
            QCOMPARE(qvariant_cast<QString>(item.data(Qt::AccessibleDescriptionRole)), accessibleDescription);

            item.setBackground(pixmap);
            QCOMPARE(item.background().texture(), pixmap);
            QCOMPARE(qvariant_cast<QBrush>(item.data(Qt::BackgroundRole)).texture(), pixmap);
        }
        item.setData(QVariant(), Qt::DisplayRole);
        item.setData(QVariant(), Qt::DecorationRole);
        item.setData(QVariant(), Qt::ToolTipRole);
        item.setData(QVariant(), Qt::StatusTipRole);
        item.setData(QVariant(), Qt::WhatsThisRole);
        item.setData(QVariant(), Qt::SizeHintRole);
        item.setData(QVariant(), Qt::FontRole);
        item.setData(QVariant(), Qt::TextAlignmentRole);
        item.setData(QVariant(), Qt::BackgroundRole);
        item.setData(QVariant(), Qt::ForegroundRole);
        item.setData(QVariant(), Qt::CheckStateRole);
        item.setData(QVariant(), Qt::AccessibleTextRole);
        item.setData(QVariant(), Qt::AccessibleDescriptionRole);

        QCOMPARE(item.data(Qt::DisplayRole), QVariant());
        QCOMPARE(item.data(Qt::DecorationRole), QVariant());
        QCOMPARE(item.data(Qt::ToolTipRole), QVariant());
        QCOMPARE(item.data(Qt::StatusTipRole), QVariant());
        QCOMPARE(item.data(Qt::WhatsThisRole), QVariant());
        QCOMPARE(item.data(Qt::SizeHintRole), QVariant());
        QCOMPARE(item.data(Qt::FontRole), QVariant());
        QCOMPARE(item.data(Qt::TextAlignmentRole), QVariant());
        QCOMPARE(item.data(Qt::BackgroundColorRole), QVariant());
        QCOMPARE(item.data(Qt::BackgroundRole), QVariant());
        QCOMPARE(item.data(Qt::TextColorRole), QVariant());
        QCOMPARE(item.data(Qt::ForegroundRole), QVariant());
        QCOMPARE(item.data(Qt::CheckStateRole), QVariant());
        QCOMPARE(item.data(Qt::AccessibleTextRole), QVariant());
        QCOMPARE(item.data(Qt::AccessibleDescriptionRole), QVariant());
    }
}

void tst_QStandardItem::getSetFlags()
{
    QStandardItem item;
    item.setEnabled(true);
    QVERIFY(item.isEnabled());
    QVERIFY(item.flags() & Qt::ItemIsEnabled);
    item.setEditable(true);
    QVERIFY(item.isEditable());
    QVERIFY(item.flags() & Qt::ItemIsEditable);
    item.setSelectable(true);
    QVERIFY(item.isSelectable());
    QVERIFY(item.flags() & Qt::ItemIsSelectable);
    item.setCheckable(true);
    QVERIFY(item.isCheckable());
    QCOMPARE(item.checkState(), Qt::Unchecked);
    QVERIFY(item.flags() & Qt::ItemIsUserCheckable);
    item.setTristate(true);
    QVERIFY(item.isTristate());
    QVERIFY(item.flags() & Qt::ItemIsTristate);
#ifndef QT_NO_DRAGANDDROP
    item.setDragEnabled(true);
    QVERIFY(item.isDragEnabled());
    QVERIFY(item.flags() & Qt::ItemIsDragEnabled);
    item.setDropEnabled(true);
    QVERIFY(item.isDropEnabled());
    QVERIFY(item.flags() & Qt::ItemIsDropEnabled);
#endif

    QVERIFY(item.isEnabled());
    item.setEnabled(false);
    QVERIFY(!item.isEnabled());
    QVERIFY(!(item.flags() & Qt::ItemIsEnabled));
    QVERIFY(item.isEditable());
    item.setEditable(false);
    QVERIFY(!item.isEditable());
    QVERIFY(!(item.flags() & Qt::ItemIsEditable));
    QVERIFY(item.isSelectable());
    item.setSelectable(false);
    QVERIFY(!item.isSelectable());
    QVERIFY(!(item.flags() & Qt::ItemIsSelectable));
    QVERIFY(item.isCheckable());
    item.setCheckable(false);
    QVERIFY(!item.isCheckable());
    QVERIFY(!(item.flags() & Qt::ItemIsUserCheckable));
    QVERIFY(item.isTristate());
    item.setTristate(false);
    QVERIFY(!item.isTristate());
    QVERIFY(!(item.flags() & Qt::ItemIsTristate));
#ifndef QT_NO_DRAGANDDROP
    QVERIFY(item.isDragEnabled());
    item.setDragEnabled(false);
    QVERIFY(!item.isDragEnabled());
    QVERIFY(!(item.flags() & Qt::ItemIsDragEnabled));
    QVERIFY(item.isDropEnabled());
    item.setDropEnabled(false);
    QVERIFY(!item.isDropEnabled());
    QVERIFY(!(item.flags() & Qt::ItemIsDropEnabled));
#endif

    item.setCheckable(false);
    item.setCheckState(Qt::Checked);
    item.setCheckable(true);
    QCOMPARE(item.checkState(), Qt::Checked);
}

void tst_QStandardItem::getSetRowAndColumnCount()
{
    QStandardItem item;

    item.setRowCount(-1);
    QCOMPARE(item.rowCount(), 0);

    item.setColumnCount(-1);
    QCOMPARE(item.columnCount(), 0);

    item.setRowCount(1);
    QCOMPARE(item.rowCount(), 1);
    QCOMPARE(item.columnCount(), 0);

    item.setColumnCount(1);
    QCOMPARE(item.columnCount(), 1);
    QCOMPARE(item.rowCount(), 1);

    item.setColumnCount(10);
    QCOMPARE(item.columnCount(), 10);
    QCOMPARE(item.rowCount(), 1);

    item.setRowCount(20);
    QCOMPARE(item.rowCount(), 20);
    QCOMPARE(item.columnCount(), 10);

    item.setRowCount(-1);
    QCOMPARE(item.rowCount(), 20);

    item.setColumnCount(-1);
    QCOMPARE(item.columnCount(), 10);

    item.setColumnCount(0);
    QCOMPARE(item.columnCount(), 0);
    QCOMPARE(item.rowCount(), 20);

    item.setRowCount(0);
    QCOMPARE(item.rowCount(), 0);
}

void tst_QStandardItem::getSetChild_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");

    QTest::newRow("0x0 children, child at (-1,-1)") << 0 << 0 << -1 << -1;
    QTest::newRow("0x0 children, child at (0,0)") << 0 << 0 << 0 << 0;
}

void tst_QStandardItem::getSetChild()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, row);
    QFETCH(int, column);

    QStandardItem item(rows, columns);
    bool shouldHaveChildren = (rows > 0) && (columns > 0);
    QCOMPARE(item.hasChildren(), shouldHaveChildren);
    QCOMPARE(item.child(row, column), static_cast<QStandardItem*>(0));

    QStandardItem *child = new QStandardItem;
    item.setChild(row, column, child);
    if ((row >= 0) && (column >= 0)) {
        QCOMPARE(item.rowCount(), qMax(rows, row + 1));
        QCOMPARE(item.columnCount(), qMax(columns, column + 1));

        QCOMPARE(item.child(row, column), child);
        QCOMPARE(child->row(), row);
        QCOMPARE(child->column(), column);

        QStandardItem *anotherChild = new QStandardItem;
        item.setChild(row, column, anotherChild);
        QCOMPARE(item.child(row, column), anotherChild);
        QCOMPARE(anotherChild->row(), row);
        QCOMPARE(anotherChild->column(), column);
        item.setChild(row, column, 0);
    } else {
        delete child;
    }
    QCOMPARE(item.child(row, column), static_cast<QStandardItem*>(0));
}

void tst_QStandardItem::parent()
{
    {
        QStandardItem item;
        QStandardItem *child = new QStandardItem;
        QCOMPARE(child->parent(), static_cast<QStandardItem*>(0));
        item.setChild(0, 0, child);
        QCOMPARE(child->parent(), &item);

        QStandardItem *childOfChild = new QStandardItem;
        child->setChild(0, 0, childOfChild);
        QCOMPARE(childOfChild->parent(), child);
    }

    {
        QStandardItemModel model;
        QStandardItem *item = new QStandardItem;
        model.appendRow(item);
        // parent of a top-level item should be 0
        QCOMPARE(item->parent(), static_cast<QStandardItem*>(0));
    }
}

void tst_QStandardItem::insertColumn_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("count");

    QTest::newRow("insert 0 at -1 in 0x0") << 0 << 0 << -1 << 0;
    QTest::newRow("insert 0 at 0 in 0x0") << 0 << 0 << 0 << 0;
    QTest::newRow("insert 0 at 0 in 1x0") << 1 << 0 << 0 << 0;
    QTest::newRow("insert 0 at 0 in 0x1") << 0 << 1 << 0 << 0;
    QTest::newRow("insert 0 at 0 in 1x1") << 1 << 1 << 0 << 0;
    QTest::newRow("insert 1 at -1 in 0x0") << 0 << 0 << -1 << 1;
    QTest::newRow("insert 1 at 0 in 0x0") << 0 << 0 << 0 << 1;
    QTest::newRow("insert 1 at 0 in 1x0") << 1 << 0 << 0 << 1;
    QTest::newRow("insert 1 at 0 in 0x1") << 0 << 1 << 0 << 1;
    QTest::newRow("insert 1 at 0 in 1x1") << 1 << 1 << 0 << 1;
    QTest::newRow("insert 1 at 1 in 1x1") << 1 << 1 << 1 << 1;
    QTest::newRow("insert 1 at 0 in 2x1") << 2 << 1 << 0 << 1;
    QTest::newRow("insert 1 at 1 in 2x1") << 2 << 1 << 1 << 1;
    QTest::newRow("insert 1 at 0 in 1x2") << 1 << 2 << 0 << 1;
    QTest::newRow("insert 1 at 1 in 1x2") << 1 << 2 << 1 << 1;
    QTest::newRow("insert 1 at 0 in 8x4") << 8 << 4 << 0 << 1;
    QTest::newRow("insert 1 at 1 in 8x4") << 8 << 4 << 1 << 1;
    QTest::newRow("insert 1 at 2 in 8x4") << 8 << 4 << 2 << 1;
    QTest::newRow("insert 1 at 3 in 8x4") << 8 << 4 << 3 << 1;
    QTest::newRow("insert 1 at 4 in 8x4") << 8 << 4 << 4 << 1;
    QTest::newRow("insert 4 at 0 in 8x4") << 8 << 4 << 0 << 4;
    QTest::newRow("insert 4 at 4 in 8x4") << 8 << 4 << 4 << 4;
    QTest::newRow("insert 6 at 0 in 8x4") << 8 << 4 << 0 << 6;
    QTest::newRow("insert 6 at 4 in 8x4") << 8 << 4 << 4 << 6;
}

void tst_QStandardItem::insertColumn()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, column);
    QFETCH(int, count);

    QStandardItem item(rows, columns);

    // make items for a new column
    QList<QStandardItem*> columnItems;
    for (int i = 0; i < count; ++i)
        columnItems.append(new QStandardItem);

    item.insertColumn(column, columnItems);

    if (column >= 0) {
        QCOMPARE(item.columnCount(), columns + 1);
        QCOMPARE(item.rowCount(), qMax(rows, count));
        // check to make sure items were inserted in correct place
        for (int i = 0; i < count; ++i)
            QCOMPARE(item.child(i, column), columnItems.at(i));
        for (int i = count; i < item.rowCount(); ++i)
            QCOMPARE(item.child(i, column), static_cast<QStandardItem*>(0));
    } else {
        QCOMPARE(item.columnCount(), columns);
        QCOMPARE(item.rowCount(), rows);
        qDeleteAll(columnItems);
    }
}

void tst_QStandardItem::insertColumns_data()
{
}

void tst_QStandardItem::insertColumns()
{
}

void tst_QStandardItem::insertRow_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("count");

    QTest::newRow("insert 0 at -1 in 0x0") << 0 << 0 << -1 << 0;
    QTest::newRow("insert 0 at 0 in 0x0") << 0 << 0 << 0 << 0;
    QTest::newRow("insert 0 at 0 in 1x0") << 1 << 0 << 0 << 0;
    QTest::newRow("insert 0 at 0 in 0x1") << 0 << 1 << 0 << 0;
    QTest::newRow("insert 0 at 0 in 1x1") << 1 << 1 << 0 << 0;
    QTest::newRow("insert 1 at -1 in 0x0") << 0 << 0 << -1 << 1;
    QTest::newRow("insert 1 at 0 in 0x0") << 0 << 0 << 0 << 1;
    QTest::newRow("insert 1 at 0 in 1x0") << 1 << 0 << 0 << 1;
    QTest::newRow("insert 1 at 0 in 0x1") << 0 << 1 << 0 << 1;
    QTest::newRow("insert 1 at 0 in 1x1") << 1 << 1 << 0 << 1;
    QTest::newRow("insert 1 at 1 in 1x1") << 1 << 1 << 1 << 1;
    QTest::newRow("insert 1 at 0 in 2x1") << 2 << 1 << 0 << 1;
    QTest::newRow("insert 1 at 1 in 2x1") << 2 << 1 << 1 << 1;
    QTest::newRow("insert 1 at 0 in 1x2") << 1 << 2 << 0 << 1;
    QTest::newRow("insert 1 at 1 in 1x2") << 1 << 2 << 1 << 1;
    QTest::newRow("insert 1 at 0 in 4x8") << 4 << 8 << 0 << 1;
    QTest::newRow("insert 1 at 1 in 4x8") << 4 << 8 << 1 << 1;
    QTest::newRow("insert 1 at 2 in 4x8") << 4 << 8 << 2 << 1;
    QTest::newRow("insert 1 at 3 in 4x8") << 4 << 8 << 3 << 1;
    QTest::newRow("insert 1 at 4 in 4x8") << 4 << 8 << 4 << 1;
    QTest::newRow("insert 4 at 0 in 4x8") << 4 << 8 << 0 << 4;
    QTest::newRow("insert 4 at 4 in 4x8") << 4 << 8 << 4 << 4;
    QTest::newRow("insert 6 at 0 in 4x8") << 4 << 8 << 0 << 6;
    QTest::newRow("insert 6 at 4 in 4x8") << 4 << 8 << 4 << 6;
}

void tst_QStandardItem::insertRow()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, row);
    QFETCH(int, count);

    QStandardItem item(rows, columns);

    // make items for a new column
    QList<QStandardItem*> rowItems;
    for (int i = 0; i < count; ++i)
        rowItems.append(new QStandardItem);

    item.insertRow(row, rowItems);

    if (row >= 0) {
        QCOMPARE(item.columnCount(), qMax(columns, count));
        QCOMPARE(item.rowCount(), rows + 1);
        // check to make sure items were inserted in correct place
        for (int i = 0; i < count; ++i)
            QCOMPARE(item.child(row, i), rowItems.at(i));
        for (int i = count; i < item.columnCount(); ++i)
            QCOMPARE(item.child(row, i), static_cast<QStandardItem*>(0));
    } else {
        QCOMPARE(item.columnCount(), columns);
        QCOMPARE(item.rowCount(), rows);
        qDeleteAll(rowItems);
    }
}

void tst_QStandardItem::insertRows_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("insertAt");
    QTest::addColumn<int>("insertCount");

    QTest::newRow("insert {0,1} at 0 in 0x0") << 0 << 0 << 0 << 2;
}

void tst_QStandardItem::insertRows()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, insertAt);
    QFETCH(int, insertCount);

    QStandardItem item(rows, columns);

    QList<QStandardItem*> items;
    for (int i = 0; i < insertCount; ++i) {
        items.append(new QStandardItem());
    }
    item.insertRows(insertAt, items);

    QCOMPARE(item.rowCount(), rows + insertCount);
}

void tst_QStandardItem::appendColumn_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("count");

    QTest::newRow("append 0 to 0x0") << 0 << 0 << 0;
    QTest::newRow("append 1 to 0x0") << 0 << 0 << 1;
    QTest::newRow("append 1 to 1x0") << 1 << 0 << 1;
    QTest::newRow("append 1 to 0x1") << 0 << 1 << 1;
    QTest::newRow("append 1 to 1x1") << 1 << 1 << 1;
    QTest::newRow("append 1 to 2x0") << 2 << 0 << 1;
    QTest::newRow("append 1 to 0x2") << 0 << 2 << 1;
    QTest::newRow("append 1 to 2x1") << 2 << 1 << 1;
    QTest::newRow("append 1 to 1x2") << 1 << 2 << 1;
    QTest::newRow("append 1 to 2x2") << 2 << 2 << 1;
    QTest::newRow("append 2 to 0x0") << 0 << 0 << 2;
    QTest::newRow("append 2 to 1x0") << 1 << 0 << 2;
    QTest::newRow("append 2 to 0x1") << 0 << 1 << 2;
    QTest::newRow("append 2 to 1x1") << 1 << 1 << 2;
    QTest::newRow("append 2 to 2x0") << 2 << 0 << 2;
    QTest::newRow("append 2 to 0x2") << 0 << 2 << 2;
    QTest::newRow("append 2 to 2x1") << 2 << 1 << 2;
    QTest::newRow("append 2 to 1x2") << 1 << 2 << 2;
    QTest::newRow("append 2 to 2x2") << 2 << 2 << 2;
    QTest::newRow("append 3 to 2x1") << 2 << 1 << 3;
    QTest::newRow("append 3 to 1x2") << 1 << 2 << 3;
    QTest::newRow("append 3 to 2x2") << 2 << 2 << 3;
    QTest::newRow("append 3 to 4x2") << 4 << 2 << 3;
    QTest::newRow("append 3 to 2x4") << 2 << 4 << 3;
    QTest::newRow("append 3 to 4x4") << 4 << 4 << 3;
    QTest::newRow("append 7 to 4x2") << 4 << 2 << 7;
    QTest::newRow("append 7 to 2x4") << 2 << 4 << 7;
    QTest::newRow("append 7 to 4x4") << 4 << 4 << 7;
}

void tst_QStandardItem::appendColumn()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, count);

    QStandardItem item(rows, columns);
    QList<QStandardItem*> originalChildren;
    // initialize children
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            QStandardItem *child = new QStandardItem;
            originalChildren.append(child);
            item.setChild(i, j, child);
        }
    }

    // make items for a new column
    QList<QStandardItem*> columnItems;
    for (int i = 0; i < count; ++i)
        columnItems.append(new QStandardItem);

    item.appendColumn(columnItems);

    QCOMPARE(item.columnCount(), columns + 1);
    QCOMPARE(item.rowCount(), qMax(rows, count));
    // check to make sure items were inserted in correct place
    for (int i = 0; i < count; ++i)
        QCOMPARE(item.child(i, columns), columnItems.at(i));
    for (int i = count; i < item.rowCount(); ++i)
        QCOMPARE(item.child(i, columns), static_cast<QStandardItem*>(0));

    // make sure original children remained unchanged
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j)
            QCOMPARE(item.child(i, j), originalChildren.at(i*columns+j));
    }
}

void tst_QStandardItem::appendRow_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("count");

    QTest::newRow("append 0 to 0x0") << 0 << 0 << 0;
    QTest::newRow("append 1 to 0x0") << 0 << 0 << 1;
    QTest::newRow("append 1 to 1x0") << 1 << 0 << 1;
    QTest::newRow("append 1 to 0x1") << 0 << 1 << 1;
    QTest::newRow("append 1 to 1x1") << 1 << 1 << 1;
    QTest::newRow("append 1 to 2x0") << 2 << 0 << 1;
    QTest::newRow("append 1 to 0x2") << 0 << 2 << 1;
    QTest::newRow("append 1 to 2x1") << 2 << 1 << 1;
    QTest::newRow("append 1 to 1x2") << 1 << 2 << 1;
    QTest::newRow("append 1 to 2x2") << 2 << 2 << 1;
    QTest::newRow("append 2 to 0x0") << 0 << 0 << 2;
    QTest::newRow("append 2 to 1x0") << 1 << 0 << 2;
    QTest::newRow("append 2 to 0x1") << 0 << 1 << 2;
    QTest::newRow("append 2 to 1x1") << 1 << 1 << 2;
    QTest::newRow("append 2 to 2x0") << 2 << 0 << 2;
    QTest::newRow("append 2 to 0x2") << 0 << 2 << 2;
    QTest::newRow("append 2 to 2x1") << 2 << 1 << 2;
    QTest::newRow("append 2 to 1x2") << 1 << 2 << 2;
    QTest::newRow("append 2 to 2x2") << 2 << 2 << 2;
    QTest::newRow("append 3 to 2x1") << 2 << 1 << 3;
    QTest::newRow("append 3 to 1x2") << 1 << 2 << 3;
    QTest::newRow("append 3 to 2x2") << 2 << 2 << 3;
    QTest::newRow("append 3 to 4x2") << 4 << 2 << 3;
    QTest::newRow("append 3 to 2x4") << 2 << 4 << 3;
    QTest::newRow("append 3 to 4x4") << 4 << 4 << 3;
    QTest::newRow("append 7 to 4x2") << 4 << 2 << 7;
    QTest::newRow("append 7 to 2x4") << 2 << 4 << 7;
    QTest::newRow("append 7 to 4x4") << 4 << 4 << 7;
}

void tst_QStandardItem::appendRow()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, count);

    QStandardItem item(rows, columns);
    QList<QStandardItem*> originalChildren;
    // initialize children
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            QStandardItem *child = new QStandardItem;
            originalChildren.append(child);
            item.setChild(i, j, child);
        }
    }

    // make items for a new row
    QList<QStandardItem*> rowItems;
    for (int i = 0; i < count; ++i)
        rowItems.append(new QStandardItem);

    item.appendRow(rowItems);

    QCOMPARE(item.rowCount(), rows + 1);
    QCOMPARE(item.columnCount(), qMax(columns, count));
    // check to make sure items were inserted in correct place
    for (int i = 0; i < count; ++i)
        QCOMPARE(item.child(rows, i), rowItems.at(i));
    for (int i = count; i < item.columnCount(); ++i)
        QCOMPARE(item.child(rows, i), static_cast<QStandardItem*>(0));

    // make sure original children remained unchanged
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j)
            QCOMPARE(item.child(i, j), originalChildren.at(i*columns+j));
    }
}

void tst_QStandardItem::takeChild()
{
    QList<QStandardItem*> itemList;
    for (int i = 0; i < 10; ++i)
        itemList.append(new QStandardItem);
    QStandardItem item;
    item.appendColumn(itemList);

    for (int i = 0; i < item.rowCount(); ++i) {
        QCOMPARE(item.takeChild(i), itemList.at(i));
        QCOMPARE(item.takeChild(0, 0), static_cast<QStandardItem*>(0));
        for (int j = i + 1; j < item.rowCount(); ++j)
            QCOMPARE(item.child(j), itemList.at(j));
    }
    qDeleteAll(itemList);
}

void tst_QStandardItem::takeColumn_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("column");
    QTest::addColumn<bool>("expectSuccess");

    QTest::newRow("take -1 from 0x0") << 0 << 0 << -1 << false;
    QTest::newRow("take 0 from 0x0") << 0 << 0 << 0 << false;
    QTest::newRow("take 0 from 1x0") << 1 << 0 << 0 << false;
    QTest::newRow("take 0 from 0x1") << 0 << 1 << 0 << true;
    QTest::newRow("take 1 from 0x1") << 0 << 1 << 1 << false;
    QTest::newRow("take 0 from 1x1") << 1 << 1 << 0 << true;
    QTest::newRow("take 1 from 1x1") << 0 << 1 << 1 << false;
    QTest::newRow("take 0 from 4x1") << 4 << 1 << 0 << true;
    QTest::newRow("take 1 from 4x1") << 4 << 1 << 1 << false;
    QTest::newRow("take 0 from 4x8") << 4 << 8 << 0 << true;
    QTest::newRow("take 7 from 4x8") << 4 << 8 << 7 << true;
    QTest::newRow("take 8 from 4x8") << 4 << 8 << 8 << false;
}

void tst_QStandardItem::takeColumn()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, column);
    QFETCH(bool, expectSuccess);

    QStandardItem item(rows, columns);
    QList<QStandardItem*> originalChildren;
    // initialize children
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            QStandardItem *child = new QStandardItem;
            originalChildren.append(child);
            item.setChild(i, j, child);
        }
    }

    QList<QStandardItem *> taken = item.takeColumn(column);
    if (expectSuccess) {
        QCOMPARE(taken.count(), item.rowCount());
        QCOMPARE(item.columnCount(), columns - 1);
        int index = column;
        for (int i = 0; i < taken.count(); ++i) {
            QCOMPARE(taken.at(i), originalChildren.takeAt(index));
            index += item.columnCount();
        }
        index = 0;
        for (int i = 0; i < item.rowCount(); ++i) {
            for (int j = 0; j < item.columnCount(); ++j) {
                QCOMPARE(item.child(i, j), originalChildren.at(index));
                ++index;
            }
        }
    } else {
        QVERIFY(taken.isEmpty());
    }
    qDeleteAll(taken);
}

void tst_QStandardItem::takeRow_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("row");
    QTest::addColumn<bool>("expectSuccess");

    QTest::newRow("take -1 from 0x0") << 0 << 0 << -1 << false;
    QTest::newRow("take 0 from 0x0") << 0 << 0 << 0 << false;
    QTest::newRow("take 0 from 1x0") << 1 << 0 << 0 << true;
    QTest::newRow("take 0 from 0x1") << 0 << 1 << 0 << false;
    QTest::newRow("take 1 from 0x1") << 0 << 1 << 1 << false;
    QTest::newRow("take 0 from 1x1") << 1 << 1 << 0 << true;
    QTest::newRow("take 1 from 1x1") << 0 << 1 << 1 << false;
    QTest::newRow("take 0 from 1x4") << 1 << 4 << 0 << true;
    QTest::newRow("take 1 from 1x4") << 1 << 4 << 1 << false;
    QTest::newRow("take 0 from 8x4") << 8 << 4 << 0 << true;
    QTest::newRow("take 7 from 8x4") << 8 << 4 << 7 << true;
    QTest::newRow("take 8 from 8x4") << 8 << 4 << 8 << false;
}

void tst_QStandardItem::takeRow()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, row);
    QFETCH(bool, expectSuccess);

    QStandardItem item(rows, columns);
    QList<QStandardItem*> originalChildren;
    // initialize children
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            QStandardItem *child = new QStandardItem;
            originalChildren.append(child);
            item.setChild(i, j, child);
        }
    }

    QList<QStandardItem *> taken = item.takeRow(row);
    if (expectSuccess) {
        QCOMPARE(taken.count(), item.columnCount());
        QCOMPARE(item.rowCount(), rows - 1);
        int index = row * columns;
        for (int i = 0; i < taken.count(); ++i) {
            QCOMPARE(taken.at(i), originalChildren.takeAt(index));
        }
        index = 0;
        for (int i = 0; i < item.rowCount(); ++i) {
            for (int j = 0; j < item.columnCount(); ++j) {
                QCOMPARE(item.child(i, j), originalChildren.at(index));
                ++index;
            }
        }
    } else {
        QVERIFY(taken.isEmpty());
    }
    qDeleteAll(taken);
}

void tst_QStandardItem::streamItem()
{
    QStandardItem item;

    item.setText(QLatin1String("text"));
    item.setToolTip(QLatin1String("toolTip"));
    item.setStatusTip(QLatin1String("statusTip"));
    item.setWhatsThis(QLatin1String("whatsThis"));
    item.setSizeHint(QSize(64, 48));
    item.setFont(QFont());
    item.setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    item.setBackground(QColor(Qt::blue));
    item.setForeground(QColor(Qt::green));
    item.setCheckState(Qt::PartiallyChecked);
    item.setAccessibleText(QLatin1String("accessibleText"));
    item.setAccessibleDescription(QLatin1String("accessibleDescription"));

    QByteArray ba;
    {
        QDataStream ds(&ba, QIODevice::WriteOnly);
        ds << item;
    }
    {
        QStandardItem streamedItem;
        QDataStream ds(&ba, QIODevice::ReadOnly);
        ds >> streamedItem;
        QCOMPARE(streamedItem.text(), item.text());
        QCOMPARE(streamedItem.toolTip(), item.toolTip());
        QCOMPARE(streamedItem.statusTip(), item.statusTip());
        QCOMPARE(streamedItem.whatsThis(), item.whatsThis());
        QCOMPARE(streamedItem.sizeHint(), item.sizeHint());
        QCOMPARE(streamedItem.font(), item.font());
        QCOMPARE(streamedItem.textAlignment(), item.textAlignment());
        QCOMPARE(streamedItem.background(), item.background());
        QCOMPARE(streamedItem.foreground(), item.foreground());
        QCOMPARE(streamedItem.checkState(), item.checkState());
        QCOMPARE(streamedItem.accessibleText(), item.accessibleText());
        QCOMPARE(streamedItem.accessibleDescription(), item.accessibleDescription());
        QCOMPARE(streamedItem.flags(), item.flags());
    }
}

void tst_QStandardItem::deleteItem()
{
    QStandardItemModel model(4, 6);
    // initialize items
    for (int i = 0; i < model.rowCount(); ++i) {
        for (int j = 0; j < model.columnCount(); ++j) {
            QStandardItem *item = new QStandardItem();
            model.setItem(i, j, item);
        }
    }
    // delete items
    for (int i = 0; i < model.rowCount(); ++i) {
        for (int j = 0; j < model.columnCount(); ++j) {
            QStandardItem *item = model.item(i, j);
            delete item;
            QCOMPARE(model.item(i, j), static_cast<QStandardItem*>(0));
        }
    }
}

void tst_QStandardItem::clone()
{
    QStandardItem item;
    item.setText(QLatin1String("text"));
    item.setToolTip(QLatin1String("toolTip"));
    item.setStatusTip(QLatin1String("statusTip"));
    item.setWhatsThis(QLatin1String("whatsThis"));
    item.setSizeHint(QSize(64, 48));
    item.setFont(QFont());
    item.setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    item.setBackground(QColor(Qt::blue));
    item.setForeground(QColor(Qt::green));
    item.setCheckState(Qt::PartiallyChecked);
    item.setAccessibleText(QLatin1String("accessibleText"));
    item.setAccessibleDescription(QLatin1String("accessibleDescription"));
    item.setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);

    QStandardItem *clone = item.clone();
    QCOMPARE(clone->text(), item.text());
    QCOMPARE(clone->toolTip(), item.toolTip());
    QCOMPARE(clone->statusTip(), item.statusTip());
    QCOMPARE(clone->whatsThis(), item.whatsThis());
    QCOMPARE(clone->sizeHint(), item.sizeHint());
    QCOMPARE(clone->font(), item.font());
    QCOMPARE(clone->textAlignment(), item.textAlignment());
    QCOMPARE(clone->background(), item.background());
    QCOMPARE(clone->foreground(), item.foreground());
    QCOMPARE(clone->checkState(), item.checkState());
    QCOMPARE(clone->accessibleText(), item.accessibleText());
    QCOMPARE(clone->accessibleDescription(), item.accessibleDescription());
    QCOMPARE(clone->flags(), item.flags());
    QVERIFY(!(*clone < item));
    delete clone;
}

void tst_QStandardItem::sortChildren()
{
    for (int x = 0; x < 2; ++x) {
        QStandardItemModel *model = new QStandardItemModel;
        QStandardItem *item = (x == 0) ? new QStandardItem : model->invisibleRootItem();
        QStandardItem *one = new QStandardItem;
        one->appendRow(new QStandardItem(QLatin1String("a")));
        one->appendRow(new QStandardItem(QLatin1String("b")));
        one->appendRow(new QStandardItem(QLatin1String("c")));
        QStandardItem *two = new QStandardItem;
        two->appendRow(new QStandardItem(QLatin1String("f")));
        two->appendRow(new QStandardItem(QLatin1String("d")));
        two->appendRow(new QStandardItem(QLatin1String("e")));
        item->appendRow(one);
        item->appendRow(two);

        QSignalSpy layoutAboutToBeChangedSpy(
            model, SIGNAL(layoutAboutToBeChanged()));
        QSignalSpy layoutChangedSpy(
            model, SIGNAL(layoutChanged()));

        one->sortChildren(0, Qt::DescendingOrder);
        // verify sorted
        QCOMPARE(one->child(0)->text(), QLatin1String("c"));
        QCOMPARE(one->child(1)->text(), QLatin1String("b"));
        QCOMPARE(one->child(2)->text(), QLatin1String("a"));
        // verify siblings unaffected
        QCOMPARE(two->child(0)->text(), QLatin1String("f"));
        QCOMPARE(two->child(1)->text(), QLatin1String("d"));
        QCOMPARE(two->child(2)->text(), QLatin1String("e"));

        two->sortChildren(0, Qt::AscendingOrder);
        // verify sorted
        QCOMPARE(two->child(0)->text(), QLatin1String("d"));
        QCOMPARE(two->child(1)->text(), QLatin1String("e"));
        QCOMPARE(two->child(2)->text(), QLatin1String("f"));
        // verify siblings unaffected
        QCOMPARE(one->child(0)->text(), QLatin1String("c"));
        QCOMPARE(one->child(1)->text(), QLatin1String("b"));
        QCOMPARE(one->child(2)->text(), QLatin1String("a"));

        item->sortChildren(0, Qt::AscendingOrder);
        // verify everything sorted
        QCOMPARE(one->child(0)->text(), QLatin1String("a"));
        QCOMPARE(one->child(1)->text(), QLatin1String("b"));
        QCOMPARE(one->child(2)->text(), QLatin1String("c"));
        QCOMPARE(two->child(0)->text(), QLatin1String("d"));
        QCOMPARE(two->child(1)->text(), QLatin1String("e"));
        QCOMPARE(two->child(2)->text(), QLatin1String("f"));

        QCOMPARE(layoutAboutToBeChangedSpy.count(), (x == 0) ? 0 : 3);
        QCOMPARE(layoutChangedSpy.count(), (x == 0) ? 0 : 3);

        if (x == 0)
            delete item;
        delete model;
    }
}

class CustomItem : public QStandardItem
{
public:
    CustomItem(const QString &text) : QStandardItem(text) { }
    CustomItem() { }
    virtual ~CustomItem() { }

    virtual int type() const { return QStandardItem::UserType + 1; }

    virtual QStandardItem *clone() const { return QStandardItem::clone(); }

    void emitDataChanged() { QStandardItem::emitDataChanged(); }

    virtual bool operator<(const QStandardItem &other) const {
        return text().length() < other.text().length();
    }
};

Q_DECLARE_METATYPE(QStandardItem*)

void tst_QStandardItem::subclassing()
{
    qMetaTypeId<QStandardItem*>();

    CustomItem *item = new CustomItem;
    QCOMPARE(item->type(), int(QStandardItem::UserType + 1));

    item->setText(QString::fromLatin1("foo"));
    QCOMPARE(item->text(), QString::fromLatin1("foo"));

    item->emitDataChanged(); // does nothing

    QStandardItemModel model;
    model.appendRow(item);

    QSignalSpy itemChangedSpy(&model, SIGNAL(itemChanged(QStandardItem*)));
    item->emitDataChanged();
    QCOMPARE(itemChangedSpy.count(), 1);
    QCOMPARE(itemChangedSpy.at(0).count(), 1);
    QCOMPARE(qvariant_cast<QStandardItem*>(itemChangedSpy.at(0).at(0)), (QStandardItem*)item);

    CustomItem *child0 = new CustomItem("cc");
    CustomItem *child1 = new CustomItem("bbb");
    CustomItem *child2 = new CustomItem("a");
    item->appendRow(child0);
    item->appendRow(child1);
    item->appendRow(child2);
    item->sortChildren(0);
    QCOMPARE(item->child(0), (QStandardItem*)child2);
    QCOMPARE(item->child(1), (QStandardItem*)child0);
    QCOMPARE(item->child(2), (QStandardItem*)child1);
}

QTEST_MAIN(tst_QStandardItem)
#include "tst_qstandarditem.moc"
