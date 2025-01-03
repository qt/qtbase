// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractitemmodeltester.h"

#include <private/qobject_p.h>
#include <private/qabstractitemmodel_p.h>
#include <QtCore/QPointer>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QStack>
#include <QTest>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcModelTest, "qt.modeltest")

#define MODELTESTER_VERIFY(statement) \
do { \
    if (!verify(static_cast<bool>(statement), #statement, "", __FILE__, __LINE__)) \
        return; \
} while (false)

#define MODELTESTER_COMPARE(actual, expected) \
do { \
    if (!compare((actual), (expected), #actual, #expected, __FILE__, __LINE__)) \
        return; \
} while (false)

class QAbstractItemModelTesterPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractItemModelTester)
public:
    QAbstractItemModelTesterPrivate(QAbstractItemModel *model, QAbstractItemModelTester::FailureReportingMode failureReportingMode);

    void nonDestructiveBasicTest();
    void rowAndColumnCount();
    void hasIndex();
    void index();
    void parent();
    void data();

    void runAllTests();

    void layoutAboutToBeChanged();
    void layoutChanged();

    void modelAboutToBeReset();
    void modelReset();

    void columnsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void columnsInserted(const QModelIndex &parent, int first, int last);
    void columnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                               const QModelIndex &destinationParent, int destinationColumn);
    void columnsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination,
                      int column);
    void columnsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void columnsRemoved(const QModelIndex &parent, int first, int last);

    void rowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                            const QModelIndex &destinationParent, int destinationRow);
    void rowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination,
                   int row);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void headerDataChanged(Qt::Orientation orientation, int start, int end);

private:
    void checkChildren(const QModelIndex &parent, int currentDepth = 0);

    bool verify(bool statement, const char *statementStr, const char *description, const char *file, int line);

    template<typename T1, typename T2>
    bool compare(const T1 &t1, const T2 &t2,
                 const char *actual, const char *expected,
                 const char *file, int line);

    QPointer<QAbstractItemModel> model;
    QAbstractItemModelTester::FailureReportingMode failureReportingMode;

    struct Changing {
        QModelIndex parent;
        int oldSize;
        QVariant last;
        QVariant next;
    };
    QStack<Changing> insert;
    QStack<Changing> remove;

    bool useFetchMore = true;
    bool fetchingMore;

    enum class ChangeInFlight {
        None,
        ColumnsInserted,
        ColumnsMoved,
        ColumnsRemoved,
        LayoutChanged,
        ModelReset,
        RowsInserted,
        RowsMoved,
        RowsRemoved
    };
    ChangeInFlight changeInFlight = ChangeInFlight::None;

    QList<QPersistentModelIndex> changing;
};

/*!
    \class QAbstractItemModelTester
    \since 5.11
    \inmodule QtTest

    \brief The QAbstractItemModelTester class helps testing QAbstractItemModel subclasses.

    The QAbstractItemModelTester class is a utility class to test item models.

    When implementing an item model (that is, a concrete QAbstractItemModel
    subclass) one must abide to a very strict set of rules that ensure
    consistency for users of the model (views, proxy models, and so on).

    For instance, for a given index, a model's reimplementation of
    \l{QAbstractItemModel::hasChildren()}{hasChildren()} must be consistent
    with the values returned by \l{QAbstractItemModel::rowCount()}{rowCount()}
    and \l{QAbstractItemModel::columnCount()}{columnCount()}.

    QAbstractItemModelTester helps catching the most common errors in custom
    item model classes. By performing a series of tests, it
    will try to check that the model status is consistent at all times. The
    tests will be repeated automatically every time the model is modified.

    QAbstractItemModelTester employs non-destructive tests, which typically
    consist in reading data and metadata out of a given item model.
    QAbstractItemModelTester will also attempt illegal modifications of
    the model. In models which are properly implemented, such attempts
    should be rejected, and no data should be changed as a consequence.

    \section1 Usage

    Using QAbstractItemModelTester is straightforward. In a \l{Qt Test Overview}{test case}
    it is sufficient to create an instance, passing the model that
    needs to be tested to the constructor:

    \code
    MyModel *modelToBeTested = ...;
    auto tester = new QAbstractItemModelTester(modelToBeTested);
    \endcode

    QAbstractItemModelTester will report testing failures through the
    Qt Test logging mechanisms.

    It is also possible to use QAbstractItemModelTester outside of a test case.
    For instance, it may be useful to test an item model used by an application
    without the need of building an explicit unit test for such a model (which
    might be challenging). In order to use QAbstractItemModelTester outside of
    a test case, pass one of the \c QAbstractItemModelTester::FailureReportingMode
    enumerators to its constructor, therefore specifying how failures should
    be logged.

    QAbstractItemModelTester may also report additional debugging information
    as logging messages under the \c qt.modeltest logging category. Such
    debug logging is disabled by default; refer to the
    QLoggingCategory documentation to learn how to enable it.

    \note While QAbstractItemModelTester is a valid help for development and
    testing of custom item models, it does not (and cannot) catch all possible
    problems in QAbstractItemModel subclasses. Notably, it will never perform
    meaningful destructive testing of a model, which must be therefore tested
    separately.

    \sa {Model/View Programming}, QAbstractItemModel
*/

