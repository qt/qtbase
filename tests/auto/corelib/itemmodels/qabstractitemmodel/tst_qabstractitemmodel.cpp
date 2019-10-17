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
#include <QtCore/QCoreApplication>

#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QStringListModel>
#include <QtGui/QStandardItemModel>

#include "dynamictreemodel.h"

/*!
    Note that this doesn't test models, but any functionality that QAbstractItemModel should provide
 */
class tst_QAbstractItemModel : public QObject
{
    Q_OBJECT

public slots:
    void init();
    void cleanup();

private slots:
    void index();
    void parent();
    void hasChildren();
    void data();
    void headerData();
    void itemData();
    void itemFlags();
    void match();
    void dropMimeData_data();
    void dropMimeData();
    void canDropMimeData();
    void changePersistentIndex();
    void movePersistentIndex();

    void insertRows();
    void insertColumns();
    void removeRows();
    void removeColumns();
    void moveRows();
    void moveColumns();

    void reset();

    void complexChangesWithPersistent();

    void testMoveSameParentUp_data();
    void testMoveSameParentUp();

    void testMoveSameParentDown_data();
    void testMoveSameParentDown();

    void testMoveToGrandParent_data();
    void testMoveToGrandParent();

    void testMoveToSibling_data();
    void testMoveToSibling();

    void testMoveToUncle_data();
    void testMoveToUncle();

    void testMoveToDescendants();

    void testMoveWithinOwnRange_data();
    void testMoveWithinOwnRange();

    void testMoveThroughProxy();

    void testReset();

    void testDataChanged();

    void testChildrenLayoutsChanged();

    void testRoleNames();
    void testDragActions();
    void dragActionsFallsBackToDropActions();

    void testFunctionPointerSignalConnection();

    void checkIndex();

private:
    DynamicTreeModel *m_model;
};

/*!
    Test model that impliments the pure vitual functions and anything else that is
    needed.

    It is a table implemented as a vector of vectors of strings.
 */
class QtTestModel: public QAbstractItemModel
{
public:
    QtTestModel(int rows, int columns, QObject *parent = 0);
    QtTestModel(const QVector<QVector<QString> > tbl, QObject *parent = 0);
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &) const;
    QVariant data(const QModelIndex &idx, int) const;
    bool setData(const QModelIndex &idx, const QVariant &value, int);
    bool insertRows(int row, int count, const QModelIndex &parent= QModelIndex());
    bool insertColumns(int column, int count, const QModelIndex &parent= QModelIndex());
    void setPersistent(const QModelIndex &from, const QModelIndex &to);
    bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );
    bool removeColumns( int column, int count, const QModelIndex & parent = QModelIndex());
    bool moveRows (const QModelIndex &sourceParent, int sourceRow, int count,
                   const QModelIndex &destinationParent, int destinationChild);
    bool moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count,
                     const QModelIndex &destinationParent, int destinationChild);
    void reset();

    bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int column, const QModelIndex &parent) const;

    int cCount, rCount;
    mutable bool wrongIndex;
    QVector<QVector<QString> > table;
};

Q_DECLARE_METATYPE(QAbstractItemModel::LayoutChangeHint);

QtTestModel::QtTestModel(int rows, int columns, QObject *parent)
    : QAbstractItemModel(parent), cCount(columns), rCount(rows), wrongIndex(false)
{
    table.resize(rows);
    for (int r = 0; r < rows; ++r) {
        const QString prefix = QString::number(r) + QLatin1Char('/');
        table[r].resize(columns);
        for (int c = 0; c < columns; ++c)
            table[r][c] = prefix + QString::number(c);
    }
}

QtTestModel::QtTestModel(const QVector<QVector<QString> > tbl, QObject *parent)
    : QAbstractItemModel(parent), wrongIndex(false)
{
    table = tbl;
    rCount = tbl.count();
    cCount = tbl.at(0).count();
}

QModelIndex QtTestModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

QModelIndex QtTestModel::parent(const QModelIndex &) const { return QModelIndex(); }
int QtTestModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : rCount; }
int QtTestModel::columnCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : cCount; }
bool QtTestModel::hasChildren(const QModelIndex &) const { return false; }

QVariant QtTestModel::data(const QModelIndex &idx, int) const
{
    if (idx.row() < 0 || idx.column() < 0 || idx.column() > cCount || idx.row() > rCount) {
        wrongIndex = true;
        qWarning("got invalid modelIndex %d/%d", idx.row(), idx.column());
        return QVariant();
    }
    return table.at(idx.row()).at(idx.column());
}

bool QtTestModel::setData(const QModelIndex &idx, const QVariant &value, int)
{
    table[idx.row()][idx.column()] = value.toString();
    return true;
}

bool QtTestModel::insertRows(int row, int count, const QModelIndex &parent)
{
    QAbstractItemModel::beginInsertRows(parent, row, row + count - 1);
    int cc = columnCount(parent);
    table.insert(row, count, QVector<QString>(cc));
    rCount = table.count();
    QAbstractItemModel::endInsertRows();
    return true;
}

bool QtTestModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    QAbstractItemModel::beginInsertColumns(parent, column, column + count - 1);
    int rc = rowCount(parent);
    for (int i = 0; i < rc; ++i)
        table[i].insert(column, 1, "");
    cCount = table.at(0).count();
    QAbstractItemModel::endInsertColumns();
    return true;
}

void QtTestModel::setPersistent(const QModelIndex &from, const QModelIndex &to)
{
    changePersistentIndex(from, to);
}

bool QtTestModel::removeRows( int row, int count, const QModelIndex & parent)
{
    QAbstractItemModel::beginRemoveRows(parent, row, row + count - 1);

    for (int r = row+count-1; r >= row; --r)
        table.remove(r);
    rCount = table.count();

    QAbstractItemModel::endRemoveRows();
    return true;
}

bool QtTestModel::removeColumns(int column, int count, const QModelIndex & parent)
{
    QAbstractItemModel::beginRemoveColumns(parent, column, column + count - 1);

    for (int c = column+count-1; c > column; --c)
        for (int r = 0; r < rCount; ++r)
            table[r].remove(c);

    cCount = table.at(0).count();

    QAbstractItemModel::endRemoveColumns();
    return true;
}

bool QtTestModel::moveRows(const QModelIndex &sourceParent, int src, int cnt,
                           const QModelIndex &destinationParent, int dst)
{
    if (!QAbstractItemModel::beginMoveRows(sourceParent, src, src + cnt - 1,
                                           destinationParent, dst))
        return false;

    QVector<QString> buf;
    if (dst < src) {
        for (int i  = 0; i < cnt; ++i) {
            buf.swap(table[src + i]);
            table.remove(src + 1);
            table.insert(dst, buf);
        }
    } else if (src < dst) {
        for (int i  = 0; i < cnt; ++i) {
            buf.swap(table[src]);
            table.remove(src);
            table.insert(dst + i, buf);
        }
    }

    rCount = table.count();

    QAbstractItemModel::endMoveRows();
    return true;
}

bool QtTestModel::moveColumns(const QModelIndex &sourceParent, int src, int cnt,
                              const QModelIndex &destinationParent, int dst)
{
    if (!QAbstractItemModel::beginMoveColumns(sourceParent, src, src + cnt - 1,
                                              destinationParent, dst))
        return false;

    for (int r = 0; r < rCount; ++r) {
        QString buf;
        if (dst < src) {
            for (int i  = 0; i < cnt; ++i) {
                buf = table[r][src + i];
                table[r].remove(src + 1);
                table[r].insert(dst, buf);
            }
        } else if (src < dst) {
            for (int i  = 0; i < cnt; ++i) {
                buf = table[r][src];
                table[r].remove(src);
                table[r].insert(dst + i, buf);
            }
        }
    }

    cCount = table.at(0).count();

    QAbstractItemModel::endMoveColumns();
    return true;
}

void QtTestModel::reset()
{
    QAbstractItemModel::beginResetModel();
    QAbstractItemModel::endResetModel();
}

bool QtTestModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(data);
    Q_UNUSED(action);

    // For testing purposes, we impose some arbitrary rules on what may be dropped.
    if (!parent.isValid() && row < 0 && column < 0) {
        // a drop in emtpy space in the view is allowed.
        // For example, in a filesystem view, a file may be dropped into empty space
        // if it represents a writable directory.
        return true;
    }

    // We then arbitrarily decide to only allow drops on odd rows.
    // A filesystem view/model might be able to drop onto (writable) directories.
    return row % 2 == 0;
}

void tst_QAbstractItemModel::init()
{
    m_model = new DynamicTreeModel(this);

    ModelInsertCommand *insertCommand = new ModelInsertCommand(m_model, this);
    insertCommand->setNumCols(4);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();

    insertCommand = new ModelInsertCommand(m_model, this);
    insertCommand->setAncestorRowNumbers(QList<int>() << 5);
    insertCommand->setNumCols(4);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();

    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>();
}

void tst_QAbstractItemModel::cleanup()
{
    delete m_model;
}

/*
  tests
*/

void tst_QAbstractItemModel::index()
{
    QtTestModel model(1, 1);
    QModelIndex idx = model.index(0, 0, QModelIndex());
    QVERIFY(idx.isValid());
}

void tst_QAbstractItemModel::parent()
{
    QtTestModel model(1, 1);
    QModelIndex idx = model.index(0, 0, QModelIndex());
    QModelIndex par = model.parent(idx);
    QVERIFY(!par.isValid());
}

void tst_QAbstractItemModel::hasChildren()
{
    QtTestModel model(1, 1);
    QModelIndex idx = model.index(0, 0, QModelIndex());
    QVERIFY(!model.hasChildren(idx));
}

void tst_QAbstractItemModel::data()
{
    QtTestModel model(1, 1);
    QModelIndex idx = model.index(0, 0, QModelIndex());
    QVERIFY(idx.isValid());
    QCOMPARE(model.data(idx, Qt::DisplayRole).toString(), QString("0/0"));

    // Default does nothing
    QCOMPARE(model.setHeaderData(0, Qt::Horizontal, QVariant(0), 0), false);
}

void tst_QAbstractItemModel::headerData()
{
    QtTestModel model(1, 1);
    QCOMPARE(model.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
            QString("1"));

    // Default text alignment for header must be invalid
    QVERIFY( !model.headerData(0, Qt::Horizontal, Qt::TextAlignmentRole).isValid() );
}

void tst_QAbstractItemModel::itemData()
{
    QtTestModel model(1, 1);
    QModelIndex idx = model.index(0, 0, QModelIndex());
    QVERIFY(idx.isValid());
    QMap<int, QVariant> dat = model.itemData(idx);
    QCOMPARE(dat.count(Qt::DisplayRole), 1);
    QCOMPARE(dat.value(Qt::DisplayRole).toString(), QString("0/0"));
}

