// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qitemselectionmodel.h"
#include "qitemselectionmodel_p.h"

#include <private/qitemselectionmodel_p.h>
#include <private/qabstractitemmodel_p.h>
#include <private/qduplicatetracker_p.h>
#include <private/qoffsetstringarray_p.h>
#include <qdebug.h>

#include <algorithm>
#include <functional>

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QItemSelectionRange)
QT_IMPL_METATYPE_EXTERN(QItemSelection)

/*!
    \class QItemSelectionRange
    \inmodule QtCore

    \brief The QItemSelectionRange class manages information about a
    range of selected items in a model.

    \ingroup model-view

    A QItemSelectionRange contains information about a range of
    selected items in a model. A range of items is a contiguous array
    of model items, extending to cover a number of adjacent rows and
    columns with a common parent item; this can be visualized as a
    two-dimensional block of cells in a table. A selection range has a
    top(), left() a bottom(), right() and a parent().

    The QItemSelectionRange class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    The model items contained in the selection range can be obtained
    using the indexes() function. Use QItemSelectionModel::selectedIndexes()
    to get a list of all selected items for a view.

    You can determine whether a given model item lies within a
    particular range by using the contains() function. Ranges can also
    be compared using the overloaded operators for equality and
    inequality, and the intersects() function allows you to determine
    whether two ranges overlap.

    \sa {Model/View Programming}, QAbstractItemModel, QItemSelection,
        QItemSelectionModel
*/

/*!
    \fn QItemSelectionRange::QItemSelectionRange()

    Constructs an empty selection range.
*/

/*!
    \fn QItemSelectionRange::QItemSelectionRange(const QModelIndex &topLeft, const QModelIndex &bottomRight)

    Constructs a new selection range containing only the index specified
    by the \a topLeft and the index \a bottomRight.

*/

/*!
    \fn QItemSelectionRange::QItemSelectionRange(const QModelIndex &index)

    Constructs a new selection range containing only the model item specified
    by the model index \a index.
*/

/*!
    \fn QItemSelectionRange::swap(QItemSelectionRange &other)
    \since 5.6

    Swaps this selection range's contents with \a other.
    This function is very fast and never fails.
*/

/*!
    \fn int QItemSelectionRange::top() const

    Returns the row index corresponding to the uppermost selected row in the
    selection range.

*/

/*!
    \fn int QItemSelectionRange::left() const

    Returns the column index corresponding to the leftmost selected column in the
    selection range.
*/

/*!
    \fn int QItemSelectionRange::bottom() const

    Returns the row index corresponding to the lowermost selected row in the
    selection range.

*/

/*!
    \fn int QItemSelectionRange::right() const

    Returns the column index corresponding to the rightmost selected column in
    the selection range.

*/

/*!
    \fn int QItemSelectionRange::width() const

    Returns the number of selected columns in the selection range.

*/

/*!
    \fn int QItemSelectionRange::height() const

    Returns the number of selected rows in the selection range.

*/

/*!
    \fn const QAbstractItemModel *QItemSelectionRange::model() const

    Returns the model that the items in the selection range belong to.
*/

/*!
    \fn QModelIndex QItemSelectionRange::topLeft() const

    Returns the index for the item located at the top-left corner of
    the selection range.

    \sa top(), left(), bottomRight()
*/

/*!
    \fn QModelIndex QItemSelectionRange::bottomRight() const

    Returns the index for the item located at the bottom-right corner
    of the selection range.

    \sa bottom(), right(), topLeft()
*/

/*!
    \fn QModelIndex QItemSelectionRange::parent() const

    Returns the parent model item index of the items in the selection range.

*/

/*!
    \fn bool QItemSelectionRange::contains(const QModelIndex &index) const

    Returns \c true if the model item specified by the \a index lies within the
    range of selected items; otherwise returns \c false.
*/

/*!
    \fn bool QItemSelectionRange::contains(int row, int column,
                                           const QModelIndex &parentIndex) const
    \overload

    Returns \c true if the model item specified by (\a row, \a column)
    and with \a parentIndex as the parent item lies within the range
    of selected items; otherwise returns \c false.
*/

/*!
    \fn bool QItemSelectionRange::intersects(const QItemSelectionRange &other) const

    Returns \c true if this selection range intersects (overlaps with) the \a other
    range given; otherwise returns \c false.

*/
bool QItemSelectionRange::intersects(const QItemSelectionRange &other) const
{
    // isValid() and parent() last since they are more expensive
    return (model() == other.model()
            && ((top() <= other.top() && bottom() >= other.top())
                || (top() >= other.top() && top() <= other.bottom()))
            && ((left() <= other.left() && right() >= other.left())
                || (left() >= other.left() && left() <= other.right()))
            && parent() == other.parent()
            && isValid() && other.isValid()
            );
}

/*!
    \fn QItemSelectionRange QItemSelectionRange::intersected(const QItemSelectionRange &other) const
    \since 4.2

    Returns a new selection range containing only the items that are found in
    both the selection range and the \a other selection range.
*/

QItemSelectionRange QItemSelectionRange::intersected(const QItemSelectionRange &other) const
{
    if (model() == other.model() && parent() == other.parent()) {
        QModelIndex topLeft = model()->index(qMax(top(), other.top()),
                                             qMax(left(), other.left()),
                                             other.parent());
        QModelIndex bottomRight = model()->index(qMin(bottom(), other.bottom()),
                                                 qMin(right(), other.right()),
                                                 other.parent());
        return QItemSelectionRange(topLeft, bottomRight);
    }
    return QItemSelectionRange();
}

/*!
    \fn bool QItemSelectionRange::operator==(const QItemSelectionRange &other) const

    Returns \c true if the selection range is exactly the same as the \a other
    range given; otherwise returns \c false.

*/

/*!
    \fn bool QItemSelectionRange::operator!=(const QItemSelectionRange &other) const

    Returns \c true if the selection range differs from the \a other range given;
    otherwise returns \c false.

*/

/*!
    \fn bool QItemSelectionRange::isValid() const

    Returns \c true if the selection range is valid; otherwise returns \c false.

*/

static void rowLengthsFromRange(const QItemSelectionRange &range, QList<QPair<QPersistentModelIndex, uint>> &result)
{
    if (range.isValid() && range.model()) {
        const QModelIndex topLeft = range.topLeft();
        const int bottom = range.bottom();
        const uint width = range.width();
        const int column = topLeft.column();
        for (int row = topLeft.row(); row <= bottom; ++row) {
            // We don't need to keep track of ItemIsSelectable and ItemIsEnabled here. That is
            // required in indexesFromRange() because that method is called from public API
            // which requires the limitation.
            result.push_back(qMakePair(QPersistentModelIndex(topLeft.sibling(row, column)), width));
        }
    }
}

