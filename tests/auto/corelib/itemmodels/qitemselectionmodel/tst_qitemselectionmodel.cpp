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

#include <QtTest/QtTest>

#include <QtGui/QtGui>

#include <algorithm>

class tst_QItemSelectionModel : public QObject
{
    Q_OBJECT

public:
    tst_QItemSelectionModel();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
private slots:
    void clear_data();
    void clear();
    void clearAndSelect();
    void toggleSelection();
    void select_data();
    void select();
    void persistentselections_data();
    void persistentselections();
    void resetModel();
    void removeRows_data();
    void removeRows();
    void removeColumns_data();
    void removeColumns();
    void modelLayoutChanged_data();
    void modelLayoutChanged();
    void selectedRows_data();
    void selectedRows();
    void selectedColumns_data();
    void selectedColumns();
    void setCurrentIndex();
    void splitOnInsert();
    void rowIntersectsSelection1();
    void rowIntersectsSelection2();
    void rowIntersectsSelection3();
    void unselectable();
    void selectedIndexes();
    void layoutChanged();
    void merge_data();
    void merge();
    void isRowSelected();
    void childrenDeselectionSignal();
    void layoutChangedWithAllSelected1();
    void layoutChangedWithAllSelected2();
    void layoutChangedTreeSelection();
    void deselectRemovedMiddleRange();
    void rangeOperatorLessThan_data();
    void rangeOperatorLessThan();
    void setModel();

    void testDifferentModels();

    void testValidRangesInSelectionsAfterReset();
    void testChainedSelectionClear();
    void testClearCurrentIndex();

    void QTBUG48402_data();
    void QTBUG48402();

private:
    QAbstractItemModel *model;
    QItemSelectionModel *selection;
};

QDataStream &operator<<(QDataStream &, const QModelIndex &);
QDataStream &operator>>(QDataStream &, QModelIndex &);
QDataStream &operator<<(QDataStream &, const QModelIndexList &);
QDataStream &operator>>(QDataStream &, QModelIndexList &);

typedef QList<int> IntList;
typedef QPair<int, int> IntPair;
typedef QList<IntPair> PairList;

class QStreamHelper: public QAbstractItemModel
{
public:
    QStreamHelper() {}
    static QModelIndex create(int row = -1, int column = -1, void *data = 0)
    {
        QStreamHelper helper;
        return helper.QAbstractItemModel::createIndex(row, column, data);
    }

    QModelIndex index(int, int, const QModelIndex&) const
        { return QModelIndex(); }
    QModelIndex parent(const QModelIndex&) const
        { return QModelIndex(); }
    int rowCount(const QModelIndex & = QModelIndex()) const
        { return 0; }
    int columnCount(const QModelIndex & = QModelIndex()) const
        { return 0; }
    QVariant data(const QModelIndex &, int = Qt::DisplayRole) const
        { return QVariant(); }
    bool hasChildren(const QModelIndex &) const
        { return false; }
};

QDataStream &operator<<(QDataStream &s, const QModelIndex &input)
{
    s << input.row()
      << input.column()
      << reinterpret_cast<qlonglong>(input.internalPointer());
    return s;
}

QDataStream &operator>>(QDataStream &s, QModelIndex &output)
{
    int r, c;
    qlonglong ptr;
    s >> r;
    s >> c;
    s >> ptr;
    output = QStreamHelper::create(r, c, reinterpret_cast<void *>(ptr));
    return s;
}

QDataStream &operator<<(QDataStream &s, const QModelIndexList &input)
{
    s << input.count();
    for (int i=0; i<input.count(); ++i)
        s << input.at(i);
    return s;
}

QDataStream &operator>>(QDataStream &s, QModelIndexList &output)
{
    QModelIndex tmpIndex;
    int count;
    s >> count;
    for (int i=0; i<count; ++i) {
        s >> tmpIndex;
        output << tmpIndex;
    }
    return s;
}

tst_QItemSelectionModel::tst_QItemSelectionModel()
    : model(0), selection(0)
{
}

/*
  This test usually uses a model with a 5x5 table
  -------------------------------------------
  |  0,0  |  0,1    |  0,2  |  0,3    |  0,4  |
  -------------------------------------------
  |  1,0  |  1,1    |  1,2  |  1,3    |  1,4  |
  -------------------------------------------
  |  2,0  |  2,1    |  2,2  |  2,3    |  2,4  |
  -------------------------------------------
  |  3,0  |  3,1    |  3,2  |  3,3    |  3,4  |
  -------------------------------------------
  |  4,0  |  4,1    |  4,2  |  4,3    |  4,4  |
  -------------------------------------------

  ...that for each row has a children in a new 5x5 table ad infinitum.

*/
void tst_QItemSelectionModel::initTestCase()
{
    qRegisterMetaType<QItemSelection>("QItemSelection");

    model = new QStandardItemModel(5, 5);
    QModelIndex parent = model->index(0, 0, QModelIndex());
    model->insertRows(0, 5, parent);
    model->insertColumns(0, 5, parent);
    selection  = new QItemSelectionModel(model);
}

void tst_QItemSelectionModel::cleanupTestCase()
{
    delete selection;
    delete model;
}

void tst_QItemSelectionModel::init()
{
    selection->clear();
    while (model->rowCount(QModelIndex()) > 5)
        model->removeRow(0, QModelIndex());
    while (model->rowCount(QModelIndex()) < 5)
        model->insertRow(0, QModelIndex());
}