void tst_QAbstractItemModel::itemFlags()
{
    QtTestModel model(1, 1);
    QModelIndex idx = model.index(0, 0, QModelIndex());
    QVERIFY(idx.isValid());
    Qt::ItemFlags flags = model.flags(idx);
    QCOMPARE(Qt::ItemIsSelectable|Qt::ItemIsEnabled, flags);
}

void tst_QAbstractItemModel::match()
{
    QtTestModel model(4, 1);
    QModelIndex start = model.index(0, 0, QModelIndex());
    QVERIFY(start.isValid());
    QModelIndexList res = model.match(start, Qt::DisplayRole, QVariant("1"), 3);
    QCOMPARE(res.count(), 1);
    QModelIndex idx = model.index(1, 0, QModelIndex());
    bool areEqual = (idx == res.first());
    QVERIFY(areEqual);

    model.setData(model.index(0, 0, QModelIndex()), "bat", Qt::DisplayRole);
    model.setData(model.index(1, 0, QModelIndex()), "cat", Qt::DisplayRole);
    model.setData(model.index(2, 0, QModelIndex()), "dog", Qt::DisplayRole);
    model.setData(model.index(3, 0, QModelIndex()), "boar", Qt::DisplayRole);

    res = model.match(start, Qt::DisplayRole, QVariant("dog"), -1, Qt::MatchExactly);
    QCOMPARE(res.count(), 1);
    res = model.match(start, Qt::DisplayRole, QVariant("a"), -1, Qt::MatchContains);
    QCOMPARE(res.count(), 3);
    res = model.match(start, Qt::DisplayRole, QVariant("b"), -1, Qt::MatchStartsWith);
    QCOMPARE(res.count(), 2);
    res = model.match(start, Qt::DisplayRole, QVariant("t"), -1, Qt::MatchEndsWith);
    QCOMPARE(res.count(), 2);
    res = model.match(start, Qt::DisplayRole, QVariant("*a*"), -1, Qt::MatchWildcard);
    QCOMPARE(res.count(), 3);
    res = model.match(start, Qt::DisplayRole, QVariant(".*O.*"), -1, Qt::MatchRegExp);
    QCOMPARE(res.count(), 2);
    res = model.match(start, Qt::DisplayRole, QVariant(".*O.*"), -1, Qt::MatchRegExp | Qt::MatchCaseSensitive);
    QCOMPARE(res.count(), 0);
    res = model.match(start, Qt::DisplayRole, QVariant("BOAR"), -1, Qt::MatchFixedString);
    QCOMPARE(res.count(), 1);
    res = model.match(start, Qt::DisplayRole, QVariant("bat"), -1,
                      Qt::MatchFixedString | Qt::MatchCaseSensitive);
    QCOMPARE(res.count(), 1);

    res = model.match(start, Qt::DisplayRole, QVariant(".*O.*"), -1,
                      Qt::MatchRegularExpression);
    QCOMPARE(res.count(), 2);
    res = model.match(start, Qt::DisplayRole, QVariant(".*O.*"), -1,
                      Qt::MatchRegularExpression | Qt::MatchCaseSensitive);
    QCOMPARE(res.count(), 0);

    res = model.match(start, Qt::DisplayRole, QVariant(QRegularExpression(".*O.*")),
                      -1, Qt::MatchRegularExpression);
    QCOMPARE(res.count(), 0);
    res = model.match(start,
                      Qt::DisplayRole,
                      QVariant(QRegularExpression(".*O.*",
                                                  QRegularExpression::CaseInsensitiveOption)),
                      -1,
                      Qt::MatchRegularExpression);
    QCOMPARE(res.count(), 2);

    // Ensure that the case sensitivity is properly ignored when passing a
    // QRegularExpression object.
    res = model.match(start,
                      Qt::DisplayRole,
                      QVariant(QRegularExpression(".*O.*",
                                                  QRegularExpression::CaseInsensitiveOption)),
                      -1,
                      Qt::MatchRegularExpression | Qt::MatchCaseSensitive);
    QCOMPARE(res.count(), 2);
}

typedef QPair<int, int> Position;
typedef QVector<QPair<int, int> > Selection;
typedef QVector<QVector<QString> > StringTable;
typedef QVector<QString> StringTableRow;

static StringTableRow qStringTableRow(const QString &s1, const QString &s2, const QString &s3)
{
    StringTableRow row;
    row << s1 << s2 << s3;
    return row;
}

#ifdef Q_CC_MSVC
# define STRINGTABLE (StringTable())
#else
# define STRINGTABLE StringTable()
#endif

void tst_QAbstractItemModel::dropMimeData_data()
{
    QTest::addColumn<StringTable>("src_table"); // drag source
    QTest::addColumn<StringTable>("dst_table"); // drop target
    QTest::addColumn<Selection>("selection"); // dragged items
    QTest::addColumn<Position>("dst_position"); // drop position
    QTest::addColumn<StringTable>("res_table"); // expected result

    {
        QTest::newRow("2x2 dropped at [0, 0]")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection
                << Position(0, 0) << Position(0, 1)
                << Position(1, 0) << Position(1, 1))
            << Position(0, 0) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("A", "B", "" ))
                << (qStringTableRow("D", "E", "" ))
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")));
    }

    {
        QTest::newRow("2x2 dropped at [1, 0]")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection
                << Position(0, 0) << Position(0, 1)
                << Position(1, 0) << Position(1, 1))
            << Position(1, 0) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("A", "B", "" ))
                << (qStringTableRow("D", "E", "" ))
                << (qStringTableRow("3", "4", "5")));
    }

    {
        QTest::newRow("2x2 dropped at [3, 0]")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection
                << Position(0, 0) << Position(0, 1)
                << Position(1, 0) << Position(1, 1))
            << Position(3, 0) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5"))
                << (qStringTableRow("A", "B", "" ))
                << (qStringTableRow("D", "E", "" )));
    }

    {
        QTest::newRow("2x2 dropped at [0, 1]")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection
                << Position(0, 0) << Position(0, 1)
                << Position(1, 0) << Position(1, 1))
            << Position(0, 1) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("" , "A", "B"))
                << (qStringTableRow("" , "D", "E"))
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")));
    }

    {
        QTest::newRow("2x2 dropped at [0, 2] (line break)")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection
                << Position(0, 0) << Position(0, 1)
                << Position(1, 0) << Position(1, 1))
            << Position(0, 2) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("" , "" , "A"))
                << (qStringTableRow("" , "" , "D"))
                << (qStringTableRow("" , "" , "B"))
                << (qStringTableRow("" , "" , "E"))
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")));
    }

    {
        QTest::newRow("2x2 dropped at [3, 2] (line break)")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection
                << Position(0, 0) << Position(0, 1)
                << Position(1, 0) << Position(1, 1))
            << Position(3, 2) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5"))
                << (qStringTableRow("" , "" , "A"))
                << (qStringTableRow("" , "" , "D"))
                << (qStringTableRow("" , "" , "B"))
                << (qStringTableRow("" , "" , "E")));
    }

    {
        QTest::newRow("non-square dropped at [0, 0]")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection
                << Position(0, 0) << Position(0, 1)
                << Position(1, 0))
            << Position(0, 0) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("A", "B", "" ))
                << (qStringTableRow("D", "" , "" ))
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")));
    }

    {
        QTest::newRow("non-square dropped at [0, 2]")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection
                << Position(0, 0) << Position(0, 1)
                << Position(1, 0))
            << Position(0, 2) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("" , "" , "A"))
                << (qStringTableRow("" , "" , "D"))
                << (qStringTableRow("" , "" , "B"))
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")));
    }

    {
        QTest::newRow("2x 1x2 dropped at [0, 0] (duplicates)")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection; 2x the same row (to simulate selections in hierarchy)
                << Position(0, 0) << Position(0, 1)
                << Position(0, 0) << Position(0, 1))
            << Position(0, 0) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("A", "B", "" ))
                << (qStringTableRow("A", "" , "" ))
                << (qStringTableRow("" , "B", "" )) // ### FIXME: strange behavior, but rare case
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")));
    }

    {
        QTest::newRow("2x 1x2 dropped at [3, 2] (duplicates)")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection; 2x the same row (to simulate selections in hierarchy)
                << Position(0, 0) << Position(0, 1)
                << Position(0, 0) << Position(0, 1))
            << Position(3, 2) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5"))
                << (qStringTableRow("" , "" , "A"))
                << (qStringTableRow("" , "" , "B"))
                << (qStringTableRow("" , "" , "A"))
                << (qStringTableRow("" , "" , "B")));
    }
    {
        QTest::newRow("2x 1x2 dropped at [3, 2] (different rows)")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F"))
                << (qStringTableRow("G", "H", "I")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection; 2x the same row (to simulate selections in hierarchy)
                << Position(0, 0) << Position(0, 1)
                << Position(2, 0) << Position(2, 1))
            << Position(2, 1) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5"))
                << (qStringTableRow("" , "A" , "B"))
                << (qStringTableRow("" , "G" , "H")));
    }

    {
        QTest::newRow("2x 1x2 dropped at [3, 2] (different rows, over the edge)")
            << (STRINGTABLE // source table
                << (qStringTableRow("A", "B", "C"))
                << (qStringTableRow("D", "E", "F"))
                << (qStringTableRow("G", "H", "I")))
            << (STRINGTABLE // destination table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5")))
            << (Selection() // selection; 2x the same row (to simulate selections in hierarchy)
                << Position(0, 0) << Position(0, 1)
                << Position(2, 0) << Position(2, 1))
            << Position(3, 2) // drop position
            << (STRINGTABLE // resulting table
                << (qStringTableRow("0", "1", "2"))
                << (qStringTableRow("3", "4", "5"))
                << (qStringTableRow("" , "" , "A"))
                << (qStringTableRow("" , "" , "G"))
                << (qStringTableRow("" , "" , "B"))
                << (qStringTableRow("" , "" , "H")));
    }
}

void tst_QAbstractItemModel::dropMimeData()
{
    QFETCH(StringTable, src_table);
    QFETCH(StringTable, dst_table);
    QFETCH(Selection, selection);
    QFETCH(Position, dst_position);
    QFETCH(StringTable, res_table);

    QtTestModel src(src_table);
    QtTestModel dst(dst_table);
    QtTestModel res(res_table);

    // get the mimeData from the "selected" indexes
    QModelIndexList selectedIndexes;
    for (int i = 0; i < selection.count(); ++i)
        selectedIndexes << src.index(selection.at(i).first, selection.at(i).second, QModelIndex());
    QMimeData *md = src.mimeData(selectedIndexes);
    // do the drop
    dst.dropMimeData(md, Qt::CopyAction, dst_position.first, dst_position.second, QModelIndex());
    delete md;

    // compare to the expected results
    QCOMPARE(dst.rowCount(QModelIndex()), res.rowCount(QModelIndex()));
    QCOMPARE(dst.columnCount(QModelIndex()), res.columnCount(QModelIndex()));
    for (int r = 0; r < dst.rowCount(QModelIndex()); ++r) {
        for (int c = 0; c < dst.columnCount(QModelIndex()); ++c) {
            QModelIndex dst_idx = dst.index(r, c, QModelIndex());
            QModelIndex res_idx = res.index(r, c, QModelIndex());
            QMap<int, QVariant> dst_data = dst.itemData(dst_idx);
            QMap<int, QVariant> res_data = res.itemData(res_idx);
            QCOMPARE(dst_data , res_data);
        }
    }
}