/*!
    \enum QAbstractItemModelTester::FailureReportingMode

    This enumeration specifies how QAbstractItemModelTester should report
    a failure when it tests a QAbstractItemModel subclass.

    \value QtTest The failures will be reported as QtTest test failures.

    \value Warning The failures will be reported as
    warning messages in the \c{qt.modeltest} logging category.

    \value Fatal A failure will cause immediate and
    abnormal program termination. The reason for the failure will be reported
    using \c{qFatal()}.
*/

/*!
    Creates a model tester instance, with the given \a parent, that will test
    the model \a model.
*/
QAbstractItemModelTester::QAbstractItemModelTester(QAbstractItemModel *model, QObject *parent)
    : QAbstractItemModelTester(model, FailureReportingMode::QtTest, parent)
{
}

/*!
    Creates a model tester instance, with the given \a parent, that will test
    the model \a model, using the specified \a mode to report test failures.

    \sa QAbstractItemModelTester::FailureReportingMode
*/
QAbstractItemModelTester::QAbstractItemModelTester(QAbstractItemModel *model, FailureReportingMode mode, QObject *parent)
    : QObject(*new QAbstractItemModelTesterPrivate(model, mode), parent)
{
    if (!model)
        qFatal("%s: model must not be null", Q_FUNC_INFO);

    Q_D(QAbstractItemModelTester);

    auto runAllTests = [d] { d->runAllTests(); };

    connect(model, &QAbstractItemModel::columnsAboutToBeInserted,
            this, runAllTests);
    connect(model, &QAbstractItemModel::columnsAboutToBeRemoved,
            this, runAllTests);
    connect(model, &QAbstractItemModel::columnsInserted,
            this, runAllTests);
    connect(model, &QAbstractItemModel::columnsRemoved,
            this, runAllTests);
    connect(model, &QAbstractItemModel::dataChanged,
            this, runAllTests);
    connect(model, &QAbstractItemModel::headerDataChanged,
            this, runAllTests);
    connect(model, &QAbstractItemModel::layoutAboutToBeChanged,
            this, runAllTests);
    connect(model, &QAbstractItemModel::layoutChanged,
            this, runAllTests);
    connect(model, &QAbstractItemModel::modelReset,
            this, runAllTests);
    connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, runAllTests);
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, runAllTests);
    connect(model, &QAbstractItemModel::rowsInserted,
            this, runAllTests);
    connect(model, &QAbstractItemModel::rowsRemoved,
            this, runAllTests);

    // Special checks for changes
    connect(model, &QAbstractItemModel::layoutAboutToBeChanged,
            this, [d]{ d->layoutAboutToBeChanged(); });
    connect(model, &QAbstractItemModel::layoutChanged,
            this, [d]{ d->layoutChanged(); });

    // column operations
    connect(model, &QAbstractItemModel::columnsAboutToBeInserted,
            this, [d](const QModelIndex &parent, int start, int end) { d->columnsAboutToBeInserted(parent, start, end); });
    connect(model, &QAbstractItemModel::columnsAboutToBeMoved,
            this, [d](const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationColumn) {
                d->columnsAboutToBeMoved(sourceParent, sourceStart, sourceEnd, destinationParent, destinationColumn); });
    connect(model, &QAbstractItemModel::columnsAboutToBeRemoved,
            this, [d](const QModelIndex &parent, int start, int end) { d->columnsAboutToBeRemoved(parent, start, end); });
    connect(model, &QAbstractItemModel::columnsInserted,
            this, [d](const QModelIndex &parent, int start, int end) { d->columnsInserted(parent, start, end); });
    connect(model, &QAbstractItemModel::columnsMoved,
            this, [d](const QModelIndex &parent, int start, int end, const QModelIndex &destination, int col) {
                d->columnsMoved(parent, start, end, destination, col); });
    connect(model, &QAbstractItemModel::columnsRemoved,
            this, [d](const QModelIndex &parent, int start, int end) { d->columnsRemoved(parent, start, end); });

    // row operations
    connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, [d](const QModelIndex &parent, int start, int end) { d->rowsAboutToBeInserted(parent, start, end); });
    connect(model, &QAbstractItemModel::rowsAboutToBeMoved,
            this, [d](const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow) {
        d->rowsAboutToBeMoved(sourceParent, sourceStart, sourceEnd, destinationParent, destinationRow); });
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, [d](const QModelIndex &parent, int start, int end) { d->rowsAboutToBeRemoved(parent, start, end); });
    connect(model, &QAbstractItemModel::rowsInserted,
            this, [d](const QModelIndex &parent, int start, int end) { d->rowsInserted(parent, start, end); });
    connect(model, &QAbstractItemModel::rowsMoved,
            this, [d](const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row) {
        d->rowsMoved(parent, start, end, destination, row); });
    connect(model, &QAbstractItemModel::rowsRemoved,
            this, [d](const QModelIndex &parent, int start, int end) { d->rowsRemoved(parent, start, end); });

    // reset
    connect(model, &QAbstractItemModel::modelAboutToBeReset,
            this, [d]() { d->modelAboutToBeReset(); });
    connect(model, &QAbstractItemModel::modelReset,
            this, [d]() { d->modelReset(); });

    // data
    connect(model, &QAbstractItemModel::dataChanged,
            this, [d](const QModelIndex &topLeft, const QModelIndex &bottomRight) { d->dataChanged(topLeft, bottomRight); });
    connect(model, &QAbstractItemModel::headerDataChanged,
            this, [d](Qt::Orientation orientation, int start, int end) { d->headerDataChanged(orientation, start, end); });

    runAllTests();
}