void tst_QItemSelectionModel::clear_data()
{
    QTest::addColumn<QModelIndexList>("indexList");
    QTest::addColumn<IntList>("commandList");
    {
        QModelIndexList index;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Rows);
        index << model->index(1, 0, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Rows);
        QTest::newRow("(0, 0) and (1, 0): Select|Rows")
            << index
            << command;
    }
    {
        QModelIndexList index;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Columns);
        index << model->index(0, 1, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Columns);
        QTest::newRow("(0, 0) and (1, 0): Select|Columns")
            << index
            << command;
    }
    {
        QModelIndexList index;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(1, 1, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::SelectCurrent;
        QTest::newRow("(0, 0), (1, 1) and (2, 2): Select, Select, SelectCurrent")
            << index
            << command;
    }
    {
        QModelIndexList index;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(1, 1, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(1, 1, QModelIndex());
        command << QItemSelectionModel::Toggle;
        QTest::newRow("(0, 0), (1, 1) and (1, 1): Select, Select, Toggle")
            << index
            << command;
    }
    {
        QModelIndexList index;
        IntList command;
        index << model->index(0, 0, model->index(0, 0, QModelIndex()));
        command << (QItemSelectionModel::Select | QItemSelectionModel::Rows);
        QTest::newRow("child (0, 0) of (0, 0): Select|Rows")
            << index
            << command;
    }
}

void tst_QItemSelectionModel::clear()
{
    QFETCH(QModelIndexList, indexList);
    QFETCH(IntList, commandList);

    // do selections
    for (int i=0; i<indexList.count(); ++i) {
        selection->select(indexList.at(i), (QItemSelectionModel::SelectionFlags)commandList.at(i));
    }
    // test that we have selected items
    QVERIFY(!selection->selectedIndexes().isEmpty());
    selection->clear();
    // test that they were all cleared
    QVERIFY(selection->selectedIndexes().isEmpty());
}

void tst_QItemSelectionModel::clearAndSelect()
{
    // populate selectionmodel
    selection->select(model->index(1, 1, QModelIndex()), QItemSelectionModel::Select);
    QCOMPARE(selection->selectedIndexes().count(), 1);
    QVERIFY(selection->hasSelection());

    // ClearAndSelect with empty selection
    QItemSelection emptySelection;
    selection->select(emptySelection, QItemSelectionModel::ClearAndSelect);

    // verify the selectionmodel is empty
    QVERIFY(selection->selectedIndexes().isEmpty());
    QVERIFY(selection->hasSelection()==false);
}

void tst_QItemSelectionModel::toggleSelection()
{
    //test the toggle selection and checks whether selectedIndex
    //and hasSelection returns the correct value

    selection->clearSelection();
    QCOMPARE(selection->selectedIndexes().count(), 0);
    QVERIFY(selection->hasSelection()==false);

    QModelIndex index=model->index(1, 1, QModelIndex());
    // populate selectionmodel
    selection->select(index, QItemSelectionModel::Toggle);
    QCOMPARE(selection->selectedIndexes().count(), 1);
    QVERIFY(selection->hasSelection()==true);

    selection->select(index, QItemSelectionModel::Toggle);
    QCOMPARE(selection->selectedIndexes().count(), 0);
    QVERIFY(selection->hasSelection()==false);

    // populate selectionmodel with rows
    selection->select(index, QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
    QCOMPARE(selection->selectedIndexes().count(), model->columnCount());
    QVERIFY(selection->hasSelection()==true);

    selection->select(index, QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
    QCOMPARE(selection->selectedIndexes().count(), 0);
    QVERIFY(selection->hasSelection()==false);
}

void tst_QItemSelectionModel::select_data()
{
    QTest::addColumn<QModelIndexList>("indexList");
    QTest::addColumn<bool>("useRanges");
    QTest::addColumn<IntList>("commandList");
    QTest::addColumn<QModelIndexList>("expectedList");

    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        expected << model->index(0, 0, QModelIndex());
        QTest::newRow("(0, 0): Select")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, model->index(0, 0, QModelIndex()));
        command << QItemSelectionModel::Select;
        expected << model->index(0, 0, model->index(0, 0, QModelIndex()));
        QTest::newRow("child (0, 0) of (0, 0): Select")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Deselect;
        QTest::newRow("(0, 0): Deselect")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Toggle;
        expected << model->index(0, 0, QModelIndex());
        QTest::newRow("(0, 0): Toggle")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Toggle;
        QTest::newRow("(0, 0) and (0, 0): Select and Toggle")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Deselect;
        QTest::newRow("(0, 0) and (0, 0): Select and Deselect")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(0, 0, model->index(0, 0, QModelIndex()));
        command << QItemSelectionModel::ClearAndSelect;
        expected << model->index(0, 0, model->index(0, 0, QModelIndex()));
        QTest::newRow("(0, 0) and child (0, 0) of (0, 0): Select and ClearAndSelect")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(4, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(0, 1, QModelIndex());
        index << model->index(4, 1, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(0, 0, QModelIndex());
        index << model->index(4, 1, QModelIndex());
        command << QItemSelectionModel::Deselect;
        QTest::newRow("(0, 0 to 4, 0) and (0, 1 to 4, 1) and (0, 0 to 4, 1): Select and Select and Deselect")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(4, 4, QModelIndex());
        command << QItemSelectionModel::Select;
        expected << model->index(0, 0, QModelIndex()) << model->index(4, 4, QModelIndex());
        QTest::newRow("(0, 0) and (4, 4): Select")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(4, 4, QModelIndex());
        command << QItemSelectionModel::ClearAndSelect;
        expected << model->index(4, 4, QModelIndex());
        QTest::newRow("(0, 0) and (4, 4): Select and ClearAndSelect")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Rows);
        index << model->index(4, 4, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Rows);
        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(0, 3, QModelIndex())
                 << model->index(0, 4, QModelIndex())
                 << model->index(4, 0, QModelIndex())
                 << model->index(4, 1, QModelIndex())
                 << model->index(4, 2, QModelIndex())
                 << model->index(4, 3, QModelIndex())
                 << model->index(4, 4, QModelIndex());
        QTest::newRow("(0, 0) and (4, 4): Select|Rows")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, model->index(0, 0, QModelIndex()));
        command << (QItemSelectionModel::Select | QItemSelectionModel::Rows);
        index << model->index(4, 4, model->index(0, 0, QModelIndex()));
        command << (QItemSelectionModel::Select | QItemSelectionModel::Rows);
        QModelIndex parent = model->index(0, 0, QModelIndex());
        expected << model->index(0, 0, parent)
                 << model->index(0, 1, parent)
                 << model->index(0, 2, parent)
                 << model->index(0, 3, parent)
                 << model->index(0, 4, parent)
                 << model->index(4, 0, parent)
                 << model->index(4, 1, parent)
                 << model->index(4, 2, parent)
                 << model->index(4, 3, parent)
                 << model->index(4, 4, parent);
        QTest::newRow("child (0, 0) and (4, 4) of (0, 0): Select|Rows")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Columns);
        index << model->index(4, 4, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Columns);
        expected << model->index(0, 0, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(3, 0, QModelIndex())
                 << model->index(4, 0, QModelIndex())
                 << model->index(0, 4, QModelIndex())
                 << model->index(1, 4, QModelIndex())
                 << model->index(2, 4, QModelIndex())
                 << model->index(3, 4, QModelIndex())
                 << model->index(4, 4, QModelIndex());
        QTest::newRow("(0, 0) and (4, 4): Select|Columns")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, model->index(0, 0, QModelIndex()));
        command << (QItemSelectionModel::Select | QItemSelectionModel::Columns);
        index << model->index(4, 4, model->index(0, 0, QModelIndex()));
        command << (QItemSelectionModel::Select | QItemSelectionModel::Columns);
        expected << model->index(0, 0, model->index(0, 0, QModelIndex()))
                 << model->index(1, 0, model->index(0, 0, QModelIndex()))
                 << model->index(2, 0, model->index(0, 0, QModelIndex()))
                 << model->index(3, 0, model->index(0, 0, QModelIndex()))
                 << model->index(4, 0, model->index(0, 0, QModelIndex()))
                 << model->index(0, 4, model->index(0, 0, QModelIndex()))
                 << model->index(1, 4, model->index(0, 0, QModelIndex()))
                 << model->index(2, 4, model->index(0, 0, QModelIndex()))
                 << model->index(3, 4, model->index(0, 0, QModelIndex()))
                 << model->index(4, 4, model->index(0, 0, QModelIndex()));
        QTest::newRow("child (0, 0) and (4, 4) of (0, 0): Select|Columns")
            << index
            << false
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(4, 0, QModelIndex());
        command << QItemSelectionModel::Select;
        expected << model->index(0, 0, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(3, 0, QModelIndex())
                 << model->index(4, 0, QModelIndex());
        QTest::newRow("(0, 0 to 4, 0): Select")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(0, 0, model->index(0, 0, QModelIndex()));
        command << QItemSelectionModel::Select;
        QTest::newRow("(0, 0 to child 0, 0): Select")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, model->index(0, 0, QModelIndex()));
        index << model->index(0, 0, model->index(1, 0, QModelIndex()));
        command << QItemSelectionModel::Select;
        QTest::newRow("child (0, 0) of (0, 0) to child (0, 0) of (1, 0): Select")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(4, 4, QModelIndex());
        command << QItemSelectionModel::Select;
        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(0, 3, QModelIndex())
                 << model->index(0, 4, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(1, 3, QModelIndex())
                 << model->index(1, 4, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex())
                 << model->index(2, 3, QModelIndex())
                 << model->index(2, 4, QModelIndex())
                 << model->index(3, 0, QModelIndex())
                 << model->index(3, 1, QModelIndex())
                 << model->index(3, 2, QModelIndex())
                 << model->index(3, 3, QModelIndex())
                 << model->index(3, 4, QModelIndex())
                 << model->index(4, 0, QModelIndex())
                 << model->index(4, 1, QModelIndex())
                 << model->index(4, 2, QModelIndex())
                 << model->index(4, 3, QModelIndex())
                 << model->index(4, 4, QModelIndex());
        QTest::newRow("(0, 0 to 4, 4): Select")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(4, 0, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Rows);
        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(0, 3, QModelIndex())
                 << model->index(0, 4, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(1, 3, QModelIndex())
                 << model->index(1, 4, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex())
                 << model->index(2, 3, QModelIndex())
                 << model->index(2, 4, QModelIndex())
                 << model->index(3, 0, QModelIndex())
                 << model->index(3, 1, QModelIndex())
                 << model->index(3, 2, QModelIndex())
                 << model->index(3, 3, QModelIndex())
                 << model->index(3, 4, QModelIndex())
                 << model->index(4, 0, QModelIndex())
                 << model->index(4, 1, QModelIndex())
                 << model->index(4, 2, QModelIndex())
                 << model->index(4, 3, QModelIndex())
                 << model->index(4, 4, QModelIndex());
        QTest::newRow("(0, 0 to 4, 0): Select|Rows")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(0, 4, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Columns);
        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(0, 3, QModelIndex())
                 << model->index(0, 4, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(1, 3, QModelIndex())
                 << model->index(1, 4, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex())
                 << model->index(2, 3, QModelIndex())
                 << model->index(2, 4, QModelIndex())
                 << model->index(3, 0, QModelIndex())
                 << model->index(3, 1, QModelIndex())
                 << model->index(3, 2, QModelIndex())
                 << model->index(3, 3, QModelIndex())
                 << model->index(3, 4, QModelIndex())
                 << model->index(4, 0, QModelIndex())
                 << model->index(4, 1, QModelIndex())
                 << model->index(4, 2, QModelIndex())
                 << model->index(4, 3, QModelIndex())
                 << model->index(4, 4, QModelIndex());
        QTest::newRow("(0, 0 to 0, 4): Select|Columns")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(4, 4, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Rows);
        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(0, 3, QModelIndex())
                 << model->index(0, 4, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(1, 3, QModelIndex())
                 << model->index(1, 4, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex())
                 << model->index(2, 3, QModelIndex())
                 << model->index(2, 4, QModelIndex())
                 << model->index(3, 0, QModelIndex())
                 << model->index(3, 1, QModelIndex())
                 << model->index(3, 2, QModelIndex())
                 << model->index(3, 3, QModelIndex())
                 << model->index(3, 4, QModelIndex())
                 << model->index(4, 0, QModelIndex())
                 << model->index(4, 1, QModelIndex())
                 << model->index(4, 2, QModelIndex())
                 << model->index(4, 3, QModelIndex())
                 << model->index(4, 4, QModelIndex());
        QTest::newRow("(0, 0 to 4, 4): Select|Rows")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(4, 4, QModelIndex());
        command << (QItemSelectionModel::Select | QItemSelectionModel::Columns);
        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(0, 3, QModelIndex())
                 << model->index(0, 4, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(1, 3, QModelIndex())
                 << model->index(1, 4, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex())
                 << model->index(2, 3, QModelIndex())
                 << model->index(2, 4, QModelIndex())
                 << model->index(3, 0, QModelIndex())
                 << model->index(3, 1, QModelIndex())
                 << model->index(3, 2, QModelIndex())
                 << model->index(3, 3, QModelIndex())
                 << model->index(3, 4, QModelIndex())
                 << model->index(4, 0, QModelIndex())
                 << model->index(4, 1, QModelIndex())
                 << model->index(4, 2, QModelIndex())
                 << model->index(4, 3, QModelIndex())
                 << model->index(4, 4, QModelIndex());
        QTest::newRow("(0, 0 to 4, 4): Select|Columns")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 2, QModelIndex());
        index << model->index(4, 2, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(2, 0, QModelIndex());
        index << model->index(2, 4, QModelIndex());
        command << QItemSelectionModel::Select;
        expected << model->index(0, 2, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(2, 2, QModelIndex())
                 << model->index(3, 2, QModelIndex())
                 << model->index(4, 2, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 3, QModelIndex())
                 << model->index(2, 4, QModelIndex());
        QTest::newRow("(0, 2 to 4, 2) and (2, 0 to 2, 4): Select")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 2, QModelIndex());
        index << model->index(4, 2, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(2, 0, QModelIndex());
        index << model->index(2, 4, QModelIndex());
        command << QItemSelectionModel::SelectCurrent;
        expected << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex())
                 << model->index(2, 3, QModelIndex())
                 << model->index(2, 4, QModelIndex());
        QTest::newRow("(0, 2 to 4, 2) and (2, 0 to 2, 4): Select and SelectCurrent")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 2, QModelIndex());
        index << model->index(4, 2, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(2, 0, QModelIndex());
        index << model->index(2, 4, QModelIndex());
        command << QItemSelectionModel::Toggle;
        expected << model->index(0, 2, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(3, 2, QModelIndex())
                 << model->index(4, 2, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 3, QModelIndex())
                 << model->index(2, 4, QModelIndex());
        QTest::newRow("(0, 2 to 4, 2) and (2, 0 to 2, 4): Select and Toggle")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 2, QModelIndex());
        index << model->index(4, 2, QModelIndex());
        command << QItemSelectionModel::Select;
        index << model->index(2, 0, QModelIndex());
        index << model->index(2, 4, QModelIndex());
        command << QItemSelectionModel::Deselect;
        expected << model->index(0, 2, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(3, 2, QModelIndex())
                 << model->index(4, 2, QModelIndex());
        QTest::newRow("(0, 2 to 4, 2) and (2, 0 to 2, 4): Select and Deselect")
            << index
            << true
            << command
            << expected;
    }

    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Select;

        index << model->index(0, 0, QModelIndex());
        index << model->index(0, 0, QModelIndex());
        command << QItemSelectionModel::Toggle;

        expected << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex());

        QTest::newRow("(0, 0 to 2, 2) and (0, 0 to 0, 0): Select and Toggle at selection boundary")
            << index
            << true
            << command
            << expected;
    }

    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Select;

        index << model->index(0, 1, QModelIndex());
        index << model->index(0, 1, QModelIndex());
        command << QItemSelectionModel::Toggle;

        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex());

        QTest::newRow("(0, 0 to 2, 2) and (0, 1 to 0, 1): Select and Toggle at selection boundary")
            << index
            << true
            << command
            << expected;
    }

    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Select;

        index << model->index(0, 2, QModelIndex());
        index << model->index(0, 2, QModelIndex());
        command << QItemSelectionModel::Toggle;

        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex());

        QTest::newRow("(0, 0 to 2, 2) and (0, 2 to 0, 2): Select and Toggle at selection boundary")
            << index
            << true
            << command
            << expected;
    }

    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Select;

        index << model->index(1, 0, QModelIndex());
        index << model->index(1, 0, QModelIndex());
        command << QItemSelectionModel::Toggle;

        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex());

        QTest::newRow("(0, 0 to 2, 2) and (1, 0 to 1, 0): Select and Toggle at selection boundary")
            << index
            << true
            << command
            << expected;
    }

    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Select;

        index << model->index(1, 1, QModelIndex());
        index << model->index(1, 1, QModelIndex());
        command << QItemSelectionModel::Toggle;

        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex());

        QTest::newRow("(0, 0 to 2, 2) and (1, 1 to 1, 1): Select and Toggle at selection boundary")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Select;

        index << model->index(1, 2, QModelIndex());
        index << model->index(1, 2, QModelIndex());
        command << QItemSelectionModel::Toggle;

        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex());

        QTest::newRow("(0, 0 to 2, 2) and (1, 2 to 1, 2): Select and Toggle at selection boundary")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Select;

        index << model->index(2, 0, QModelIndex());
        index << model->index(2, 0, QModelIndex());
        command << QItemSelectionModel::Toggle;

        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex());

        QTest::newRow("(0, 0 to 2, 2) and (2, 0 to 2, 0): Select and Toggle at selection boundary")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;
        index << model->index(0, 0, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Select;

        index << model->index(2, 1, QModelIndex());
        index << model->index(2, 1, QModelIndex());
        command << QItemSelectionModel::Toggle;

        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 2, QModelIndex());

        QTest::newRow("(0, 0 to 2, 2) and (2, 1 to 2, 1): Select and Toggle at selection boundary")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList index;
        QModelIndexList expected;
        IntList command;

        index << model->index(0, 0, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Select;

        index << model->index(2, 2, QModelIndex());
        index << model->index(2, 2, QModelIndex());
        command << QItemSelectionModel::Toggle;

        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex());

        QTest::newRow("(0, 0 to 2, 2) and (2, 2 to 2, 2): Select and Toggle at selection boundary")
            << index
            << true
            << command
            << expected;
    }
    {
        QModelIndexList indexes;
        IntList commands;
        QModelIndexList expected;

        indexes  << model->index(0, 0, QModelIndex()) << model->index(0, 0, QModelIndex()) // press 0
                 << model->index(0, 0, QModelIndex()) << model->index(0, 0, QModelIndex()) // release 0
                 << model->index(1, 0, QModelIndex()) << model->index(1, 0, QModelIndex()) // press 1
                 << model->index(1, 0, QModelIndex()) << model->index(1, 0, QModelIndex()) // release 1
                 << model->index(2, 0, QModelIndex()) << model->index(2, 0, QModelIndex()) // press 2
                 << model->index(2, 0, QModelIndex()) << model->index(2, 0, QModelIndex()) // release 2
                 << model->index(3, 0, QModelIndex()) << model->index(3, 0, QModelIndex()) // press 3
                 << model->index(3, 0, QModelIndex()) << model->index(3, 0, QModelIndex()) // release 3
                 << model->index(2, 0, QModelIndex()) << model->index(2, 0, QModelIndex()) // press 2 again
                 << model->index(2, 0, QModelIndex()) << model->index(2, 0, QModelIndex());// move 2

        commands << (QItemSelectionModel::NoUpdate)                                // press 0
                 << (QItemSelectionModel::Toggle|QItemSelectionModel::Rows)        // release 0
                 << (QItemSelectionModel::NoUpdate)                                // press 1
                 << (QItemSelectionModel::Toggle|QItemSelectionModel::Rows)        // release 1
                 << (QItemSelectionModel::NoUpdate)                                // press 2
                 << (QItemSelectionModel::Toggle|QItemSelectionModel::Rows)        // release 2
                 << (QItemSelectionModel::NoUpdate)                                // press 3
                 << (QItemSelectionModel::Toggle|QItemSelectionModel::Rows)        // release 3
                 << (QItemSelectionModel::NoUpdate)                                // press 2 again
                 << (QItemSelectionModel::Toggle/*Current*/|QItemSelectionModel::Rows);// move 2

        expected << model->index(0, 0, QModelIndex())
                 << model->index(0, 1, QModelIndex())
                 << model->index(0, 2, QModelIndex())
                 << model->index(0, 3, QModelIndex())
                 << model->index(0, 4, QModelIndex())

                 << model->index(1, 0, QModelIndex())
                 << model->index(1, 1, QModelIndex())
                 << model->index(1, 2, QModelIndex())
                 << model->index(1, 3, QModelIndex())
                 << model->index(1, 4, QModelIndex())
          /*
                 << model->index(2, 0, QModelIndex())
                 << model->index(2, 1, QModelIndex())
                 << model->index(2, 2, QModelIndex())
                 << model->index(2, 3, QModelIndex())
                 << model->index(2, 4, QModelIndex())
          */
                 << model->index(3, 0, QModelIndex())
                 << model->index(3, 1, QModelIndex())
                 << model->index(3, 2, QModelIndex())
                 << model->index(3, 3, QModelIndex())
                 << model->index(3, 4, QModelIndex());

        QTest::newRow("simulated treeview multiselection behavior")
            << indexes
            << true
            << commands
            << expected;
    }
}