static bool isSelectableAndEnabled(Qt::ItemFlags flags)
{
    return flags.testFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

template<typename ModelIndexContainer>
static void indexesFromRange(const QItemSelectionRange &range, ModelIndexContainer &result)
{
    if (range.isValid() && range.model()) {
        const QModelIndex topLeft = range.topLeft();
        const int bottom = range.bottom();
        const int right = range.right();
        for (int row = topLeft.row(); row <= bottom; ++row) {
            const QModelIndex columnLeader = topLeft.sibling(row, topLeft.column());
            for (int column = topLeft.column(); column <= right; ++column) {
                QModelIndex index = columnLeader.sibling(row, column);
                if (isSelectableAndEnabled(range.model()->flags(index)))
                    result.push_back(index);
            }
        }
    }
}

template<typename ModelIndexContainer>
static ModelIndexContainer qSelectionIndexes(const QItemSelection &selection)
{
    ModelIndexContainer result;
    for (const auto &range : selection)
        indexesFromRange(range, result);
    return result;
}

/*!
    Returns \c true if the selection range contains either no items
    or only items which are either disabled or marked as not selectable.

    \since 4.7
*/

bool QItemSelectionRange::isEmpty() const
{
    if (!isValid() || !model())
        return true;

    for (int column = left(); column <= right(); ++column) {
        for (int row = top(); row <= bottom(); ++row) {
            QModelIndex index = model()->index(row, column, parent());
            if (isSelectableAndEnabled(model()->flags(index)))
                return false;
        }
    }
    return true;
}

/*!
    Returns the list of model index items stored in the selection.
*/

QModelIndexList QItemSelectionRange::indexes() const
{
    QModelIndexList result;
    indexesFromRange(*this, result);
    return result;
}

/*!
    \class QItemSelection
    \inmodule QtCore

    \brief The QItemSelection class manages information about selected items in a model.

    \ingroup model-view

    A QItemSelection describes the items in a model that have been
    selected by the user. A QItemSelection is basically a list of
    selection ranges, see QItemSelectionRange. It provides functions for
    creating and manipulating selections, and selecting a range of items
    from a model.

    The QItemSelection class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    An item selection can be constructed and initialized to contain a
    range of items from an existing model. The following example constructs
    a selection that contains a range of items from the given \c model,
    beginning at the \c topLeft, and ending at the \c bottomRight.

    \snippet code/src_gui_itemviews_qitemselectionmodel.cpp 0

    An empty item selection can be constructed, and later populated as
    required. So, if the model is going to be unavailable when we construct
    the item selection, we can rewrite the above code in the following way:

    \snippet code/src_gui_itemviews_qitemselectionmodel.cpp 1

    QItemSelection saves memory, and avoids unnecessary work, by working with
    selection ranges rather than recording the model item index for each
    item in the selection. Generally, an instance of this class will contain
    a list of non-overlapping selection ranges.

    Use merge() to merge one item selection into another without making
    overlapping ranges. Use split() to split one selection range into
    smaller ranges based on a another selection range.

    \sa {Model/View Programming}, QItemSelectionModel
*/

/*!
    \fn QItemSelection::QItemSelection()

    Constructs an empty selection.
*/

/*!
    Constructs an item selection that extends from the top-left model item,
    specified by the \a topLeft index, to the bottom-right item, specified
    by \a bottomRight.
*/
QItemSelection::QItemSelection(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    select(topLeft, bottomRight);
}

/*!
    Adds the items in the range that extends from the top-left model
    item, specified by the \a topLeft index, to the bottom-right item,
    specified by \a bottomRight to the list.

    \note \a topLeft and \a bottomRight must have the same parent.
*/
void QItemSelection::select(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!topLeft.isValid() || !bottomRight.isValid())
        return;

    if ((topLeft.model() != bottomRight.model())
        || topLeft.parent() != bottomRight.parent()) {
        qWarning("Can't select indexes from different model or with different parents");
        return;
    }
    if (topLeft.row() > bottomRight.row() || topLeft.column() > bottomRight.column()) {
        int top = qMin(topLeft.row(), bottomRight.row());
        int bottom = qMax(topLeft.row(), bottomRight.row());
        int left = qMin(topLeft.column(), bottomRight.column());
        int right = qMax(topLeft.column(), bottomRight.column());
        QModelIndex tl = topLeft.sibling(top, left);
        QModelIndex br = bottomRight.sibling(bottom, right);
        append(QItemSelectionRange(tl, br));
        return;
    }
    append(QItemSelectionRange(topLeft, bottomRight));
}

/*!
    Returns \c true if the selection contains the given \a index; otherwise
    returns \c false.
*/

bool QItemSelection::contains(const QModelIndex &index) const
{
    if (isSelectableAndEnabled(index.flags())) {
        QList<QItemSelectionRange>::const_iterator it = begin();
        for (; it != end(); ++it)
            if ((*it).contains(index))
                return true;
    }
    return false;
}

/*!
    Returns a list of model indexes that correspond to the selected items.
*/

QModelIndexList QItemSelection::indexes() const
{
    return qSelectionIndexes<QModelIndexList>(*this);
}

static QList<QPair<QPersistentModelIndex, uint>> qSelectionPersistentRowLengths(const QItemSelection &sel)
{
    QList<QPair<QPersistentModelIndex, uint>> result;
    for (const QItemSelectionRange &range : sel)
        rowLengthsFromRange(range, result);
    return result;
}

/*!
    Merges the \a other selection with this QItemSelection using the
    \a command given. This method guarantees that no ranges are overlapping.

    Note that only QItemSelectionModel::Select,
    QItemSelectionModel::Deselect, and QItemSelectionModel::Toggle are
    supported.

    \sa split()
*/
void QItemSelection::merge(const QItemSelection &other, QItemSelectionModel::SelectionFlags command)
{
    if (other.isEmpty() ||
          !(command & QItemSelectionModel::Select ||
          command & QItemSelectionModel::Deselect ||
          command & QItemSelectionModel::Toggle))
        return;

    QItemSelection newSelection;
    newSelection.reserve(other.size());
    // Collect intersections
    QItemSelection intersections;
    for (const auto &range : other) {
        if (!range.isValid())
            continue;
        newSelection.push_back(range);
        for (int t = 0; t < size(); ++t) {
            if (range.intersects(at(t)))
                intersections.append(at(t).intersected(range));
        }
    }

    //  Split the old (and new) ranges using the intersections
    for (int i = 0; i < intersections.size(); ++i) { // for each intersection
        for (int t = 0; t < size();) { // splitt each old range
            if (at(t).intersects(intersections.at(i))) {
                split(at(t), intersections.at(i), this);
                removeAt(t);
            } else {
                ++t;
            }
        }
        // only split newSelection if Toggle is specified
        for (int n = 0; (command & QItemSelectionModel::Toggle) && n < newSelection.size();) {
            if (newSelection.at(n).intersects(intersections.at(i))) {
                split(newSelection.at(n), intersections.at(i), &newSelection);
                newSelection.removeAt(n);
            } else {
                ++n;
            }
        }
    }
    // do not add newSelection for Deselect
    if (!(command & QItemSelectionModel::Deselect))
        operator+=(newSelection);
}

/*!
    Splits the selection \a range using the selection \a other range.
    Removes all items in \a other from \a range and puts the result in \a result.
    This can be compared with the semantics of the \e subtract operation of a set.
    \sa merge()
*/

void QItemSelection::split(const QItemSelectionRange &range,
                           const QItemSelectionRange &other, QItemSelection *result)
{
    if (range.parent() != other.parent() || range.model() != other.model())
        return;

    QModelIndex parent = other.parent();
    int top = range.top();
    int left = range.left();
    int bottom = range.bottom();
    int right = range.right();
    int other_top = other.top();
    int other_left = other.left();
    int other_bottom = other.bottom();
    int other_right = other.right();
    const QAbstractItemModel *model = range.model();
    Q_ASSERT(model);
    if (other_top > top) {
        QModelIndex tl = model->index(top, left, parent);
        QModelIndex br = model->index(other_top - 1, right, parent);
        result->append(QItemSelectionRange(tl, br));
        top = other_top;
    }
    if (other_bottom < bottom) {
        QModelIndex tl = model->index(other_bottom + 1, left, parent);
        QModelIndex br = model->index(bottom, right, parent);
        result->append(QItemSelectionRange(tl, br));
        bottom = other_bottom;
    }
    if (other_left > left) {
        QModelIndex tl = model->index(top, left, parent);
        QModelIndex br = model->index(bottom, other_left - 1, parent);
        result->append(QItemSelectionRange(tl, br));
        left = other_left;
    }
    if (other_right < right) {
        QModelIndex tl = model->index(top, other_right + 1, parent);
        QModelIndex br = model->index(bottom, right, parent);
        result->append(QItemSelectionRange(tl, br));
        right = other_right;
    }
}


