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
#include <QtCore/QCoreApplication>
#include <qdebug.h>
#include "modelstotest.cpp"
#include <QMetaType>

/*!
    See modelstotest.cpp for instructions on how to have your model tested with these tests.

    Each test such as rowCount have a _data() function which populate the QTest data with
    the tests specified by modelstotest.cpp and any extra data needed for that particular test.

    setupWithNoTestData() fills the QTest data with just the tests and is used by most tests.
 */
class tst_QItemModel : public QObject
{
    Q_OBJECT

public slots:
    void init();
    void cleanup();

private slots:
    void nonDestructiveBasicTest_data();
    void nonDestructiveBasicTest();

    void rowCount_data();
    void rowCount();
    void columnCount_data();
    void columnCount();

    void hasIndex_data();
    void hasIndex();
    void index_data();
    void index();

    void parent_data();
    void parent();

    void data_data();
    void data();

    void setData_data();
    void setData();

    void setHeaderData_data();
    void setHeaderData();

    void remove_data();
    void remove();

    void insert_data();
    void insert();

    void sort_data();
    void sort();

protected slots:
    void slot_rowsAboutToRemove(const QModelIndex &parent);
    void slot_rowsRemoved(const QModelIndex &parent);
    void slot_columnsAboutToRemove(const QModelIndex &parent);
    void slot_columnsRemoved(const QModelIndex &parent);

    void slot_rowsAboutToInserted(const QModelIndex &parent);
    void slot_rowsInserted(const QModelIndex &parent);
    void slot_columnsAboutToInserted(const QModelIndex &parent);
    void slot_columnsInserted(const QModelIndex &parent);
private:
    void setupWithNoTestData();
    QAbstractItemModel *currentModel;
    ModelsToTest *testModels;

    // used by remove()
    QPersistentModelIndex parentOfRemoved;
    int afterAboutToRemoveRowCount;
    int afterRemoveRowCount;
    int afterAboutToRemoveColumnCount;
    int afterRemoveColumnCount;

    // remove() recursive
    bool removeRecursively;

    // used by insert()
    QPersistentModelIndex parentOfInserted;
    int afterAboutToInsertRowCount;
    int afterInsertRowCount;
    int afterAboutToInsertColumnCount;
    int afterInsertColumnCount;

    // insert() recursive
    bool insertRecursively;
};

void tst_QItemModel::init()
{
    testModels = new ModelsToTest();
    removeRecursively = false;
    insertRecursively = false;
}

void tst_QItemModel::cleanup()
{
    testModels->cleanupTestArea(currentModel);
    delete testModels;
    delete currentModel;
    currentModel = 0;
}

void tst_QItemModel::setupWithNoTestData()
{
    ModelsToTest modelsToTest;
    QTest::addColumn<QString>("modelType");
    QTest::addColumn<bool>("readOnly");
    QTest::addColumn<bool>("isEmpty");
    for (int i = 0; i < modelsToTest.tests.size(); ++i) {
        ModelsToTest::test t = modelsToTest.tests.at(i);
        bool readOnly = (t.read == ModelsToTest::ReadOnly);
        bool isEmpty = (t.contains == ModelsToTest::Empty);
        QTest::newRow(t.modelType.toLatin1().data()) << t.modelType << readOnly << isEmpty;
    }
}

void tst_QItemModel::nonDestructiveBasicTest_data()
{
    setupWithNoTestData();
}

/*!
    nonDestructiveBasicTest tries to call a number of the basic functions (not all)
    to make sure the model doesn't segfault, testing the functions that makes sense.
 */
void tst_QItemModel::nonDestructiveBasicTest()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    QCOMPARE(currentModel->buddy(QModelIndex()), QModelIndex());
    currentModel->canFetchMore(QModelIndex());
    QVERIFY(currentModel->columnCount(QModelIndex()) >= 0);
    QCOMPARE(currentModel->data(QModelIndex()), QVariant());
    currentModel->fetchMore(QModelIndex());
    Qt::ItemFlags flags = currentModel->flags(QModelIndex());
    QVERIFY(flags == Qt::ItemIsDropEnabled || flags == 0);
    currentModel->hasChildren(QModelIndex());
    currentModel->hasIndex(0, 0);
    currentModel->headerData(0, Qt::Horizontal);
    currentModel->index(0,0), QModelIndex();
    currentModel->itemData(QModelIndex());
    QVariant cache;
    currentModel->match(QModelIndex(), -1, cache);
    currentModel->mimeTypes();
    QCOMPARE(currentModel->parent(QModelIndex()), QModelIndex());
    QVERIFY(currentModel->rowCount() >= 0);
    QVariant variant;
    currentModel->setData(QModelIndex(), variant, -1);
    currentModel->setHeaderData(-1, Qt::Horizontal, QVariant());
    currentModel->setHeaderData(0, Qt::Horizontal, QVariant());
    currentModel->setHeaderData(currentModel->columnCount() + 100, Qt::Horizontal, QVariant());
    QMap<int, QVariant> roles;
    currentModel->setItemData(QModelIndex(), roles);
    currentModel->sibling(0,0,QModelIndex());
    currentModel->span(QModelIndex());
    currentModel->supportedDropActions();
    currentModel->revert();
    currentModel->submit();
}


void tst_QItemModel::rowCount_data()
{
    setupWithNoTestData();
}

/*!
    Tests model's implimentation of QAbstractItemModel::rowCount() and hasChildren()
 */
void tst_QItemModel::rowCount()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    QFETCH(bool, isEmpty);
    if (isEmpty) {
        QCOMPARE(currentModel->rowCount(), 0);
        QCOMPARE(currentModel->hasChildren(), false);
    }
    else {
        QVERIFY(currentModel->rowCount() > 0);
        QCOMPARE(currentModel->hasChildren(), true);
    }

    // check top row
    QModelIndex topIndex = currentModel->index(0, 0, QModelIndex());
    int rows = currentModel->rowCount(topIndex);
    QVERIFY(rows >= 0);
    if (rows > 0)
        QCOMPARE(currentModel->hasChildren(topIndex), true);
    else
        QCOMPARE(currentModel->hasChildren(topIndex), false);

    QModelIndex secondLevelIndex = currentModel->index(0, 0, topIndex);
    if (secondLevelIndex.isValid()) { // not the top level
        // check a row count where parent is valid
        rows = currentModel->rowCount(secondLevelIndex);
        QVERIFY(rows >= 0);
        if (rows > 0)
            QCOMPARE(currentModel->hasChildren(secondLevelIndex), true);
        else
            QCOMPARE(currentModel->hasChildren(secondLevelIndex), false);
    }

    // rowCount is tested more extensivly more later in checkChildren(),
    // but this catches the big mistakes
}