void tst_QItemSelectionModel::select()
{
    QFETCH(QModelIndexList, indexList);
    QFETCH(bool, useRanges);
    QFETCH(IntList, commandList);
    QFETCH(QModelIndexList, expectedList);

    int lastCommand = 0;
    // do selections
    for (int i = 0; i<commandList.count(); ++i) {
        if (useRanges) {
            selection->select(QItemSelection(indexList.at(2*i), indexList.at(2*i+1)),
                              (QItemSelectionModel::SelectionFlags)commandList.at(i));
        } else {
            selection->select(indexList.at(i),
                              (QItemSelectionModel::SelectionFlags)commandList.at(i));
        }
        lastCommand = commandList.at(i);
    }


    QModelIndexList selectedList = selection->selectedIndexes();

    QVERIFY(selection->hasSelection()!=selectedList.isEmpty());

    // test that the number of indices are as expected
    QVERIFY2(selectedList.count() == expectedList.count(),
            QString("expected indices: %1 actual indices: %2")
            .arg(expectedList.count())
            .arg(selectedList.count()).toLatin1());

    // test existence of each index
    for (int i=0; i<expectedList.count(); ++i) {
        QVERIFY2(selectedList.contains(expectedList.at(i)),
                QString("expected index(%1, %2) not found in selectedIndexes()")
                .arg(expectedList.at(i).row())
                .arg(expectedList.at(i).column()).toLatin1());
    }

    // test that isSelected agrees
    for (int i=0; i<indexList.count(); ++i) {
        QModelIndex idx = indexList.at(i);
        QVERIFY2(selection->isSelected(idx) == selectedList.contains(idx),
                 QString("isSelected(index: %1, %2) does not match selectedIndexes()")
                 .arg(idx.row())
                 .arg(idx.column()).toLatin1());
    }

    //for now we assume Rows/Columns flag is the same for all commands, therefore we just check lastCommand
    // test that isRowSelected agrees
    if (lastCommand & QItemSelectionModel::Rows) {
        for (int i=0; i<selectedList.count(); ++i)
            QVERIFY2(selection->isRowSelected(selectedList.at(i).row(),
                                             model->parent(selectedList.at(i))),
                    QString("isRowSelected(row: %1) does not match selectedIndexes()")
                    .arg(selectedList.at(i).row()).toLatin1());
    }

    // test that isColumnSelected agrees
    if (lastCommand & QItemSelectionModel::Columns) {
        for (int i=0; i<selectedList.count(); ++i)
            QVERIFY2(selection->isColumnSelected(selectedList.at(i).column(),
                                                model->parent(selectedList.at(i))),
                    QString("isColumnSelected(column: %1) does not match selectedIndexes()")
                    .arg(selectedList.at(i).column()).toLatin1());
    }
}