void QItemSelectionModelPrivate::initModel(QAbstractItemModel *m)
{
    Q_Q(QItemSelectionModel);
    const QAbstractItemModel *oldModel = model.valueBypassingBindings();
    if (oldModel == m)
        return;

    if (oldModel) {
        q->reset();
        disconnectModel();
    }

    // Caller has to call notify(), unless calling during construction (the common case).
    model.setValueBypassingBindings(m);

    if (m) {
        connections = std::array<QMetaObject::Connection, 12> {
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsAboutToBeRemoved,
                                    this, &QItemSelectionModelPrivate::rowsAboutToBeRemoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsAboutToBeRemoved,
                                    this, &QItemSelectionModelPrivate::columnsAboutToBeRemoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsAboutToBeInserted,
                                    this, &QItemSelectionModelPrivate::rowsAboutToBeInserted),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsAboutToBeInserted,
                                    this, &QItemSelectionModelPrivate::columnsAboutToBeInserted),
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsAboutToBeMoved,
                                    this, &QItemSelectionModelPrivate::triggerLayoutToBeChanged),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsAboutToBeMoved,
                                    this, &QItemSelectionModelPrivate::triggerLayoutToBeChanged),
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsMoved,
                                    this, &QItemSelectionModelPrivate::triggerLayoutChanged),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsMoved,
                                    this, &QItemSelectionModelPrivate::triggerLayoutChanged),
            QObjectPrivate::connect(m, &QAbstractItemModel::layoutAboutToBeChanged,
                                    this, &QItemSelectionModelPrivate::layoutAboutToBeChanged),
            QObjectPrivate::connect(m, &QAbstractItemModel::layoutChanged,
                                    this, &QItemSelectionModelPrivate::layoutChanged),
            QObject::connect(m, &QAbstractItemModel::modelReset,
                             q, &QItemSelectionModel::reset),
            QObjectPrivate::connect(m, &QAbstractItemModel::destroyed,
                                    this, &QItemSelectionModelPrivate::modelDestroyed)
        };
    }
}

void QItemSelectionModelPrivate::disconnectModel()
{
    for (auto &connection : connections)
        QObject::disconnect(connection);
}

/*!
    \internal

    returns a QItemSelection where all ranges have been expanded to:
    Rows: left: 0 and right: columnCount()-1
    Columns: top: 0 and bottom: rowCount()-1
*/

QItemSelection QItemSelectionModelPrivate::expandSelection(const QItemSelection &selection,
                                                           QItemSelectionModel::SelectionFlags command) const
{
    if (selection.isEmpty() && !((command & QItemSelectionModel::Rows) ||
                                 (command & QItemSelectionModel::Columns)))
        return selection;

    QItemSelection expanded;
    if (command & QItemSelectionModel::Rows) {
        for (int i = 0; i < selection.size(); ++i) {
            QModelIndex parent = selection.at(i).parent();
            int colCount = model->columnCount(parent);
            QModelIndex tl = model->index(selection.at(i).top(), 0, parent);
            QModelIndex br = model->index(selection.at(i).bottom(), colCount - 1, parent);
            //we need to merge because the same row could have already been inserted
            expanded.merge(QItemSelection(tl, br), QItemSelectionModel::Select);
        }
    }
    if (command & QItemSelectionModel::Columns) {
        for (int i = 0; i < selection.size(); ++i) {
            QModelIndex parent = selection.at(i).parent();
            int rowCount = model->rowCount(parent);
            QModelIndex tl = model->index(0, selection.at(i).left(), parent);
            QModelIndex br = model->index(rowCount - 1, selection.at(i).right(), parent);
            //we need to merge because the same column could have already been inserted
            expanded.merge(QItemSelection(tl, br), QItemSelectionModel::Select);
        }
    }
    return expanded;
}

/*!
    \internal
*/
void QItemSelectionModelPrivate::rowsAboutToBeRemoved(const QModelIndex &parent,
                                                         int start, int end)
{
    Q_Q(QItemSelectionModel);
    Q_ASSERT(start <= end);
    finalize();

    // update current index
    if (currentIndex.isValid() && parent == currentIndex.parent()
        && currentIndex.row() >= start && currentIndex.row() <= end) {
        QModelIndex old = currentIndex;
        if (start > 0) {
            // there are rows left above the change
            currentIndex = model->index(start - 1, old.column(), parent);
        } else if (model.value() && end < model->rowCount(parent) - 1) {
            // there are rows left below the change
            currentIndex = model->index(end + 1, old.column(), parent);
        } else {
            // there are no rows left in the table
            currentIndex = QModelIndex();
        }
        emit q->currentChanged(currentIndex, old);
        emit q->currentRowChanged(currentIndex, old);
        if (currentIndex.column() != old.column())
            emit q->currentColumnChanged(currentIndex, old);
    }

    QItemSelection deselected;
    QItemSelection newParts;
    bool indexesOfSelectionChanged = false;
    QItemSelection::iterator it = ranges.begin();
    while (it != ranges.end()) {
        if (it->topLeft().parent() != parent) {  // Check parents until reaching root or contained in range
            QModelIndex itParent = it->topLeft().parent();
            while (itParent.isValid() && itParent.parent() != parent)
                itParent = itParent.parent();

            if (itParent.isValid() && start <= itParent.row() && itParent.row() <= end) {
                deselected.append(*it);
                it = ranges.erase(it);
            } else {
                if (itParent.isValid() && end < itParent.row())
                    indexesOfSelectionChanged = true;
                ++it;
            }
        } else if (start <= it->bottom() && it->bottom() <= end    // Full inclusion
                   && start <= it->top() && it->top() <= end) {
            deselected.append(*it);
            it = ranges.erase(it);
        } else if (start <= it->top() && it->top() <= end) {      // Top intersection
            deselected.append(QItemSelectionRange(it->topLeft(), model->index(end, it->right(), it->parent())));
            *it = QItemSelectionRange(model->index(end + 1, it->left(), it->parent()), it->bottomRight());
            ++it;
        } else if (start <= it->bottom() && it->bottom() <= end) {    // Bottom intersection
            deselected.append(QItemSelectionRange(model->index(start, it->left(), it->parent()), it->bottomRight()));
            *it = QItemSelectionRange(it->topLeft(), model->index(start - 1, it->right(), it->parent()));
            ++it;
        } else if (it->top() < start && end < it->bottom()) { // Middle intersection
            // If the parent contains (1, 2, 3, 4, 5, 6, 7, 8) and [3, 4, 5, 6] is selected,
            // and [4, 5] is removed, we need to split [3, 4, 5, 6] into [3], [4, 5] and [6].
            // [4, 5] is appended to deselected, and [3] and [6] remain part of the selection
            // in ranges.
            const QItemSelectionRange removedRange(model->index(start, it->left(), it->parent()),
                                                    model->index(end, it->right(), it->parent()));
            deselected.append(removedRange);
            QItemSelection::split(*it, removedRange, &newParts);
            it = ranges.erase(it);
        } else if (end < it->top()) { // deleted row before selection
            indexesOfSelectionChanged = true;
            ++it;
        } else {
            ++it;
        }
    }
    ranges.append(newParts);

    if (!deselected.isEmpty() || indexesOfSelectionChanged)
        emit q->selectionChanged(QItemSelection(), deselected);
}