void tst_QItemModel::columnCount_data()
{
    setupWithNoTestData();
}

/*!
    Tests model's implimentation of QAbstractItemModel::columnCount() and hasChildren()
 */
void tst_QItemModel::columnCount()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    QFETCH(bool, isEmpty);
    if (isEmpty) {
        QCOMPARE(currentModel->hasChildren(), false);
    }
    else {
        QVERIFY(currentModel->columnCount() > 0);
        QCOMPARE(currentModel->hasChildren(), true);
    }

    // check top row
    QModelIndex topIndex = currentModel->index(0, 0, QModelIndex());
    int columns = currentModel->columnCount(topIndex);

    // check a row count where parent is valid
    columns = currentModel->columnCount(currentModel->index(0, 0, topIndex));
    QVERIFY(columns >= 0);

    // columnCount is tested more extensivly more later in checkChildren(),
    // but this catches the big mistakes
}

void tst_QItemModel::hasIndex_data()
{
    setupWithNoTestData();
}

/*!
    Tests model's implimentation of QAbstractItemModel::hasIndex()
 */
void tst_QItemModel::hasIndex()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    // Make sure that invalid values returns an invalid index
    QCOMPARE(currentModel->hasIndex(-2, -2), false);
    QCOMPARE(currentModel->hasIndex(-2, 0), false);
    QCOMPARE(currentModel->hasIndex(0, -2), false);

    int rows = currentModel->rowCount();
    int columns = currentModel->columnCount();

    QCOMPARE(currentModel->hasIndex(rows, columns), false);
    QCOMPARE(currentModel->hasIndex(rows+1, columns+1), false);

    QFETCH(bool, isEmpty);
    if (isEmpty)
        return;

    QCOMPARE(currentModel->hasIndex(0,0), true);

    // hasIndex is tested more extensivly more later in checkChildren(),
    // but this catches the big mistakes
}

void tst_QItemModel::index_data()
{
    setupWithNoTestData();
}

/*!
    Tests model's implimentation of QAbstractItemModel::index()
 */
void tst_QItemModel::index()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    // Make sure that invalid values returns an invalid index
    QCOMPARE(currentModel->index(-2, -2), QModelIndex());
    QCOMPARE(currentModel->index(-2, 0), QModelIndex());
    QCOMPARE(currentModel->index(0, -2), QModelIndex());

    QFETCH(bool, isEmpty);
    if (isEmpty)
        return;

    int rows = currentModel->rowCount();
    int columns = currentModel->columnCount();

    // Catch off by one errors
    QCOMPARE(currentModel->index(rows,columns), QModelIndex());
    QCOMPARE(currentModel->index(0,0).isValid(), true);

    // Make sure that the same index is always returned
    QModelIndex a = currentModel->index(0,0);
    QModelIndex b = currentModel->index(0,0);
    QVERIFY(a == b);

    // index is tested more extensivly more later in checkChildren(),
    // but this catches the big mistakes
}


void tst_QItemModel::parent_data()
{
    setupWithNoTestData();
}

/*!
    A model that returns an index of parent X should also return X when asking
    for the parent of the index.

    This recursive function does pretty extensive testing on the whole model in an
    effort to catch edge cases.

    This function assumes that rowCount(), columnCount() and index() work.  If they have
    a bug it will point it out, but the above tests should have already found the basic bugs
    because it is easier to figure out the problem in those tests then this one.
 */
void checkChildren(QAbstractItemModel *currentModel, const QModelIndex &parent, int currentDepth=0)
{
    QFETCH(bool, readOnly);

    if (currentModel->canFetchMore(parent))
        currentModel->fetchMore(parent);

    int rows = currentModel->rowCount(parent);
    int columns = currentModel->columnCount(parent);

    const QModelIndex topLeftChild = currentModel->index( 0, 0, parent );

    QCOMPARE(rows > 0, (currentModel->hasChildren(parent)));

    // Some reasuring testing against rows(),columns(), and hasChildren()
    QVERIFY(rows >= 0);
    QVERIFY(columns >= 0);
    if (rows > 0 || columns > 0)
        QCOMPARE(currentModel->hasChildren(parent), true);
    else
        QCOMPARE(currentModel->hasChildren(parent), false);

    QCOMPARE(currentModel->hasIndex(rows+1, 0, parent), false);
    for (int r = 0; r < rows; ++r) {
        if (currentModel->canFetchMore(parent))
            currentModel->fetchMore(parent);

        QCOMPARE(currentModel->hasIndex(r, columns+1, parent), false);
        for (int c = 0; c < columns; ++c) {
            QCOMPARE(currentModel->hasIndex(r, c, parent), true);
            QModelIndex index = currentModel->index(r, c, parent);
            QCOMPARE(index.isValid(), true);

            if (!readOnly)
                currentModel->setData(index, "I'm a little tea pot short and stout");
            QModelIndex modifiedIndex = currentModel->index(r, c, parent);
            QCOMPARE(index, modifiedIndex);

            // Make sure we get the same index if we request it twice in a row
            QModelIndex a = currentModel->index(r, c, parent);
            QModelIndex b = currentModel->index(r, c, parent);
            QVERIFY(a == b);

            {
                const QModelIndex sibling = currentModel->sibling( r, c, topLeftChild );
                QVERIFY( index == sibling );
            }
            {
                const QModelIndex sibling = topLeftChild.sibling( r, c );
                QVERIFY( index == sibling );
            }

            // Some basic checking on the index that is returned
            QVERIFY(index.model() == currentModel);
            QCOMPARE(index.row(), r);
            QCOMPARE(index.column(), c);
            QCOMPARE(currentModel->data(index, Qt::DisplayRole).isValid(), true);

            // If the next test fails here is some somewhat useful debug you play with.
            /*
            if (currentModel->parent(index) != parent) {
                qDebug() << r << c << currentDepth << currentModel->data(index).toString()
                         << currentModel->data(parent).toString();
                qDebug() << index << parent << currentModel->parent(index);
                QTreeView view;
                view.setModel(currentModel);
                view.show();
                QTest::qWait(9000);
            }*/
            QCOMPARE(currentModel->parent(index), parent);

            // recursivly go down
            if (currentModel->hasChildren(index) && currentDepth < 5) {
                checkChildren(currentModel, index, ++currentDepth);
                // Because this is recursive we will return at the first failure rather then
                // reporting it over and over
                if (QTest::currentTestFailed())
                    return;
            }

            // make sure that after testing the children that the index pointer doesn't change.
            QModelIndex newerIndex = currentModel->index(r, c, parent);
            QCOMPARE(index, newerIndex);
        }
    }
}