/*!
    Returns the model that this instance is testing.
*/
QAbstractItemModel *QAbstractItemModelTester::model() const
{
    Q_D(const QAbstractItemModelTester);
    return d->model.data();
}

/*!
    Returns the mode that this instancing is using to report test failures.

    \sa QAbstractItemModelTester::FailureReportingMode
*/
QAbstractItemModelTester::FailureReportingMode QAbstractItemModelTester::failureReportingMode() const
{
    Q_D(const QAbstractItemModelTester);
    return d->failureReportingMode;
}

/*!
    If \a value is true, enables dynamic population of the
    tested model, which is the default.
    If \a value is false, it disables it.

    \since 6.4
    \sa QAbstractItemModel::fetchMore()
*/
void QAbstractItemModelTester::setUseFetchMore(bool value)
{
    Q_D(QAbstractItemModelTester);
    d->useFetchMore = value;
}

bool QAbstractItemModelTester::verify(bool statement, const char *statementStr, const char *description, const char *file, int line)
{
    Q_D(QAbstractItemModelTester);
    return d->verify(statement, statementStr, description, file, line);
}

QAbstractItemModelTesterPrivate::QAbstractItemModelTesterPrivate(QAbstractItemModel *model, QAbstractItemModelTester::FailureReportingMode failureReportingMode)
    : model(model),
      failureReportingMode(failureReportingMode),
      fetchingMore(false)
{
}

void QAbstractItemModelTesterPrivate::runAllTests()
{
    if (fetchingMore)
        return;
    nonDestructiveBasicTest();
    rowAndColumnCount();
    hasIndex();
    index();
    parent();
    data();
}

/*
    nonDestructiveBasicTest tries to call a number of the basic functions (not all)
    to make sure the model doesn't outright segfault, testing the functions that makes sense.
*/
void QAbstractItemModelTesterPrivate::nonDestructiveBasicTest()
{
    MODELTESTER_VERIFY(!model->buddy(QModelIndex()).isValid());
    model->canFetchMore(QModelIndex());
    MODELTESTER_VERIFY(model->columnCount(QModelIndex()) >= 0);
    if (useFetchMore) {
        fetchingMore = true;
        model->fetchMore(QModelIndex());
        fetchingMore = false;
    }
    Qt::ItemFlags flags = model->flags(QModelIndex());
    MODELTESTER_VERIFY(flags == Qt::ItemIsDropEnabled || flags == 0);
    model->hasChildren(QModelIndex());
    const bool hasRow = model->hasIndex(0, 0);
    QVariant cache;
    if (hasRow)
        model->match(model->index(0, 0), -1, cache);
    model->mimeTypes();
    MODELTESTER_VERIFY(!model->parent(QModelIndex()).isValid());
    MODELTESTER_VERIFY(model->rowCount() >= 0);
    model->span(QModelIndex());
    model->supportedDropActions();
    model->roleNames();
}

/*
    Tests model's implementation of QAbstractItemModel::rowCount(),
    columnCount() and hasChildren().

    Models that are dynamically populated are not as fully tested here.
 */