/*!
    \internal
*/
void QItemSelectionModelPrivate::columnsAboutToBeRemoved(const QModelIndex &parent,
                                                            int start, int end)
{
    Q_Q(QItemSelectionModel);

    // update current index
    if (currentIndex.isValid() && parent == currentIndex.parent()
        && currentIndex.column() >= start && currentIndex.column() <= end) {
        QModelIndex old = currentIndex;
        if (start > 0) {
            // there are columns to the left of the change
            currentIndex = model->index(old.row(), start - 1, parent);
        } else if (model.value() && end < model->columnCount() - 1) {
            // there are columns to the right of the change
            currentIndex = model->index(old.row(), end + 1, parent);
        } else {
            // there are no columns left in the table
            currentIndex = QModelIndex();
        }
        emit q->currentChanged(currentIndex, old);
        if (currentIndex.row() != old.row())
            emit q->currentRowChanged(currentIndex, old);
        emit q->currentColumnChanged(currentIndex, old);
    }

    // update selections
    QModelIndex tl = model->index(0, start, parent);
    QModelIndex br = model->index(model->rowCount(parent) - 1, end, parent);
    q->select(QItemSelection(tl, br), QItemSelectionModel::Deselect);
    finalize();
}

/*!
    \internal

    Split selection ranges if columns are about to be inserted in the middle.
*/
void QItemSelectionModelPrivate::columnsAboutToBeInserted(const QModelIndex &parent,
                                                             int start, int end)
{
    Q_UNUSED(end);
    finalize();
    QList<QItemSelectionRange> split;
    QList<QItemSelectionRange>::iterator it = ranges.begin();
    for (; it != ranges.end(); ) {
        const QModelIndex &itParent = it->parent();
        if ((*it).isValid() && itParent == parent
            && (*it).left() < start && (*it).right() >= start) {
            QModelIndex bottomMiddle = model->index((*it).bottom(), start - 1, itParent);
            QItemSelectionRange left((*it).topLeft(), bottomMiddle);
            QModelIndex topMiddle = model->index((*it).top(), start, itParent);
            QItemSelectionRange right(topMiddle, (*it).bottomRight());
            it = ranges.erase(it);
            split.append(left);
            split.append(right);
        } else {
            ++it;
        }
    }
    ranges += split;
}

/*!
    \internal

    Split selection ranges if rows are about to be inserted in the middle.
*/
void QItemSelectionModelPrivate::rowsAboutToBeInserted(const QModelIndex &parent,
                                                          int start, int end)
{
    Q_Q(QItemSelectionModel);
    Q_UNUSED(end);
    finalize();
    QList<QItemSelectionRange> split;
    QList<QItemSelectionRange>::iterator it = ranges.begin();
    bool indexesOfSelectionChanged = false;
    for (; it != ranges.end(); ) {
        const QModelIndex &itParent = it->parent();
        if ((*it).isValid() && itParent == parent
            && (*it).top() < start && (*it).bottom() >= start) {
            QModelIndex middleRight = model->index(start - 1, (*it).right(), itParent);
            QItemSelectionRange top((*it).topLeft(), middleRight);
            QModelIndex middleLeft = model->index(start, (*it).left(), itParent);
            QItemSelectionRange bottom(middleLeft, (*it).bottomRight());
            it = ranges.erase(it);
            split.append(top);
            split.append(bottom);
        } else if ((*it).isValid() && itParent == parent      // insertion before selection
            && (*it).top() >= start) {
            indexesOfSelectionChanged = true;
            ++it;
        } else {
            ++it;
        }
    }
    ranges += split;

    if (indexesOfSelectionChanged)
        emit q->selectionChanged(QItemSelection(), QItemSelection());
}

/*!
    \internal

    Split selection into individual (persistent) indexes. This is done in
    preparation for the layoutChanged() signal, where the indexes can be
    merged again.
*/
void QItemSelectionModelPrivate::layoutAboutToBeChanged(const QList<QPersistentModelIndex> &,
                                                        QAbstractItemModel::LayoutChangeHint hint)
{
    savedPersistentIndexes.clear();
    savedPersistentCurrentIndexes.clear();
    savedPersistentRowLengths.clear();
    savedPersistentCurrentRowLengths.clear();

    // optimization for when all indexes are selected
    // (only if there is lots of items (1000) because this is not entirely correct)
    if (ranges.isEmpty() && currentSelection.size() == 1) {
        QItemSelectionRange range = currentSelection.constFirst();
        QModelIndex parent = range.parent();
        tableRowCount = model->rowCount(parent);
        tableColCount = model->columnCount(parent);
        if (tableRowCount * tableColCount > 1000
            && range.top() == 0
            && range.left() == 0
            && range.bottom() == tableRowCount - 1
            && range.right() == tableColCount - 1) {
            tableSelected = true;
            tableParent = parent;
            return;
        }
    }
    tableSelected = false;

    if (hint == QAbstractItemModel::VerticalSortHint) {
        // Special case when we know we're sorting vertically. We can assume that all indexes for columns
        // are displaced the same way, and therefore we only need to track an index from one column per
        // row with a QPersistentModelIndex together with the length of items to the right of it
        // which are displaced the same way.
        // An algorithm which contains the same assumption is used to process layoutChanged.
        savedPersistentRowLengths = qSelectionPersistentRowLengths(ranges);
        savedPersistentCurrentRowLengths = qSelectionPersistentRowLengths(currentSelection);
    } else {
        savedPersistentIndexes = qSelectionIndexes<QList<QPersistentModelIndex>>(ranges);
        savedPersistentCurrentIndexes = qSelectionIndexes<QList<QPersistentModelIndex>>(currentSelection);
    }
}
/*!
    \internal
*/
static QItemSelection mergeRowLengths(const QList<QPair<QPersistentModelIndex, uint>> &rowLengths)
{
    if (rowLengths.isEmpty())
      return QItemSelection();

    QItemSelection result;
    int i = 0;
    while (i < rowLengths.size()) {
        const QPersistentModelIndex &tl = rowLengths.at(i).first;
        if (!tl.isValid()) {
            ++i;
            continue;
        }
        QPersistentModelIndex br = tl;
        const uint length = rowLengths.at(i).second;
        while (++i < rowLengths.size()) {
            const QPersistentModelIndex &next = rowLengths.at(i).first;
            if (!next.isValid())
                continue;
            const uint nextLength = rowLengths.at(i).second;
            if ((nextLength == length)
                && (next.row() == br.row() + 1)
                && (next.column() == br.column())
                && (next.parent() == br.parent())) {
                br = next;
            } else {
                break;
            }
        }
        result.append(QItemSelectionRange(tl, br.sibling(br.row(), br.column() + length - 1)));
    }
    return result;
}

/*!
    \internal

    Merges \a indexes into an item selection made up of ranges.
    Assumes that the indexes are sorted.
*/
static QItemSelection mergeIndexes(const QList<QPersistentModelIndex> &indexes)
{
    QItemSelection colSpans;
    // merge columns
    int i = 0;
    while (i < indexes.size()) {
        const QPersistentModelIndex &tl = indexes.at(i);
        if (!tl.isValid()) {
            ++i;
            continue;
        }
        QPersistentModelIndex br = tl;
        QModelIndex brParent = br.parent();
        int brRow = br.row();
        int brColumn = br.column();
        while (++i < indexes.size()) {
            const QPersistentModelIndex &next = indexes.at(i);
            if (!next.isValid())
                continue;
            const QModelIndex nextParent = next.parent();
            const int nextRow = next.row();
            const int nextColumn = next.column();
            if ((nextParent == brParent)
                 && (nextRow == brRow)
                 && (nextColumn == brColumn + 1)) {
                br = next;
                brParent = nextParent;
                brRow = nextRow;
                brColumn = nextColumn;
            } else {
                break;
            }
        }
        colSpans.append(QItemSelectionRange(tl, br));
    }
    // merge rows
    QItemSelection rowSpans;
    i = 0;
    while (i < colSpans.size()) {
        QModelIndex tl = colSpans.at(i).topLeft();
        QModelIndex br = colSpans.at(i).bottomRight();
        QModelIndex prevTl = tl;
        while (++i < colSpans.size()) {
            QModelIndex nextTl = colSpans.at(i).topLeft();
            QModelIndex nextBr = colSpans.at(i).bottomRight();

            if (nextTl.parent() != tl.parent())
                break; // we can't merge selection ranges from different parents

            if ((nextTl.column() == prevTl.column()) && (nextBr.column() == br.column())
                && (nextTl.row() == prevTl.row() + 1) && (nextBr.row() == br.row() + 1)) {
                br = nextBr;
                prevTl = nextTl;
            } else {
                break;
            }
        }
        rowSpans.append(QItemSelectionRange(tl, br));
    }
    return rowSpans;
}