/*!
    Tests model's implimentation of QAbstractItemModel::parent()
 */
void tst_QItemModel::parent()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    // Make sure the model won't crash and will return an invalid QModelIndex
    // when asked for the parent of an invalid index.
    QCOMPARE(currentModel->parent(QModelIndex()), QModelIndex());

    QFETCH(bool, isEmpty);
    if (isEmpty)
        return;

    // Common error test #1, make sure that a top level index has a parent
    // that is a invalid QModelIndex.  You model
    QModelIndex topIndex = currentModel->index(0, 0, QModelIndex());
    QCOMPARE(currentModel->parent(topIndex), QModelIndex());

    // Common error test #2, make sure that a second level index has a parent
    // that is the top level index.
    if (currentModel->rowCount(topIndex) > 0) {
        QModelIndex childIndex = currentModel->index(0, 0, topIndex);
        QCOMPARE(currentModel->parent(childIndex), topIndex);
    }

    // Common error test #3, the second column has the same children
    // as the first column in a row.
    QModelIndex topIndex1 = currentModel->index(0, 1, QModelIndex());
    if (currentModel->rowCount(topIndex1) > 0) {
        QModelIndex childIndex = currentModel->index(0, 0, topIndex);
        QModelIndex childIndex1 = currentModel->index(0, 0, topIndex1);
        QVERIFY(childIndex != childIndex1);
    }

    // Full test, walk 10 levels deap through the model making sure that all
    // parents's children correctly specify their parent
    QModelIndex top = QModelIndex();
    checkChildren(currentModel, top);
}


void tst_QItemModel::data_data()
{
    setupWithNoTestData();
}

/*!
    Tests model's implimentation of QAbstractItemModel::data()
 */
void tst_QItemModel::data()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    // Invalid index should return an invalid qvariant
    QVERIFY(!currentModel->data(QModelIndex()).isValid());

    QFETCH(bool, isEmpty);
    if (isEmpty)
        return;

    // A valid index should have a valid qvariant data
    QVERIFY(currentModel->index(0,0).isValid());

    // shouldn't be able to set data on an invalid index
    QCOMPARE(currentModel->setData(QModelIndex(), "foo", Qt::DisplayRole), false);

    // General Purpose roles
    QVariant variant = currentModel->data(currentModel->index(0,0), Qt::ToolTipRole);
    if (variant.isValid()) {
        QVERIFY(variant.canConvert<QString>());
    }
    variant = currentModel->data(currentModel->index(0,0), Qt::StatusTipRole);
    if (variant.isValid()) {
        QVERIFY(variant.canConvert<QString>());
    }
    variant = currentModel->data(currentModel->index(0,0), Qt::WhatsThisRole);
    if (variant.isValid()) {
        QVERIFY(variant.canConvert<QString>());
    }

    variant = currentModel->data(currentModel->index(0,0), Qt::SizeHintRole);
    if (variant.isValid()) {
        QVERIFY(variant.canConvert<QSize>());
    }


    // Appearance roles
    QVariant fontVariant = currentModel->data(currentModel->index(0,0), Qt::FontRole);
    if (fontVariant.isValid()) {
        QVERIFY(fontVariant.canConvert<QFont>());
    }

    QVariant textAlignmentVariant = currentModel->data(currentModel->index(0,0), Qt::TextAlignmentRole);
    if (textAlignmentVariant.isValid()) {
        int alignment = textAlignmentVariant.toInt();
        QVERIFY(alignment == Qt::AlignLeft ||
                alignment == Qt::AlignRight ||
                alignment == Qt::AlignHCenter ||
                alignment == Qt::AlignJustify);
    }

    QVariant colorVariant = currentModel->data(currentModel->index(0,0), Qt::BackgroundColorRole);
    if (colorVariant.isValid()) {
        QVERIFY(colorVariant.canConvert<QColor>());
    }

    colorVariant = currentModel->data(currentModel->index(0,0), Qt::TextColorRole);
    if (colorVariant.isValid()) {
        QVERIFY(colorVariant.canConvert<QColor>());
    }

    QVariant checkStateVariant = currentModel->data(currentModel->index(0,0), Qt::CheckStateRole);
    if (checkStateVariant.isValid()) {
        int state = checkStateVariant.toInt();
        QVERIFY(state == Qt::Unchecked ||
                state == Qt::PartiallyChecked ||
                state == Qt::Checked);
    }
}

void tst_QItemModel::setData_data()
{
    setupWithNoTestData();
}

/*!
    Tests model's implimentation of QAbstractItemModel::setData()
 */
void tst_QItemModel::setData()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);
    QSignalSpy spy(currentModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)));
    QVERIFY(spy.isValid());
    QCOMPARE(currentModel->setData(QModelIndex(), QVariant()), false);
    QCOMPARE(spy.count(), 0);

    QFETCH(bool, isEmpty);
    if (isEmpty)
        return;

    QFETCH(bool, readOnly);
    if (readOnly)
        return;

    // Populate the test area so we can chage stuff.  See: cleanup()
    QModelIndex topIndex = testModels->populateTestArea(currentModel);
    QVERIFY(currentModel->hasChildren(topIndex));
    QModelIndex index = currentModel->index(0, 0, topIndex);
    QVERIFY(index.isValid());

    spy.clear();
    QString text = "Index private pointers should always be the same";
    QCOMPARE(currentModel->setData(index, text, Qt::EditRole), true);
    QCOMPARE(index.data(Qt::EditRole).toString(), text);

    // Changing the text shouldn't change the layout, parent, pointer etc.
    QModelIndex changedIndex = currentModel->index(0, 0, topIndex);
    QCOMPARE(changedIndex, index);
    QCOMPARE(spy.count(), 1);
}

void tst_QItemModel::setHeaderData_data()
{
    setupWithNoTestData();
}

/*!
    Tests model's implimentation of QAbstractItemModel::setHeaderData()
 */