void tst_QItemSelectionModel::persistentselections_data()
{
    QTest::addColumn<PairList>("indexList");
    QTest::addColumn<IntList>("commandList");
    QTest::addColumn<IntList>("insertRows"); // start, count
    QTest::addColumn<IntList>("insertColumns"); // start, count
    QTest::addColumn<IntList>("deleteRows"); // start, count
    QTest::addColumn<IntList>("deleteColumns"); // start, count
    QTest::addColumn<PairList>("expectedList");

    PairList index, expected;
    IntList command, insertRows, insertColumns, deleteRows, deleteColumns;


    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0);
    command << QItemSelectionModel::ClearAndSelect;
    deleteRows << 4 << 1;
    expected << IntPair(0, 0);
    QTest::newRow("ClearAndSelect (0, 0). Delete last row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0);
    command << QItemSelectionModel::ClearAndSelect;
    deleteRows << 0 << 1;
    QTest::newRow("ClearAndSelect (0, 0). Delete first row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(1, 0);
    command << QItemSelectionModel::ClearAndSelect;
    deleteRows << 0 << 1;
    expected << IntPair(0, 0);
    QTest::newRow("ClearAndSelect (1, 0). Delete first row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0);
    command << QItemSelectionModel::ClearAndSelect;
    insertRows << 5 << 1;
    expected << IntPair(0, 0);
    QTest::newRow("ClearAndSelect (0, 0). Append row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0);
    command << QItemSelectionModel::ClearAndSelect;
    insertRows << 0 << 1;
    expected << IntPair(1, 0);
    QTest::newRow("ClearAndSelect (0, 0). Insert before first row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0)
          << IntPair(4, 0);
    command << QItemSelectionModel::ClearAndSelect;
    insertRows << 5 << 1;
    expected << IntPair(0, 0)
             << IntPair(1, 0)
             << IntPair(2, 0)
             << IntPair(3, 0)
             << IntPair(4, 0);
    QTest::newRow("ClearAndSelect (0, 0) to (4, 0). Append row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0)
          << IntPair(4, 0);
    command << QItemSelectionModel::ClearAndSelect;
    insertRows << 0  << 1;
    expected << IntPair(1, 0)
             << IntPair(2, 0)
             << IntPair(3, 0)
             << IntPair(4, 0)
             << IntPair(5, 0);
    QTest::newRow("ClearAndSelect (0, 0) to (4, 0). Insert before first row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0)
          << IntPair(4, 0);
    command << QItemSelectionModel::ClearAndSelect;
    deleteRows << 0  << 1;
    expected << IntPair(0, 0)
             << IntPair(1, 0)
             << IntPair(2, 0)
             << IntPair(3, 0);
    QTest::newRow("ClearAndSelect (0, 0) to (4, 0). Delete first row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0)
          << IntPair(4, 0);
    command << QItemSelectionModel::ClearAndSelect;
    deleteRows << 4  << 1;
    expected << IntPair(0, 0)
             << IntPair(1, 0)
             << IntPair(2, 0)
             << IntPair(3, 0);
    QTest::newRow("ClearAndSelect (0, 0) to (4, 0). Delete last row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0)
          << IntPair(4, 0);
    command << QItemSelectionModel::ClearAndSelect;
    deleteRows << 1  << 3;
    expected << IntPair(0, 0)
             << IntPair(1, 0);
    QTest::newRow("ClearAndSelect (0, 0) to (4, 0). Deleting all but first and last row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;

    index.clear(); expected.clear(); command.clear();
    insertRows.clear(); insertColumns.clear(); deleteRows.clear(); deleteColumns.clear();
    index << IntPair(0, 0)
          << IntPair(4, 0);
    command << QItemSelectionModel::ClearAndSelect;
    insertRows << 1 << 1;
    expected << IntPair(0, 0)
        // the inserted row should not be selected
             << IntPair(2, 0)
             << IntPair(3, 0)
             << IntPair(4, 0)
             << IntPair(5, 0);
    QTest::newRow("ClearAndSelect (0, 0) to (4, 0). Insert after first row.")
        << index << command
        << insertRows << insertColumns << deleteRows << deleteColumns
        << expected;
}

void tst_QItemSelectionModel::persistentselections()
{
    QFETCH(PairList, indexList);
    QFETCH(IntList, commandList);
    QFETCH(IntList, insertRows);
    QFETCH(IntList, insertColumns);
    QFETCH(IntList, deleteRows);
    QFETCH(IntList, deleteColumns);
    QFETCH(PairList, expectedList);

    // make sure the model is sane (5x5)
    QCOMPARE(model->rowCount(QModelIndex()), 5);
    QCOMPARE(model->columnCount(QModelIndex()), 5);

    // do selections
    for (int i=0; i<commandList.count(); ++i) {
        if (indexList.count() == commandList.count()) {
            QModelIndex index = model->index(indexList.at(i).first,
                                             indexList.at(i).second,
                                             QModelIndex());
            selection->select(index, (QItemSelectionModel::SelectionFlags)commandList.at(i));
        } else {
            QModelIndex tl = model->index(indexList.at(2*i).first,
                                          indexList.at(2*i).second,
                                          QModelIndex());
            QModelIndex br = model->index(indexList.at(2*i+1).first,
                                          indexList.at(2*i+1).second,
                                          QModelIndex());
            selection->select(QItemSelection(tl, br),
                              (QItemSelectionModel::SelectionFlags)commandList.at(i));
        }
    }
    // test that we have selected items
    QVERIFY(!selection->selectedIndexes().isEmpty());
    QVERIFY(selection->hasSelection());

    // insert/delete row and/or columns
    if (insertRows.count() > 1)
        model->insertRows(insertRows.at(0), insertRows.at(1), QModelIndex());
    if (insertColumns.count() > 1)
        model->insertColumns(insertColumns.at(0), insertColumns.at(1), QModelIndex());
    if (deleteRows.count() > 1)
        model->removeRows(deleteRows.at(0), deleteRows.at(1), QModelIndex());
    if (deleteColumns.count() > 1)
        model->removeColumns(deleteColumns.at(0), deleteColumns.at(1), QModelIndex());

    // check that the selected items are the correct number and indexes
    QModelIndexList selectedList = selection->selectedIndexes();
    QCOMPARE(selectedList.count(), expectedList.count());
    foreach(IntPair pair, expectedList) {
        QModelIndex index = model->index(pair.first, pair.second, QModelIndex());
        QVERIFY(selectedList.contains(index));
    }
}

// "make reset public"-model
class MyStandardItemModel: public QStandardItemModel
{
    Q_OBJECT
public:
    inline MyStandardItemModel(int i1, int i2): QStandardItemModel(i1, i2) {}
    inline void reset() { beginResetModel(); endResetModel(); }
};

void tst_QItemSelectionModel::resetModel()
{
    MyStandardItemModel model(20, 20);
    QItemSelectionModel *selectionModel = new QItemSelectionModel(&model);

    QSignalSpy spy(selectionModel, &QItemSelectionModel::selectionChanged);
    QVERIFY(spy.isValid());

    selectionModel->select(QItemSelection(model.index(0, 0), model.index(5, 5)), QItemSelectionModel::Select);

    QCOMPARE(spy.count(), 1);

    model.reset();

    QVERIFY(selectionModel->selection().isEmpty());
    QVERIFY(!selectionModel->hasSelection());

    selectionModel->select(QItemSelection(model.index(0, 0), model.index(5, 5)), QItemSelectionModel::Select);

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).count(), 2);
    // make sure we don't get an "old selection"
    QCOMPARE(spy.at(1).at(1).userType(), qMetaTypeId<QItemSelection>());
    QVERIFY(qvariant_cast<QItemSelection>(spy.at(1).at(1)).isEmpty());
}

void tst_QItemSelectionModel::removeRows_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");

    QTest::addColumn<int>("selectTop");
    QTest::addColumn<int>("selectLeft");
    QTest::addColumn<int>("selectBottom");
    QTest::addColumn<int>("selectRight");

    QTest::addColumn<int>("removeTop");
    QTest::addColumn<int>("removeBottom");

    QTest::addColumn<int>("expectedTop");
    QTest::addColumn<int>("expectedLeft");
    QTest::addColumn<int>("expectedBottom");
    QTest::addColumn<int>("expectedRight");

    QTest::newRow("4x4 <0,1><1,1>")
        << 4 << 4
        << 0 << 1 << 1 << 1
        << 0 << 0
        << 0 << 1 << 0 << 1;
}

void tst_QItemSelectionModel::removeRows()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, selectTop);
    QFETCH(int, selectLeft);
    QFETCH(int, selectBottom);
    QFETCH(int, selectRight);
    QFETCH(int, removeTop);
    QFETCH(int, removeBottom);
    QFETCH(int, expectedTop);
    QFETCH(int, expectedLeft);
    QFETCH(int, expectedBottom);
    QFETCH(int, expectedRight);

    MyStandardItemModel model(rowCount, columnCount);
    QItemSelectionModel selections(&model);
    QSignalSpy spy(&selections, &QItemSelectionModel::selectionChanged);
    QVERIFY(spy.isValid());

    QModelIndex tl = model.index(selectTop, selectLeft);
    QModelIndex br = model.index(selectBottom, selectRight);
    selections.select(QItemSelection(tl, br), QItemSelectionModel::ClearAndSelect);

    QCOMPARE(spy.count(), 1);
    QVERIFY(selections.isSelected(tl));
    QVERIFY(selections.isSelected(br));
    QVERIFY(selections.hasSelection());

    model.removeRows(removeTop, removeBottom - removeTop + 1);

    QCOMPARE(spy.count(), 2);
    tl = model.index(expectedTop, expectedLeft);
    br = model.index(expectedBottom, expectedRight);
    QVERIFY(selections.isSelected(tl));
    QVERIFY(selections.isSelected(br));
}

void tst_QItemSelectionModel::removeColumns_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");

    QTest::addColumn<int>("selectTop");
    QTest::addColumn<int>("selectLeft");
    QTest::addColumn<int>("selectBottom");
    QTest::addColumn<int>("selectRight");

    QTest::addColumn<int>("removeLeft");
    QTest::addColumn<int>("removeRight");

    QTest::addColumn<int>("expectedTop");
    QTest::addColumn<int>("expectedLeft");
    QTest::addColumn<int>("expectedBottom");
    QTest::addColumn<int>("expectedRight");

    QTest::newRow("4x4 <0,1><1,1>")
        << 4 << 4
        << 1 << 0 << 1 << 1
        << 0 << 0
        << 1 << 0 << 1 << 0;
}

void tst_QItemSelectionModel::removeColumns()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, selectTop);
    QFETCH(int, selectLeft);
    QFETCH(int, selectBottom);
    QFETCH(int, selectRight);
    QFETCH(int, removeLeft);
    QFETCH(int, removeRight);
    QFETCH(int, expectedTop);
    QFETCH(int, expectedLeft);
    QFETCH(int, expectedBottom);
    QFETCH(int, expectedRight);

    MyStandardItemModel model(rowCount, columnCount);
    QItemSelectionModel selections(&model);
    QSignalSpy spy(&selections, &QItemSelectionModel::selectionChanged);
    QVERIFY(spy.isValid());

    QModelIndex tl = model.index(selectTop, selectLeft);
    QModelIndex br = model.index(selectBottom, selectRight);
    selections.select(QItemSelection(tl, br), QItemSelectionModel::ClearAndSelect);

    QCOMPARE(spy.count(), 1);
    QVERIFY(selections.isSelected(tl));
    QVERIFY(selections.isSelected(br));
    QVERIFY(selections.hasSelection());

    model.removeColumns(removeLeft, removeRight - removeLeft + 1);

    QCOMPARE(spy.count(), 2);
    tl = model.index(expectedTop, expectedLeft);
    br = model.index(expectedBottom, expectedRight);
    QVERIFY(selections.isSelected(tl));
    QVERIFY(selections.isSelected(br));
}

typedef QList<IntList> IntListList;
typedef QPair<IntPair, IntPair> IntPairPair;
typedef QList<IntPairPair> IntPairPairList;

void tst_QItemSelectionModel::modelLayoutChanged_data()
{
    QTest::addColumn<IntListList>("items");
    QTest::addColumn<IntPairPairList>("initialSelectedRanges");
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<int>("sortColumn");
    QTest::addColumn<IntPairPairList>("expectedSelectedRanges");

    QTest::newRow("everything selected, then row order reversed")
        << (IntListList()
            << (IntList() << 0 << 1 << 2 << 3)
            << (IntList() << 3 << 2 << 1 << 0))
        << (IntPairPairList()
            << IntPairPair(IntPair(0, 0), IntPair(3, 1)))
        << int(Qt::DescendingOrder)
        << 0
        << (IntPairPairList()
            << IntPairPair(IntPair(0, 0), IntPair(3, 1)));
    QTest::newRow("first two rows selected, then row order reversed")
        << (IntListList()
            << (IntList() << 0 << 1 << 2 << 3)
            << (IntList() << 3 << 2 << 1 << 0))
        << (IntPairPairList()
            << IntPairPair(IntPair(0, 0), IntPair(1, 1)))
        << int(Qt::DescendingOrder)
        << 0
        << (IntPairPairList()
            << IntPairPair(IntPair(2, 0), IntPair(3, 1)));
    QTest::newRow("middle two rows selected, then row order reversed")
        << (IntListList()
            << (IntList() << 0 << 1 << 2 << 3)
            << (IntList() << 3 << 2 << 1 << 0))
        << (IntPairPairList()
            << IntPairPair(IntPair(1, 0), IntPair(2, 1)))
        << int(Qt::DescendingOrder)
        << 0
        << (IntPairPairList()
            << IntPairPair(IntPair(1, 0), IntPair(2, 1)));
    QTest::newRow("two ranges")
        << (IntListList()
            << (IntList() << 2 << 0 << 3 << 1)
            << (IntList() << 2 << 0 << 3 << 1))
        << (IntPairPairList()
            << IntPairPair(IntPair(1, 0), IntPair(1, 1))
            << IntPairPair(IntPair(3, 0), IntPair(3, 1)))
        << int(Qt::AscendingOrder)
        << 0
        << (IntPairPairList()
            << IntPairPair(IntPair(0, 0), IntPair(0, 1))
            << IntPairPair(IntPair(1, 0), IntPair(1, 1)));
}

void tst_QItemSelectionModel::modelLayoutChanged()
{
    QFETCH(IntListList, items);
    QFETCH(IntPairPairList, initialSelectedRanges);
    QFETCH(int, sortOrder);
    QFETCH(int, sortColumn);
    QFETCH(IntPairPairList, expectedSelectedRanges);

    MyStandardItemModel model(items.at(0).count(), items.count());
    // initialize model data
    for (int i = 0; i < model.rowCount(); ++i) {
        for (int j = 0; j < model.columnCount(); ++j) {
            QModelIndex index = model.index(i, j);
            model.setData(index, items.at(j).at(i), Qt::DisplayRole);
        }
    }

    // select initial ranges
    QItemSelectionModel selectionModel(&model);
    foreach (IntPairPair range, initialSelectedRanges) {
        IntPair tl = range.first;
        IntPair br = range.second;
        QItemSelection selection(
            model.index(tl.first, tl.second),
            model.index(br.first, br.second));
        selectionModel.select(selection, QItemSelectionModel::Select);
    }

    // sort the model
    model.sort(sortColumn, Qt::SortOrder(sortOrder));

    // verify that selection is as expected
    QItemSelection selection = selectionModel.selection();
    QCOMPARE(selection.count(), expectedSelectedRanges.count());
    QCOMPARE(selectionModel.hasSelection(), !expectedSelectedRanges.isEmpty());

    for (int i = 0; i < expectedSelectedRanges.count(); ++i) {
        IntPairPair expectedRange = expectedSelectedRanges.at(i);
        IntPair expectedTl = expectedRange.first;
        IntPair expectedBr = expectedRange.second;
        QItemSelectionRange actualRange = selection.at(i);
        QModelIndex actualTl = actualRange.topLeft();
        QModelIndex actualBr = actualRange.bottomRight();
        QCOMPARE(actualTl.row(), expectedTl.first);
        QCOMPARE(actualTl.column(), expectedTl.second);
        QCOMPARE(actualBr.row(), expectedBr.first);
        QCOMPARE(actualBr.column(), expectedBr.second);
    }
}

void tst_QItemSelectionModel::selectedRows_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<int>("column");
    QTest::addColumn<IntList>("selectRows");
    QTest::addColumn<IntList>("expectedRows");
    QTest::addColumn<IntList>("unexpectedRows");

    QTest::newRow("10x10, first row")
        << 10 << 10 << 0
        << (IntList() << 0)
        << (IntList() << 0)
        << (IntList() << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9);

    QTest::newRow("10x10, first 4 rows")
        << 10 << 10 << 0
        << (IntList() << 0 << 1 << 2 << 3)
        << (IntList() << 0 << 1 << 2 << 3)
        << (IntList() << 4 << 5 << 6 << 7 << 8 << 9);

    QTest::newRow("10x10, last 4 rows")
        << 10 << 10 << 0
        << (IntList() << 6 << 7 << 8 << 9)
        << (IntList() << 6 << 7 << 8 << 9)
        << (IntList() << 0 << 1 << 2 << 3 << 4 << 6);
}