/*!
    \internal

    Sort predicate function for QItemSelectionModelPrivate::layoutChanged(),
    sorting by parent first in addition to operator<(). This is to prevent
    fragmentation of the selection by grouping indexes with the same row, column
    of different parents next to each other, which may happen when a selection
    spans sub-trees.
*/
static bool qt_PersistentModelIndexLessThan(const QPersistentModelIndex &i1, const QPersistentModelIndex &i2)
{
    const QModelIndex parent1 = i1.parent();
    const QModelIndex parent2 = i2.parent();
    return parent1 == parent2 ? i1 < i2 : parent1 < parent2;
}

/*!
    \internal

    Merge the selected indexes into selection ranges again.
*/
void QItemSelectionModelPrivate::layoutChanged(const QList<QPersistentModelIndex> &, QAbstractItemModel::LayoutChangeHint hint)
{
    // special case for when all indexes are selected
    if (tableSelected && tableColCount == model->columnCount(tableParent)
        && tableRowCount == model->rowCount(tableParent)) {
        ranges.clear();
        currentSelection.clear();
        int bottom = tableRowCount - 1;
        int right = tableColCount - 1;
        QModelIndex tl = model->index(0, 0, tableParent);
        QModelIndex br = model->index(bottom, right, tableParent);
        currentSelection << QItemSelectionRange(tl, br);
        tableParent = QModelIndex();
        tableSelected = false;
        return;
    }

    if ((hint != QAbstractItemModel::VerticalSortHint && savedPersistentCurrentIndexes.isEmpty() && savedPersistentIndexes.isEmpty())
     || (hint == QAbstractItemModel::VerticalSortHint && savedPersistentRowLengths.isEmpty() && savedPersistentCurrentRowLengths.isEmpty())) {
        // either the selection was actually empty, or we
        // didn't get the layoutAboutToBeChanged() signal
        return;
    }

    // clear the "old" selection
    ranges.clear();
    currentSelection.clear();

    if (hint != QAbstractItemModel::VerticalSortHint) {
        // sort the "new" selection, as preparation for merging
        std::stable_sort(savedPersistentIndexes.begin(), savedPersistentIndexes.end(),
                         qt_PersistentModelIndexLessThan);
        std::stable_sort(savedPersistentCurrentIndexes.begin(), savedPersistentCurrentIndexes.end(),
                         qt_PersistentModelIndexLessThan);

        // update the selection by merging the individual indexes
        ranges = mergeIndexes(savedPersistentIndexes);
        currentSelection = mergeIndexes(savedPersistentCurrentIndexes);

        // release the persistent indexes
        savedPersistentIndexes.clear();
        savedPersistentCurrentIndexes.clear();
    } else {
        // sort the "new" selection, as preparation for merging
        std::stable_sort(savedPersistentRowLengths.begin(), savedPersistentRowLengths.end());
        std::stable_sort(savedPersistentCurrentRowLengths.begin(), savedPersistentCurrentRowLengths.end());

        // update the selection by merging the individual indexes
        ranges = mergeRowLengths(savedPersistentRowLengths);
        currentSelection = mergeRowLengths(savedPersistentCurrentRowLengths);

        // release the persistent indexes
        savedPersistentRowLengths.clear();
        savedPersistentCurrentRowLengths.clear();
    }
}

/*!
    \internal

    Called when the used model gets destroyed.

    It is impossible to have a correct implementation here.
    In the following situation, there are two contradicting rules:

    \code
    QProperty<QAbstractItemModel *> leader(mymodel);
    QItemSelectionModel myItemSelectionModel;
    myItemSelectionModel.bindableModel().setBinding([&](){ return leader.value(); }
    delete mymodel;
    QAbstractItemModel *returnedModel = myItemSelectionModel.model();
    \endcode

    What should returnedModel be in this situation?

    Rules for bindable properties say that myItemSelectionModel.model()
    should return the same as leader.value(), namely the pointer to the now deleted model.

    However, backward compatibility requires myItemSelectionModel.model() to return a
    nullptr, because that was done in the past after the model used was deleted.

    We decide to break the new rule, imposed by bindable properties, and not break the old
    rule, because that may break existing code.
*/
void QItemSelectionModelPrivate::modelDestroyed()
{
    model.setValueBypassingBindings(nullptr);
    disconnectModel();
    model.notify();
}

/*!
    \class QItemSelectionModel
    \inmodule QtCore

    \brief The QItemSelectionModel class keeps track of a view's selected items.

    \ingroup model-view

    A QItemSelectionModel keeps track of the selected items in a view, or
    in several views onto the same model. It also keeps track of the
    currently selected item in a view.

    The QItemSelectionModel class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    The selected items are stored using ranges. Whenever you want to
    modify the selected items use select() and provide either a
    QItemSelection, or a QModelIndex and a QItemSelectionModel::SelectionFlag.

    The QItemSelectionModel takes a two layer approach to selection
    management, dealing with both selected items that have been committed
    and items that are part of the current selection. The current
    selected items are part of the current interactive selection (for
    example with rubber-band selection or keyboard-shift selections).

    To update the currently selected items, use the bitwise OR of
    QItemSelectionModel::Current and any of the other SelectionFlags.
    If you omit the QItemSelectionModel::Current command, a new current
    selection will be created, and the previous one added to the whole
    selection. All functions operate on both layers; for example,
    \l {QTableWidget::selectedItems()}{selecteditems()} will return items from both layers.

    \note Since 5.5, \l{QItemSelectionModel::model()}{model},
    \l{QItemSelectionModel::hasSelection()}{hasSelection}, and
    \l{QItemSelectionModel::currentIndex()}{currentIndex} are meta-object properties.

    \sa {Model/View Programming}, QAbstractItemModel
*/

/*!
    Constructs a selection model that operates on the specified item \a model.
*/
QItemSelectionModel::QItemSelectionModel(QAbstractItemModel *model)
    : QObject(*new QItemSelectionModelPrivate, model)
{
    d_func()->initModel(model);
}

/*!
    Constructs a selection model that operates on the specified item \a model with \a parent.
*/
QItemSelectionModel::QItemSelectionModel(QAbstractItemModel *model, QObject *parent)
    : QObject(*new QItemSelectionModelPrivate, parent)
{
    d_func()->initModel(model);
}

/*!
    \internal
*/
QItemSelectionModel::QItemSelectionModel(QItemSelectionModelPrivate &dd, QAbstractItemModel *model)
    : QObject(dd, model)
{
    dd.initModel(model);
}

/*!
    Destroys the selection model.
*/
QItemSelectionModel::~QItemSelectionModel()
{
}

/*!
    Selects the model item \a index using the specified \a command, and emits
    selectionChanged().

    \sa QItemSelectionModel::SelectionFlags
*/
void QItemSelectionModel::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
{
    QItemSelection selection(index, index);
    select(selection, command);
}

/*!
    \fn void QItemSelectionModel::currentChanged(const QModelIndex &current, const QModelIndex &previous)

    This signal is emitted whenever the current item changes. The \a previous
    model item index is replaced by the \a current index as the selection's
    current item.

    Note that this signal will not be emitted when the item model is reset.

    \sa currentIndex(), setCurrentIndex(), selectionChanged()
*/