void tst_QItemModel::setHeaderData()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    QCOMPARE(currentModel->setHeaderData(-1, Qt::Horizontal, QVariant()), false);
    QCOMPARE(currentModel->setHeaderData(-1, Qt::Vertical, QVariant()), false);

    QFETCH(bool, isEmpty);
    if (isEmpty)
        return;

    QFETCH(bool, readOnly);
    if (readOnly)
        return;

    // Populate the test area so we can change stuff.  See: cleanup()
    QModelIndex topIndex = testModels->populateTestArea(currentModel);
    QVERIFY(currentModel->hasChildren(topIndex));
    QModelIndex index = currentModel->index(0, 0, topIndex);
    QVERIFY(index.isValid());

    qRegisterMetaType<Qt::Orientation>("Qt::Orientation");
    QSignalSpy spy(currentModel, SIGNAL(headerDataChanged(Qt::Orientation,int,int)));
    QVERIFY(spy.isValid());

    QString text = "Index private pointers should always be the same";
    int signalCount = 0;
    for (int i = 0; i < 4; ++i){
        if(currentModel->setHeaderData(i, Qt::Horizontal, text)) {
            QCOMPARE(currentModel->headerData(i, Qt::Horizontal).toString(), text);
            ++signalCount;
        }
        if(currentModel->setHeaderData(i, Qt::Vertical, text)) {
            QCOMPARE(currentModel->headerData(i, Qt::Vertical).toString(), text);
            ++signalCount;
        }
    }
    QCOMPARE(spy.count(), signalCount);
}

void tst_QItemModel::sort_data()
{
    setupWithNoTestData();
}

/*!
    Tests model's implimentation of QAbstractItemModel::sort()
 */
void tst_QItemModel::sort()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    QFETCH(bool, isEmpty);
    if (isEmpty)
        return;

    // Populate the test area so we can chage stuff.  See: cleanup()
    QPersistentModelIndex topIndex = testModels->populateTestArea(currentModel);
    QVERIFY(currentModel->hasChildren(topIndex));
    QModelIndex index = currentModel->index(0, 0, topIndex);
    QVERIFY(index.isValid());
    QSignalSpy spy(currentModel, SIGNAL(layoutChanged()));
    QVERIFY(spy.isValid());
    for (int i=-1; i < 10; ++i){
        currentModel->sort(i);
        if (index != currentModel->index(0, 0, topIndex)){
            QVERIFY(spy.count() > 0);
            index = currentModel->index(0, 0, topIndex);
            spy.clear();
        }
    }
    currentModel->sort(99999);
}

/*!
    Tests model's implimentation of QAbstractItemModel::removeRow() and QAbstractItemModel::removeColumn()
 */
