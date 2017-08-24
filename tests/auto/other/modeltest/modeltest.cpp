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

#include "modeltest.h"

#include <QtTest/QtTest>

Q_LOGGING_CATEGORY(lcModelTest, "qt.modeltest")

/*!
    Connect to all of the models signals.  Whenever anything happens recheck everything.
*/
ModelTest::ModelTest(QAbstractItemModel *model, QObject *parent)
    : QObject(parent),
      model(model),
      fetchingMore(false)
{
    if (!model)
        qFatal("%s: model must not be null", Q_FUNC_INFO);

    connect(model, &QAbstractItemModel::columnsAboutToBeInserted,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::columnsAboutToBeRemoved,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::columnsInserted,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::columnsRemoved,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::dataChanged,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::headerDataChanged,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::layoutAboutToBeChanged,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::layoutChanged,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::modelReset,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::rowsInserted,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::rowsRemoved,
            this, &ModelTest::runAllTests);

    // Special checks for changes
    connect(model, &QAbstractItemModel::layoutAboutToBeChanged,
            this, &ModelTest::layoutAboutToBeChanged);
    connect(model, &QAbstractItemModel::layoutChanged,
            this, &ModelTest::layoutChanged);

    connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, &ModelTest::rowsAboutToBeInserted);
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &ModelTest::rowsAboutToBeRemoved);
    connect(model, &QAbstractItemModel::rowsInserted,
            this, &ModelTest::rowsInserted);
    connect(model, &QAbstractItemModel::rowsRemoved,
            this, &ModelTest::rowsRemoved);
    connect(model, &QAbstractItemModel::dataChanged,
            this, &ModelTest::dataChanged);
    connect(model, &QAbstractItemModel::headerDataChanged,
            this, &ModelTest::headerDataChanged);

    runAllTests();
}

void ModelTest::runAllTests()
{
    if (fetchingMore)
        return;
    nonDestructiveBasicTest();
    rowCount();
    columnCount();
    hasIndex();
    index();
    parent();
    data();
}

/*!
    nonDestructiveBasicTest tries to call a number of the basic functions (not all)
    to make sure the model doesn't outright segfault, testing the functions that makes sense.
*/
void ModelTest::nonDestructiveBasicTest()
{
    QVERIFY(!model->buddy(QModelIndex()).isValid());
    model->canFetchMore(QModelIndex());
    QVERIFY(model->columnCount(QModelIndex()) >= 0);
    QCOMPARE(model->data(QModelIndex()), QVariant());
    fetchingMore = true;
    model->fetchMore(QModelIndex());
    fetchingMore = false;
    Qt::ItemFlags flags = model->flags(QModelIndex());
    QVERIFY(flags == Qt::ItemIsDropEnabled || flags == 0);
    model->hasChildren(QModelIndex());
    model->hasIndex(0, 0);
    model->headerData(0, Qt::Horizontal);
    model->index(0, 0);
    model->itemData(QModelIndex());
    QVariant cache;
    model->match(QModelIndex(), -1, cache);
    model->mimeTypes();
    QVERIFY(!model->parent(QModelIndex()).isValid());
    QVERIFY(model->rowCount() >= 0);
    QVariant variant;
    model->setData(QModelIndex(), variant, -1);
    model->setHeaderData(-1, Qt::Horizontal, QVariant());
    model->setHeaderData(999999, Qt::Horizontal, QVariant());
    QMap<int, QVariant> roles;
    model->sibling(0, 0, QModelIndex());
    model->span(QModelIndex());
    model->supportedDropActions();
}

/*!
    Tests model's implementation of QAbstractItemModel::rowCount() and hasChildren()

    Models that are dynamically populated are not as fully tested here.
 */