void tst_QAbstractItemModel::canDropMimeData()
{
    QtTestModel model(3, 3);

    QVERIFY(model.canDropMimeData(0, Qt::CopyAction, -1, -1, QModelIndex()));
    QVERIFY(model.canDropMimeData(0, Qt::CopyAction, 0, 0, QModelIndex()));
    QVERIFY(!model.canDropMimeData(0, Qt::CopyAction, 1, 0, QModelIndex()));
}

void tst_QAbstractItemModel::changePersistentIndex()
{
    QtTestModel model(3, 3);
    QModelIndex a = model.index(1, 2, QModelIndex());
    QModelIndex b = model.index(2, 1, QModelIndex());
    QPersistentModelIndex p(a);
    QVERIFY(p == a);
    model.setPersistent(a, b);
    QVERIFY(p == b);
}

void tst_QAbstractItemModel::movePersistentIndex()
{
    QtTestModel model(3, 3);

    QPersistentModelIndex a = model.index(1, 1);
    QVERIFY(a.isValid());
    QCOMPARE(a.row(), 1);
    QCOMPARE(a.column(), 1);

    model.insertRow(0);
    QCOMPARE(a.row(), 2);

    model.insertRow(1);
    QCOMPARE(a.row(), 3);

    model.insertColumn(0);
    QCOMPARE(a.column(), 2);
}

void tst_QAbstractItemModel::removeRows()
{
    QtTestModel model(10, 10);

    QSignalSpy rowsAboutToBeRemovedSpy(&model, &QtTestModel::rowsAboutToBeRemoved);
    QSignalSpy rowsRemovedSpy(&model, &QtTestModel::rowsRemoved);

    QVERIFY(rowsAboutToBeRemovedSpy.isValid());
    QVERIFY(rowsRemovedSpy.isValid());

    QCOMPARE(model.removeRows(6, 4), true);
    QCOMPARE(rowsAboutToBeRemovedSpy.count(), 1);
    QCOMPARE(rowsRemovedSpy.count(), 1);
}

void tst_QAbstractItemModel::removeColumns()
{
    QtTestModel model(10, 10);

    QSignalSpy columnsAboutToBeRemovedSpy(&model, &QtTestModel::columnsAboutToBeRemoved);
    QSignalSpy columnsRemovedSpy(&model, &QtTestModel::columnsRemoved);

    QVERIFY(columnsAboutToBeRemovedSpy.isValid());
    QVERIFY(columnsRemovedSpy.isValid());

    QCOMPARE(model.removeColumns(6, 4), true);
    QCOMPARE(columnsAboutToBeRemovedSpy.count(), 1);
    QCOMPARE(columnsRemovedSpy.count(), 1);
}

void tst_QAbstractItemModel::insertRows()
{
    QtTestModel model(10, 10);

    QSignalSpy rowsAboutToBeInsertedSpy(&model, &QtTestModel::rowsAboutToBeInserted);
    QSignalSpy rowsInsertedSpy(&model, &QtTestModel::rowsInserted);

    QVERIFY(rowsAboutToBeInsertedSpy.isValid());
    QVERIFY(rowsInsertedSpy.isValid());

    QCOMPARE(model.insertRows(6, 4), true);
    QCOMPARE(rowsAboutToBeInsertedSpy.count(), 1);
    QCOMPARE(rowsInsertedSpy.count(), 1);
}

void tst_QAbstractItemModel::insertColumns()
{
    QtTestModel model(10, 10);

    QSignalSpy columnsAboutToBeInsertedSpy(&model, &QtTestModel::columnsAboutToBeInserted);
    QSignalSpy columnsInsertedSpy(&model, &QtTestModel::columnsInserted);

    QVERIFY(columnsAboutToBeInsertedSpy.isValid());
    QVERIFY(columnsInsertedSpy.isValid());

    QCOMPARE(model.insertColumns(6, 4), true);
    QCOMPARE(columnsAboutToBeInsertedSpy.count(), 1);
    QCOMPARE(columnsInsertedSpy.count(), 1);
}

void tst_QAbstractItemModel::moveRows()
{
    QtTestModel model(10, 10);

    QSignalSpy rowsAboutToBeMovedSpy(&model, &QtTestModel::rowsAboutToBeMoved);
    QSignalSpy rowsMovedSpy(&model, &QtTestModel::rowsMoved);

    QVERIFY(rowsAboutToBeMovedSpy.isValid());
    QVERIFY(rowsMovedSpy.isValid());

    QCOMPARE(model.moveRows(QModelIndex(), 6, 4, QModelIndex(), 1), true);
    QCOMPARE(rowsAboutToBeMovedSpy.count(), 1);
    QCOMPARE(rowsMovedSpy.count(), 1);
}

void tst_QAbstractItemModel::moveColumns()
{
    QtTestModel model(10, 10);

    QSignalSpy columnsAboutToBeMovedSpy(&model, &QtTestModel::columnsAboutToBeMoved);
    QSignalSpy columnsMovedSpy(&model, &QtTestModel::columnsMoved);

    QVERIFY(columnsAboutToBeMovedSpy.isValid());
    QVERIFY(columnsMovedSpy.isValid());

    QCOMPARE(model.moveColumns(QModelIndex(), 6, 4, QModelIndex(), 1), true);
    QCOMPARE(columnsAboutToBeMovedSpy.count(), 1);
    QCOMPARE(columnsMovedSpy.count(), 1);

    QCOMPARE(model.moveColumn(QModelIndex(), 4, QModelIndex(), 1), true);
    QCOMPARE(columnsAboutToBeMovedSpy.count(), 2);
    QCOMPARE(columnsMovedSpy.count(), 2);
}

void tst_QAbstractItemModel::reset()
{
    QtTestModel model(10, 10);

    QSignalSpy resetSpy(&model, &QtTestModel::modelReset);
    QVERIFY(resetSpy.isValid());
    model.reset();
    QCOMPARE(resetSpy.count(), 1);
}

void tst_QAbstractItemModel::complexChangesWithPersistent()
{
    QtTestModel model(10, 10);
    QPersistentModelIndex a = model.index(1, 1, QModelIndex());
    QPersistentModelIndex b = model.index(9, 7, QModelIndex());
    QPersistentModelIndex c = model.index(5, 6, QModelIndex());
    QPersistentModelIndex d = model.index(3, 9, QModelIndex());
    QPersistentModelIndex e[10];
    for (int i=0; i <10 ; i++) {
        e[i] = model.index(2, i , QModelIndex());
    }

    QVERIFY(a == model.index(1, 1, QModelIndex()));
    QVERIFY(b == model.index(9, 7, QModelIndex()));
    QVERIFY(c == model.index(5, 6, QModelIndex()));
    QVERIFY(d == model.index(3, 9, QModelIndex()));
    for (int i=0; i <8 ; i++)
        QVERIFY(e[i] == model.index(2, i , QModelIndex()));

    //remove a bunch of columns
    model.removeColumns(2, 4);

    QVERIFY(a == model.index(1, 1, QModelIndex()));
    QVERIFY(b == model.index(9, 3, QModelIndex()));
    QVERIFY(c == model.index(5, 2, QModelIndex()));
    QVERIFY(d == model.index(3, 5, QModelIndex()));
    for (int i=0; i <2 ; i++)
        QVERIFY(e[i] == model.index(2, i , QModelIndex()));
    for (int i=2; i <6 ; i++)
        QVERIFY(!e[i].isValid());
    for (int i=6; i <10 ; i++)
        QVERIFY(e[i] == model.index(2, i-4 , QModelIndex()));

    //move some indexes around
    model.setPersistent(model.index(1, 1 , QModelIndex()), model.index(9, 3 , QModelIndex()));
    model.setPersistent(model.index(9, 3 , QModelIndex()), model.index(8, 4 , QModelIndex()));

    QVERIFY(a == model.index(9, 3, QModelIndex()));
    QVERIFY(b == model.index(8, 4, QModelIndex()));
    QVERIFY(c == model.index(5, 2, QModelIndex()));
    QVERIFY(d == model.index(3, 5, QModelIndex()));
    for (int i=0; i <2 ; i++)
        QVERIFY(e[i] == model.index(2, i , QModelIndex()));
    for (int i=2; i <6 ; i++)
        QVERIFY(!e[i].isValid());
    for (int i=6; i <10 ; i++)
        QVERIFY(e[i] == model.index(2, i-4 , QModelIndex()));

    //inserting a bunch of columns
    model.insertColumns(2, 2);
    QVERIFY(a == model.index(9, 5, QModelIndex()));
    QVERIFY(b == model.index(8, 6, QModelIndex()));
    QVERIFY(c == model.index(5, 4, QModelIndex()));
    QVERIFY(d == model.index(3, 7, QModelIndex()));
    for (int i=0; i <2 ; i++)
        QVERIFY(e[i] == model.index(2, i , QModelIndex()));
    for (int i=2; i <6 ; i++)
        QVERIFY(!e[i].isValid());
    for (int i=6; i <10 ; i++)
        QVERIFY(e[i] == model.index(2, i-2 , QModelIndex()));
}

void tst_QAbstractItemModel::testMoveSameParentDown_data()
{
    QTest::addColumn<int>("startRow");
    QTest::addColumn<int>("endRow");
    QTest::addColumn<int>("destRow");
    // We can't put the actual parent index for the move in here because m_model is not defined until init() is run.
    QTest::addColumn<bool>("topLevel");

    // Move from the start to the middle
    QTest::newRow("move01") << 0 << 2 << 8 << true;
    // Move from the start to the end
    QTest::newRow("move02") << 0 << 2 << 10 << true;
    // Move from the middle to the middle
    QTest::newRow("move03") << 3 << 5 << 8 << true;
    // Move from the middle to the end
    QTest::newRow("move04") << 3 << 5 << 10 << true;

    QTest::newRow("move05") << 0 << 2 << 8 << false;
    QTest::newRow("move06") << 0 << 2 << 10 << false;
    QTest::newRow("move07") << 3 << 5 << 8 << false;
    QTest::newRow("move08") << 3 << 5 << 10 << false;
}