#define START 0
#define MIDDLE 6
#define END -1
#define MANY 9
#define ALL -1
#define NOSIGNALS 0
#define DEFAULTCOUNT 1
#define DNS 1 // DefaultNumberOfSignals
#define RECURSIVE true
#define SUCCESS true
#define FAIL false
void tst_QItemModel::remove_data()
{
    ModelsToTest modelsToTest;
    QTest::addColumn<QString>("modelType");
    QTest::addColumn<bool>("readOnly");
    QTest::addColumn<bool>("isEmpty");

    QTest::addColumn<int>("start");
    QTest::addColumn<int>("count");

    QTest::addColumn<int>("numberOfRowsAboutToBeRemovedSignals");
    QTest::addColumn<int>("numberOfColumnsAboutToBeRemovedSignals");
    QTest::addColumn<int>("numberOfRowsRemovedSignals");
    QTest::addColumn<int>("numberOfColumnsRemovedSignals");

    QTest::addColumn<bool>("recursive");
    QTest::addColumn<int>("recursiveRow");
    QTest::addColumn<int>("recursiveCount");

    QTest::addColumn<bool>("shouldSucceed");

#define makeTestRow(testName, start, count, sar, srr, sac, src, r, rr, rc, s) \
        QTest::newRow((t.modelType + testName).toLatin1().data()) << t.modelType << readOnly << isEmpty << \
        start << count << \
        sar << srr << sac << src << \
        r << rr << rc << \
        s;

    for (int i = 0; i < modelsToTest.tests.size(); ++i) {
        ModelsToTest::test t = modelsToTest.tests.at(i);
        QString name = t.modelType;
        bool readOnly = (t.read == ModelsToTest::ReadOnly);
        bool isEmpty = (t.contains == ModelsToTest::Empty);

        // half these
        makeTestRow(":one at the start",  START,  DEFAULTCOUNT, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);
        makeTestRow(":one at the middle", MIDDLE, DEFAULTCOUNT, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);
        makeTestRow(":one at the end",    END,    DEFAULTCOUNT, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);

        makeTestRow(":many at the start",  START,  MANY, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);
        makeTestRow(":many at the middle", MIDDLE, MANY, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);
        makeTestRow(":many at the end",    END,    MANY, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);

        makeTestRow(":remove all",        START,  ALL,  DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);

        makeTestRow(":none at the start",  START,  0, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":none at the middle", MIDDLE, 0, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":none at the end",    END,    0, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);

        makeTestRow(":invalid start, valid count 1", -99,  0,    NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 2", 9999, 0,    NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 3", -99,  1,    NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 4", 9999, 1,    NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 5", -99,  MANY, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 6", 9999, MANY, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);

        makeTestRow(":valid start, invalid count 1",  START,  -2,   NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":valid start, invalid count 2",  MIDDLE, -2,   NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":valid start, invalid count 3",  END,    -2,   NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":valid start, invalid count 4",  START,  9999, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":valid start, invalid count 5",  MIDDLE, 9999, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":valid start, invalid count 6",  END,    9999, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);

        // Recursive remove's might assert, haven't decided yet...
        //makeTestRow(":one at the start recursivly",  START,  DEFAULTCOUNT, 2, DNS, 2, DNS, RECURSIVE, START, DEFAULTCOUNT, FAIL);
        //makeTestRow(":one at the middle recursivly", MIDDLE, DEFAULTCOUNT, 2, DNS, 2, DNS, RECURSIVE, START, DEFAULTCOUNT, SUCCESS);
        //makeTestRow(":one at the end recursivly",    END,    DEFAULTCOUNT, 2, DNS, 2, DNS, RECURSIVE, START, DEFAULTCOUNT, SUCCESS);
    }
}

void tst_QItemModel::remove()
{
    QFETCH(QString, modelType);

    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    QFETCH(bool, readOnly);
    if (readOnly)
        return;

    QFETCH(int, start);
    QFETCH(int, count);

    QFETCH(bool, recursive);
    removeRecursively = recursive;

/*!
    Removes count number of rows starting at start
    if count is -1 it removes all rows
    if start is -1 then it starts at the last row - count
 */
    QFETCH(bool, shouldSucceed);

    // Populate the test area so we can remove something.  See: cleanup()
    // parentOfRemoved is stored so that the slots can make sure parentOfRemoved is the index that is emitted.
    parentOfRemoved = testModels->populateTestArea(currentModel);

    if (count == -1)
        count = currentModel->rowCount(parentOfRemoved);
    if (start == -1)
        start = currentModel->rowCount(parentOfRemoved)-count;

    if (currentModel->rowCount(parentOfRemoved) == 0 ||
        currentModel->columnCount(parentOfRemoved) == 0) {
        qWarning() << "model test area doesn't have any rows or columns, can't fully test remove(). Skipping";
        return;
    }

    // When a row or column is removed there should be two signals.
    // Watch to make sure they are emitted and get the row/column count when they do get emitted by connecting them to a slot
    QSignalSpy columnsAboutToBeRemovedSpy(currentModel, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy rowsAboutToBeRemovedSpy(currentModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy columnsRemovedSpy(currentModel, SIGNAL(columnsRemoved(QModelIndex,int,int)));
    QSignalSpy rowsRemovedSpy(currentModel, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    QSignalSpy modelResetSpy(currentModel, SIGNAL(modelReset()));
    QSignalSpy modelLayoutChangedSpy(currentModel, SIGNAL(layoutChanged()));

    QVERIFY(columnsAboutToBeRemovedSpy.isValid());
    QVERIFY(rowsAboutToBeRemovedSpy.isValid());
    QVERIFY(columnsRemovedSpy.isValid());
    QVERIFY(rowsRemovedSpy.isValid());
    QVERIFY(modelResetSpy.isValid());
    QVERIFY(modelLayoutChangedSpy.isValid());

    QFETCH(int, numberOfRowsAboutToBeRemovedSignals);
    QFETCH(int, numberOfColumnsAboutToBeRemovedSignals);
    QFETCH(int, numberOfRowsRemovedSignals);
    QFETCH(int, numberOfColumnsRemovedSignals);

    //
    // test removeRow()
    //
    connect(currentModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(slot_rowsAboutToRemove(QModelIndex)));
    connect(currentModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(slot_rowsRemoved(QModelIndex)));
    int beforeRemoveRowCount = currentModel->rowCount(parentOfRemoved);
    QPersistentModelIndex dyingIndex = currentModel->index(start + count + 1, 0, parentOfRemoved);
    QCOMPARE(currentModel->removeRows(start, count, parentOfRemoved), shouldSucceed);
    currentModel->submit();
    if (shouldSucceed && dyingIndex.isValid())
        QCOMPARE(dyingIndex.row(), start + 1);

    if (rowsAboutToBeRemovedSpy.count() > 0){
        QList<QVariant> arguments = rowsAboutToBeRemovedSpy.at(0);
        QModelIndex parent = (qvariant_cast<QModelIndex>(arguments.at(0)));
        int first = arguments.at(1).toInt();
        int last = arguments.at(2).toInt();
        QCOMPARE(first, start);
        QCOMPARE(last,  start + count - 1);
        QVERIFY(parentOfRemoved == parent);
    }

    if (rowsRemovedSpy.count() > 0){
        QList<QVariant> arguments = rowsRemovedSpy.at(0);
        QModelIndex parent = (qvariant_cast<QModelIndex>(arguments.at(0)));
        int first = arguments.at(1).toInt();
        int last = arguments.at(2).toInt();
        QCOMPARE(first, start);
        QCOMPARE(last,  start + count - 1);
        QVERIFY(parentOfRemoved == parent);
    }

    // Only the row signals should have been emitted
    if (modelResetSpy.count() >= 1 || modelLayoutChangedSpy.count() >=1 ){
        QCOMPARE(columnsAboutToBeRemovedSpy.count(), 0);
        QCOMPARE(rowsAboutToBeRemovedSpy.count(), 0);
        QCOMPARE(columnsRemovedSpy.count(), 0);
        QCOMPARE(rowsRemovedSpy.count(), 0);
    }
    else {
        QCOMPARE(columnsAboutToBeRemovedSpy.count(), 0);
        QCOMPARE(rowsAboutToBeRemovedSpy.count(), numberOfRowsAboutToBeRemovedSignals);
        QCOMPARE(columnsRemovedSpy.count(), 0);
        QCOMPARE(rowsRemovedSpy.count(), numberOfRowsRemovedSignals);
    }

    // The row count should only change *after* rowsAboutToBeRemoved has been emitted
    if (shouldSucceed) {
        if (modelResetSpy.count() == 0 && modelLayoutChangedSpy.count() == 0){
            QCOMPARE(afterAboutToRemoveRowCount, beforeRemoveRowCount);
            QCOMPARE(afterRemoveRowCount, beforeRemoveRowCount-count-(numberOfRowsRemovedSignals-1));
        }
        if (modelResetSpy.count() == 0 )
            QCOMPARE(currentModel->rowCount(parentOfRemoved), beforeRemoveRowCount-count-(numberOfRowsRemovedSignals-1));
    }
    else {
        if (recursive)
            QCOMPARE(currentModel->rowCount(parentOfRemoved), beforeRemoveRowCount-1);
        else
            QCOMPARE(currentModel->rowCount(parentOfRemoved), beforeRemoveRowCount);

    }
    disconnect(currentModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(slot_rowsAboutToRemove(QModelIndex)));
    disconnect(currentModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(slot_rowsRemoved(QModelIndex)));
    modelResetSpy.clear();
    QCOMPARE(modelResetSpy.count(), 0);

    //
    // Test remove column
    //
    connect(currentModel, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(slot_columnsAboutToRemove(QModelIndex)));
    connect(currentModel, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            this, SLOT(slot_columnsRemoved(QModelIndex)));
    int beforeRemoveColumnCount = currentModel->columnCount(parentOfRemoved);

    // Some models don't let you remove the column, only row
    if (currentModel->removeColumns(start, count, parentOfRemoved)) {
        currentModel->submit();
        // Didn't reset the rows, so they should still be at the same value
        if (modelResetSpy.count() >= 1 || modelLayoutChangedSpy.count() >= 1){
            QCOMPARE(columnsAboutToBeRemovedSpy.count(), 0);
            //QCOMPARE(rowsAboutToBeRemovedSpy.count(), numberOfRowsAboutToBeRemovedSignals);
            QCOMPARE(columnsRemovedSpy.count(), 0);
            //QCOMPARE(rowsRemovedSpy.count(), numberOfRowsRemovedSignals);
        }
        else {
            QCOMPARE(columnsAboutToBeRemovedSpy.count(), numberOfColumnsAboutToBeRemovedSignals);
            QCOMPARE(rowsAboutToBeRemovedSpy.count(), numberOfRowsAboutToBeRemovedSignals);
            QCOMPARE(columnsRemovedSpy.count(), numberOfColumnsRemovedSignals);
            QCOMPARE(rowsRemovedSpy.count(), numberOfRowsRemovedSignals);
        }

        // The column count should only change *after* rowsAboutToBeRemoved has been emitted
        if (shouldSucceed) {
            if (modelResetSpy.count() == 0 && modelLayoutChangedSpy.count() == 0){
                QCOMPARE(afterAboutToRemoveColumnCount, beforeRemoveColumnCount);
                QCOMPARE(afterRemoveColumnCount, beforeRemoveColumnCount-count-(numberOfColumnsRemovedSignals-1));
            }
            if (modelResetSpy.count() == 0)
                QCOMPARE(currentModel->columnCount(parentOfRemoved), beforeRemoveColumnCount-count-(numberOfColumnsRemovedSignals-1));
        }
        else
            QCOMPARE(currentModel->rowCount(parentOfRemoved), beforeRemoveRowCount);
    }
    disconnect(currentModel, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(slot_columnsAboutToRemove(QModelIndex)));
    disconnect(currentModel, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            this, SLOT(slot_columnsRemoved(QModelIndex)));

    if (columnsAboutToBeRemovedSpy.count() > 0){
        QList<QVariant> arguments = columnsAboutToBeRemovedSpy.at(0);
        QModelIndex parent = (qvariant_cast<QModelIndex>(arguments.at(0)));
        int first = arguments.at(1).toInt();
        int last = arguments.at(2).toInt();
        QCOMPARE(first, start);
        QCOMPARE(last,  start + count - 1);
        QVERIFY(parentOfRemoved == parent);
    }

    if (columnsRemovedSpy.count() > 0){
        QList<QVariant> arguments = columnsRemovedSpy.at(0);
        QModelIndex parent = (qvariant_cast<QModelIndex>(arguments.at(0)));
        int first = arguments.at(1).toInt();
        int last = arguments.at(2).toInt();
        QCOMPARE(first, start);
        QCOMPARE(last,  start + count - 1);
        QVERIFY(parentOfRemoved == parent);
    }

    // Cleanup the test area because remove::() is called multiple times in a test
    testModels->cleanupTestArea(currentModel);
}

/*!
    Developers like to use the slots to then *do* something on the model so it needs to be
    in a working state.
 */
void verifyState(QAbstractItemModel *currentModel) {
    // Make sure the model isn't confused right now and still knows what is root
    if (currentModel->hasChildren()) {
        QCOMPARE(currentModel->hasIndex(0, 0), true);
        QModelIndex index = currentModel->index(0,0);
        QCOMPARE(index.isValid(), true);
        QCOMPARE(currentModel->parent(index).isValid(), false);
    } else {
        QModelIndex index = currentModel->index(0,0);
        QCOMPARE(index.isValid(), false);
    }
}

void tst_QItemModel::slot_rowsAboutToRemove(const QModelIndex &parent)
{
    QVERIFY(parentOfRemoved == parent);
    afterAboutToRemoveRowCount = currentModel->rowCount(parent);
    // hasChildren() should still work
    if (afterAboutToRemoveRowCount > 0)
        QCOMPARE(currentModel->hasChildren(parent), true);
    else
        QCOMPARE(currentModel->hasChildren(parent), false);

    verifyState(currentModel);

    // This does happen
    if (removeRecursively) {
        QFETCH(int, recursiveRow);
        QFETCH(int, recursiveCount);
        removeRecursively = false;
        QCOMPARE(currentModel->removeRows(recursiveRow, recursiveCount, parent), true);
    }
}

void tst_QItemModel::slot_rowsRemoved(const QModelIndex &parent)
{
    QVERIFY(parentOfRemoved == parent);
    afterRemoveRowCount = currentModel->rowCount(parent);
    if (afterRemoveRowCount > 0)
        QCOMPARE(currentModel->hasChildren(parent), true);
    else
        QCOMPARE(currentModel->hasChildren(parent), false);

    verifyState(currentModel);
}

void tst_QItemModel::slot_columnsAboutToRemove(const QModelIndex &parent)
{
    QVERIFY(parentOfRemoved == parent);
    afterAboutToRemoveColumnCount = currentModel->columnCount(parent);
    // hasChildren() should still work
    if (afterAboutToRemoveColumnCount > 0 && currentModel->rowCount(parent) > 0)
        QCOMPARE(currentModel->hasChildren(parent), true);
    else
        QCOMPARE(currentModel->hasChildren(parent), false);

    verifyState(currentModel);
}

void tst_QItemModel::slot_columnsRemoved(const QModelIndex &parent)
{
    QVERIFY(parentOfRemoved == parent);
    afterRemoveColumnCount = currentModel->columnCount(parent);
    if (afterRemoveColumnCount > 0)
        QCOMPARE(currentModel->hasChildren(parent), true);
    else
        QCOMPARE(currentModel->hasChildren(parent), false);

    verifyState(currentModel);
}

/*!
    Tests the model's insertRow/Column()
 */
void tst_QItemModel::insert_data()
{
    ModelsToTest modelsToTest;
    QTest::addColumn<QString>("modelType");
    QTest::addColumn<bool>("readOnly");
    QTest::addColumn<bool>("isEmpty");

    QTest::addColumn<int>("start");
    QTest::addColumn<int>("count");

    QTest::addColumn<int>("numberOfRowsAboutToBeInsertedSignals");
    QTest::addColumn<int>("numberOfColumnsAboutToBeInsertedSignals");
    QTest::addColumn<int>("numberOfRowsInsertedSignals");
    QTest::addColumn<int>("numberOfColumnsInsertedSignals");

    QTest::addColumn<bool>("recursive");
    QTest::addColumn<int>("recursiveRow");
    QTest::addColumn<int>("recursiveCount");

    QTest::addColumn<bool>("shouldSucceed");

#define makeTestRow(testName, start, count, sar, srr, sac, src, r, rr, rc, s) \
        QTest::newRow((t.modelType + testName).toLatin1().data()) << t.modelType << readOnly << isEmpty << \
        start << count << \
        sar << srr << sac << src << \
        r << rr << rc << \
        s;

    for (int i = 0; i < modelsToTest.tests.size(); ++i) {
        ModelsToTest::test t = modelsToTest.tests.at(i);
        QString name = t.modelType;
        bool readOnly = (t.read == ModelsToTest::ReadOnly);
        bool isEmpty = (t.contains == ModelsToTest::Empty);

        // half these
        makeTestRow(":one at the start",  START,  DEFAULTCOUNT, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);
        makeTestRow(":one at the middle", MIDDLE, DEFAULTCOUNT, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);
        makeTestRow(":one at the end",    END,    DEFAULTCOUNT, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);

        makeTestRow(":many at the start",  START,  MANY, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);
        makeTestRow(":many at the middle", MIDDLE, MANY, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);
        makeTestRow(":many at the end",    END,    MANY, DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);

        makeTestRow(":add row count",        START,  ALL,  DNS, DNS, DNS, DNS, !RECURSIVE, 0, 0, SUCCESS);

        makeTestRow(":none at the start",  START,  0, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":none at the middle", MIDDLE, 0, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":none at the end",    END,    0, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);

        makeTestRow(":invalid start, valid count 1", -99,  0,    NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 2", 9999, 0,    NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 3", -99,  1,    NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 4", 9999, 1,    NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 5", -99,  MANY, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":invalid start, valid count 6", 9999, MANY, NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);

        makeTestRow(":valid start, invalid count 1",  START,  -2,   NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":valid start, invalid count 2",  MIDDLE, -2,   NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);
        makeTestRow(":valid start, invalid count 3",  END,    -2,   NOSIGNALS, NOSIGNALS, NOSIGNALS, NOSIGNALS, !RECURSIVE, 0, 0, FAIL);

        // Recursive insert's might assert, haven't decided yet...
        //makeTestRow(":one at the start recursivly",  START,  DEFAULTCOUNT, 2, DNS, 2, DNS, RECURSIVE, START, DEFAULTCOUNT, FAIL);
        //makeTestRow(":one at the middle recursivly", MIDDLE, DEFAULTCOUNT, 2, DNS, 2, DNS, RECURSIVE, START, DEFAULTCOUNT, SUCCESS);
        //makeTestRow(":one at the end recursivly",    END,    DEFAULTCOUNT, 2, DNS, 2, DNS, RECURSIVE, START, DEFAULTCOUNT, SUCCESS);
    }
}

void tst_QItemModel::insert()
{
    QFETCH(QString, modelType);
    currentModel = testModels->createModel(modelType);
    QVERIFY(currentModel);

    QFETCH(bool, readOnly);
    if (readOnly)
        return;

    QFETCH(int, start);
    QFETCH(int, count);

    QFETCH(bool, recursive);
    insertRecursively = recursive;

/*!
    Inserts count number of rows starting at start
    if count is -1 it inserts all rows
    if start is -1 then it starts at the last row - count
 */
    QFETCH(bool, shouldSucceed);

    // Populate the test area so we can insert something.  See: cleanup()
    // parentOfInserted is stored so that the slots can make sure parentOfInserted is the index that is emitted.
    parentOfInserted = testModels->populateTestArea(currentModel);

    if (count == -1)
        count = currentModel->rowCount(parentOfInserted);
    if (start == -1)
        start = currentModel->rowCount(parentOfInserted)-count;

    if (currentModel->rowCount(parentOfInserted) == 0 ||
        currentModel->columnCount(parentOfInserted) == 0) {
        qWarning() << "model test area doesn't have any rows, can't fully test insert(). Skipping";
        return;
    }

    // When a row or column is inserted there should be two signals.
    // Watch to make sure they are emitted and get the row/column count when they do get emitted by connecting them to a slot
    QSignalSpy columnsAboutToBeInsertedSpy(currentModel, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy rowsAboutToBeInsertedSpy(currentModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy columnsInsertedSpy(currentModel, SIGNAL(columnsInserted(QModelIndex,int,int)));
    QSignalSpy rowsInsertedSpy(currentModel, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy modelResetSpy(currentModel, SIGNAL(modelReset()));
    QSignalSpy modelLayoutChangedSpy(currentModel, SIGNAL(layoutChanged()));

    QVERIFY(columnsAboutToBeInsertedSpy.isValid());
    QVERIFY(rowsAboutToBeInsertedSpy.isValid());
    QVERIFY(columnsInsertedSpy.isValid());
    QVERIFY(rowsInsertedSpy.isValid());
    QVERIFY(modelResetSpy.isValid());
    QVERIFY(modelLayoutChangedSpy.isValid());

    QFETCH(int, numberOfRowsAboutToBeInsertedSignals);
    QFETCH(int, numberOfColumnsAboutToBeInsertedSignals);
    QFETCH(int, numberOfRowsInsertedSignals);
    QFETCH(int, numberOfColumnsInsertedSignals);

    //
    // test insertRow()
    //
    connect(currentModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
            this, SLOT(slot_rowsAboutToInserted(QModelIndex)));
    connect(currentModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(slot_rowsInserted(QModelIndex)));
    int beforeInsertRowCount = currentModel->rowCount(parentOfInserted);
    QCOMPARE(currentModel->insertRows(start, count, parentOfInserted), shouldSucceed);
    currentModel->submit();

    if (rowsAboutToBeInsertedSpy.count() > 0){
        QList<QVariant> arguments = rowsAboutToBeInsertedSpy.at(0);
        QModelIndex parent = (qvariant_cast<QModelIndex>(arguments.at(0)));
        int first = arguments.at(1).toInt();
        int last = arguments.at(2).toInt();
        QCOMPARE(first, start);
        QCOMPARE(last,  start + count - 1);
        QVERIFY(parentOfInserted == parent);
    }

    if (rowsInsertedSpy.count() > 0){
        QList<QVariant> arguments = rowsInsertedSpy.at(0);
        QModelIndex parent = (qvariant_cast<QModelIndex>(arguments.at(0)));
        int first = arguments.at(1).toInt();
        int last = arguments.at(2).toInt();
        QCOMPARE(first, start);
        QCOMPARE(last,  start + count - 1);
        QVERIFY(parentOfInserted == parent);
    }

    // Only the row signals should have been emitted
    if (modelResetSpy.count() >= 1 || modelLayoutChangedSpy.count() >= 1) {
        QCOMPARE(columnsAboutToBeInsertedSpy.count(), 0);
        QCOMPARE(rowsAboutToBeInsertedSpy.count(), 0);
        QCOMPARE(columnsInsertedSpy.count(), 0);
        QCOMPARE(rowsInsertedSpy.count(), 0);
    }
    else {
        QCOMPARE(columnsAboutToBeInsertedSpy.count(), 0);
        QCOMPARE(rowsAboutToBeInsertedSpy.count(), numberOfRowsAboutToBeInsertedSignals);
        QCOMPARE(columnsInsertedSpy.count(), 0);
        QCOMPARE(rowsInsertedSpy.count(), numberOfRowsInsertedSignals);
    }
    // The row count should only change *after* rowsAboutToBeInserted has been emitted
    if (shouldSucceed) {
        if (modelResetSpy.count() == 0 && modelLayoutChangedSpy.count() == 0) {
            QCOMPARE(afterAboutToInsertRowCount, beforeInsertRowCount);
            QCOMPARE(afterInsertRowCount, beforeInsertRowCount+count+(numberOfRowsInsertedSignals-1));
        }
        if (modelResetSpy.count() == 0)
            QCOMPARE(currentModel->rowCount(parentOfInserted), beforeInsertRowCount+count+(numberOfRowsInsertedSignals-1));
    }
    else {
        if (recursive)
            QCOMPARE(currentModel->rowCount(parentOfInserted), beforeInsertRowCount+1);
        else
            QCOMPARE(currentModel->rowCount(parentOfInserted), beforeInsertRowCount);

    }
    disconnect(currentModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
            this, SLOT(slot_rowsAboutToInserted(QModelIndex)));
    disconnect(currentModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(slot_rowsInserted(QModelIndex)));
    modelResetSpy.clear();

    //
    // Test insertColumn()
    //
    connect(currentModel, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            this, SLOT(slot_columnsAboutToInserted(QModelIndex)));
    connect(currentModel, SIGNAL(columnsInserted(QModelIndex,int,int)),
            this, SLOT(slot_columnsInserted(QModelIndex)));
    int beforeInsertColumnCount = currentModel->columnCount(parentOfInserted);

    // Some models don't let you insert the column, only row
    if (currentModel->insertColumns(start, count, parentOfInserted)) {
        currentModel->submit();
        if (modelResetSpy.count() >= 1 || modelLayoutChangedSpy.count() >= 1) {
            // Didn't reset the rows, so they should still be at the same value
            QCOMPARE(columnsAboutToBeInsertedSpy.count(), 0);
            //QCOMPARE(rowsAboutToBeInsertedSpy.count(), numberOfRowsAboutToBeInsertedSignals);
            QCOMPARE(columnsInsertedSpy.count(), 0);
            //QCOMPARE(rowsInsertedSpy.count(), numberOfRowsInsertedSignals);
        }
        else {
            // Didn't reset the rows, so they should still be at the same value
            QCOMPARE(columnsAboutToBeInsertedSpy.count(), numberOfColumnsAboutToBeInsertedSignals);
            QCOMPARE(rowsAboutToBeInsertedSpy.count(), numberOfRowsAboutToBeInsertedSignals);
            QCOMPARE(columnsInsertedSpy.count(), numberOfColumnsInsertedSignals);
            QCOMPARE(rowsInsertedSpy.count(), numberOfRowsInsertedSignals);
        }
        // The column count should only change *after* rowsAboutToBeInserted has been emitted
        if (shouldSucceed) {
            if (modelResetSpy.count() == 0 &&  modelLayoutChangedSpy.count() == 0) {
                QCOMPARE(afterAboutToInsertColumnCount, beforeInsertColumnCount);
                QCOMPARE(afterInsertColumnCount, beforeInsertColumnCount+count+(numberOfColumnsInsertedSignals-1));
            }
            if (modelResetSpy.count() == 0)
                QCOMPARE(currentModel->columnCount(parentOfInserted), beforeInsertColumnCount+count+(numberOfColumnsInsertedSignals-1));
        }
        else
            QCOMPARE(currentModel->rowCount(parentOfInserted), beforeInsertRowCount);
    }
    disconnect(currentModel, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            this, SLOT(slot_columnsAboutToInserted(QModelIndex)));
    disconnect(currentModel, SIGNAL(columnsInserted(QModelIndex,int,int)),
            this, SLOT(slot_columnsInserted(QModelIndex)));

    if (columnsAboutToBeInsertedSpy.count() > 0){
        QList<QVariant> arguments = columnsAboutToBeInsertedSpy.at(0);
        QModelIndex parent = (qvariant_cast<QModelIndex>(arguments.at(0)));
        int first = arguments.at(1).toInt();
        int last = arguments.at(2).toInt();
        QCOMPARE(first, start);
        QCOMPARE(last,  start + count - 1);
        QVERIFY(parentOfInserted == parent);
    }

    if (columnsInsertedSpy.count() > 0){
        QList<QVariant> arguments = columnsInsertedSpy.at(0);
        QModelIndex parent = (qvariant_cast<QModelIndex>(arguments.at(0)));
        int first = arguments.at(1).toInt();
        int last = arguments.at(2).toInt();
        QCOMPARE(first, start);
        QCOMPARE(last,  start + count - 1);
        QVERIFY(parentOfInserted == parent);
    }

    // Cleanup the test area because insert::() is called multiple times in a test
    testModels->cleanupTestArea(currentModel);
}

void tst_QItemModel::slot_rowsAboutToInserted(const QModelIndex &parent)
{
    QVERIFY(parentOfInserted == parent);
    afterAboutToInsertRowCount = currentModel->rowCount(parent);
    bool hasChildren = currentModel->hasChildren(parent);
    bool hasDimensions = currentModel->columnCount(parent) > 0 && currentModel->rowCount(parent) > 0;
    QCOMPARE(hasChildren, hasDimensions);
    verifyState(currentModel);

    // This does happen
    if (insertRecursively) {
        QFETCH(int, recursiveRow);
        QFETCH(int, recursiveCount);
        insertRecursively = false;
        QCOMPARE(currentModel->insertRows(recursiveRow, recursiveCount, parent), true);
    }
}

void tst_QItemModel::slot_rowsInserted(const QModelIndex &parent)
{
    QVERIFY(parentOfInserted == parent);
    afterInsertRowCount = currentModel->rowCount(parent);
    bool hasChildren = currentModel->hasChildren(parent);
    bool hasDimensions = currentModel->columnCount(parent) > 0 && currentModel->rowCount(parent) > 0;
    QCOMPARE(hasChildren, hasDimensions);
    verifyState(currentModel);
}

void tst_QItemModel::slot_columnsAboutToInserted(const QModelIndex &parent)
{
    QVERIFY(parentOfInserted == parent);
    afterAboutToInsertColumnCount = currentModel->columnCount(parent);
    bool hasChildren = currentModel->hasChildren(parent);
    bool hasDimensions = currentModel->columnCount(parent) > 0 && currentModel->rowCount(parent) > 0;
    QCOMPARE(hasChildren, hasDimensions);
    verifyState(currentModel);
}

void tst_QItemModel::slot_columnsInserted(const QModelIndex &parent)
{
    QVERIFY(parentOfInserted == parent);
    afterInsertColumnCount = currentModel->columnCount(parent);
    bool hasChildren = currentModel->hasChildren(parent);
    bool hasDimensions = currentModel->columnCount(parent) > 0 && currentModel->rowCount(parent) > 0;
    QCOMPARE(hasChildren, hasDimensions);
    verifyState(currentModel);
}

QTEST_MAIN(tst_QItemModel)
#include "tst_qitemmodel.moc"