void QAbstractItemModelTesterPrivate::rowAndColumnCount()
{
    if (!model->hasChildren())
        return;

    QModelIndex topIndex = model->index(0, 0, QModelIndex());

    // check top row
    int rows = model->rowCount(topIndex);
    MODELTESTER_VERIFY(rows >= 0);

    int columns = model->columnCount(topIndex);
    MODELTESTER_VERIFY(columns >= 0);

    if (rows == 0 || columns == 0)
        return;

    MODELTESTER_VERIFY(model->hasChildren(topIndex));

    QModelIndex secondLevelIndex = model->index(0, 0, topIndex);
    MODELTESTER_VERIFY(secondLevelIndex.isValid());

    rows = model->rowCount(secondLevelIndex);
    MODELTESTER_VERIFY(rows >= 0);

    columns = model->columnCount(secondLevelIndex);
    MODELTESTER_VERIFY(columns >= 0);

    if (rows == 0 || columns == 0)
        return;

    MODELTESTER_VERIFY(model->hasChildren(secondLevelIndex));

    // rowCount() / columnCount() are tested more extensively in checkChildren()
}

/*
    Tests model's implementation of QAbstractItemModel::hasIndex()
 */
void QAbstractItemModelTesterPrivate::hasIndex()
{
    // Make sure that invalid values returns an invalid index
    MODELTESTER_VERIFY(!model->hasIndex(-2, -2));
    MODELTESTER_VERIFY(!model->hasIndex(-2, 0));
    MODELTESTER_VERIFY(!model->hasIndex(0, -2));

    const int rows = model->rowCount();
    const int columns = model->columnCount();

    // check out of bounds
    MODELTESTER_VERIFY(!model->hasIndex(rows, columns));
    MODELTESTER_VERIFY(!model->hasIndex(rows + 1, columns + 1));

    if (rows > 0 && columns > 0)
        MODELTESTER_VERIFY(model->hasIndex(0, 0));

    // hasIndex() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*
    Tests model's implementation of QAbstractItemModel::index()
 */
void QAbstractItemModelTesterPrivate::index()
{
    const int rows = model->rowCount();
    const int columns = model->columnCount();

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            // Make sure that the same index is *always* returned
            QModelIndex a = model->index(row, column);
            QModelIndex b = model->index(row, column);
            MODELTESTER_VERIFY(a.isValid());
            MODELTESTER_VERIFY(b.isValid());
            MODELTESTER_COMPARE(a, b);
        }
    }

    // index() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*
    Tests model's implementation of QAbstractItemModel::parent()
 */
void QAbstractItemModelTesterPrivate::parent()
{
    // Make sure the model won't crash and will return an invalid QModelIndex
    // when asked for the parent of an invalid index.
    MODELTESTER_VERIFY(!model->parent(QModelIndex()).isValid());

    if (model->rowCount() == 0 || model->columnCount() == 0)
        return;

    // Column 0                | Column 1    |
    // QModelIndex()           |             |
    //    \- topIndex          | topIndex1   |
    //         \- childIndex   | childIndex1 |

    // Common error test #1, make sure that a top level index has a parent
    // that is a invalid QModelIndex.
    QModelIndex topIndex = model->index(0, 0, QModelIndex());
    MODELTESTER_VERIFY(topIndex.isValid());
    MODELTESTER_VERIFY(!model->parent(topIndex).isValid());

    // Common error test #2, make sure that a second level index has a parent
    // that is the first level index.
    if (model->rowCount(topIndex) > 0 && model->columnCount(topIndex) > 0) {
        QModelIndex childIndex = model->index(0, 0, topIndex);
        MODELTESTER_VERIFY(childIndex.isValid());
        MODELTESTER_COMPARE(model->parent(childIndex), topIndex);
    }

    // Common error test #3, the second column should NOT have the same children
    // as the first column in a row.
    // Usually the second column shouldn't have children.
    if (model->hasIndex(0, 1)) {
        QModelIndex topIndex1 = model->index(0, 1, QModelIndex());
        MODELTESTER_VERIFY(topIndex1.isValid());
        if (model->rowCount(topIndex) > 0 && model->rowCount(topIndex1) > 0) {
            QModelIndex childIndex = model->index(0, 0, topIndex);
            MODELTESTER_VERIFY(childIndex.isValid());
            QModelIndex childIndex1 = model->index(0, 0, topIndex1);
            MODELTESTER_VERIFY(childIndex1.isValid());
            MODELTESTER_VERIFY(childIndex != childIndex1);
        }
    }

    // Full test, walk n levels deep through the model making sure that all
    // parent's children correctly specify their parent.
    checkChildren(QModelIndex());
}