void tst_QAbstractItemModel::testMoveSameParentDown()
{
    QFETCH(int, startRow);
    QFETCH(int, endRow);
    QFETCH(int, destRow);
    QFETCH(bool, topLevel);

    QModelIndex moveParent = topLevel ? QModelIndex() : m_model->index(5, 0);

    QList<QPersistentModelIndex> persistentList;
    QModelIndexList indexList;

    for (int column = 0; column < m_model->columnCount(); ++column) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            QModelIndex idx = m_model->index(row, column);
            QVERIFY(idx.isValid());
            indexList << idx;
            persistentList << QPersistentModelIndex(idx);
        }
    }

    QModelIndex parent = m_model->index(5, 0);
    for (int column = 0; column < m_model->columnCount(); ++column) {
        for (int row = 0; row < m_model->rowCount(parent); ++row) {
            QModelIndex idx = m_model->index(row, column, parent);
            QVERIFY(idx.isValid());
            indexList << idx;
            persistentList << QPersistentModelIndex(idx);
        }
    }

    QSignalSpy beforeSpy(m_model, &DynamicTreeModel::rowsAboutToBeMoved);
    QSignalSpy afterSpy(m_model, &DynamicTreeModel::rowsMoved);

    QVERIFY(beforeSpy.isValid());
    QVERIFY(afterSpy.isValid());

    ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
    moveCommand->setNumCols(4);
    if (!topLevel)
        moveCommand->setAncestorRowNumbers(QList<int>() << 5);
    moveCommand->setStartRow(startRow);
    moveCommand->setEndRow(endRow);
    moveCommand->setDestRow(destRow);
    if (!topLevel)
        moveCommand->setDestAncestors(QList<int>() << 5);
    moveCommand->doCommand();

    QVariantList beforeSignal = beforeSpy.takeAt(0);
    QVariantList afterSignal = afterSpy.takeAt(0);

    QCOMPARE(beforeSignal.size(), 5);
    QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), moveParent);
    QCOMPARE(beforeSignal.at(1).toInt(), startRow);
    QCOMPARE(beforeSignal.at(2).toInt(), endRow);
    QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), moveParent);
    QCOMPARE(beforeSignal.at(4).toInt(), destRow);

    QCOMPARE(afterSignal.size(), 5);
    QCOMPARE(afterSignal.at(0).value<QModelIndex>(), moveParent);
    QCOMPARE(afterSignal.at(1).toInt(), startRow);
    QCOMPARE(afterSignal.at(2).toInt(), endRow);
    QCOMPARE(afterSignal.at(3).value<QModelIndex>(), moveParent);
    QCOMPARE(afterSignal.at(4).toInt(), destRow);

    for (int i = 0; i < indexList.size(); i++) {
        QModelIndex idx = indexList.at(i);
        QModelIndex persistentIndex = persistentList.at(i);
        if (idx.parent() == moveParent) {
            int row = idx.row();
            if ( row >= startRow) {
                if (row <= endRow) {
                    QCOMPARE(row + destRow - endRow - 1, persistentIndex.row());
                    QCOMPARE(idx.column(), persistentIndex.column());
                    QCOMPARE(idx.parent(), persistentIndex.parent());
                    QCOMPARE(idx.model(), persistentIndex.model());
                } else if (row < destRow) {
                    QCOMPARE(row - (endRow - startRow + 1), persistentIndex.row());
                    QCOMPARE(idx.column(), persistentIndex.column());
                    QCOMPARE(idx.parent(), persistentIndex.parent());
                    QCOMPARE(idx.model(), persistentIndex.model());
                } else {
                    QCOMPARE(idx, persistentIndex);
                }
            } else {
                QCOMPARE(idx, persistentIndex);
            }
        } else {
            QCOMPARE(idx, persistentIndex);
        }
    }
}

void tst_QAbstractItemModel::testMoveSameParentUp_data()
{
    QTest::addColumn<int>("startRow");
    QTest::addColumn<int>("endRow");
    QTest::addColumn<int>("destRow");
    QTest::addColumn<bool>("topLevel");

    // Move from the middle to the start
    QTest::newRow("move01") << 5 << 7 << 0 << true;
    // Move from the end to the start
    QTest::newRow("move02") << 8 << 9 << 0 << true;
    // Move from the middle to the middle
    QTest::newRow("move03") << 5 << 7 << 2 << true;
    // Move from the end to the middle
    QTest::newRow("move04") << 8 << 9 << 5 << true;

    QTest::newRow("move05") << 5 << 7 << 0 << false;
    QTest::newRow("move06") << 8 << 9 << 0 << false;
    QTest::newRow("move07") << 5 << 7 << 2 << false;
    QTest::newRow("move08") << 8 << 9 << 5 << false;
}

void tst_QAbstractItemModel::testMoveSameParentUp()
{
    QFETCH(int, startRow);
    QFETCH(int, endRow);
    QFETCH(int, destRow);
    QFETCH(bool, topLevel);

    QModelIndex moveParent = topLevel ? QModelIndex() : m_model->index(5, 0);

    QList<QPersistentModelIndex> persistentList;
    QModelIndexList indexList;

    for (int column = 0; column < m_model->columnCount(); ++column) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            QModelIndex idx = m_model->index(row, column);
            QVERIFY(idx.isValid());
            indexList << idx;
            persistentList << QPersistentModelIndex(idx);
        }
    }

    QModelIndex parent = m_model->index(2, 0);
    for (int column = 0; column < m_model->columnCount(); ++column) {
        for (int row = 0; row < m_model->rowCount(parent); ++row) {
            QModelIndex idx = m_model->index(row, column, parent);
            QVERIFY(idx.isValid());
            indexList << idx;
            persistentList << QPersistentModelIndex(idx);
        }
    }

    QSignalSpy beforeSpy(m_model, &DynamicTreeModel::rowsAboutToBeMoved);
    QSignalSpy afterSpy(m_model, &DynamicTreeModel::rowsMoved);

    QVERIFY(beforeSpy.isValid());
    QVERIFY(afterSpy.isValid());

    ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
    moveCommand->setNumCols(4);
    if (!topLevel)
        moveCommand->setAncestorRowNumbers(QList<int>() << 5);
    moveCommand->setStartRow(startRow);
    moveCommand->setEndRow(endRow);
    moveCommand->setDestRow(destRow);
    if (!topLevel)
        moveCommand->setDestAncestors(QList<int>() << 5);
    moveCommand->doCommand();

    QVariantList beforeSignal = beforeSpy.takeAt(0);
    QVariantList afterSignal = afterSpy.takeAt(0);

    QCOMPARE(beforeSignal.size(), 5);
    QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), moveParent);
    QCOMPARE(beforeSignal.at(1).toInt(), startRow);
    QCOMPARE(beforeSignal.at(2).toInt(), endRow);
    QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), moveParent);
    QCOMPARE(beforeSignal.at(4).toInt(), destRow);

    QCOMPARE(afterSignal.size(), 5);
    QCOMPARE(afterSignal.at(0).value<QModelIndex>(), moveParent);
    QCOMPARE(afterSignal.at(1).toInt(), startRow);
    QCOMPARE(afterSignal.at(2).toInt(), endRow);
    QCOMPARE(afterSignal.at(3).value<QModelIndex>(), moveParent);
    QCOMPARE(afterSignal.at(4).toInt(), destRow);

    for (int i = 0; i < indexList.size(); i++) {
        QModelIndex idx = indexList.at(i);
        QModelIndex persistentIndex = persistentList.at(i);
        if (idx.parent() == moveParent) {
            int row = idx.row();
            if ( row >= destRow) {
                if (row < startRow) {
                    QCOMPARE(row + endRow - startRow + 1, persistentIndex.row());
                    QCOMPARE(idx.column(), persistentIndex.column());
                    QCOMPARE(idx.parent(), persistentIndex.parent());
                    QCOMPARE(idx.model(), persistentIndex.model());
                } else if (row <= endRow) {
                    QCOMPARE(row + destRow - startRow, persistentIndex.row());
                    QCOMPARE(idx.column(), persistentIndex.column());
                    QCOMPARE(idx.parent(), persistentIndex.parent());
                    QCOMPARE(idx.model(), persistentIndex.model());
                } else {
                    QCOMPARE(idx, persistentIndex);
                }
            } else {
                QCOMPARE(idx, persistentIndex);
            }
        } else {
            QCOMPARE(idx, persistentIndex);
        }
    }
}

void tst_QAbstractItemModel::testMoveThroughProxy()
{
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_model);

    QList<QPersistentModelIndex> persistentList;

    persistentList.append(proxy->index(0, 0));
    persistentList.append(proxy->index(0, 0, proxy->mapFromSource(m_model->index(5, 0))));

    ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
    moveCommand->setNumCols(4);
    moveCommand->setAncestorRowNumbers(QList<int>() << 5);
    moveCommand->setStartRow(0);
    moveCommand->setEndRow(0);
    moveCommand->setDestRow(0);
    moveCommand->doCommand();
}

void tst_QAbstractItemModel::testMoveToGrandParent_data()
{
    QTest::addColumn<int>("startRow");
    QTest::addColumn<int>("endRow");
    QTest::addColumn<int>("destRow");

    // Move from the start to the middle
    QTest::newRow("move01") << 0 << 2 << 8;
    // Move from the start to the end
    QTest::newRow("move02") << 0 << 2 << 10;
    // Move from the middle to the middle
    QTest::newRow("move03") << 3 << 5 << 8;
    // Move from the middle to the end
    QTest::newRow("move04") << 3 << 5 << 10;

    // Move from the middle to the start
    QTest::newRow("move05") << 5 << 7 << 0;
    // Move from the end to the start
    QTest::newRow("move06") << 8 << 9 << 0;
    // Move from the middle to the middle
    QTest::newRow("move07") << 5 << 7 << 2;
    // Move from the end to the middle
    QTest::newRow("move08") << 8 << 9 << 5;

    // Moving to the same row in a different parent doesn't confuse things.
    QTest::newRow("move09") << 8 << 8 << 8;

    // Moving to the row of my parent and its neighbours doesn't confuse things
    QTest::newRow("move10") << 8 << 8 << 4;
    QTest::newRow("move11") << 8 << 8 << 5;
    QTest::newRow("move12") << 8 << 8 << 6;

    // Moving everything from one parent to another
    QTest::newRow("move13") << 0 << 9 << 10;
    QTest::newRow("move14") << 0 << 9 << 0;
}