void tst_QItemSelectionModel::selectedRows()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, column);
    QFETCH(IntList, selectRows);
    QFETCH(IntList, expectedRows);
    QFETCH(IntList, unexpectedRows);

    MyStandardItemModel model(rowCount, columnCount);
    QItemSelectionModel selectionModel(&model);

    for (int i = 0; i < selectRows.count(); ++i)
        selectionModel.select(model.index(selectRows.at(i), 0),
                              QItemSelectionModel::Select
                              |QItemSelectionModel::Rows);

    for (int j = 0; j < selectRows.count(); ++j)
        QVERIFY(selectionModel.isRowSelected(expectedRows.at(j), QModelIndex()));

    for (int k = 0; k < selectRows.count(); ++k)
        QVERIFY(!selectionModel.isRowSelected(unexpectedRows.at(k), QModelIndex()));

    QModelIndexList selectedRowIndexes = selectionModel.selectedRows(column);
    QCOMPARE(selectedRowIndexes.count(), expectedRows.count());
    std::sort(selectedRowIndexes.begin(), selectedRowIndexes.end());
    for (int l = 0; l < selectedRowIndexes.count(); ++l) {
        QCOMPARE(selectedRowIndexes.at(l).row(), expectedRows.at(l));
        QCOMPARE(selectedRowIndexes.at(l).column(), column);
    }
}

void tst_QItemSelectionModel::selectedColumns_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<int>("row");
    QTest::addColumn<IntList>("selectColumns");
    QTest::addColumn<IntList>("expectedColumns");
    QTest::addColumn<IntList>("unexpectedColumns");

    QTest::newRow("10x10, first columns")
        << 10 << 10 << 0
        << (IntList() << 0)
        << (IntList() << 0)
        << (IntList() << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9);

    QTest::newRow("10x10, first 4 columns")
        << 10 << 10 << 0
        << (IntList() << 0 << 1 << 2 << 3)
        << (IntList() << 0 << 1 << 2 << 3)
        << (IntList() << 4 << 5 << 6 << 7 << 8 << 9);

    QTest::newRow("10x10, last 4 columns")
        << 10 << 10 << 0
        << (IntList() << 6 << 7 << 8 << 9)
        << (IntList() << 6 << 7 << 8 << 9)
        << (IntList() << 0 << 1 << 2 << 3 << 4 << 6);
}

void tst_QItemSelectionModel::selectedColumns()
{
    QFETCH(int, rowCount);
    QFETCH(int, columnCount);
    QFETCH(int, row);
    QFETCH(IntList, selectColumns);
    QFETCH(IntList, expectedColumns);
    QFETCH(IntList, unexpectedColumns);

    MyStandardItemModel model(rowCount, columnCount);
    QItemSelectionModel selectionModel(&model);

    for (int i = 0; i < selectColumns.count(); ++i)
        selectionModel.select(model.index(0, selectColumns.at(i)),
                              QItemSelectionModel::Select
                              |QItemSelectionModel::Columns);

    for (int j = 0; j < selectColumns.count(); ++j)
        QVERIFY(selectionModel.isColumnSelected(expectedColumns.at(j), QModelIndex()));

    for (int k = 0; k < selectColumns.count(); ++k)
        QVERIFY(!selectionModel.isColumnSelected(unexpectedColumns.at(k), QModelIndex()));

    QModelIndexList selectedColumnIndexes = selectionModel.selectedColumns(row);
    QCOMPARE(selectedColumnIndexes.count(), expectedColumns.count());
    std::sort(selectedColumnIndexes.begin(), selectedColumnIndexes.end());
    for (int l = 0; l < selectedColumnIndexes.count(); ++l) {
        QCOMPARE(selectedColumnIndexes.at(l).column(), expectedColumns.at(l));
        QCOMPARE(selectedColumnIndexes.at(l).row(), row);
    }
}

void tst_QItemSelectionModel::setCurrentIndex()
{
    // Build up a simple tree
    QScopedPointer<QStandardItemModel> treemodel(new QStandardItemModel(0, 1));
    treemodel->insertRow(0, new QStandardItem(1));
    treemodel->insertRow(1, new QStandardItem(2));

    QItemSelectionModel selectionModel(treemodel.data());
    selectionModel.setCurrentIndex(
            treemodel->index(0, 0, treemodel->index(0, 0)),
            QItemSelectionModel::SelectCurrent);

    QSignalSpy currentSpy(&selectionModel, &QItemSelectionModel::currentChanged);
    QSignalSpy rowSpy(&selectionModel, &QItemSelectionModel::currentRowChanged);
    QSignalSpy columnSpy(&selectionModel, &QItemSelectionModel::currentColumnChanged);

    QVERIFY(currentSpy.isValid());
    QVERIFY(rowSpy.isValid());
    QVERIFY(columnSpy.isValid());

    // Select the same row and column indexes, but with a different parent
    selectionModel.setCurrentIndex(
            treemodel->index(0, 0, treemodel->index(1, 0)),
            QItemSelectionModel::SelectCurrent);

    QCOMPARE(currentSpy.count(), 1);
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(columnSpy.count(), 1);

    // Select another row in the same parent
    selectionModel.setCurrentIndex(
            treemodel->index(1, 0, treemodel->index(1, 0)),
            QItemSelectionModel::SelectCurrent);

    QCOMPARE(currentSpy.count(), 2);
    QCOMPARE(rowSpy.count(), 2);
    QCOMPARE(columnSpy.count(), 1);
}

void tst_QItemSelectionModel::splitOnInsert()
{
    QStandardItemModel model(4, 1);
    QItemSelectionModel selectionModel(&model);
    selectionModel.select(model.index(2, 0), QItemSelectionModel::Select);
    model.insertRow(2);
    model.removeRow(3);
    QVERIFY(!selectionModel.isSelected(model.index(1, 0)));
}

void tst_QItemSelectionModel::rowIntersectsSelection1()
{
    QStandardItemModel model;
    model.setItem(0, 0, new QStandardItem("foo"));
    QItemSelectionModel selectionModel(&model);

    QModelIndex index = model.index(0, 0, QModelIndex());

    selectionModel.select(index, QItemSelectionModel::Select);
    QVERIFY(selectionModel.rowIntersectsSelection(0, QModelIndex()));
    QVERIFY(selectionModel.columnIntersectsSelection(0, QModelIndex()));

    selectionModel.select(index, QItemSelectionModel::Deselect);
    QVERIFY(!selectionModel.rowIntersectsSelection(0, QModelIndex()));
    QVERIFY(!selectionModel.columnIntersectsSelection(0, QModelIndex()));

    selectionModel.select(index, QItemSelectionModel::Toggle);
    QVERIFY(selectionModel.rowIntersectsSelection(0, QModelIndex()));
    QVERIFY(selectionModel.columnIntersectsSelection(0, QModelIndex()));

    selectionModel.select(index, QItemSelectionModel::Toggle);
    QVERIFY(!selectionModel.rowIntersectsSelection(0, QModelIndex()));
    QVERIFY(!selectionModel.columnIntersectsSelection(0, QModelIndex()));
}