/*!
    \fn void QItemSelectionModel::currentColumnChanged(const QModelIndex &current, const QModelIndex &previous)

    This signal is emitted if the \a current item changes and its column is
    different to the column of the \a previous current item.

    Note that this signal will not be emitted when the item model is reset.

    \sa currentChanged(), currentRowChanged(), currentIndex(), setCurrentIndex()
*/

/*!
    \fn void QItemSelectionModel::currentRowChanged(const QModelIndex &current, const QModelIndex &previous)

    This signal is emitted if the \a current item changes and its row is
    different to the row of the \a previous current item.

    Note that this signal will not be emitted when the item model is reset.

    \sa currentChanged(), currentColumnChanged(), currentIndex(), setCurrentIndex()
*/

/*!
    \fn void QItemSelectionModel::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)

    This signal is emitted whenever the selection changes. The change in the
    selection is represented as an item selection of \a deselected items and
    an item selection of \a selected items.

    Note the that the current index changes independently from the selection.
    Also note that this signal will not be emitted when the item model is reset.

    Items which stay selected but change their index are not included in
    \a selected and \a deselected. Thus, this signal might be emitted with both
    \a selected and \a deselected empty, if only the indices of selected items
    change.

    \sa select(), currentChanged()
*/

/*!
    \fn void QItemSelectionModel::modelChanged(QAbstractItemModel *model)
    \since 5.5

    This signal is emitted when the \a model is successfully set with setModel().

    \sa model(), setModel()
*/


/*!
    \enum QItemSelectionModel::SelectionFlag

    This enum describes the way the selection model will be updated.

    \value NoUpdate       No selection will be made.
    \value Clear          The complete selection will be cleared.
    \value Select         All specified indexes will be selected.
    \value Deselect       All specified indexes will be deselected.
    \value Toggle         All specified indexes will be selected or
                          deselected depending on their current state.
    \value Current        The current selection will be updated.
    \value Rows           All indexes will be expanded to span rows.
    \value Columns        All indexes will be expanded to span columns.
    \value SelectCurrent  A combination of Select and Current, provided for
                          convenience.
    \value ToggleCurrent  A combination of Toggle and Current, provided for
                          convenience.
    \value ClearAndSelect A combination of Clear and Select, provided for
                          convenience.
*/

namespace {
namespace QtFunctionObjects {
struct IsNotValid {
    typedef bool result_type;
    struct is_transparent : std::true_type {};
    template <typename T>
    constexpr bool operator()(T &t) const noexcept(noexcept(t.isValid()))
    { return !t.isValid(); }
    template <typename T>
    constexpr bool operator()(T *t) const noexcept(noexcept(t->isValid()))
    { return !t->isValid(); }
};
}
} // unnamed namespace

/*!
    Selects the item \a selection using the specified \a command, and emits
    selectionChanged().

    \sa QItemSelectionModel::SelectionFlag
*/
void QItemSelectionModel::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
{
    Q_D(QItemSelectionModel);
    if (!d->model.value()) {
        qWarning("QItemSelectionModel: Selecting when no model has been set will result in a no-op.");
        return;
    }
    if (command == NoUpdate)
        return;

    // store old selection
    QItemSelection sel = selection;
    // If d->ranges is non-empty when the source model is reset the persistent indexes
    // it contains will be invalid. We can't clear them in a modelReset slot because that might already
    // be too late if another model observer is connected to the same modelReset slot and is invoked first
    // it might call select() on this selection model before any such QItemSelectionModelPrivate::modelReset() slot
    // is invoked, so it would not be cleared yet. We clear it invalid ranges in it here.
    d->ranges.removeIf(QtFunctionObjects::IsNotValid());

    QItemSelection old = d->ranges;
    old.merge(d->currentSelection, d->currentCommand);

    // expand selection according to SelectionBehavior
    if (command & Rows || command & Columns)
        sel = d->expandSelection(sel, command);

    // clear ranges and currentSelection
    if (command & Clear) {
        d->ranges.clear();
        d->currentSelection.clear();
    }

    // merge and clear currentSelection if Current was not set (ie. start new currentSelection)
    if (!(command & Current))
        d->finalize();

    // update currentSelection
    if (command & Toggle || command & Select || command & Deselect) {
        d->currentCommand = command;
        d->currentSelection = sel;
    }

    // generate new selection, compare with old and emit selectionChanged()
    QItemSelection newSelection = d->ranges;
    newSelection.merge(d->currentSelection, d->currentCommand);
    emitSelectionChanged(newSelection, old);
}

/*!
    Clears the selection model. Emits selectionChanged() and currentChanged().
*/
void QItemSelectionModel::clear()
{
    clearSelection();
    clearCurrentIndex();
}

/*!
    Clears the current index. Emits currentChanged().
 */
void QItemSelectionModel::clearCurrentIndex()
{
    Q_D(QItemSelectionModel);
    QModelIndex previous = d->currentIndex;
    d->currentIndex = QModelIndex();
    if (previous.isValid()) {
        emit currentChanged(d->currentIndex, previous);
        emit currentRowChanged(d->currentIndex, previous);
        emit currentColumnChanged(d->currentIndex, previous);
    }
}

/*!
    Clears the selection model. Does not emit any signals.
*/
void QItemSelectionModel::reset()
{
    const QSignalBlocker blocker(this);
    clear();
}

/*!
    \since 4.2
    Clears the selection in the selection model. Emits selectionChanged().
*/
void QItemSelectionModel::clearSelection()
{
    Q_D(QItemSelectionModel);
    if (d->ranges.size() == 0 && d->currentSelection.size() == 0)
        return;

    select(QItemSelection(), Clear);
}


/*!
    Sets the model item \a index to be the current item, and emits
    currentChanged(). The current item is used for keyboard navigation and
    focus indication; it is independent of any selected items, although a
    selected item can also be the current item.

    Depending on the specified \a command, the \a index can also become part
    of the current selection.
    \sa select()
*/
void QItemSelectionModel::setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
{
    Q_D(QItemSelectionModel);
    if (!d->model.value()) {
        qWarning("QItemSelectionModel: Setting the current index when no model has been set will result in a no-op.");
        return;
    }
    if (index == d->currentIndex) {
        if (command != NoUpdate)
            select(index, command); // select item
        return;
    }
    QPersistentModelIndex previous = d->currentIndex;
    d->currentIndex = index; // set current before emitting selection changed below
    if (command != NoUpdate)
        select(d->currentIndex, command); // select item
    emit currentChanged(d->currentIndex, previous);
    if (d->currentIndex.row() != previous.row() ||
            d->currentIndex.parent() != previous.parent())
        emit currentRowChanged(d->currentIndex, previous);
    if (d->currentIndex.column() != previous.column() ||
            d->currentIndex.parent() != previous.parent())
        emit currentColumnChanged(d->currentIndex, previous);
}

/*!
    Returns the model item index for the current item, or an invalid index
    if there is no current item.
*/
QModelIndex QItemSelectionModel::currentIndex() const
{
    return static_cast<QModelIndex>(d_func()->currentIndex);
}

/*!
    Returns \c true if the given model item \a index is selected.
*/
bool QItemSelectionModel::isSelected(const QModelIndex &index) const
{
    Q_D(const QItemSelectionModel);
    if (d->model != index.model() || !index.isValid())
        return false;

    bool selected = false;
    //  search model ranges
    QList<QItemSelectionRange>::const_iterator it = d->ranges.begin();
    for (; it != d->ranges.end(); ++it) {
        if ((*it).isValid() && (*it).contains(index)) {
            selected = true;
            break;
        }
    }

    // check  currentSelection
    if (d->currentSelection.size()) {
        if ((d->currentCommand & Deselect) && selected)
            selected = !d->currentSelection.contains(index);
        else if (d->currentCommand & Toggle)
            selected ^= d->currentSelection.contains(index);
        else if ((d->currentCommand & Select) && !selected)
            selected = d->currentSelection.contains(index);
    }

    if (selected)
        return isSelectableAndEnabled(d->model->flags(index));

    return false;
}