/*
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
void QAbstractItemModelTesterPrivate::checkChildren(const QModelIndex &parent, int currentDepth)
{
    // First just try walking back up the tree.
    QModelIndex p = parent;
    while (p.isValid())
        p = p.parent();

    // For models that are dynamically populated
    if (model->canFetchMore(parent) && useFetchMore) {
        fetchingMore = true;
        model->fetchMore(parent);
        fetchingMore = false;
    }

    const int rows = model->rowCount(parent);
    const int columns = model->columnCount(parent);

    if (rows > 0)
        MODELTESTER_VERIFY(model->hasChildren(parent));

    // Some further testing against rows(), columns(), and hasChildren()
    MODELTESTER_VERIFY(rows >= 0);
    MODELTESTER_VERIFY(columns >= 0);
    if (rows > 0 && columns > 0)
        MODELTESTER_VERIFY(model->hasChildren(parent));

    const QModelIndex topLeftChild = model->index(0, 0, parent);

    MODELTESTER_VERIFY(!model->hasIndex(rows, 0, parent));
    MODELTESTER_VERIFY(!model->hasIndex(rows + 1, 0, parent));

    for (int r = 0; r < rows; ++r) {
        MODELTESTER_VERIFY(!model->hasIndex(r, columns, parent));
        MODELTESTER_VERIFY(!model->hasIndex(r, columns + 1, parent));
        for (int c = 0; c < columns; ++c) {
            MODELTESTER_VERIFY(model->hasIndex(r, c, parent));
            QModelIndex index = model->index(r, c, parent);
            // rowCount() and columnCount() said that it existed...
            if (!index.isValid())
                qCWarning(lcModelTest) << "Got invalid index at row=" << r << "col=" << c << "parent=" << parent;
            MODELTESTER_VERIFY(index.isValid());

            // index() should always return the same index when called twice in a row
            QModelIndex modifiedIndex = model->index(r, c, parent);
            MODELTESTER_COMPARE(index, modifiedIndex);

            {
                const QModelIndex sibling = model->sibling(r, c, topLeftChild);
                MODELTESTER_COMPARE(index, sibling);
            }
            {
                const QModelIndex sibling = topLeftChild.sibling(r, c);
                MODELTESTER_COMPARE(index, sibling);
            }

            // Some basic checking on the index that is returned
            MODELTESTER_COMPARE(index.model(), model);
            MODELTESTER_COMPARE(index.row(), r);
            MODELTESTER_COMPARE(index.column(), c);

            // If the next test fails here is some somewhat useful debug you play with.
            if (model->parent(index) != parent) {
                qCWarning(lcModelTest) << "Inconsistent parent() implementation detected:";
                qCWarning(lcModelTest) << "    index=" << index << "exp. parent=" << parent << "act. parent=" << model->parent(index);
                qCWarning(lcModelTest) << "    row=" << r << "col=" << c << "depth=" << currentDepth;
                qCWarning(lcModelTest) << "    data for child" << model->data(index).toString();
                qCWarning(lcModelTest) << "    data for parent" << model->data(parent).toString();
            }

            // Check that we can get back our real parent.
            MODELTESTER_COMPARE(model->parent(index), parent);

            QPersistentModelIndex persistentIndex = index;

            // recursively go down the children
            if (model->hasChildren(index) && currentDepth < 10)
                checkChildren(index, currentDepth + 1);

            // make sure that after testing the children that the index doesn't change.
            QModelIndex newerIndex = model->index(r, c, parent);
            MODELTESTER_COMPARE(persistentIndex, newerIndex);
        }
    }
}

/*
    Tests model's implementation of QAbstractItemModel::data()
 */
void QAbstractItemModelTesterPrivate::data()
{
    if (model->rowCount() == 0 || model->columnCount() == 0)
        return;

    MODELTESTER_VERIFY(model->index(0, 0).isValid());

    // General Purpose roles that should return a QString
    QVariant variant;
    variant = model->data(model->index(0, 0), Qt::DisplayRole);
    if (variant.isValid())
        MODELTESTER_VERIFY(variant.canConvert<QString>());
    variant = model->data(model->index(0, 0), Qt::ToolTipRole);
    if (variant.isValid())
        MODELTESTER_VERIFY(variant.canConvert<QString>());
    variant = model->data(model->index(0, 0), Qt::StatusTipRole);
    if (variant.isValid())
        MODELTESTER_VERIFY(variant.canConvert<QString>());
    variant = model->data(model->index(0, 0), Qt::WhatsThisRole);
    if (variant.isValid())
        MODELTESTER_VERIFY(variant.canConvert<QString>());

    // General Purpose roles that should return a QSize
    variant = model->data(model->index(0, 0), Qt::SizeHintRole);
    if (variant.isValid())
        MODELTESTER_VERIFY(variant.canConvert<QSize>());

    // Check that the alignment is one we know about
    QVariant textAlignmentVariant = model->data(model->index(0, 0), Qt::TextAlignmentRole);
    if (textAlignmentVariant.isValid()) {
        Qt::Alignment alignment = QtPrivate::legacyFlagValueFromModelData<Qt::Alignment>(textAlignmentVariant);
        MODELTESTER_COMPARE(alignment, (alignment & (Qt::AlignHorizontal_Mask | Qt::AlignVertical_Mask)));
    }

    // Check that the "check state" is one we know about.
    QVariant checkStateVariant = model->data(model->index(0, 0), Qt::CheckStateRole);
    if (checkStateVariant.isValid()) {
        Qt::CheckState state = QtPrivate::legacyEnumValueFromModelData<Qt::CheckState>(checkStateVariant);
        MODELTESTER_VERIFY(state == Qt::Unchecked
                || state == Qt::PartiallyChecked
                || state == Qt::Checked);
    }

    Q_Q(QAbstractItemModelTester);

    if (!QTestPrivate::testDataGuiRoles(q))
        return;
}