void tst_QItemSelectionModel::rowIntersectsSelection2()
{
    QStandardItemModel m;
    for (int i=0; i<8; ++i) {
        const QString text = QLatin1String("Item number ") + QString::number(i);
        for (int j=0; j<8; ++j) {
            QStandardItem *item = new QStandardItem(text);
            if ((i % 2 == 0 && j == 0)  ||
                (j % 2 == 0 && i == 0)  ||
                 j == 5 || i == 5 ) {
                item->setEnabled(false);
                //item->setSelectable(false);
            }
            m.setItem(i, j, item);
        }
    }

    QItemSelectionModel selected(&m);
    //nothing is selected
    QVERIFY(!selected.rowIntersectsSelection(0, QModelIndex()));
    QVERIFY(!selected.rowIntersectsSelection(2, QModelIndex()));
    QVERIFY(!selected.rowIntersectsSelection(3, QModelIndex()));
    QVERIFY(!selected.rowIntersectsSelection(5, QModelIndex()));
    QVERIFY(!selected.columnIntersectsSelection(0, QModelIndex()));
    QVERIFY(!selected.columnIntersectsSelection(2, QModelIndex()));
    QVERIFY(!selected.columnIntersectsSelection(3, QModelIndex()));
    QVERIFY(!selected.columnIntersectsSelection(5, QModelIndex()));
    selected.select(m.index(2, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    QVERIFY(!selected.rowIntersectsSelection(0, QModelIndex()));
    QVERIFY( selected.rowIntersectsSelection(2, QModelIndex()));
    QVERIFY(!selected.rowIntersectsSelection(3, QModelIndex()));
    QVERIFY(!selected.rowIntersectsSelection(5, QModelIndex()));
    QVERIFY(!selected.columnIntersectsSelection(0, QModelIndex()));
    QVERIFY( selected.columnIntersectsSelection(2, QModelIndex()));
    QVERIFY( selected.columnIntersectsSelection(3, QModelIndex()));
    QVERIFY(!selected.columnIntersectsSelection(5, QModelIndex()));
    selected.select(m.index(0, 5), QItemSelectionModel::Select | QItemSelectionModel::Columns);
    QVERIFY(!selected.rowIntersectsSelection(0, QModelIndex()));
    QVERIFY( selected.rowIntersectsSelection(2, QModelIndex()));
    QVERIFY(!selected.rowIntersectsSelection(3, QModelIndex()));
    QVERIFY(!selected.rowIntersectsSelection(5, QModelIndex()));
    QVERIFY(!selected.columnIntersectsSelection(0, QModelIndex()));
    QVERIFY( selected.columnIntersectsSelection(2, QModelIndex()));
    QVERIFY( selected.columnIntersectsSelection(3, QModelIndex()));
    QVERIFY(!selected.columnIntersectsSelection(5, QModelIndex()));
}

void tst_QItemSelectionModel::rowIntersectsSelection3()
{
    QStandardItemModel model;
    QStandardItem *parentItem = model.invisibleRootItem();
    for (int i = 0; i < 4; ++i) {
        QStandardItem *item = new QStandardItem(QLatin1String("item ") + QString::number(i));
        parentItem->appendRow(item);
        parentItem = item;
    }

    QItemSelectionModel selectionModel(&model);

    selectionModel.select(model.index(0, 0, model.index(0, 0)), QItemSelectionModel::Select);

    QModelIndex parent;
    QVERIFY(!selectionModel.rowIntersectsSelection(0, parent));
    parent = model.index(0, 0, parent);
    QVERIFY(selectionModel.rowIntersectsSelection(0, parent));
    parent = model.index(0, 0, parent);
    QVERIFY(!selectionModel.rowIntersectsSelection(0, parent));
    parent = model.index(0, 0, parent);
    QVERIFY(!selectionModel.rowIntersectsSelection(0, parent));
}

void tst_QItemSelectionModel::unselectable()
{
    QStandardItemModel model;
    QStandardItem *parentItem = model.invisibleRootItem();

    for (int i = 0; i < 10; ++i) {
        QStandardItem *item = new QStandardItem(QLatin1String("item ") + QString::number(i));
        parentItem->appendRow(item);
    }
    QItemSelectionModel selectionModel(&model);
    selectionModel.select(QItemSelection(model.index(0, 0), model.index(9, 0)), QItemSelectionModel::Select);
    QCOMPARE(selectionModel.selectedIndexes().count(), 10);
    QCOMPARE(selectionModel.selectedRows().count(), 10);
    for (int j = 0; j < 10; ++j)
        model.item(j)->setFlags(0);
    QCOMPARE(selectionModel.selectedIndexes().count(), 0);
    QCOMPARE(selectionModel.selectedRows().count(), 0);
}

void tst_QItemSelectionModel::selectedIndexes()
{
    QStandardItemModel model(2, 2);
    QItemSelectionModel selectionModel(&model);
    QItemSelection selection;
    selection.append(QItemSelectionRange(model.index(0,0)));
    selection.append(QItemSelectionRange(model.index(0,1)));

    //we select the 1st row
    selectionModel.select(selection, QItemSelectionModel::Rows | QItemSelectionModel::Select);

    QCOMPARE(selectionModel.selectedRows().count(), 1);
    QCOMPARE(selectionModel.selectedIndexes().count(), model.columnCount());
}


class QtTestTableModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    QtTestTableModel(int rows = 0, int columns = 0, QObject *parent = 0)
        : QAbstractTableModel(parent)
        , row_count(rows)
        , column_count(columns)
    {
    }

    int rowCount(const QModelIndex& = QModelIndex()) const { return row_count; }
    int columnCount(const QModelIndex& = QModelIndex()) const { return column_count; }
    bool isEditable(const QModelIndex &) const { return true; }

    QVariant data(const QModelIndex &idx, int role) const
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return QLatin1Char('[') + QString::number(idx.row()) + QLatin1Char(',')
                + QString::number(idx.column()) + QLatin1Char(']');
        return QVariant();
    }

    int row_count;
    int column_count;
    friend class tst_QItemSelectionModel;
};


void tst_QItemSelectionModel::layoutChanged()
{
    QtTestTableModel model(1,1);
    QItemSelectionModel selectionModel(&model);
    selectionModel.select(model.index(0,0), QItemSelectionModel::Select);
    QCOMPARE(selectionModel.selectedIndexes().count() , 1);

    emit model.layoutAboutToBeChanged();
    model.row_count = 5;
    emit model.layoutChanged();

    //The selection should not change.
    QCOMPARE(selectionModel.selectedIndexes().count() , 1);
    QCOMPARE(selectionModel.selectedIndexes().first() , model.index(0,0));
}

void tst_QItemSelectionModel::merge_data()
{
    QTest::addColumn<QItemSelection>("init");
    QTest::addColumn<QItemSelection>("other");
    QTest::addColumn<int>("command");
    QTest::addColumn<QItemSelection>("result");

    QTest::newRow("Simple select")
        << QItemSelection()
        << QItemSelection(model->index(2, 1) , model->index(3, 4))
        << int(QItemSelectionModel::Select)
        << QItemSelection(model->index(2, 1) , model->index(3, 4));

    QTest::newRow("Simple deselect")
        << QItemSelection(model->index(2, 1) , model->index(3, 4))
        << QItemSelection(model->index(2, 1) , model->index(3, 4))
        << int(QItemSelectionModel::Deselect)
        << QItemSelection();

    QTest::newRow("Simple Toggle deselect")
        << QItemSelection(model->index(2, 1) , model->index(3, 4))
        << QItemSelection(model->index(2, 1) , model->index(3, 4))
        << int(QItemSelectionModel::Toggle)
        << QItemSelection();

    QTest::newRow("Simple Toggle select")
        << QItemSelection()
        << QItemSelection(model->index(2, 1) , model->index(3, 4))
        << int(QItemSelectionModel::Toggle)
        << QItemSelection(model->index(2, 1) , model->index(3, 4));

    QTest::newRow("Add select")
        << QItemSelection(model->index(2, 1) , model->index(3, 3))
        << QItemSelection(model->index(2, 2) , model->index(3, 4))
        << int(QItemSelectionModel::Select)
        << QItemSelection(model->index(2, 1) , model->index(3, 4));

    QTest::newRow("Deselect")
        << QItemSelection(model->index(2, 1) , model->index(3, 4))
        << QItemSelection(model->index(2, 2) , model->index(3, 4))
        << int(QItemSelectionModel::Deselect)
        << QItemSelection(model->index(2, 1) , model->index(3, 1));

    QItemSelection r1(model->index(2, 1) , model->index(3, 1));
    r1.select(model->index(2, 4) , model->index(3, 4));
    QTest::newRow("Toggle")
        << QItemSelection(model->index(2, 1) , model->index(3, 3))
        << QItemSelection(model->index(2, 2) , model->index(3, 4))
        << int(QItemSelectionModel::Toggle)
        << r1;
}

void tst_QItemSelectionModel::merge()
{
    QFETCH(QItemSelection, init);
    QFETCH(QItemSelection, other);
    QFETCH(int, command);
    QFETCH(QItemSelection, result);

    init.merge(other, QItemSelectionModel::SelectionFlags(command));

    foreach(const QModelIndex &idx, init.indexes())
        QVERIFY(result.contains(idx));
    foreach(const QModelIndex &idx, result.indexes())
        QVERIFY(init.contains(idx));
}

void tst_QItemSelectionModel::isRowSelected()
{
    QStandardItemModel model(2,2);
    model.setData(model.index(0,0), 0, Qt::UserRole - 1);
    QItemSelectionModel sel(&model);
    sel.select( QItemSelection(model.index(0,0), model.index(0, 1)), QItemSelectionModel::Select);
    QCOMPARE(sel.selectedIndexes().count(), 1);
    QVERIFY(sel.isRowSelected(0, QModelIndex()));
}

void tst_QItemSelectionModel::childrenDeselectionSignal()
{
    QStandardItemModel model;

    QStandardItem *parentItem = model.invisibleRootItem();
    for (int i = 0; i < 4; ++i) {
        QStandardItem *item = new QStandardItem(QLatin1String("item ") + QString::number(i));
        parentItem->appendRow(item);
        parentItem = item;
    }

    QModelIndex root = model.index(0,0);
    QModelIndex par = root.child(0,0);
    QModelIndex sel = par.child(0,0);

    QItemSelectionModel selectionModel(&model);
    selectionModel.select(sel, QItemSelectionModel::SelectCurrent);

    QSignalSpy deselectSpy(&selectionModel, &QItemSelectionModel::selectionChanged);
    QVERIFY(deselectSpy.isValid());
    model.removeRows(0, 1, root);
    QCOMPARE(deselectSpy.count(), 1);

    // More testing stress for the patch.
    model.clear();
    selectionModel.clear();

    parentItem = model.invisibleRootItem();
    for (int i = 0; i < 2; ++i) {
        QStandardItem *item = new QStandardItem(QLatin1String("item ") + QString::number(i));
        parentItem->appendRow(item);
    }
    for (int i = 0; i < 2; ++i) {
        parentItem = model.invisibleRootItem()->child(i, 0);
        const QString prefix = QLatin1String("item ") + QString::number(i) + QLatin1Char('.');
        for (int j = 0; j < 2; ++j) {
            QStandardItem *item = new QStandardItem(prefix + QString::number(j));
            parentItem->appendRow(item);
        }
    }

    sel = model.index(0, 0).child(0, 0);
    selectionModel.select(sel, QItemSelectionModel::Select);
    QModelIndex sel2 = model.index(1, 0).child(0, 0);
    selectionModel.select(sel2, QItemSelectionModel::Select);

    QVERIFY(selectionModel.selection().contains(sel));
    QVERIFY(selectionModel.selection().contains(sel2));
    deselectSpy.clear();
    model.removeRow(0, model.index(0, 0));
    QCOMPARE(deselectSpy.count(), 1);
    QVERIFY(!selectionModel.selection().contains(sel));
    QVERIFY(selectionModel.selection().contains(sel2));
}