/*!
    Returns \c true if all items are selected in the \a row with the given
    \a parent.

    Note that this function is usually faster than calling isSelected()
    on all items in the same row and that unselectable items are
    ignored.

    \note Since Qt 5.15, the default argument for \a parent is an empty
          model index.
*/
bool QItemSelectionModel::isRowSelected(int row, const QModelIndex &parent) const
{
    Q_D(const QItemSelectionModel);
    if (!d->model.value())
        return false;
    if (parent.isValid() && d->model != parent.model())
        return false;

    // return false if row exist in currentSelection (Deselect)
    if (d->currentCommand & Deselect && d->currentSelection.size()) {
        for (int i=0; i<d->currentSelection.size(); ++i) {
            if (d->currentSelection.at(i).parent() == parent &&
                row >= d->currentSelection.at(i).top() &&
                row <= d->currentSelection.at(i).bottom())
                return false;
        }
    }
    // return false if ranges in both currentSelection and ranges
    // intersect and have the same row contained
    if (d->currentCommand & Toggle && d->currentSelection.size()) {
        for (int i=0; i<d->currentSelection.size(); ++i)
            if (d->currentSelection.at(i).top() <= row &&
                d->currentSelection.at(i).bottom() >= row)
                for (int j=0; j<d->ranges.size(); ++j)
                    if (d->ranges.at(j).top() <= row && d->ranges.at(j).bottom() >= row
                        && d->currentSelection.at(i).intersected(d->ranges.at(j)).isValid())
                        return false;
    }

    auto isSelectable = [&](int row, int column) {
        return isSelectableAndEnabled(d->model->index(row, column, parent).flags());
    };

    const int colCount = d->model->columnCount(parent);
    int unselectable = 0;
    // add ranges and currentSelection and check through them all
    QList<QItemSelectionRange>::const_iterator it;
    QList<QItemSelectionRange> joined = d->ranges;
    if (d->currentSelection.size())
        joined += d->currentSelection;
    for (int column = 0; column < colCount; ++column) {
        if (!isSelectable(row, column)) {
            ++unselectable;
            continue;
        }

        for (it = joined.constBegin(); it != joined.constEnd(); ++it) {
            if ((*it).contains(row, column, parent)) {
                for (int i = column; i <= (*it).right(); ++i) {
                    if (!isSelectable(row, i))
                        ++unselectable;
                }

                column = qMax(column, (*it).right());
                break;
            }
        }
        if (it == joined.constEnd())
            return false;
    }
    return unselectable < colCount;
}

/*!
    Returns \c true if all items are selected in the \a column with the given
    \a parent.

    Note that this function is usually faster than calling isSelected()
    on all items in the same column and that unselectable items are
    ignored.

    \note Since Qt 5.15, the default argument for \a parent is an empty
          model index.
*/
bool QItemSelectionModel::isColumnSelected(int column, const QModelIndex &parent) const
{
    Q_D(const QItemSelectionModel);
    if (!d->model.value())
        return false;
    if (parent.isValid() && d->model != parent.model())
        return false;

    // return false if column exist in currentSelection (Deselect)
    if (d->currentCommand & Deselect && d->currentSelection.size()) {
        for (int i = 0; i < d->currentSelection.size(); ++i) {
            if (d->currentSelection.at(i).parent() == parent &&
                column >= d->currentSelection.at(i).left() &&
                column <= d->currentSelection.at(i).right())
                return false;
        }
    }
    // return false if ranges in both currentSelection and the selection model
    // intersect and have the same column contained
    if (d->currentCommand & Toggle && d->currentSelection.size()) {
        for (int i = 0; i < d->currentSelection.size(); ++i) {
            if (d->currentSelection.at(i).left() <= column &&
                d->currentSelection.at(i).right() >= column) {
                for (int j = 0; j < d->ranges.size(); ++j) {
                    if (d->ranges.at(j).left() <= column && d->ranges.at(j).right() >= column
                        && d->currentSelection.at(i).intersected(d->ranges.at(j)).isValid()) {
                        return false;
                    }
                }
            }
        }
    }

    auto isSelectable = [&](int row, int column) {
        return isSelectableAndEnabled(d->model->index(row, column, parent).flags());
    };
    const int rowCount = d->model->rowCount(parent);
    int unselectable = 0;

    // add ranges and currentSelection and check through them all
    QList<QItemSelectionRange>::const_iterator it;
    QList<QItemSelectionRange> joined = d->ranges;
    if (d->currentSelection.size())
        joined += d->currentSelection;
    for (int row = 0; row < rowCount; ++row) {
        if (!isSelectable(row, column)) {
            ++unselectable;
            continue;
        }
        for (it = joined.constBegin(); it != joined.constEnd(); ++it) {
            if ((*it).contains(row, column, parent)) {
                for (int i = row; i <= (*it).bottom(); ++i) {
                    if (!isSelectable(i, column)) {
                        ++unselectable;
                    }
                }
                row = qMax(row, (*it).bottom());
                break;
            }
        }
        if (it == joined.constEnd())
            return false;
    }
    return unselectable < rowCount;
}

/*!
    Returns \c true if there are any items selected in the \a row with the given
    \a parent.

    \note Since Qt 5.15, the default argument for \a parent is an empty
          model index.
*/
bool QItemSelectionModel::rowIntersectsSelection(int row, const QModelIndex &parent) const
{
    Q_D(const QItemSelectionModel);
    if (!d->model.value())
        return false;
    if (parent.isValid() && d->model != parent.model())
         return false;

    QItemSelection sel = d->ranges;
    sel.merge(d->currentSelection, d->currentCommand);
    for (const QItemSelectionRange &range : std::as_const(sel)) {
        if (range.parent() != parent)
            return false;
        int top = range.top();
        int bottom = range.bottom();
        int left = range.left();
        int right = range.right();
        if (top <= row && bottom >= row) {
            for (int j = left; j <= right; j++) {
                if (isSelectableAndEnabled(d->model->index(row, j, parent).flags()))
                    return true;
            }
        }
    }

    return false;
}

/*!
    Returns \c true if there are any items selected in the \a column with the given
    \a parent.

    \note Since Qt 5.15, the default argument for \a parent is an empty
          model index.
*/
bool QItemSelectionModel::columnIntersectsSelection(int column, const QModelIndex &parent) const
{
    Q_D(const QItemSelectionModel);
    if (!d->model.value())
        return false;
    if (parent.isValid() && d->model != parent.model())
        return false;

    QItemSelection sel = d->ranges;
    sel.merge(d->currentSelection, d->currentCommand);
    for (const QItemSelectionRange &range : std::as_const(sel)) {
        if (range.parent() != parent)
            return false;
        int top = range.top();
        int bottom = range.bottom();
        int left = range.left();
        int right = range.right();
        if (left <= column && right >= column) {
            for (int j = top; j <= bottom; j++) {
                if (isSelectableAndEnabled(d->model->index(j, column, parent).flags()))
                    return true;
            }
        }
    }

    return false;
}

/*!
    \internal

    Check whether the selection is empty.
    In contrast to selection.isEmpty(), this takes into account
    whether items are enabled and whether they are selectable.
*/
static bool selectionIsEmpty(const QItemSelection &selection)
{
    return std::all_of(selection.begin(), selection.end(),
                       [](const QItemSelectionRange &r) { return r.isEmpty(); });
}