void tst_QAbstractItemModel::testMoveToGrandParent()
{
    QFETCH(int, startRow);
    QFETCH(int, endRow);
    QFETCH(int, destRow);

    QList<QPersistentModelIndex> persistentList;
    QModelIndexList indexList;
    QModelIndexList parentsList;

    for (int column = 0; column < m_model->columnCount(); ++column) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            QModelIndex idx = m_model->index(row, column);
            QVERIFY(idx.isValid());
            indexList << idx;
            parentsList << idx.parent();
            persistentList << QPersistentModelIndex(idx);
        }
    }

    QModelIndex sourceIndex = m_model->index(5, 0);
    for (int column = 0; column < m_model->columnCount(); ++column) {
        for (int row = 0; row < m_model->rowCount(sourceIndex); ++row) {
            QModelIndex idx = m_model->index(row, column, sourceIndex);
            QVERIFY(idx.isValid());
            indexList << idx;
            parentsList << idx.parent();
            persistentList << QPersistentModelIndex(idx);
        }
    }

    QSignalSpy beforeSpy(m_model, &DynamicTreeModel::rowsAboutToBeMoved);
    QSignalSpy afterSpy(m_model, &DynamicTreeModel::rowsMoved);

    QVERIFY(beforeSpy.isValid());
    QVERIFY(afterSpy.isValid());

    QPersistentModelIndex persistentSource = sourceIndex;

    ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
    moveCommand->setAncestorRowNumbers(QList<int>() << 5);
    moveCommand->setNumCols(4);
    moveCommand->setStartRow(startRow);
    moveCommand->setEndRow(endRow);
    moveCommand->setDestRow(destRow);
    moveCommand->doCommand();

    QVariantList beforeSignal = beforeSpy.takeAt(0);
    QVariantList afterSignal = afterSpy.takeAt(0);

    QCOMPARE(beforeSignal.size(), 5);
    QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), sourceIndex);
    QCOMPARE(beforeSignal.at(1).toInt(), startRow);
    QCOMPARE(beforeSignal.at(2).toInt(), endRow);
    QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), QModelIndex());
    QCOMPARE(beforeSignal.at(4).toInt(), destRow);

    QCOMPARE(afterSignal.size(), 5);
    QCOMPARE(afterSignal.at(0).value<QModelIndex>(), static_cast<QModelIndex>(persistentSource));
    QCOMPARE(afterSignal.at(1).toInt(), startRow);
    QCOMPARE(afterSignal.at(2).toInt(), endRow);
    QCOMPARE(afterSignal.at(3).value<QModelIndex>(), QModelIndex());
    QCOMPARE(afterSignal.at(4).toInt(), destRow);

    for (int i = 0; i < indexList.size(); i++) {
        QModelIndex idx = indexList.at(i);
        QModelIndex idxParent = parentsList.at(i);
        QModelIndex persistentIndex = persistentList.at(i);
        int row = idx.row();
        if (idxParent == QModelIndex()) {
            if (row >= destRow) {
                QCOMPARE(row + endRow - startRow + 1, persistentIndex.row());
                QCOMPARE(idx.column(), persistentIndex.column());
                QCOMPARE(idxParent, persistentIndex.parent());
                QCOMPARE(idx.model(), persistentIndex.model());
            } else {
                QCOMPARE(idx, persistentIndex);
            }
        } else {
            if (row < startRow) {
                QCOMPARE(idx, persistentIndex);
            } else if (row <= endRow) {
                QCOMPARE(row + destRow - startRow, persistentIndex.row());
                QCOMPARE(idx.column(), persistentIndex.column());
                QCOMPARE(QModelIndex(), persistentIndex.parent());
                QCOMPARE(idx.model(), persistentIndex.model());
            } else {
                QCOMPARE(row - (endRow - startRow + 1), persistentIndex.row());
                QCOMPARE(idx.column(), persistentIndex.column());

                if (idxParent.row() >= destRow) {
                    QModelIndex adjustedParent;
                    adjustedParent = idxParent.sibling(idxParent.row() + endRow - startRow + 1, idxParent.column());
                    QCOMPARE(adjustedParent, persistentIndex.parent());
                } else {
                    QCOMPARE(idxParent, persistentIndex.parent());
                }
                QCOMPARE(idx.model(), persistentIndex.model());
            }
        }
    }
}

void tst_QAbstractItemModel::testMoveToSibling_data()
{
    QTest::addColumn<int>("startRow");
    QTest::addColumn<int>("endRow");
    QTest::addColumn<int>("destRow");

    // Move from the start to the middle
    QTest::newRow("move01") << 0 << 2 << 8;
    // Move from the start to the end
    QTest::newRow("move02") << 0 << 2 << 10;
    // Move from the middle to the middle
    QTest::newRow("move03") << 2 << 4 << 8;
    // Move from the middle to the end
    QTest::newRow("move04") << 2 << 4 << 10;

    // Move from the middle to the start
    QTest::newRow("move05") << 8 << 8 << 0;
    // Move from the end to the start
    QTest::newRow("move06") << 8 << 9 << 0;
    // Move from the middle to the middle
    QTest::newRow("move07") << 6 << 8 << 2;
    // Move from the end to the middle
    QTest::newRow("move08") << 8 << 9 << 5;

    // Moving to the same row in a different parent doesn't confuse things.
    QTest::newRow("move09") << 8 << 8 << 8;

    // Moving to the row of my target and its neighbours doesn't confuse things
    QTest::newRow("move10") << 8 << 8 << 4;
    QTest::newRow("move11") << 8 << 8 << 5;
    QTest::newRow("move12") << 8 << 8 << 6;

    // Move such that the destination parent no longer valid after the move.
    // The destination parent is always QMI(5, 0), but after this move the
    // row count is 5, so (5, 0) (used internally in QAIM) no longer refers to a valid index.
    QTest::newRow("move13") << 0 << 4 << 0;
}

void tst_QAbstractItemModel::testMoveToSibling()
{
    QFETCH(int, startRow);
    QFETCH(int, endRow);
    QFETCH(int, destRow);

    QList<QPersistentModelIndex> persistentList;
    QModelIndexList indexList;
    QModelIndexList parentsList;

    const int column = 0;

    for (int i = 0; i < m_model->rowCount(); ++i) {
        QModelIndex idx = m_model->index(i, column);
        QVERIFY(idx.isValid());
        indexList << idx;
        parentsList << idx.parent();
        persistentList << QPersistentModelIndex(idx);
    }

    QModelIndex destIndex = m_model->index(5, 0);
    QModelIndex sourceIndex;
    for (int i = 0; i < m_model->rowCount(destIndex); ++i) {
        QModelIndex idx = m_model->index(i, column, destIndex);
        QVERIFY(idx.isValid());
        indexList << idx;
        parentsList << idx.parent();
        persistentList << QPersistentModelIndex(idx);
    }

    QSignalSpy beforeSpy(m_model, &DynamicTreeModel::rowsAboutToBeMoved);
    QSignalSpy afterSpy(m_model, &DynamicTreeModel::rowsMoved);

    QVERIFY(beforeSpy.isValid());
    QVERIFY(afterSpy.isValid());

    QPersistentModelIndex persistentDest = destIndex;

    ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
    moveCommand->setNumCols(4);
    moveCommand->setStartRow(startRow);
    moveCommand->setEndRow(endRow);
    moveCommand->setDestAncestors(QList<int>() << 5);
    moveCommand->setDestRow(destRow);
    moveCommand->doCommand();

    QVariantList beforeSignal = beforeSpy.takeAt(0);
    QVariantList afterSignal = afterSpy.takeAt(0);

    QCOMPARE(beforeSignal.size(), 5);
    QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), sourceIndex);
    QCOMPARE(beforeSignal.at(1).toInt(), startRow);
    QCOMPARE(beforeSignal.at(2).toInt(), endRow);
    QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), destIndex);
    QCOMPARE(beforeSignal.at(4).toInt(), destRow);

    QCOMPARE(afterSignal.size(), 5);
    QCOMPARE(afterSignal.at(0).value<QModelIndex>(), sourceIndex);
    QCOMPARE(afterSignal.at(1).toInt(), startRow);
    QCOMPARE(afterSignal.at(2).toInt(), endRow);
    QCOMPARE(afterSignal.at(3).value<QModelIndex>(), static_cast<QModelIndex>(persistentDest));
    QCOMPARE(afterSignal.at(4).toInt(), destRow);

    for (int i = 0; i < indexList.size(); i++) {
        QModelIndex idx = indexList.at(i);
        QModelIndex idxParent = parentsList.at(i);
        QModelIndex persistentIndex = persistentList.at(i);

        QModelIndex adjustedDestination = destIndex.sibling(destIndex.row() - (endRow - startRow + 1), destIndex.column());
        int row = idx.row();
        if (idxParent == destIndex) {
            if (row >= destRow) {
                QCOMPARE(row + endRow - startRow + 1, persistentIndex.row());
                QCOMPARE(idx.column(), persistentIndex.column());
                if (idxParent.row() > startRow) {
                    QCOMPARE(adjustedDestination, persistentIndex.parent());
                } else {
                    QCOMPARE(destIndex, persistentIndex.parent());
                }
                QCOMPARE(idx.model(), persistentIndex.model());
            } else {
                QCOMPARE(idx, persistentIndex);
            }
        } else {
            if (row < startRow) {
                QCOMPARE(idx, persistentIndex);
            } else if (row <= endRow) {
                QCOMPARE(row + destRow - startRow, persistentIndex.row());
                QCOMPARE(idx.column(), persistentIndex.column());
                if (destIndex.row() > startRow) {
                    QCOMPARE(adjustedDestination, persistentIndex.parent());
                } else {
                    QCOMPARE(destIndex, persistentIndex.parent());
                }

                QCOMPARE(idx.model(), persistentIndex.model());
            } else {
                QCOMPARE(row - (endRow - startRow + 1), persistentIndex.row());
                QCOMPARE(idx.column(), persistentIndex.column());
                QCOMPARE(idxParent, persistentIndex.parent());
                QCOMPARE(idx.model(), persistentIndex.model());
            }
        }
    }
}

void tst_QAbstractItemModel::testMoveToUncle_data()
{
    QTest::addColumn<int>("startRow");
    QTest::addColumn<int>("endRow");
    QTest::addColumn<int>("destRow");

    // Move from the start to the middle
    QTest::newRow("move01") << 0 << 2 << 8;
    // Move from the start to the end
    QTest::newRow("move02") << 0 << 2 << 10;
    // Move from the middle to the middle
    QTest::newRow("move03") << 3 << 5 << 8;
    // Move from the middle to the end
    QTest::newRow("move04") << 3 << 5 << 10;

    // Move from the middle to the start
    QTest::newRow("move05") << 5 << 7 << 0;
    // Move from the end to the start
    QTest::newRow("move06") << 8 << 9 << 0;
    // Move from the middle to the middle
    QTest::newRow("move07") << 5 << 7 << 2;
    // Move from the end to the middle
    QTest::newRow("move08") << 8 << 9 << 5;

    // Moving to the same row in a different parent doesn't confuse things.
    QTest::newRow("move09") << 8 << 8 << 8;

    // Moving to the row of my parent and its neighbours doesn't confuse things
    QTest::newRow("move10") << 8 << 8 << 4;
    QTest::newRow("move11") << 8 << 8 << 5;
    QTest::newRow("move12") << 8 << 8 << 6;

    // Moving everything from one parent to another
    QTest::newRow("move13") << 0 << 9 << 10;
}