void tst_QItemSelectionModel::layoutChangedWithAllSelected1()
{
    QStringListModel model( QStringList() << "foo" << "bar" << "foo2");
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    QItemSelectionModel selection(&proxy);


    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(proxy.rowCount(), 3);
    proxy.setFilterRegExp( QRegExp("f"));
    QCOMPARE(proxy.rowCount(), 2);

    QList<QPersistentModelIndex> indexList;
    indexList << proxy.index(0,0) << proxy.index(1,0);
    selection.select( QItemSelection(indexList.first(), indexList.last()), QItemSelectionModel::Select);

    //let's check the selection hasn't changed
    QCOMPARE(selection.selectedIndexes().count(), indexList.count());
    foreach(QPersistentModelIndex index, indexList)
        QVERIFY(selection.isSelected(index));

    proxy.setFilterRegExp(QRegExp());
    QCOMPARE(proxy.rowCount(), 3);

    //let's check the selection hasn't changed
    QCOMPARE(selection.selectedIndexes().count(), indexList.count());
    foreach(QPersistentModelIndex index, indexList)
        QVERIFY(selection.isSelected(index));
}

// Same as layoutChangedWithAllSelected1, but with a slightly bigger model.
// This test is a regression test for QTBUG-5671.
void tst_QItemSelectionModel::layoutChangedWithAllSelected2()
{
    struct MyFilterModel : public QSortFilterProxyModel
    {     // Override sort filter proxy to remove even numbered rows.
        bool filtering;
        virtual bool filterAcceptsRow( int source_row, const QModelIndex& /* source_parent */) const
        {
            return !filtering || !( source_row & 1 );
        }
    };

    enum { cNumRows=30, cNumCols=20 };

    QStandardItemModel model(cNumRows, cNumCols);
    MyFilterModel proxy;
    proxy.filtering = true;
    proxy.setSourceModel(&model);
    QItemSelectionModel selection(&proxy);

    // Populate the tree view.
    for (unsigned int i = 0; i < cNumCols; i++)
        model.setHeaderData( i, Qt::Horizontal, QLatin1String("Column ") + QString::number(i));

    for (unsigned int r = 0; r < cNumRows; r++) {
        const QString prefix = QLatin1String("r:") + QString::number(r) + QLatin1String("/c:");
        for (unsigned int c = 0; c < cNumCols; c++)
            model.setData(model.index(r, c, QModelIndex()), prefix + QString::number(c));
    }

    QCOMPARE(model.rowCount(), int(cNumRows));
    QCOMPARE(proxy.rowCount(), int(cNumRows/2));

    selection.select( QItemSelection(proxy.index(0,0), proxy.index(proxy.rowCount() - 1, proxy.columnCount() - 1)), QItemSelectionModel::Select);

    QList<QPersistentModelIndex> indexList;
    foreach(const QModelIndex &id, selection.selectedIndexes())
        indexList << id;

    proxy.filtering = false;
    proxy.invalidate();
    QCOMPARE(proxy.rowCount(), int(cNumRows));

    //let's check the selection hasn't changed
    QCOMPARE(selection.selectedIndexes().count(), indexList.count());
    foreach(QPersistentModelIndex index, indexList)
        QVERIFY(selection.isSelected(index));
}

// This test is a regression test for QTBUG-2804.
void tst_QItemSelectionModel::layoutChangedTreeSelection()
{
    QStandardItemModel model;
    QStandardItem top1("Child1"), top2("Child2"), top3("Child3");
    QStandardItem sub11("Alpha"), sub12("Beta"), sub13("Gamma"), sub14("Delta"),
        sub21("Alpha"), sub22("Beta"), sub23("Gamma"), sub24("Delta");
    top1.appendColumn(QList<QStandardItem*>() << &sub11 << &sub12 << &sub13 << &sub14);
    top2.appendColumn(QList<QStandardItem*>() << &sub21 << &sub22 << &sub23 << &sub24);
    model.appendColumn(QList<QStandardItem*>() << &top1 << &top2 << &top3);

    QItemSelectionModel selModel(&model);

    selModel.select(sub11.index(), QItemSelectionModel::Select);
    selModel.select(sub12.index(), QItemSelectionModel::Select);
    selModel.select(sub21.index(), QItemSelectionModel::Select);
    selModel.select(sub23.index(), QItemSelectionModel::Select);

    QModelIndexList list = selModel.selectedIndexes();
    QCOMPARE(list.count(), 4);

    model.sort(0); //this will provoke a relayout

    QCOMPARE(selModel.selectedIndexes().count(), 4);
}

class RemovalObserver : public QObject
{
    Q_OBJECT
    QItemSelectionModel *m_itemSelectionModel;
public:
    RemovalObserver(QItemSelectionModel *selectionModel)
      : m_itemSelectionModel(selectionModel)
    {
        connect(m_itemSelectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(selectionChanged(QItemSelection,QItemSelection)));
    }

public slots:
    void selectionChanged(const QItemSelection & /* selected */, const QItemSelection &deselected)
    {
        foreach(const QModelIndex &index, deselected.indexes()) {
            QVERIFY(!m_itemSelectionModel->selection().contains(index));
        }
        QCOMPARE(m_itemSelectionModel->selection().size(), 2);
    }
};

void tst_QItemSelectionModel::deselectRemovedMiddleRange()
{
    QStandardItemModel model(8, 0);

    for (int row = 0; row < 8; ++row) {
        static const int column = 0;
        QStandardItem *item = new QStandardItem(QString::number(row));
        model.setItem(row, column, item);
    }

    QItemSelectionModel selModel(&model);

    selModel.select(QItemSelection(model.index(3, 0), model.index(6, 0)), QItemSelectionModel::Select);

    QCOMPARE(selModel.selection().size(), 1);

    RemovalObserver ro(&selModel);

    QSignalSpy spy(&selModel, &QItemSelectionModel::selectionChanged);
    QVERIFY(spy.isValid());
    bool ok = model.removeRows(4, 2);

    QVERIFY(ok);
    QCOMPARE(spy.size(), 1);
}

static QStandardItemModel* getModel(QObject *parent)
{
    QStandardItemModel *model = new QStandardItemModel(parent);

    for (int i = 0; i < 4; ++i) {
        QStandardItem *parentItem = model->invisibleRootItem();
        QList<QStandardItem*> list;
        const QString prefix = QLatin1String("item ") + QString::number(i) + QLatin1String(", ");
        for (int j = 0; j < 4; ++j) {
            list.append(new QStandardItem(prefix + QString::number(j)));
        }
        parentItem->appendRow(list);
        parentItem = list.first();
        for (int j = 0; j < 4; ++j) {
            QList<QStandardItem*> list;
            for (int k = 0; k < 4; ++k) {
                list.append(new QStandardItem(prefix + QString::number(j)));
            }
            parentItem->appendRow(list);
        }
    }
    return model;
}

enum Result {
    LessThan,
    NotLessThan,
    NotEqual
};

Q_DECLARE_METATYPE(Result);

void tst_QItemSelectionModel::rangeOperatorLessThan_data()
{
    QTest::addColumn<int>("parent1");
    QTest::addColumn<int>("top1");
    QTest::addColumn<int>("left1");
    QTest::addColumn<int>("bottom1");
    QTest::addColumn<int>("right1");
    QTest::addColumn<int>("parent2");
    QTest::addColumn<int>("top2");
    QTest::addColumn<int>("left2");
    QTest::addColumn<int>("bottom2");
    QTest::addColumn<int>("right2");
    QTest::addColumn<Result>("result");

    QTest::newRow("lt01") << -1 << 0 << 0 << 3 << 3
                          << -1 << 0 << 0 << 3 << 3 << NotLessThan;

    QTest::newRow("lt02") << -1 << 0 << 0 << 2 << 3
                          << -1 << 0 << 0 << 3 << 3 << LessThan;
    QTest::newRow("lt03") << -1 << 0 << 0 << 3 << 2
                          << -1 << 0 << 0 << 3 << 3 << LessThan;
    QTest::newRow("lt04") << -1 << 0 << 0 << 2 << 2
                          << -1 << 0 << 0 << 3 << 3 << LessThan;

    QTest::newRow("lt05") << -1 << 0 << 0 << 3 << 3
                          << -1 << 0 << 0 << 2 << 3 << NotLessThan;
    QTest::newRow("lt06") << -1 << 0 << 0 << 3 << 3
                          << -1 << 0 << 0 << 3 << 2 << NotLessThan;
    QTest::newRow("lt07") << -1 << 0 << 0 << 3 << 3
                          << -1 << 0 << 0 << 2 << 2 << NotLessThan;

    QTest::newRow("lt08") << -1 << 0 << 0 << 3 << 3
                          << 0 << 0 << 0 << 3 << 3 << NotEqual;
    QTest::newRow("lt09") << 1 << 0 << 0 << 3 << 3
                          << 0 << 0 << 0 << 3 << 3 << NotEqual;
    QTest::newRow("lt10") << 1 << 0 << 0 << 1 << 1
                          << 0 << 2 << 2 << 3 << 3 << NotEqual;
    QTest::newRow("lt11") << 1 << 2 << 2 << 3 << 3
                          << 0 << 0 << 0 << 1 << 1 << NotEqual;

    QTest::newRow("lt12") << -1 << 0 << 0 << 1 << 1
                          << -1 << 2 << 2 << 3 << 3 << LessThan;
    QTest::newRow("lt13") << -1 << 2 << 2 << 3 << 3
                          << -1 << 0 << 0 << 1 << 1 << NotLessThan;
    QTest::newRow("lt14") << 1 << 0 << 0 << 1 << 1
                          << 1 << 2 << 2 << 3 << 3 << LessThan;
    QTest::newRow("lt15") << 1 << 2 << 2 << 3 << 3
                          << 1 << 0 << 0 << 1 << 1 << NotLessThan;

    QTest::newRow("lt16") << -1 << 0 << 0 << 2 << 2
                          << -1 << 1 << 1 << 3 << 3 << LessThan;
    QTest::newRow("lt17") << -1 << 1 << 1 << 3 << 3
                          << -1 << 0 << 0 << 2 << 2 << NotLessThan;
    QTest::newRow("lt18") << 1 << 0 << 0 << 2 << 2
                          << 1 << 1 << 1 << 3 << 3 << LessThan;
    QTest::newRow("lt19") << 1 << 1 << 1 << 3 << 3
                          << 1 << 0 << 0 << 2 << 2 << NotLessThan;
}