/*!
    \since 4.2

    Returns \c true if the selection model contains any selected item,
    otherwise returns \c false.
*/
bool QItemSelectionModel::hasSelection() const
{
    Q_D(const QItemSelectionModel);

    // QTreeModel unfortunately sorts itself lazily.
    // When it sorts itself, it emits are layoutChanged signal.
    // This layoutChanged signal invalidates d->ranges here.
    // So QTreeModel must not sort itself while we are iterating over
    // d->ranges here. It sorts itself in executePendingOperations,
    // thus preventing the sort to happen inside of selectionIsEmpty below.
    // Sad story, read more in QTBUG-94546
    const QAbstractItemModel *model = QItemSelectionModel::model();
    if (model != nullptr) {
        auto model_p = static_cast<const QAbstractItemModelPrivate *>(QObjectPrivate::get(model));
        model_p->executePendingOperations();
    }

    if (d->currentCommand & (Toggle | Deselect)) {
        QItemSelection sel = d->ranges;
        sel.merge(d->currentSelection, d->currentCommand);
        return !selectionIsEmpty(sel);
    } else {
        return !(selectionIsEmpty(d->ranges) && selectionIsEmpty(d->currentSelection));
    }
}

/*!
    Returns a list of all selected model item indexes. The list contains no
    duplicates, and is not sorted.
*/
QModelIndexList QItemSelectionModel::selectedIndexes() const
{
    Q_D(const QItemSelectionModel);
    QItemSelection selected = d->ranges;
    selected.merge(d->currentSelection, d->currentCommand);
    return selected.indexes();
}

struct RowOrColumnDefinition {
    QModelIndex parent;
    int rowOrColumn;

    friend bool operator==(const RowOrColumnDefinition &lhs, const RowOrColumnDefinition &rhs) noexcept
    { return lhs.parent == rhs.parent && lhs.rowOrColumn == rhs.rowOrColumn; }
    friend bool operator!=(const RowOrColumnDefinition &lhs, const RowOrColumnDefinition &rhs) noexcept
    { return !operator==(lhs, rhs); }
};
size_t qHash(const RowOrColumnDefinition &key, size_t seed = 0) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.parent);
    seed = hash(seed, key.rowOrColumn);
    return seed;
}

QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(RowOrColumnDefinition)

/*!
    \since 4.2
    Returns the indexes in the given \a column for the rows where all columns are selected.

    \sa selectedIndexes(), selectedColumns()
*/

QModelIndexList QItemSelectionModel::selectedRows(int column) const
{
    QModelIndexList indexes;

    QDuplicateTracker<RowOrColumnDefinition> rowsSeen;

    const QItemSelection ranges = selection();
    for (int i = 0; i < ranges.size(); ++i) {
        const QItemSelectionRange &range = ranges.at(i);
        QModelIndex parent = range.parent();
        for (int row = range.top(); row <= range.bottom(); row++) {
            if (!rowsSeen.hasSeen({parent, row})) {
                if (isRowSelected(row, parent)) {
                    indexes.append(model()->index(row, column, parent));
                }
            }
        }
    }

    return indexes;
}

/*!
    \since 4.2
    Returns the indexes in the given \a row for columns where all rows are selected.

    \sa selectedIndexes(), selectedRows()
*/

QModelIndexList QItemSelectionModel::selectedColumns(int row) const
{
    QModelIndexList indexes;

    QDuplicateTracker<RowOrColumnDefinition> columnsSeen;

    const QItemSelection ranges = selection();
    for (int i = 0; i < ranges.size(); ++i) {
        const QItemSelectionRange &range = ranges.at(i);
        QModelIndex parent = range.parent();
        for (int column = range.left(); column <= range.right(); column++) {
            if (!columnsSeen.hasSeen({parent, column})) {
                if (isColumnSelected(column, parent)) {
                    indexes.append(model()->index(row, column, parent));
                }
            }
        }
    }

    return indexes;
}

/*!
    Returns the selection ranges stored in the selection model.
*/
const QItemSelection QItemSelectionModel::selection() const
{
    Q_D(const QItemSelectionModel);
    QItemSelection selected = d->ranges;
    selected.merge(d->currentSelection, d->currentCommand);
    // make sure we have no invalid ranges
    // ###  should probably be handled more generic somewhere else
    selected.removeIf(QtFunctionObjects::IsNotValid());
    return selected;
}

/*!
    \since 5.5

    \property QItemSelectionModel::hasSelection
    \internal
*/
/*!
    \since 5.5

    \property QItemSelectionModel::currentIndex
    \internal
*/
/*!
    \since 5.5

    \property QItemSelectionModel::selectedIndexes
*/

/*!
    \since 5.5

    \property QItemSelectionModel::selection
    \internal
*/
/*!
    \since 5.5

    \property QItemSelectionModel::model
    \internal
*/
/*!
    \since 5.5

    Returns the item model operated on by the selection model.
*/
QAbstractItemModel *QItemSelectionModel::model()
{
    return d_func()->model.value();
}

/*!
    Returns the item model operated on by the selection model.
*/
const QAbstractItemModel *QItemSelectionModel::model() const
{
    return d_func()->model.value();
}

QBindable<QAbstractItemModel *> QItemSelectionModel::bindableModel()
{
    return &d_func()->model;
}

/*!
    \since 5.5

    Sets the model to \a model. The modelChanged() signal will be emitted.

    \sa model(), modelChanged()
*/
void QItemSelectionModel::setModel(QAbstractItemModel *model)
{
    Q_D(QItemSelectionModel);
    d->model.removeBindingUnlessInWrapper();
    if (d->model.valueBypassingBindings() == model)
        return;
    d->initModel(model);
    d->model.notify();
}

/*!
    Compares the two selections \a newSelection and \a oldSelection
    and emits selectionChanged() with the deselected and selected items.
*/
void QItemSelectionModel::emitSelectionChanged(const QItemSelection &newSelection,
                                               const QItemSelection &oldSelection)
{
    // if both selections are empty or equal we return
    if ((oldSelection.isEmpty() && newSelection.isEmpty()) ||
        oldSelection == newSelection)
        return;

    // if either selection is empty we do not need to compare
    if (oldSelection.isEmpty() || newSelection.isEmpty()) {
        emit selectionChanged(newSelection, oldSelection);
        return;
    }

    QItemSelection deselected = oldSelection;
    QItemSelection selected = newSelection;

    // remove equal ranges
    bool advance;
    for (int o = 0; o < deselected.size(); ++o) {
        advance = true;
        for (int s = 0; s < selected.size() && o < deselected.size();) {
            if (deselected.at(o) == selected.at(s)) {
                deselected.removeAt(o);
                selected.removeAt(s);
                advance = false;
            } else {
                ++s;
            }
        }
        if (advance)
            ++o;
    }

    // find intersections
    QItemSelection intersections;
    for (int o = 0; o < deselected.size(); ++o) {
        for (int s = 0; s < selected.size(); ++s) {
            if (deselected.at(o).intersects(selected.at(s)))
                intersections.append(deselected.at(o).intersected(selected.at(s)));
        }
    }

    // compare remaining ranges with intersections and split them to find deselected and selected
    for (int i = 0; i < intersections.size(); ++i) {
        // split deselected
        for (int o = 0; o < deselected.size();) {
            if (deselected.at(o).intersects(intersections.at(i))) {
                QItemSelection::split(deselected.at(o), intersections.at(i), &deselected);
                deselected.removeAt(o);
            } else {
                ++o;
            }
        }
        // split selected
        for (int s = 0; s < selected.size();) {
            if (selected.at(s).intersects(intersections.at(i))) {
                QItemSelection::split(selected.at(s), intersections.at(i), &selected);
                selected.removeAt(s);
            } else {
                ++s;
            }
        }
    }

    if (!selected.isEmpty() || !deselected.isEmpty())
        emit selectionChanged(selected, deselected);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QItemSelectionRange &range)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QItemSelectionRange(" << range.topLeft()
                  << ',' << range.bottomRight() << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE

#include "moc_qitemselectionmodel.cpp"