void tst_QAbstractItemModel::testMoveToUncle()
{
    // Need to have some extra rows available.
    ModelInsertCommand *insertCommand = new ModelInsertCommand(m_model, this);
    insertCommand->setAncestorRowNumbers(QList<int>() << 9);
    insertCommand->setNumCols(4);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();

    QFETCH(int, startRow);
    QFETCH(int, endRow);
    QFETCH(int, destRow);

    QList<QPersistentModelIndex> persistentList;
    QModelIndexList indexList;
    QModelIndexList parentsList;

    const int column = 0;

    QModelIndex sourceIndex = m_model->index(9, 0);
    for (int i = 0; i < m_model->rowCount(sourceIndex); ++i) {
        QModelIndex idx = m_model->index(i, column, sourceIndex);
        QVERIFY(idx.isValid());
        indexList << idx;
        parentsList << idx.parent();
        persistentList << QPersistentModelIndex(idx);
    }

    QModelIndex destIndex = m_model->index(5, 0);
    for (int i = 0; i < m_model->rowCount(destIndex); ++i) {
        QModelIndex idx = m_model->index(i, column, destIndex);
        QVERIFY(idx.isValid());
        indexList << idx;
        parentsList << idx.parent();
        persistentList << QPersistentModelIndex(idx);
    }

    QSignalSpy beforeSpy(m_model, &DynamicTreeModel::rowsAboutToBeMoved);
    QSignalSpy afterSpy(m_model, &DynamicTreeModel::rowsMoved);

    QVERIFY(beforeSpy.isValid());
    QVERIFY(afterSpy.isValid());

    ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
    moveCommand->setAncestorRowNumbers(QList<int>() << 9);
    moveCommand->setNumCols(4);
    moveCommand->setStartRow(startRow);
    moveCommand->setEndRow(endRow);
    moveCommand->setDestAncestors(QList<int>() << 5);
    moveCommand->setDestRow(destRow);
    moveCommand->doCommand();

    QVariantList beforeSignal = beforeSpy.takeAt(0);
    QVariantList afterSignal = afterSpy.takeAt(0);

    QCOMPARE(beforeSignal.size(), 5);
    QCOMPARE(beforeSignal.at(0).value<QModelIndex>(), sourceIndex);
    QCOMPARE(beforeSignal.at(1).toInt(), startRow);
    QCOMPARE(beforeSignal.at(2).toInt(), endRow);
    QCOMPARE(beforeSignal.at(3).value<QModelIndex>(), destIndex);
    QCOMPARE(beforeSignal.at(4).toInt(), destRow);

    QCOMPARE(afterSignal.size(), 5);
    QCOMPARE(afterSignal.at(0).value<QModelIndex>(), sourceIndex);
    QCOMPARE(afterSignal.at(1).toInt(), startRow);
    QCOMPARE(afterSignal.at(2).toInt(), endRow);
    QCOMPARE(afterSignal.at(3).value<QModelIndex>(), destIndex);
    QCOMPARE(afterSignal.at(4).toInt(), destRow);

    for (int i = 0; i < indexList.size(); i++) {
        QModelIndex idx = indexList.at(i);
        QModelIndex idxParent = parentsList.at(i);
        QModelIndex persistentIndex = persistentList.at(i);

        int row = idx.row();
        if (idxParent == destIndex) {
            if (row >= destRow) {
                QCOMPARE(row + endRow - startRow + 1, persistentIndex.row());
                QCOMPARE(idx.column(), persistentIndex.column());
                QCOMPARE(destIndex, persistentIndex.parent());
                QCOMPARE(idx.model(), persistentIndex.model());
            } else {
                QCOMPARE(idx, persistentIndex);
            }
        } else {
            if (row < startRow) {
                QCOMPARE(idx, persistentIndex);
            } else if (row <= endRow) {
                QCOMPARE(row + destRow - startRow, persistentIndex.row());
                QCOMPARE(idx.column(), persistentIndex.column());
                QCOMPARE(destIndex, persistentIndex.parent());
                QCOMPARE(idx.model(), persistentIndex.model());
            } else {
                QCOMPARE(row - (endRow - startRow + 1), persistentIndex.row());
                QCOMPARE(idx.column(), persistentIndex.column());
                QCOMPARE(idxParent, persistentIndex.parent());
                QCOMPARE(idx.model(), persistentIndex.model());
            }
        }
    }
}

void tst_QAbstractItemModel::testMoveToDescendants()
{
    // Attempt to move a row to its ancestors depth rows deep.
    const int depth = 6;

    // Need to have some extra rows available in a tree.
    QList<int> rows;
    ModelInsertCommand *insertCommand;
    for (int i = 0; i < depth; i++) {
        insertCommand = new ModelInsertCommand(m_model, this);
        insertCommand->setAncestorRowNumbers(rows);
        insertCommand->setNumCols(4);
        insertCommand->setStartRow(0);
        insertCommand->setEndRow(9);
        insertCommand->doCommand();
        rows << 9;
    }

    QList<QPersistentModelIndex> persistentList;
    QModelIndexList indexList;
    QModelIndexList parentsList;

    const int column = 0;

    QModelIndex sourceIndex = m_model->index(9, 0);
    for (int i = 0; i < m_model->rowCount(sourceIndex); ++i) {
        QModelIndex idx = m_model->index(i, column, sourceIndex);
        QVERIFY(idx.isValid());
        indexList << idx;
        parentsList << idx.parent();
        persistentList << QPersistentModelIndex(idx);
    }

    QModelIndex destIndex = m_model->index(5, 0);
    for (int i = 0; i < m_model->rowCount(destIndex); ++i) {
        QModelIndex idx = m_model->index(i, column, destIndex);
        QVERIFY(idx.isValid());
        indexList << idx;
        parentsList << idx.parent();
        persistentList << QPersistentModelIndex(idx);
    }

    QSignalSpy beforeSpy(m_model, &DynamicTreeModel::rowsAboutToBeMoved);
    QSignalSpy afterSpy(m_model, &DynamicTreeModel::rowsMoved);

    QVERIFY(beforeSpy.isValid());
    QVERIFY(afterSpy.isValid());

    ModelMoveCommand *moveCommand;
    QList<int> ancestors;
    while (ancestors.size() < depth) {
        ancestors << 9;
        for (int row = 0; row <= 9; row++) {
            moveCommand = new ModelMoveCommand(m_model, this);
            moveCommand->setNumCols(4);
            moveCommand->setStartRow(9);
            moveCommand->setEndRow(9);
            moveCommand->setDestAncestors(ancestors);
            moveCommand->setDestRow(row);
            moveCommand->doCommand();

            QCOMPARE(beforeSpy.size(), 0);
            QCOMPARE(afterSpy.size(), 0);
        }
    }
}

void tst_QAbstractItemModel::testMoveWithinOwnRange_data()
{
    QTest::addColumn<int>("startRow");
    QTest::addColumn<int>("endRow");
    QTest::addColumn<int>("destRow");

    QTest::newRow("move01") << 0 << 0 << 0;
    QTest::newRow("move02") << 0 << 0 << 1;
    QTest::newRow("move03") << 0 << 5 << 0;
    QTest::newRow("move04") << 0 << 5 << 1;
    QTest::newRow("move05") << 0 << 5 << 2;
    QTest::newRow("move06") << 0 << 5 << 3;
    QTest::newRow("move07") << 0 << 5 << 4;
    QTest::newRow("move08") << 0 << 5 << 5;
    QTest::newRow("move09") << 0 << 5 << 6;
    QTest::newRow("move10") << 3 << 5 << 5;
    QTest::newRow("move11") << 3 << 5 << 6;
    QTest::newRow("move12") << 4 << 5 << 5;
    QTest::newRow("move13") << 4 << 5 << 6;
    QTest::newRow("move14") << 5 << 5 << 5;
    QTest::newRow("move15") << 5 << 5 << 6;
    QTest::newRow("move16") << 5 << 9 << 9;
    QTest::newRow("move17") << 5 << 9 << 10;
    QTest::newRow("move18") << 6 << 9 << 9;
    QTest::newRow("move19") << 6 << 9 << 10;
    QTest::newRow("move20") << 7 << 9 << 9;
    QTest::newRow("move21") << 7 << 9 << 10;
    QTest::newRow("move22") << 8 << 9 << 9;
    QTest::newRow("move23") << 8 << 9 << 10;
    QTest::newRow("move24") << 9 << 9 << 9;
    QTest::newRow("move25") << 0 << 9 << 10;
}

void tst_QAbstractItemModel::testMoveWithinOwnRange()
{
    QFETCH(int, startRow);
    QFETCH(int, endRow);
    QFETCH(int, destRow);

    QSignalSpy beforeSpy(m_model, &DynamicTreeModel::rowsAboutToBeMoved);
    QSignalSpy afterSpy(m_model, &DynamicTreeModel::rowsMoved);

    QVERIFY(beforeSpy.isValid());
    QVERIFY(afterSpy.isValid());

    ModelMoveCommand *moveCommand = new ModelMoveCommand(m_model, this);
    moveCommand->setNumCols(4);
    moveCommand->setStartRow(startRow);
    moveCommand->setEndRow(endRow);
    moveCommand->setDestRow(destRow);
    moveCommand->doCommand();

    QCOMPARE(beforeSpy.size(), 0);
    QCOMPARE(afterSpy.size(), 0);
}

class ListenerObject : public QObject
{
    Q_OBJECT
public:
    ListenerObject(QAbstractProxyModel *parent);

protected:
    void fillIndexStores(const QModelIndex &parent);

public slots:
    void slotAboutToBeReset();
    void slotReset();

private:
    QAbstractProxyModel *m_model;
    QList<QPersistentModelIndex> m_persistentIndexes;
    QModelIndexList m_nonPersistentIndexes;
};


class ModelWithCustomRole : public QStringListModel
{
  Q_OBJECT
public:
    using QStringListModel::QStringListModel;

    QHash<int, QByteArray> roleNames() const override
    {
        return {{Qt::UserRole + 1, QByteArrayLiteral("custom")}};
    }
};

ListenerObject::ListenerObject(QAbstractProxyModel *parent)
    : QObject(parent), m_model(parent)
{
    connect(m_model, SIGNAL(modelAboutToBeReset()), SLOT(slotAboutToBeReset()));
    connect(m_model, SIGNAL(modelReset()), SLOT(slotReset()));

    fillIndexStores(QModelIndex());
}