void QAbstractItemModelTesterPrivate::columnsAboutToBeInserted(const QModelIndex &parent, int start,
                                                               int end)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::None);
    changeInFlight = ChangeInFlight::ColumnsInserted;

    qCDebug(lcModelTest) << "columnsAboutToBeInserted"
                         << "start=" << start << "end=" << end << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent)
                         << "last before insertion=" << model->index(start - 1, 0, parent)
                         << model->data(model->index(start - 1, 0, parent));
}

void QAbstractItemModelTesterPrivate::columnsInserted(const QModelIndex &parent, int first,
                                                      int last)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::ColumnsInserted);
    changeInFlight = ChangeInFlight::None;

    qCDebug(lcModelTest) << "columnsInserted"
                         << "start=" << first << "end=" << last << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent);
}

void QAbstractItemModelTesterPrivate::columnsAboutToBeMoved(const QModelIndex &sourceParent,
                                                            int sourceStart, int sourceEnd,
                                                            const QModelIndex &destinationParent,
                                                            int destinationColumn)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::None);
    changeInFlight = ChangeInFlight::ColumnsMoved;

    qCDebug(lcModelTest) << "columnsAboutToBeMoved"
                         << "sourceStart=" << sourceStart << "sourceEnd=" << sourceEnd
                         << "sourceParent=" << sourceParent
                         << "sourceParent data=" << model->data(sourceParent).toString()
                         << "destinationParent=" << destinationParent
                         << "destinationColumn=" << destinationColumn;
}

void QAbstractItemModelTesterPrivate::columnsMoved(const QModelIndex &sourceParent, int sourceStart,
                                                   int sourceEnd,
                                                   const QModelIndex &destinationParent,
                                                   int destinationColumn)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::ColumnsMoved);
    changeInFlight = ChangeInFlight::None;

    qCDebug(lcModelTest) << "columnsMoved"
                         << "sourceStart=" << sourceStart << "sourceEnd=" << sourceEnd
                         << "sourceParent=" << sourceParent
                         << "sourceParent data=" << model->data(sourceParent).toString()
                         << "destinationParent=" << destinationParent
                         << "destinationColumn=" << destinationColumn;
}

void QAbstractItemModelTesterPrivate::columnsAboutToBeRemoved(const QModelIndex &parent, int first,
                                                              int last)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::None);
    changeInFlight = ChangeInFlight::ColumnsRemoved;

    qCDebug(lcModelTest) << "columnsAboutToBeRemoved"
                         << "start=" << first << "end=" << last << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent)
                         << "last before removal=" << model->index(first - 1, 0, parent)
                         << model->data(model->index(first - 1, 0, parent));
}

void QAbstractItemModelTesterPrivate::columnsRemoved(const QModelIndex &parent, int first, int last)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::ColumnsRemoved);
    changeInFlight = ChangeInFlight::None;

    qCDebug(lcModelTest) << "columnsRemoved"
                         << "start=" << first << "end=" << last << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent);
}

/*
    Store what is about to be inserted to make sure it actually happens

    \sa rowsInserted()
 */
void QAbstractItemModelTesterPrivate::rowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::None);
    changeInFlight = ChangeInFlight::RowsInserted;

    qCDebug(lcModelTest) << "rowsAboutToBeInserted"
                         << "start=" << start << "end=" << end << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent)
                         << "last before insertion=" << model->index(start - 1, 0, parent) << model->data(model->index(start - 1, 0, parent));

    Changing c;
    c.parent = parent;
    c.oldSize = model->rowCount(parent);
    c.last = (start - 1 >= 0) ? model->index(start - 1, 0, parent).data() : QVariant();
    c.next = (start < c.oldSize) ? model->index(start, 0, parent).data() : QVariant();
    insert.push(c);
}