void tst_QItemSelectionModel::rangeOperatorLessThan()
{
    QStandardItemModel *model1 = getModel(this);
    QStandardItemModel *model2 = getModel(this);

    QFETCH(int, parent1);
    QFETCH(int, top1);
    QFETCH(int, left1);
    QFETCH(int, bottom1);
    QFETCH(int, right1);
    QFETCH(int, parent2);
    QFETCH(int, top2);
    QFETCH(int, left2);
    QFETCH(int, bottom2);
    QFETCH(int, right2);
    QFETCH(Result, result);

    QModelIndex p1 = model1->index(parent1, 0);

    QModelIndex tl1 = model1->index(top1, left1, p1);
    QModelIndex br1 = model1->index(bottom1, right1, p1);

    QItemSelectionRange r1(tl1, br1);

    QModelIndex p2 = model1->index(parent2, 0);

    QModelIndex tl2 = model1->index(top2, left2, p2);
    QModelIndex br2 = model1->index(bottom2, right2, p2);

    QItemSelectionRange r2(tl2, br2);

    if (result == LessThan)
        QVERIFY(r1 < r2);
    else if (result == NotLessThan)
        QVERIFY(!(r1 < r2));
    else if (result == NotEqual)
        if (!(r1 < r2))
            QVERIFY(r2 < r1);

    // Ranges in different models are always non-equal

    QModelIndex p3 = model2->index(parent1, 0);

    QModelIndex tl3 = model2->index(top1, left1, p3);
    QModelIndex br3 = model2->index(bottom1, right1, p3);

    QItemSelectionRange r3(tl3, br3);

    if (!(r1 < r3))
        QVERIFY(r3 < r1);

    if (!(r2 < r3))
        QVERIFY(r3 < r2);

    QModelIndex p4 = model2->index(parent2, 0);

    QModelIndex tl4 = model2->index(top2, left2, p4);
    QModelIndex br4 = model2->index(bottom2, right2, p4);

    QItemSelectionRange r4(tl4, br4);

    if (!(r1 < r4))
        QVERIFY(r4 < r1);

    if (!(r2 < r4))
        QVERIFY(r4 < r2);
}

void tst_QItemSelectionModel::setModel()
{
    QItemSelectionModel sel;
    QVERIFY(!sel.model());
    QSignalSpy modelChangedSpy(&sel, SIGNAL(modelChanged(QAbstractItemModel*)));
    QStringListModel model(QStringList() << "Blah" << "Blah" << "Blah");
    sel.setModel(&model);
    QCOMPARE(sel.model(), &model);
    QCOMPARE(modelChangedSpy.count(), 1);
    sel.select(model.index(0), QItemSelectionModel::Select);
    QVERIFY(!sel.selection().isEmpty());
    sel.setModel(0);
    QVERIFY(sel.selection().isEmpty());
}

void tst_QItemSelectionModel::testDifferentModels()
{
    QStandardItemModel model1;
    QStandardItemModel model2;
    QStandardItem top11("Child1"), top12("Child2"), top13("Child3");
    QStandardItem top21("Child1"), top22("Child2"), top23("Child3");

    model1.appendColumn(QList<QStandardItem*>() << &top11 << &top12 << &top13);
    model2.appendColumn(QList<QStandardItem*>() << &top21 << &top22 << &top23);

    QModelIndex topIndex1 = model1.index(0, 0);
    QModelIndex bottomIndex1 = model1.index(2, 0);
    QModelIndex topIndex2 = model2.index(0, 0);

    QItemSelectionRange range(topIndex1, bottomIndex1);

    QVERIFY(range.intersects(QItemSelectionRange(topIndex1, topIndex1)));
    QVERIFY(!range.intersects(QItemSelectionRange(topIndex2, topIndex2)));

    QItemSelection newSelection;
    QItemSelection::split(range, QItemSelectionRange(topIndex2, topIndex2), &newSelection);

    QVERIFY(newSelection.isEmpty());
}

class SelectionObserver : public QObject
{
    Q_OBJECT
public:
    SelectionObserver(QAbstractItemModel *model, QObject *parent = 0)
        : QObject(parent), m_model(model), m_selectionModel(0)
    {
        connect(model, SIGNAL(modelReset()), SLOT(modelReset()));
    }

    void setSelectionModel(QItemSelectionModel *selectionModel)
    {
        m_selectionModel = selectionModel;
        connect(m_selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(selectionChanged(QItemSelection,QItemSelection)));
    }

private slots:
    void modelReset()
    {
        const QModelIndex idx = m_model->index(2, 0);
        QVERIFY(idx.isValid());
        m_selectionModel->select(QItemSelection(idx, idx), QItemSelectionModel::Clear);
    }

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
    {
        foreach(const QItemSelectionRange &range, selected)
            QVERIFY(range.isValid());
        foreach(const QItemSelectionRange &range, deselected)
            QVERIFY(range.isValid());
    }

private:
    QAbstractItemModel *m_model;
    QItemSelectionModel *m_selectionModel;
};

void tst_QItemSelectionModel::testValidRangesInSelectionsAfterReset()
{
    QStringListModel model;

    QStringList strings;
    strings << "one"
            << "two"
            << "three"
            << "four"
            << "five";

    model.setStringList(strings);

    SelectionObserver observer(&model);

    QItemSelectionModel selectionModel(&model);

    selectionModel.select(QItemSelection(model.index(1, 0), model.index(3, 0)), QItemSelectionModel::Select);

    // Cause d->ranges to contain something.
    model.insertRows(2, 1);

    observer.setSelectionModel(&selectionModel);

    model.setStringList(strings);
}

class DuplicateItemSelectionModel : public QItemSelectionModel
{
    Q_OBJECT
public:
    DuplicateItemSelectionModel(QItemSelectionModel *target, QAbstractItemModel *model, QObject *parent = 0)
        : QItemSelectionModel(model, parent), m_target(target)
    {
    }

    void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
    {
        QItemSelectionModel::select(selection, command);
        m_target->select(selection, command);
    }

    using QItemSelectionModel::select;

    void setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
    {
        QItemSelectionModel::setCurrentIndex(index, command);
        m_target->setCurrentIndex(index, command);
    }

    void clearCurrentIndex()
    {
        QItemSelectionModel::clearCurrentIndex();
        m_target->clearCurrentIndex();
    }

private:
    QItemSelectionModel *m_target;
};

void tst_QItemSelectionModel::testChainedSelectionClear()
{
    QStringListModel model(QStringList() << "Apples" << "Pears");

    QItemSelectionModel selectionModel(&model, 0);
    DuplicateItemSelectionModel duplicate(&selectionModel, &model, 0);

    duplicate.select(model.index(0, 0), QItemSelectionModel::Select);

    {
        QModelIndexList selectedIndexes = selectionModel.selection().indexes();
        QModelIndexList duplicatedIndexes = duplicate.selection().indexes();

        QCOMPARE(selectedIndexes.size(), duplicatedIndexes.size());
        QCOMPARE(selectedIndexes.size(), 1);
        QVERIFY(selectedIndexes.first() == model.index(0, 0));
    }

    duplicate.clearSelection();

    {
        QModelIndexList selectedIndexes = selectionModel.selection().indexes();
        QModelIndexList duplicatedIndexes = duplicate.selection().indexes();

        QCOMPARE(selectedIndexes.size(), duplicatedIndexes.size());
        QCOMPARE(selectedIndexes.size(), 0);
    }

    duplicate.setCurrentIndex(model.index(0, 0), QItemSelectionModel::NoUpdate);

    QCOMPARE(selectionModel.currentIndex(), duplicate.currentIndex());

    duplicate.clearCurrentIndex();

    QVERIFY(!duplicate.currentIndex().isValid());
    QCOMPARE(selectionModel.currentIndex(), duplicate.currentIndex());
}

void tst_QItemSelectionModel::testClearCurrentIndex()
{
    QStringListModel model(QStringList() << "Apples" << "Pears");

    QItemSelectionModel selectionModel(&model, 0);

    QSignalSpy currentIndexSpy(&selectionModel, &QItemSelectionModel::currentChanged);
    QVERIFY(currentIndexSpy.isValid());

    QModelIndex firstIndex = model.index(0, 0);
    QVERIFY(firstIndex.isValid());
    selectionModel.setCurrentIndex(firstIndex, QItemSelectionModel::NoUpdate);
    QCOMPARE(selectionModel.currentIndex(), firstIndex);
    QCOMPARE(currentIndexSpy.size(), 1);

    selectionModel.clearCurrentIndex();

    QCOMPARE(selectionModel.currentIndex(), QModelIndex());
    QCOMPARE(currentIndexSpy.size(), 2);
}

void tst_QItemSelectionModel::QTBUG48402_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");

    QTest::addColumn<int>("selectTop");
    QTest::addColumn<int>("selectLeft");
    QTest::addColumn<int>("selectBottom");
    QTest::addColumn<int>("selectRight");

    QTest::addColumn<int>("removeTop");
    QTest::addColumn<int>("removeBottom");

    QTest::addColumn<int>("deselectTop");
    QTest::addColumn<int>("deselectLeft");
    QTest::addColumn<int>("deselectBottom");
    QTest::addColumn<int>("deselectRight");

    QTest::newRow("4x4 top intersection")
        << 4 << 4
        << 0 << 2 << 1 << 3
        << 1 << 1
        << 1 << 2 << 1 << 3;

    QTest::newRow("4x4 bottom intersection")
        << 4 << 4
        << 0 << 2 << 1 << 3
        << 0 << 0
        << 0 << 2 << 0 << 3;

    QTest::newRow("4x4 middle intersection")
        << 4 << 4
        << 0 << 2 << 2 << 3
        << 1 << 1
        << 1 << 2 << 1 << 3;

    QTest::newRow("4x4 full inclusion")
        << 4 << 4
        << 0 << 2 << 1 << 3
        << 0 << 1
        << 0 << 2 << 1 << 3;
}
class QTBUG48402_helper : public QObject
{
    Q_OBJECT
public:
    QModelIndex tl;
    QModelIndex br;
public slots:
    void changed(const QItemSelection &, const QItemSelection &deselected)
    {
        tl = deselected.first().topLeft();
        br = deselected.first().bottomRight();
    }
};

void tst_QItemSelectionModel::QTBUG48402()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(int, selectTop);
    QFETCH(int, selectLeft);
    QFETCH(int, selectBottom);
    QFETCH(int, selectRight);
    QFETCH(int, removeTop);
    QFETCH(int, removeBottom);
    QFETCH(int, deselectTop);
    QFETCH(int, deselectLeft);
    QFETCH(int, deselectBottom);
    QFETCH(int, deselectRight);

    MyStandardItemModel model(rows, columns);
    QItemSelectionModel selections(&model);

    QModelIndex stl = model.index(selectTop, selectLeft);
    QModelIndex sbr = model.index(selectBottom, selectRight);
    QModelIndex dtl = model.index(deselectTop, deselectLeft);
    QModelIndex dbr = model.index(deselectBottom, deselectRight);

    selections.select(QItemSelection(stl, sbr), QItemSelectionModel::ClearAndSelect);
    QTBUG48402_helper helper;
    helper.connect(&selections, &QItemSelectionModel::selectionChanged, &helper, &QTBUG48402_helper::changed);
    QVERIFY(selections.isSelected(stl));
    QVERIFY(selections.isSelected(sbr));
    QVERIFY(selections.hasSelection());

    model.removeRows(removeTop, removeBottom - removeTop + 1);

    QCOMPARE(QItemSelectionRange(helper.tl, helper.br), QItemSelectionRange(dtl, dbr));
}

QTEST_MAIN(tst_QItemSelectionModel)
#include "tst_qitemselectionmodel.moc"