void ListenerObject::fillIndexStores(const QModelIndex &parent)
{
    const int column = 0;
    int row = 0;
    QModelIndex idx = m_model->index(row, column, parent);
    while (idx.isValid()) {
        m_persistentIndexes << QPersistentModelIndex(idx);
        m_nonPersistentIndexes << idx;
        if (m_model->hasChildren(idx))
            fillIndexStores(idx);
        ++row;
        idx = m_model->index(row, column, parent);
    }
}

void ListenerObject::slotAboutToBeReset()
{
    // Nothing has been changed yet. All indexes should be the same.
    for (int i = 0; i < m_persistentIndexes.size(); ++i) {
        QModelIndex idx = m_persistentIndexes.at(i);
        QCOMPARE(idx, m_nonPersistentIndexes.at(i));
        QVERIFY(m_model->mapToSource(idx).isValid());
    }
}

void ListenerObject::slotReset()
{
    for (const auto &idx : qAsConst(m_persistentIndexes)) {
        QVERIFY(!idx.isValid());
    }
}

void tst_QAbstractItemModel::testReset()
{
    QSignalSpy beforeResetSpy(m_model, &DynamicTreeModel::modelAboutToBeReset);
    QSignalSpy afterResetSpy(m_model, &DynamicTreeModel::modelReset);

    QVERIFY(beforeResetSpy.isValid());
    QVERIFY(afterResetSpy.isValid());

    QSortFilterProxyModel *nullProxy = new QSortFilterProxyModel(this);
    nullProxy->setSourceModel(m_model);

    // Makes sure the model and proxy are in a consistent state. before and after reset.
    ListenerObject *listener = new ListenerObject(nullProxy);

    ModelResetCommandFixed *resetCommand = new ModelResetCommandFixed(m_model, this);

    resetCommand->setNumCols(4);
    resetCommand->setStartRow(0);
    resetCommand->setEndRow(0);
    resetCommand->setDestRow(0);
    resetCommand->setDestAncestors(QList<int>() << 5);
    resetCommand->doCommand();

    // Verify that the correct signals were emitted
    QCOMPARE(beforeResetSpy.size(), 1);
    QCOMPARE(afterResetSpy.size(), 1);

    // Verify that the move actually happened.
    QCOMPARE(m_model->rowCount(), 9);
    QModelIndex destIndex = m_model->index(4, 0);
    QCOMPARE(m_model->rowCount(destIndex), 11);

    // Delete it because its slots test things which are not true after this point.
    delete listener;

    QSignalSpy proxyBeforeResetSpy(nullProxy, &QSortFilterProxyModel::modelAboutToBeReset);
    QSignalSpy proxyAfterResetSpy(nullProxy, &QSortFilterProxyModel::modelReset);

    // Before setting it, it does not have custom roles.
    QCOMPARE(nullProxy->roleNames().value(Qt::UserRole + 1), QByteArray());

    nullProxy->setSourceModel(new ModelWithCustomRole(this));
    QCOMPARE(proxyBeforeResetSpy.size(), 1);
    QCOMPARE(proxyAfterResetSpy.size(), 1);

    QCOMPARE(nullProxy->roleNames().value(Qt::UserRole + 1), QByteArray("custom"));

    nullProxy->setSourceModel(m_model);
    QCOMPARE(proxyBeforeResetSpy.size(), 2);
    QCOMPARE(proxyAfterResetSpy.size(), 2);

    // After being reset the proxy must be queried again.
    QCOMPARE(nullProxy->roleNames().value(Qt::UserRole + 1), QByteArray());
}

class CustomRoleModel : public QStringListModel
{
    Q_OBJECT
    Q_ENUMS(Roles)
public:
    enum Roles {
        Custom1 = Qt::UserRole + 1,
        Custom2,
        UserRole
    };

    CustomRoleModel(QObject *parent = 0)
      : QStringListModel(QStringList() << "a" << "b" << "c", parent)
    {
    }

    void emitSignals()
    {
        const QModelIndex top = index(0, 0);
        const QModelIndex bottom = index(2, 0);

        emit dataChanged(top, bottom);
        emit dataChanged(top, bottom, QVector<int>() << Qt::ToolTipRole);
        emit dataChanged(top, bottom, QVector<int>() << Qt::ToolTipRole << Custom1);
    }
};

void tst_QAbstractItemModel::testDataChanged()
{
    CustomRoleModel model;

    QSignalSpy withRoles(&model, &CustomRoleModel::dataChanged);
    QSignalSpy withoutRoles(&model, &CustomRoleModel::dataChanged);

    QVERIFY(withRoles.isValid());
    QVERIFY(withoutRoles.isValid());

    model.emitSignals();

    QCOMPARE(withRoles.size(), withoutRoles.size());
    QCOMPARE(withRoles.size(), 3);

    const QVariantList secondEmission = withRoles.at(1);
    const QVariantList thirdEmission = withRoles.at(2);

    const QVector<int> secondRoles = secondEmission.at(2).value<QVector<int> >();
    const QVector<int> thirdRoles = thirdEmission.at(2).value<QVector<int> >();

    QCOMPARE(secondRoles.size(), 1);
    QVERIFY(secondRoles.contains(Qt::ToolTipRole));

    QCOMPARE(thirdRoles.size(), 2);
    QVERIFY(thirdRoles.contains(Qt::ToolTipRole));
    QVERIFY(thirdRoles.contains(CustomRoleModel::Custom1));
}

Q_DECLARE_METATYPE(QList<QPersistentModelIndex>)

class SignalArgumentChecker : public QObject
{
    Q_OBJECT
public:
    SignalArgumentChecker(const QModelIndex &p1, const QModelIndex &p2, QObject *parent = 0)
      : QObject(parent), m_p1(p1), m_p2(p2), m_p1Persistent(p1), m_p2Persistent(p2)
    {
      connect(p1.model(), SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>)), SLOT(layoutAboutToBeChanged(QList<QPersistentModelIndex>)));
      connect(p1.model(), SIGNAL(layoutChanged(QList<QPersistentModelIndex>)), SLOT(layoutChanged(QList<QPersistentModelIndex>)));
    }

private slots:
    void layoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents)
    {
        QCOMPARE(parents.size(), 2);
        QVERIFY(parents.first() != parents.at(1));
        QVERIFY(parents.contains(m_p1));
        QVERIFY(parents.contains(m_p2));
    }

    void layoutChanged(const QList<QPersistentModelIndex> &parents)
    {
        QCOMPARE(parents.size(), 2);
        QVERIFY(parents.first() != parents.at(1));
        QVERIFY(parents.contains(m_p1Persistent));
        QVERIFY(parents.contains(m_p2Persistent));
        QVERIFY(!parents.contains(m_p2)); // Has changed
    }

private:
    QModelIndex m_p1;
    QModelIndex m_p2;
    QPersistentModelIndex m_p1Persistent;
    QPersistentModelIndex m_p2Persistent;
};

void tst_QAbstractItemModel::testChildrenLayoutsChanged()
{
    DynamicTreeModel model;

    ModelInsertCommand *insertCommand = new ModelInsertCommand(&model, this);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();

    insertCommand = new ModelInsertCommand(&model, this);
    insertCommand->setAncestorRowNumbers(QList<int>() << 2);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();

    insertCommand = new ModelInsertCommand(&model, this);
    insertCommand->setAncestorRowNumbers(QList<int>() << 5);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();

    qRegisterMetaType<QList<QPersistentModelIndex> >();

    {
        const QModelIndex p1 = model.index(2, 0);
        const QModelIndex p2 = model.index(5, 0);

        const QPersistentModelIndex p1FirstPersistent = model.index(0, 0, p1);
        const QPersistentModelIndex p1LastPersistent = model.index(9, 0, p1);
        const QPersistentModelIndex p2FirstPersistent = model.index(0, 0, p2);
        const QPersistentModelIndex p2LastPersistent = model.index(9, 0, p2);

        QVERIFY(p1.isValid());
        QVERIFY(p2.isValid());

        QCOMPARE(model.rowCount(), 10);
        QCOMPARE(model.rowCount(p1), 10);
        QCOMPARE(model.rowCount(p2), 10);

        QSignalSpy beforeSpy(&model, &DynamicTreeModel::layoutAboutToBeChanged);
        QSignalSpy afterSpy(&model, &DynamicTreeModel::layoutChanged);

        QVERIFY(beforeSpy.isValid());
        QVERIFY(afterSpy.isValid());

        ModelChangeChildrenLayoutsCommand *changeCommand = new ModelChangeChildrenLayoutsCommand(&model, this);
        changeCommand->setAncestorRowNumbers(QList<int>() << 2);
        changeCommand->setSecondAncestorRowNumbers(QList<int>() << 5);
        changeCommand->doCommand();

        QCOMPARE(beforeSpy.size(), 1);
        QCOMPARE(afterSpy.size(), 1);

        const QVariantList beforeSignal = beforeSpy.first();
        const QVariantList afterSignal = afterSpy.first();
        QCOMPARE(beforeSignal.size(), 2);
        QCOMPARE(afterSignal.size(), 2);

        const QList<QPersistentModelIndex> beforeParents = beforeSignal.first().value<QList<QPersistentModelIndex> >();
        QCOMPARE(beforeParents.size(), 2);
        QVERIFY(beforeParents.first() != beforeParents.at(1));
        QVERIFY(beforeParents.contains(p1));
        QVERIFY(beforeParents.contains(p2));

        const QList<QPersistentModelIndex> afterParents = afterSignal.first().value<QList<QPersistentModelIndex> >();
        QCOMPARE(afterParents.size(), 2);
        QVERIFY(afterParents.first() != afterParents.at(1));
        QVERIFY(afterParents.contains(p1));
        QVERIFY(afterParents.contains(p2));

        // The first will be the last, and the lest will be the first.
        QCOMPARE(p1FirstPersistent.row(), 1);
        QCOMPARE(p1LastPersistent.row(), 0);
        QCOMPARE(p2FirstPersistent.row(), 9);
        QCOMPARE(p2LastPersistent.row(), 8);
    }

    insertCommand = new ModelInsertCommand(&model, this);
    insertCommand->setAncestorRowNumbers(QList<int>() << 5 << 4);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();

    delete insertCommand;

    // Even when p2 itself is moved around, signal emission remains correct for its children.
    {
        const QModelIndex p1 = model.index(5, 0);
        const QModelIndex p2 = model.index(4, 0, p1);

        QVERIFY(p1.isValid());
        QVERIFY(p2.isValid());

        QCOMPARE(model.rowCount(), 10);
        QCOMPARE(model.rowCount(p1), 10);
        QCOMPARE(model.rowCount(p2), 10);

        const QPersistentModelIndex p1Persistent = p1;
        const QPersistentModelIndex p2Persistent = p2;

        const QPersistentModelIndex p1FirstPersistent = model.index(0, 0, p1);
        const QPersistentModelIndex p1LastPersistent = model.index(9, 0, p1);
        const QPersistentModelIndex p2FirstPersistent = model.index(0, 0, p2);
        const QPersistentModelIndex p2LastPersistent = model.index(9, 0, p2);

        QSignalSpy beforeSpy(&model, &DynamicTreeModel::layoutAboutToBeChanged);
        QSignalSpy afterSpy(&model, &DynamicTreeModel::layoutChanged);

        QVERIFY(beforeSpy.isValid());
        QVERIFY(afterSpy.isValid());

        // Because the arguments in the signal are persistent, we need to check them for the aboutToBe
        // case at emission time - before they get updated.
        SignalArgumentChecker checker(p1, p2);

        ModelChangeChildrenLayoutsCommand *changeCommand = new ModelChangeChildrenLayoutsCommand(&model, this);
        changeCommand->setAncestorRowNumbers(QList<int>() << 5);
        changeCommand->setSecondAncestorRowNumbers(QList<int>() << 5 << 4);
        changeCommand->doCommand();

        // p2 has been moved.
        QCOMPARE(p2Persistent.row(), p2.row() + 1);

        QCOMPARE(beforeSpy.size(), 1);
        QCOMPARE(afterSpy.size(), 1);

        const QVariantList beforeSignal = beforeSpy.first();
        const QVariantList afterSignal = afterSpy.first();
        QCOMPARE(beforeSignal.size(), 2);
        QCOMPARE(afterSignal.size(), 2);

        QCOMPARE(p1FirstPersistent.row(), 1);
        QCOMPARE(p1LastPersistent.row(), 0);
        QCOMPARE(p2FirstPersistent.row(), 9);
        QCOMPARE(p2LastPersistent.row(), 8);
    }
}