void ModelTest::rowCount()
{
    // check top row
    QModelIndex topIndex = model->index(0, 0, QModelIndex());
    int rows = model->rowCount(topIndex);
    QVERIFY(rows >= 0);
    if (rows > 0)
        QVERIFY(model->hasChildren(topIndex));

    QModelIndex secondLevelIndex = model->index(0, 0, topIndex);
    if (secondLevelIndex.isValid()) {   // not the top level
        // check a row count where parent is valid
        rows = model->rowCount(secondLevelIndex);
        QVERIFY(rows >= 0);
        if (rows > 0)
            QVERIFY(model->hasChildren(secondLevelIndex));
    }

    // The models rowCount() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*!
    Tests model's implementation of QAbstractItemModel::columnCount() and hasChildren()
 */
void ModelTest::columnCount()
{
    // check top row
    QModelIndex topIndex = model->index(0, 0, QModelIndex());
    QVERIFY(model->columnCount(topIndex) >= 0);

    // check a column count where parent is valid
    QModelIndex childIndex = model->index(0, 0, topIndex);
    if (childIndex.isValid())
        QVERIFY(model->columnCount(childIndex) >= 0);

    // columnCount() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*!
    Tests model's implementation of QAbstractItemModel::hasIndex()
 */
void ModelTest::hasIndex()
{
    // Make sure that invalid values returns an invalid index
    QVERIFY(!model->hasIndex(-2, -2));
    QVERIFY(!model->hasIndex(-2, 0));
    QVERIFY(!model->hasIndex(0, -2));

    int rows = model->rowCount();
    int columns = model->columnCount();

    // check out of bounds
    QVERIFY(!model->hasIndex(rows, columns));
    QVERIFY(!model->hasIndex(rows + 1, columns + 1));

    if (rows > 0)
        QVERIFY(model->hasIndex(0, 0));

    // hasIndex() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*!
    Tests model's implementation of QAbstractItemModel::index()
 */
void ModelTest::index()
{
    // Make sure that invalid values returns an invalid index
    QVERIFY(!model->index(-2, -2).isValid());
    QVERIFY(!model->index(-2, 0).isValid());
    QVERIFY(!model->index(0, -2).isValid());

    int rows = model->rowCount();
    int columns = model->columnCount();

    if (rows == 0)
        return;

    // Catch off by one errors
    QVERIFY(!model->index(rows, columns).isValid());
    QVERIFY(model->index(0, 0).isValid());

    // Make sure that the same index is *always* returned
    QModelIndex a = model->index(0, 0);
    QModelIndex b = model->index(0, 0);
    QCOMPARE(a, b);

    // index() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*!
    Tests model's implementation of QAbstractItemModel::parent()
 */
void ModelTest::parent()
{
    // Make sure the model won't crash and will return an invalid QModelIndex
    // when asked for the parent of an invalid index.
    QVERIFY(!model->parent(QModelIndex()).isValid());

    if (model->rowCount() == 0)
        return;

    // Column 0                | Column 1    |
    // QModelIndex()           |             |
    //    \- topIndex          | topIndex1   |
    //         \- childIndex   | childIndex1 |

    // Common error test #1, make sure that a top level index has a parent
    // that is a invalid QModelIndex.
    QModelIndex topIndex = model->index(0, 0, QModelIndex());
    QVERIFY(!model->parent(topIndex).isValid());

    // Common error test #2, make sure that a second level index has a parent
    // that is the first level index.
    if (model->rowCount(topIndex) > 0) {
        QModelIndex childIndex = model->index(0, 0, topIndex);
        QCOMPARE(model->parent(childIndex), topIndex);
    }

    // Common error test #3, the second column should NOT have the same children
    // as the first column in a row.
    // Usually the second column shouldn't have children.
    QModelIndex topIndex1 = model->index(0, 1, QModelIndex());
    if (model->rowCount(topIndex1) > 0) {
        QModelIndex childIndex = model->index(0, 0, topIndex);
        QModelIndex childIndex1 = model->index(0, 0, topIndex1);
        QVERIFY(childIndex != childIndex1);
    }

    // Full test, walk n levels deep through the model making sure that all
    // parent's children correctly specify their parent.
    checkChildren(QModelIndex());
}

/*!
    Called from the parent() test.

    A model that returns an index of parent X should also return X when asking
    for the parent of the index.

    This recursive function does pretty extensive testing on the whole model in an
    effort to catch edge cases.

    This function assumes that rowCount(), columnCount() and index() already work.
    If they have a bug it will point it out, but the above tests should have already
    found the basic bugs because it is easier to figure out the problem in
    those tests then this one.
 */
void ModelTest::checkChildren(const QModelIndex &parent, int currentDepth)
{
    // First just try walking back up the tree.
    QModelIndex p = parent;
    while (p.isValid())
        p = p.parent();

    // For models that are dynamically populated
    if (model->canFetchMore(parent)) {
        fetchingMore = true;
        model->fetchMore(parent);
        fetchingMore = false;
    }

    const int rows = model->rowCount(parent);
    const int columns = model->columnCount(parent);

    if (rows > 0)
        QVERIFY(model->hasChildren(parent));

    // Some further testing against rows(), columns(), and hasChildren()
    QVERIFY(rows >= 0);
    QVERIFY(columns >= 0);
    if (rows > 0)
        QVERIFY(model->hasChildren(parent));

    const QModelIndex topLeftChild = model->index(0, 0, parent);

    QVERIFY(!model->hasIndex(rows + 1, 0, parent));
    for (int r = 0; r < rows; ++r) {
        if (model->canFetchMore(parent)) {
            fetchingMore = true;
            model->fetchMore(parent);
            fetchingMore = false;
        }
        QVERIFY(!model->hasIndex(r, columns + 1, parent));
        for (int c = 0; c < columns; ++c) {
            QVERIFY(model->hasIndex(r, c, parent));
            QModelIndex index = model->index(r, c, parent);
            // rowCount() and columnCount() said that it existed...
            if (!index.isValid())
                qCWarning(lcModelTest) << "Got invalid index at row=" << r << "col=" << c << "parent=" << parent;
            QVERIFY(index.isValid());

            // index() should always return the same index when called twice in a row
            QModelIndex modifiedIndex = model->index(r, c, parent);
            QCOMPARE(index, modifiedIndex);

            // Make sure we get the same index if we request it twice in a row
            QModelIndex a = model->index(r, c, parent);
            QModelIndex b = model->index(r, c, parent);
            QCOMPARE(a, b);

            {
                const QModelIndex sibling = model->sibling(r, c, topLeftChild);
                QCOMPARE(index, sibling);
            }
            {
                const QModelIndex sibling = topLeftChild.sibling(r, c);
                QCOMPARE(index, sibling);
            }

            // Some basic checking on the index that is returned
            QCOMPARE(index.model(), model);
            QCOMPARE(index.row(), r);
            QCOMPARE(index.column(), c);

            // If the next test fails here is some somewhat useful debug you play with.
            if (model->parent(index) != parent) {
                qCWarning(lcModelTest) << "Inconsistent parent() implementation detected:";
                qCWarning(lcModelTest) << "    index=" << index << "exp. parent=" << parent << "act. parent=" << model->parent(index);
                qCWarning(lcModelTest) << "    row=" << r << "col=" << c << "depth=" << currentDepth;
                qCWarning(lcModelTest) << "    data for child" << model->data(index).toString();
                qCWarning(lcModelTest) << "    data for parent" << model->data(parent).toString();
            }

            // Check that we can get back our real parent.
            QCOMPARE(model->parent(index), parent);

            // recursively go down the children
            if (model->hasChildren(index) && currentDepth < 10)
                checkChildren(index, ++currentDepth);

            // make sure that after testing the children that the index doesn't change.
            QModelIndex newerIndex = model->index(r, c, parent);
            QCOMPARE(index, newerIndex);
        }
    }
}

/*!
    Tests model's implementation of QAbstractItemModel::data()
 */
void ModelTest::data()
{
    // Invalid index should return an invalid qvariant
    QVERIFY(!model->data(QModelIndex()).isValid());

    if (model->rowCount() == 0)
        return;

    // A valid index should have a valid QVariant data
    QVERIFY(model->index(0, 0).isValid());

    // shouldn't be able to set data on an invalid index
    QVERIFY(!model->setData(QModelIndex(), QLatin1String("foo"), Qt::DisplayRole));

    // General Purpose roles that should return a QString
    QVariant variant = model->data(model->index(0, 0), Qt::ToolTipRole);
    if (variant.isValid())
        QVERIFY(variant.canConvert<QString>());
    variant = model->data(model->index(0, 0), Qt::StatusTipRole);
    if (variant.isValid())
        QVERIFY(variant.canConvert<QString>());
    variant = model->data(model->index(0, 0), Qt::WhatsThisRole);
    if (variant.isValid())
        QVERIFY(variant.canConvert<QString>());

    // General Purpose roles that should return a QSize
    variant = model->data(model->index(0, 0), Qt::SizeHintRole);
    if (variant.isValid())
        QVERIFY(variant.canConvert<QSize>());

    // General Purpose roles that should return a QFont
    QVariant fontVariant = model->data(model->index(0, 0), Qt::FontRole);
    if (fontVariant.isValid())
        QVERIFY(fontVariant.canConvert<QFont>());

    // Check that the alignment is one we know about
    QVariant textAlignmentVariant = model->data(model->index(0, 0), Qt::TextAlignmentRole);
    if (textAlignmentVariant.isValid()) {
        Qt::Alignment alignment = textAlignmentVariant.value<Qt::Alignment>();
        QCOMPARE(alignment, (alignment & (Qt::AlignHorizontal_Mask | Qt::AlignVertical_Mask)));
    }

    // General Purpose roles that should return a QColor
    QVariant colorVariant = model->data(model->index(0, 0), Qt::BackgroundColorRole);
    if (colorVariant.isValid())
        QVERIFY(colorVariant.canConvert<QColor>());

    colorVariant = model->data(model->index(0, 0), Qt::TextColorRole);
    if (colorVariant.isValid())
        QVERIFY(colorVariant.canConvert<QColor>());

    // Check that the "check state" is one we know about.
    QVariant checkStateVariant = model->data(model->index(0, 0), Qt::CheckStateRole);
    if (checkStateVariant.isValid()) {
        int state = checkStateVariant.toInt();
        QVERIFY(state == Qt::Unchecked
                || state == Qt::PartiallyChecked
                || state == Qt::Checked);
    }
}

/*!
    Store what is about to be inserted to make sure it actually happens

    \sa rowsInserted()
 */
void ModelTest::rowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    qCDebug(lcModelTest) << "rowsAboutToBeInserted"
                         << "start=" << start << "end=" << end << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent)
                         << "last before insertion=" << model->index(start - 1, 0, parent) << model->data(model->index(start - 1, 0, parent));

    Changing c;
    c.parent = parent;
    c.oldSize = model->rowCount(parent);
    c.last = model->data(model->index(start - 1, 0, parent));
    c.next = model->data(model->index(start, 0, parent));
    insert.push(c);
}

/*!
    Confirm that what was said was going to happen actually did

    \sa rowsAboutToBeInserted()
 */
void ModelTest::rowsInserted(const QModelIndex &parent, int start, int end)
{
    qCDebug(lcModelTest) << "rowsInserted"
                         << "start=" << start << "end=" << end << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent);

    for (int i = start; i <= end; ++i) {
        qCDebug(lcModelTest) << "    itemWasInserted:" << i
                             << model->index(i, 0, parent).data();
    }


    Changing c = insert.pop();
    QCOMPARE(parent, c.parent);

    QCOMPARE(model->rowCount(parent), c.oldSize + (end - start + 1));
    QCOMPARE(model->data(model->index(start - 1, 0, c.parent)), c.last);

    if (c.next != model->data(model->index(end + 1, 0, c.parent))) {
        qDebug() << start << end;
        for (int i = 0; i < model->rowCount(); ++i)
            qDebug() << model->index(i, 0).data().toString();
        qDebug() << c.next << model->data(model->index(end + 1, 0, c.parent));
    }

    QCOMPARE(model->data(model->index(end + 1, 0, c.parent)), c.next);
}

void ModelTest::layoutAboutToBeChanged()
{
    for (int i = 0; i < qBound(0, model->rowCount(), 100); ++i)
        changing.append(QPersistentModelIndex(model->index(i, 0)));
}

void ModelTest::layoutChanged()
{
    for (int i = 0; i < changing.count(); ++i) {
        QPersistentModelIndex p = changing[i];
        QCOMPARE(model->index(p.row(), p.column(), p.parent()), QModelIndex(p));
    }
    changing.clear();
}

/*!
    Store what is about to be inserted to make sure it actually happens

    \sa rowsRemoved()
 */
void ModelTest::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    qCDebug(lcModelTest) << "rowsAboutToBeRemoved"
                         << "start=" << start << "end=" << end << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent)
                         << "last before removal=" << model->index(start - 1, 0, parent) << model->data(model->index(start - 1, 0, parent));

    Changing c;
    c.parent = parent;
    c.oldSize = model->rowCount(parent);
    c.last = model->data(model->index(start - 1, 0, parent));
    c.next = model->data(model->index(end + 1, 0, parent));
    remove.push(c);
}