/*
    Confirm that what was said was going to happen actually did

    \sa rowsAboutToBeInserted()
 */
void QAbstractItemModelTesterPrivate::rowsInserted(const QModelIndex &parent, int start, int end)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::RowsInserted);
    changeInFlight = ChangeInFlight::None;

    qCDebug(lcModelTest) << "rowsInserted"
                         << "start=" << start << "end=" << end << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent);

    for (int i = start; i <= end; ++i) {
        qCDebug(lcModelTest) << "    itemWasInserted:" << i
                             << model->index(i, 0, parent).data();
    }


    Changing c = insert.pop();
    MODELTESTER_COMPARE(parent, c.parent);

    MODELTESTER_COMPARE(model->rowCount(parent), c.oldSize + (end - start + 1));
    if (start - 1 >= 0)
        MODELTESTER_COMPARE(model->data(model->index(start - 1, 0, c.parent)), c.last);

    if (end + 1 < model->rowCount(c.parent)) {
        if (c.next != model->data(model->index(end + 1, 0, c.parent))) {
            qDebug() << start << end;
            for (int i = 0; i < model->rowCount(); ++i)
                qDebug() << model->index(i, 0).data().toString();
            qDebug() << c.next << model->data(model->index(end + 1, 0, c.parent));
        }

        MODELTESTER_COMPARE(model->data(model->index(end + 1, 0, c.parent)), c.next);
    }
}

void QAbstractItemModelTesterPrivate::rowsAboutToBeMoved(const QModelIndex &sourceParent,
                                                         int sourceStart, int sourceEnd,
                                                         const QModelIndex &destinationParent,
                                                         int destinationRow)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::None);
    changeInFlight = ChangeInFlight::RowsMoved;

    qCDebug(lcModelTest) << "rowsAboutToBeMoved"
                         << "sourceStart=" << sourceStart << "sourceEnd=" << sourceEnd
                         << "sourceParent=" << sourceParent
                         << "sourceParent data=" << model->data(sourceParent).toString()
                         << "destinationParent=" << destinationParent
                         << "destinationRow=" << destinationRow;
}

void QAbstractItemModelTesterPrivate::rowsMoved(const QModelIndex &sourceParent, int sourceStart,
                                                int sourceEnd, const QModelIndex &destinationParent,
                                                int destinationRow)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::RowsMoved);
    changeInFlight = ChangeInFlight::None;

    qCDebug(lcModelTest) << "rowsMoved"
                         << "sourceStart=" << sourceStart << "sourceEnd=" << sourceEnd
                         << "sourceParent=" << sourceParent
                         << "sourceParent data=" << model->data(sourceParent).toString()
                         << "destinationParent=" << destinationParent
                         << "destinationRow=" << destinationRow;
}

void QAbstractItemModelTesterPrivate::layoutAboutToBeChanged()
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::None);
    changeInFlight = ChangeInFlight::LayoutChanged;

    for (int i = 0; i < qBound(0, model->rowCount(), 100); ++i)
        changing.append(QPersistentModelIndex(model->index(i, 0)));
}

void QAbstractItemModelTesterPrivate::layoutChanged()
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::LayoutChanged);
    changeInFlight = ChangeInFlight::None;

    for (int i = 0; i < changing.size(); ++i) {
        QPersistentModelIndex p = changing[i];
        MODELTESTER_COMPARE(model->index(p.row(), p.column(), p.parent()), QModelIndex(p));
    }
    changing.clear();
}

void QAbstractItemModelTesterPrivate::modelAboutToBeReset()
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::None);
    changeInFlight = ChangeInFlight::ModelReset;
}

void QAbstractItemModelTesterPrivate::modelReset()
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::ModelReset);
    changeInFlight = ChangeInFlight::None;
}

/*
    Store what is about to be inserted to make sure it actually happens

    \sa rowsRemoved()
 */
void QAbstractItemModelTesterPrivate::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::None);
    changeInFlight = ChangeInFlight::RowsRemoved;

    qCDebug(lcModelTest) << "rowsAboutToBeRemoved"
                         << "start=" << start << "end=" << end << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent)
                         << "last before removal=" << model->index(start - 1, 0, parent) << model->data(model->index(start - 1, 0, parent));

    Changing c;
    c.parent = parent;
    c.oldSize = model->rowCount(parent);
    if (start > 0 && model->columnCount(parent) > 0) {
        const QModelIndex startIndex = model->index(start - 1, 0, parent);
        MODELTESTER_VERIFY(startIndex.isValid());
        c.last = model->data(startIndex);
    }
    if (end < c.oldSize - 1 && model->columnCount(parent) > 0) {
        const QModelIndex endIndex = model->index(end + 1, 0, parent);
        MODELTESTER_VERIFY(endIndex.isValid());
        c.next = model->data(endIndex);
    }

    remove.push(c);
}