class OverrideRoleNamesAndDragActions : public QStringListModel
{
    Q_OBJECT
public:
    OverrideRoleNamesAndDragActions(QObject *parent = 0)
      : QStringListModel(parent)
    {

    }

    QHash<int, QByteArray> roleNames() const
    {
        QHash<int, QByteArray> roles = QStringListModel::roleNames();
        roles.insert(Qt::UserRole + 2, "custom");
        return roles;
    }

    Qt::DropActions supportedDragActions() const
    {
        return QStringListModel::supportedDragActions() | Qt::MoveAction;
    }
};

void tst_QAbstractItemModel::testRoleNames()
{
    QAbstractItemModel *model = new OverrideRoleNamesAndDragActions(this);
    QHash<int, QByteArray> roles = model->roleNames();
    QVERIFY(roles.contains(Qt::UserRole + 2));
    QVERIFY(roles.value(Qt::UserRole + 2) == "custom");
}

void tst_QAbstractItemModel::testDragActions()
{
    QAbstractItemModel *model = new OverrideRoleNamesAndDragActions(this);
    const Qt::DropActions actions = model->supportedDragActions();
    QVERIFY(actions & Qt::CopyAction); // Present by default
    QVERIFY(actions & Qt::MoveAction);
}

class OverrideDropActions : public QStringListModel
{
    Q_OBJECT
public:
    OverrideDropActions(QObject *parent = 0)
      : QStringListModel(parent)
    {
    }
    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }
};

void tst_QAbstractItemModel::dragActionsFallsBackToDropActions()
{
    QAbstractItemModel *model = new OverrideDropActions(this);
    QCOMPARE(model->supportedDragActions(), Qt::MoveAction);
    QCOMPARE(model->supportedDropActions(), Qt::MoveAction);
}

class SignalConnectionTester : public QObject
{
    Q_OBJECT
public:
    SignalConnectionTester(QObject *parent = 0)
      : QObject(parent), testPassed(false)
    {

    }

public Q_SLOTS:
    void testSlot()
    {
      testPassed = true;
    }
    void testSlotWithParam_1(const QModelIndex &idx)
    {
      testPassed = !idx.isValid();
    }
    void testSlotWithParam_2(const QModelIndex &idx, int start)
    {
      testPassed = !idx.isValid() && start == 0;
    }
    void testSlotWithParam_3(const QModelIndex &idx, int start, int end)
    {
      testPassed = !idx.isValid() && start == 0 && end == 1;
    }

public:
    bool testPassed;
};

void tst_QAbstractItemModel::testFunctionPointerSignalConnection()
{
    QStringListModel model;
    {
        SignalConnectionTester tester;
        QObject::connect(&model, &QAbstractItemModel::rowsInserted, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        model.insertRows(0, 2);

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&model, "rowsInserted", Q_ARG(QModelIndex, QModelIndex()), Q_ARG(int, 0), Q_ARG(int, 1));
        QVERIFY(tester.testPassed);
    }
    {
        SignalConnectionTester tester;
        QObject::connect(&model, &QAbstractItemModel::rowsInserted, &tester, &SignalConnectionTester::testSlotWithParam_1);

        QVERIFY(!tester.testPassed);

        model.insertRows(0, 2);

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&model, "rowsInserted", Q_ARG(QModelIndex, QModelIndex()), Q_ARG(int, 0), Q_ARG(int, 1));
        QVERIFY(tester.testPassed);
    }
    {
        SignalConnectionTester tester;
        QObject::connect(&model, &QAbstractItemModel::rowsInserted, &tester, &SignalConnectionTester::testSlotWithParam_2);

        QVERIFY(!tester.testPassed);

        model.insertRows(0, 2);

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&model, "rowsInserted", Q_ARG(QModelIndex, QModelIndex()), Q_ARG(int, 0), Q_ARG(int, 1));
        QVERIFY(tester.testPassed);
    }
    {
        SignalConnectionTester tester;
        QObject::connect(&model, &QAbstractItemModel::rowsInserted, &tester, &SignalConnectionTester::testSlotWithParam_3);

        QVERIFY(!tester.testPassed);

        model.insertRows(0, 2);

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&model, "rowsInserted", Q_ARG(QModelIndex, QModelIndex()), Q_ARG(int, 0), Q_ARG(int, 1));
        QVERIFY(tester.testPassed);
    }
    {
        SignalConnectionTester tester;
        QObject::connect(&model, SIGNAL(rowsInserted(QModelIndex,int,int)), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        model.insertRows(0, 2);

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&model, "rowsInserted", Q_ARG(QModelIndex, QModelIndex()), Q_ARG(int, 0), Q_ARG(int, 1));
        QVERIFY(tester.testPassed);
    }
    // Intentionally does not compile.
//     model.rowsInserted(QModelIndex(), 0, 0);
}

void tst_QAbstractItemModel::checkIndex()
{
    const QRegularExpression ignorePattern("^Index QModelIndex");

    // checkIndex is QAbstractItemModel API; using QStandardItem as an easy
    // way to build a tree model
    QStandardItemModel model;
    QStandardItem *topLevel = new QStandardItem("topLevel");
    model.appendRow(topLevel);

    topLevel->appendRow(new QStandardItem("child1"));
    topLevel->appendRow(new QStandardItem("child2"));

    QVERIFY(model.checkIndex(QModelIndex()));
    QVERIFY(model.checkIndex(QModelIndex(), QAbstractItemModel::CheckIndexOption::DoNotUseParent));
    QVERIFY(model.checkIndex(QModelIndex(), QAbstractItemModel::CheckIndexOption::ParentIsInvalid));
    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(QModelIndex(), QAbstractItemModel::CheckIndexOption::IndexIsValid));

    QModelIndex topLevelIndex = model.index(0, 0);
    QVERIFY(topLevelIndex.isValid());
    QVERIFY(model.checkIndex(topLevelIndex));
    QVERIFY(model.checkIndex(topLevelIndex, QAbstractItemModel::CheckIndexOption::DoNotUseParent));
    QVERIFY(model.checkIndex(topLevelIndex, QAbstractItemModel::CheckIndexOption::ParentIsInvalid));
    QVERIFY(model.checkIndex(topLevelIndex, QAbstractItemModel::CheckIndexOption::IndexIsValid));

    QModelIndex childIndex = model.index(0, 0, topLevelIndex);
    QVERIFY(childIndex.isValid());
    QVERIFY(model.checkIndex(childIndex));
    QVERIFY(model.checkIndex(childIndex, QAbstractItemModel::CheckIndexOption::DoNotUseParent));
    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(childIndex, QAbstractItemModel::CheckIndexOption::ParentIsInvalid));
    QVERIFY(model.checkIndex(childIndex, QAbstractItemModel::CheckIndexOption::IndexIsValid));

    childIndex = model.index(1, 0, topLevelIndex);
    QVERIFY(childIndex.isValid());
    QVERIFY(model.checkIndex(childIndex));
    QVERIFY(model.checkIndex(childIndex, QAbstractItemModel::CheckIndexOption::DoNotUseParent));
    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(childIndex, QAbstractItemModel::CheckIndexOption::ParentIsInvalid));
    QVERIFY(model.checkIndex(childIndex, QAbstractItemModel::CheckIndexOption::IndexIsValid));

    topLevel->removeRow(1);
    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(childIndex));
    QVERIFY(model.checkIndex(childIndex, QAbstractItemModel::CheckIndexOption::DoNotUseParent));
    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(childIndex, QAbstractItemModel::CheckIndexOption::ParentIsInvalid));
    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(childIndex, QAbstractItemModel::CheckIndexOption::IndexIsValid));

    QStandardItemModel model2;
    model2.appendRow(new QStandardItem("otherTopLevel"));
    topLevelIndex = model2.index(0, 0);
    QVERIFY(topLevelIndex.isValid());
    QVERIFY(model2.checkIndex(topLevelIndex));
    QVERIFY(model2.checkIndex(topLevelIndex, QAbstractItemModel::CheckIndexOption::DoNotUseParent));
    QVERIFY(model2.checkIndex(topLevelIndex, QAbstractItemModel::CheckIndexOption::ParentIsInvalid));
    QVERIFY(model2.checkIndex(topLevelIndex, QAbstractItemModel::CheckIndexOption::IndexIsValid));

    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(topLevelIndex));
    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(topLevelIndex, QAbstractItemModel::CheckIndexOption::DoNotUseParent));
    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(topLevelIndex, QAbstractItemModel::CheckIndexOption::ParentIsInvalid));
    QTest::ignoreMessage(QtWarningMsg, ignorePattern);
    QVERIFY(!model.checkIndex(topLevelIndex, QAbstractItemModel::CheckIndexOption::IndexIsValid));
}

QTEST_MAIN(tst_QAbstractItemModel)
#include "tst_qabstractitemmodel.moc"