/*!
    Confirm that what was said was going to happen actually did

    \sa rowsAboutToBeRemoved()
 */
void ModelTest::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    qCDebug(lcModelTest) << "rowsRemoved"
                         << "start=" << start << "end=" << end << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent);

    Changing c = remove.pop();
    QCOMPARE(parent, c.parent);
    QCOMPARE(model->rowCount(parent), c.oldSize - (end - start + 1));
    QCOMPARE(model->data(model->index(start - 1, 0, c.parent)), c.last);
    QCOMPARE(model->data(model->index(start, 0, c.parent)), c.next);
}

void ModelTest::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QVERIFY(topLeft.isValid());
    QVERIFY(bottomRight.isValid());
    QModelIndex commonParent = bottomRight.parent();
    QCOMPARE(topLeft.parent(), commonParent);
    QVERIFY(topLeft.row() <= bottomRight.row());
    QVERIFY(topLeft.column() <= bottomRight.column());
    int rowCount = model->rowCount(commonParent);
    int columnCount = model->columnCount(commonParent);
    QVERIFY(bottomRight.row() < rowCount);
    QVERIFY(bottomRight.column() < columnCount);
}

void ModelTest::headerDataChanged(Qt::Orientation orientation, int start, int end)
{
    QVERIFY(start >= 0);
    QVERIFY(end >= 0);
    QVERIFY(start <= end);
    int itemCount = orientation == Qt::Vertical ? model->rowCount() : model->columnCount();
    QVERIFY(start < itemCount);
    QVERIFY(end < itemCount);
}