/*
    Confirm that what was said was going to happen actually did

    \sa rowsAboutToBeRemoved()
 */
void QAbstractItemModelTesterPrivate::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    MODELTESTER_COMPARE(changeInFlight, ChangeInFlight::RowsRemoved);
    changeInFlight = ChangeInFlight::None;

    qCDebug(lcModelTest) << "rowsRemoved"
                         << "start=" << start << "end=" << end << "parent=" << parent
                         << "parent data=" << model->data(parent).toString()
                         << "current count of parent=" << model->rowCount(parent);

    Changing c = remove.pop();
    MODELTESTER_COMPARE(parent, c.parent);
    MODELTESTER_COMPARE(model->rowCount(parent), c.oldSize - (end - start + 1));
    if (start > 0)
        MODELTESTER_COMPARE(model->data(model->index(start - 1, 0, c.parent)), c.last);
    if (end < c.oldSize - 1)
        MODELTESTER_COMPARE(model->data(model->index(start, 0, c.parent)), c.next);
}

void QAbstractItemModelTesterPrivate::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    MODELTESTER_VERIFY(topLeft.isValid());
    MODELTESTER_VERIFY(bottomRight.isValid());
    QModelIndex commonParent = bottomRight.parent();
    MODELTESTER_COMPARE(topLeft.parent(), commonParent);
    MODELTESTER_VERIFY(topLeft.row() <= bottomRight.row());
    MODELTESTER_VERIFY(topLeft.column() <= bottomRight.column());
    int rowCount = model->rowCount(commonParent);
    int columnCount = model->columnCount(commonParent);
    MODELTESTER_VERIFY(bottomRight.row() < rowCount);
    MODELTESTER_VERIFY(bottomRight.column() < columnCount);
}

void QAbstractItemModelTesterPrivate::headerDataChanged(Qt::Orientation orientation, int start, int end)
{
    MODELTESTER_VERIFY(start >= 0);
    MODELTESTER_VERIFY(end >= 0);
    MODELTESTER_VERIFY(start <= end);
    int itemCount = orientation == Qt::Vertical ? model->rowCount() : model->columnCount();
    MODELTESTER_VERIFY(start < itemCount);
    MODELTESTER_VERIFY(end < itemCount);
}

bool QAbstractItemModelTesterPrivate::verify(bool statement,
                                             const char *statementStr, const char *description,
                                             const char *file, int line)
{
    static const char formatString[] = "FAIL! %s (%s) returned FALSE (%s:%d)";

    switch (failureReportingMode) {
    case QAbstractItemModelTester::FailureReportingMode::QtTest:
        return QTest::qVerify(statement, statementStr, description, file, line);
        break;

    case QAbstractItemModelTester::FailureReportingMode::Warning:
        if (!statement)
            qCWarning(lcModelTest, formatString, statementStr, description, file, line);
        break;

    case QAbstractItemModelTester::FailureReportingMode::Fatal:
        if (!statement)
            qFatal(formatString, statementStr, description, file, line);
        break;
    }

    return statement;
}


template<typename T1, typename T2>
bool QAbstractItemModelTesterPrivate::compare(const T1 &t1, const T2 &t2,
                                              const char *actual, const char *expected,
                                              const char *file, int line)
{
    const bool result = static_cast<bool>(t1 == t2);

    static const char formatString[] = "FAIL! Compared values are not the same:\n   Actual (%s) %s\n   Expected (%s) %s\n   (%s:%d)";

    switch (failureReportingMode) {
    case QAbstractItemModelTester::FailureReportingMode::QtTest:
        return QTest::qCompare(t1, t2, actual, expected, file, line);
        break;

    case QAbstractItemModelTester::FailureReportingMode::Warning:
        if (!result) {
            auto t1string = QTest::toString(t1);
            auto t2string = QTest::toString(t2);
            qCWarning(lcModelTest, formatString, actual, t1string ? t1string : "(nullptr)",
                                                 expected, t2string ? t2string : "(nullptr)",
                                                 file, line);
            delete [] t1string;
            delete [] t2string;
        }
        break;

    case QAbstractItemModelTester::FailureReportingMode::Fatal:
        if (!result) {
            auto t1string = QTest::toString(t1);
            auto t2string = QTest::toString(t2);
            qFatal(formatString, actual, t1string ? t1string : "(nullptr)",
                                 expected, t2string ? t2string : "(nullptr)",
                                 file, line);
            delete [] t1string;
            delete [] t2string;
        }
        break;
    }

    return result;
}


QT_END_NAMESPACE

#include "moc_qabstractitemmodeltester.cpp"
