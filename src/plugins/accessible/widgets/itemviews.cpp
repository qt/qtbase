/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "itemviews.h"

#include <qheaderview.h>
#include <qtableview.h>
#include <qlistview.h>
#include <qtreeview.h>
#include <private/qtreewidget_p.h>
#include <qaccessible2.h>
#include <QDebug>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

QString Q_GUI_EXPORT qt_accStripAmp(const QString &text);

#ifndef QT_NO_ITEMVIEWS
/*
Implementation of the IAccessible2 table2 interface. Much simpler than
the other table interfaces since there is only the main table and cells:

TABLE/LIST/TREE
  |- HEADER CELL
  |- CELL
  |- CELL
  ...
*/

int QAccessibleTable2::logicalIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return -1;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    // row * number columns + column + 1 for one based counting
    return (index.row() + hHeader)*(index.model()->columnCount() + vHeader) + (index.column() + vHeader) + 1;
}

QAccessibleInterface *QAccessibleTable2::childFromLogical(int logicalIndex) const
{
    logicalIndex--; // one based counting ftw
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;

    int columns = view->model()->columnCount() + vHeader;

    int row = logicalIndex / columns;
    int column = logicalIndex % columns;

    if (vHeader) {
        if (column == 0) {
            if (row == 0) {
                return new QAccessibleTable2CornerButton(view);
            }
            return new QAccessibleTable2HeaderCell(view, row-1, Qt::Vertical);
        }
        --column;
    }
    if (hHeader) {
        if (row == 0) {
            return new QAccessibleTable2HeaderCell(view, column, Qt::Horizontal);
        }
        --row;
    }
    return new QAccessibleTable2Cell(view, view->model()->index(row, column), cellRole());
}

QAccessibleTable2::QAccessibleTable2(QWidget *w)
    : QAccessibleObject(w)
{
    view = qobject_cast<QAbstractItemView *>(w);
    Q_ASSERT(view);

    if (qobject_cast<const QTableView*>(view)) {
        m_role = QAccessible::Table;
    } else if (qobject_cast<const QTreeView*>(view)) {
        m_role = QAccessible::Tree;
    } else if (qobject_cast<const QListView*>(view)) {
        m_role = QAccessible::List;
    } else {
        // is this our best guess?
        m_role = QAccessible::Table;
    }
}

QAccessibleTable2::~QAccessibleTable2()
{
}

QHeaderView *QAccessibleTable2::horizontalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        header = tv->horizontalHeader();
#endif
#ifndef QT_NO_TREEVIEW
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
#endif
    }
    return header;
}

QHeaderView *QAccessibleTable2::verticalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        header = tv->verticalHeader();
#endif
    }
    return header;
}

void QAccessibleTable2::modelReset()
{}

void QAccessibleTable2::rowsInserted(const QModelIndex &, int first, int last)
{
    lastChange.firstRow = first;
    lastChange.lastRow = last;
    lastChange.firstColumn = 0;
    lastChange.lastColumn = 0;
    lastChange.type = QAccessible2::TableModelChangeInsert;
}

void QAccessibleTable2::rowsRemoved(const QModelIndex &, int first, int last)
{
    lastChange.firstRow = first;
    lastChange.lastRow = last;
    lastChange.firstColumn = 0;
    lastChange.lastColumn = 0;
    lastChange.type = QAccessible2::TableModelChangeDelete;
}

void QAccessibleTable2::columnsInserted(const QModelIndex &, int first, int last)
{
    lastChange.firstRow = 0;
    lastChange.lastRow = 0;
    lastChange.firstColumn = first;
    lastChange.lastColumn = last;
    lastChange.type = QAccessible2::TableModelChangeInsert;
}

void QAccessibleTable2::columnsRemoved(const QModelIndex &, int first, int last)
{
    lastChange.firstRow = 0;
    lastChange.lastRow = 0;
    lastChange.firstColumn = first;
    lastChange.lastColumn = last;
    lastChange.type = QAccessible2::TableModelChangeDelete;
}

void QAccessibleTable2::rowsMoved( const QModelIndex &, int, int, const QModelIndex &, int)
{
    lastChange.firstRow = 0;
    lastChange.lastRow = 0;
    lastChange.firstColumn = 0;
    lastChange.lastColumn = 0;
    lastChange.type = QAccessible2::TableModelChangeUpdate;
}

void QAccessibleTable2::columnsMoved( const QModelIndex &, int, int, const QModelIndex &, int)
{
    lastChange.firstRow = 0;
    lastChange.lastRow = 0;
    lastChange.firstColumn = 0;
    lastChange.lastColumn = 0;
    lastChange.type = QAccessible2::TableModelChangeUpdate;
}

QAccessibleTable2Cell *QAccessibleTable2::cell(const QModelIndex &index) const
{
    if (index.isValid())
        return new QAccessibleTable2Cell(view, index, cellRole());
    return 0;
}

QAccessibleTable2CellInterface *QAccessibleTable2::cellAt(int row, int column) const
{
    Q_ASSERT(role() != QAccessible::Tree);
    QModelIndex index = view->model()->index(row, column);
    //Q_ASSERT(index.isValid());
    if (!index.isValid()) {
        qWarning() << "QAccessibleTable2::cellAt: invalid index: " << index << " for " << view;
        return 0;
    }
    return cell(index);
}

QAccessibleInterface *QAccessibleTable2::caption() const
{
    return 0;
}

QString QAccessibleTable2::columnDescription(int column) const
{
    return view->model()->headerData(column, Qt::Horizontal).toString();
}

int QAccessibleTable2::columnCount() const
{
    return view->model()->columnCount();
}

int QAccessibleTable2::rowCount() const
{
    return view->model()->rowCount();
}

int QAccessibleTable2::selectedCellCount() const
{
    return view->selectionModel()->selectedIndexes().count();
}

int QAccessibleTable2::selectedColumnCount() const
{
    return view->selectionModel()->selectedColumns().count();
}

int QAccessibleTable2::selectedRowCount() const
{
    return view->selectionModel()->selectedRows().count();
}

QString QAccessibleTable2::rowDescription(int row) const
{
    return view->model()->headerData(row, Qt::Vertical).toString();
}

QList<QAccessibleTable2CellInterface*> QAccessibleTable2::selectedCells() const
{
    QList<QAccessibleTable2CellInterface*> cells;
    Q_FOREACH (const QModelIndex &index, view->selectionModel()->selectedIndexes()) {
        cells.append(cell(index));
    }
    return cells;
}

QList<int> QAccessibleTable2::selectedColumns() const
{
    QList<int> columns;
    Q_FOREACH (const QModelIndex &index, view->selectionModel()->selectedColumns()) {
        columns.append(index.column());
    }
    return columns;
}

QList<int> QAccessibleTable2::selectedRows() const
{
    QList<int> rows;
    Q_FOREACH (const QModelIndex &index, view->selectionModel()->selectedRows()) {
        rows.append(index.row());
    }
    return rows;
}

QAccessibleInterface *QAccessibleTable2::summary() const
{
    return 0;
}

bool QAccessibleTable2::isColumnSelected(int column) const
{
    return view->selectionModel()->isColumnSelected(column, QModelIndex());
}

bool QAccessibleTable2::isRowSelected(int row) const
{
    return view->selectionModel()->isRowSelected(row, QModelIndex());
}

bool QAccessibleTable2::selectRow(int row)
{
    QModelIndex index = view->model()->index(row, 0);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Select);
    return true;
}

bool QAccessibleTable2::selectColumn(int column)
{
    QModelIndex index = view->model()->index(0, column);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Select);
    return true;
}

bool QAccessibleTable2::unselectRow(int row)
{
    QModelIndex index = view->model()->index(row, 0);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Deselect);
    return true;
}

bool QAccessibleTable2::unselectColumn(int column)
{
    QModelIndex index = view->model()->index(0, column);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Columns & QItemSelectionModel::Deselect);
    return true;
}

QAccessible2::TableModelChange QAccessibleTable2::modelChange() const
{
    QAccessible2::TableModelChange change;
    // FIXME
    return change;
}

QAccessible::Role QAccessibleTable2::role() const
{
    return m_role;
}

QAccessible::State QAccessibleTable2::state() const
{
    return QAccessible::Normal;
}

int QAccessibleTable2::childAt(int x, int y) const
{
    QPoint viewportOffset = view->viewport()->mapTo(view, QPoint(0,0));
    QPoint indexPosition = view->mapFromGlobal(QPoint(x, y) - viewportOffset);
    // FIXME: if indexPosition < 0 in one coordinate, return header

    QModelIndex index = view->indexAt(indexPosition);
    if (index.isValid()) {
        return logicalIndex(index);
    }
    return -1;
}

int QAccessibleTable2::childCount() const
{
    if (!view->model())
        return 0;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    return (view->model()->rowCount()+hHeader) * (view->model()->columnCount()+vHeader);
}

int QAccessibleTable2::indexOfChild(const QAccessibleInterface *iface) const
{
    Q_ASSERT(iface->role() != QAccessible::TreeItem); // should be handled by tree class
    if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
        const QAccessibleTable2Cell* cell = static_cast<const QAccessibleTable2Cell*>(iface);
        return logicalIndex(cell->m_index);
    } else if (iface->role() == QAccessible::ColumnHeader){
        const QAccessibleTable2HeaderCell* cell = static_cast<const QAccessibleTable2HeaderCell*>(iface);
        return cell->index + (verticalHeader() ? 1 : 0) + 1;
    } else if (iface->role() == QAccessible::RowHeader){
        const QAccessibleTable2HeaderCell* cell = static_cast<const QAccessibleTable2HeaderCell*>(iface);
        return (cell->index+1) * (view->model()->rowCount()+1)  + 1;
    } else if (iface->role() == QAccessible::Pane) {
        return 1; // corner button
    } else {
        qWarning() << "WARNING QAccessibleTable2::indexOfChild Fix my children..."
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: we are in denial of our children. this should stop.
    return -1;
}

QString QAccessibleTable2::text(Text t) const
{
    if (t == QAccessible::Description)
        return view->accessibleDescription();
    return view->accessibleName();
}

QRect QAccessibleTable2::rect() const
{
    if (!view->isVisible())
        return QRect();
    QPoint pos = view->mapToGlobal(QPoint(0, 0));
    return QRect(pos.x(), pos.y(), view->width(), view->height());
}

QAccessibleInterface *QAccessibleTable2::parent() const
{
    if (view->parent()) {
        if (qstrcmp("QComboBoxPrivateContainer", view->parent()->metaObject()->className()) == 0) {
            return QAccessible::queryAccessibleInterface(view->parent()->parent());
        }
        return QAccessible::queryAccessibleInterface(view->parent());
    }
    return 0;
}

QAccessibleInterface *QAccessibleTable2::child(int index) const
{
    // Fixme: get rid of the +1 madness
    return childFromLogical(index + 1);
}

int QAccessibleTable2::navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    *iface = 0;
    switch (relation) {
    case Ancestor: {
        *iface = parent();
        return *iface ? 0 : -1;
    }
    case QAccessible::Child: {
        Q_ASSERT(index > 0);
        *iface = child(index - 1);
        if (*iface) {
            return 0;
        }
        break;
    }
    default:
        break;
    }
    return -1;
}

QAccessible::Relation QAccessibleTable2::relationTo(const QAccessibleInterface *) const
{
    return QAccessible::Unrelated;
}

void *QAccessibleTable2::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::Table2Interface)
       return static_cast<QAccessibleTable2Interface*>(this);
   return 0;
}

// TREE VIEW

QModelIndex QAccessibleTree::indexFromLogical(int row, int column) const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
    QModelIndex modelIndex = treeView->d_func()->viewItems.at(row).index;

    if (modelIndex.isValid() && column > 0) {
        modelIndex = view->model()->index(modelIndex.row(), column, modelIndex.parent());
    }
    return modelIndex;
}

int QAccessibleTree::childAt(int x, int y) const
{
    QPoint viewportOffset = view->viewport()->mapTo(view, QPoint(0,0));
    QPoint indexPosition = view->mapFromGlobal(QPoint(x, y) - viewportOffset);

    QModelIndex index = view->indexAt(indexPosition);
    if (!index.isValid())
        return -1;

    const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
    int row = treeView->d_func()->viewIndex(index) + (horizontalHeader() ? 1 : 0);
    int column = index.column();

    int i = row * view->model()->columnCount() + column + 1;
    Q_ASSERT(i > view->model()->columnCount());
    return i;
}

int QAccessibleTree::childCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
    Q_ASSERT(treeView);
    if (!view->model())
        return 0;

    int hHeader = horizontalHeader() ? 1 : 0;
    return (treeView->d_func()->viewItems.count() + hHeader)* view->model()->columnCount();
}

int QAccessibleTree::rowCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
    Q_ASSERT(treeView);
    return treeView->d_func()->viewItems.count();
}

int QAccessibleTree::indexOfChild(const QAccessibleInterface *iface) const
{
     if (iface->role() == QAccessible::TreeItem) {
        const QAccessibleTable2Cell* cell = static_cast<const QAccessibleTable2Cell*>(iface);
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
        Q_ASSERT(treeView);
        int row = treeView->d_func()->viewIndex(cell->m_index) + (horizontalHeader() ? 1 : 0);
        int column = cell->m_index.column();

        int index = row * view->model()->columnCount() + column + 1;
        //qDebug() << "QAccessibleTree::indexOfChild r " << row << " c " << column << "index " << index;
        Q_ASSERT(index > treeView->model()->columnCount());
        return index;
    } else if (iface->role() == QAccessible::ColumnHeader){
        const QAccessibleTable2HeaderCell* cell = static_cast<const QAccessibleTable2HeaderCell*>(iface);
        //qDebug() << "QAccessibleTree::indexOfChild header " << cell->index << "is: " << cell->index + 1;
        return cell->index + 1;
    } else {
        qWarning() << "WARNING QAccessibleTable2::indexOfChild invalid child"
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: add scrollbars and don't just ignore them
    return -1;
}

int QAccessibleTree::navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    switch (relation) {
    case QAccessible::Child: {
        Q_ASSERT(index > 0);
        --index;
        int hHeader = horizontalHeader() ? 1 : 0;

        if (hHeader) {
            if (index < view->model()->columnCount()) {
                *iface = new QAccessibleTable2HeaderCell(view, index, Qt::Horizontal);
                return 0;
            } else {
                index -= view->model()->columnCount();
            }
        }

        int row = index / view->model()->columnCount();
        int column = index % view->model()->columnCount();
        QModelIndex modelIndex = indexFromLogical(row, column);
        if (modelIndex.isValid()) {
            *iface = cell(modelIndex);
            return 0;
        }
        return -1;
    }
    default:
        break;
    }
    return QAccessibleTable2::navigate(relation, index, iface);
}

QAccessible::Relation QAccessibleTree::relationTo(const QAccessibleInterface *) const
{
    return QAccessible::Unrelated;
}

QAccessibleTable2CellInterface *QAccessibleTree::cellAt(int row, int column) const
{
    QModelIndex index = indexFromLogical(row, column);
    if (!index.isValid()) {
        qWarning() << "Requested invalid tree cell: " << row << column;
        return 0;
    }
    return new QAccessibleTable2Cell(view, index, cellRole());
}

QString QAccessibleTree::rowDescription(int) const
{
    return QString(); // no headers for rows in trees
}

bool QAccessibleTree::isRowSelected(int row) const
{
    QModelIndex index = indexFromLogical(row);
    return view->selectionModel()->isRowSelected(index.row(), index.parent());
}

bool QAccessibleTree::selectRow(int row)
{
    QModelIndex index = indexFromLogical(row);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Select);
    return true;
}

// TABLE CELL

QAccessibleTable2Cell::QAccessibleTable2Cell(QAbstractItemView *view_, const QModelIndex &index_, QAccessible::Role role_)
    : /* QAccessibleSimpleEditableTextInterface(this), */ view(view_), m_index(index_), m_role(role_)
{
    Q_ASSERT(index_.isValid());
}

int QAccessibleTable2Cell::columnExtent() const { return 1; }
int QAccessibleTable2Cell::rowExtent() const { return 1; }

QList<QAccessibleInterface*> QAccessibleTable2Cell::rowHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (verticalHeader()) {
        headerCell.append(new QAccessibleTable2HeaderCell(view, m_index.row(), Qt::Vertical));
    }
    return headerCell;
}

QList<QAccessibleInterface*> QAccessibleTable2Cell::columnHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (horizontalHeader()) {
        headerCell.append(new QAccessibleTable2HeaderCell(view, m_index.column(), Qt::Horizontal));
    }
    return headerCell;
}

QHeaderView *QAccessibleTable2Cell::horizontalHeader() const
{
    QHeaderView *header = 0;

    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        header = tv->horizontalHeader();
#endif
#ifndef QT_NO_TREEVIEW
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
#endif
    }

    return header;
}

QHeaderView *QAccessibleTable2Cell::verticalHeader() const
{
    QHeaderView *header = 0;
#ifndef QT_NO_TABLEVIEW
    if (const QTableView *tv = qobject_cast<const QTableView*>(view))
        header = tv->verticalHeader();
#endif
    return header;
}

int QAccessibleTable2Cell::columnIndex() const
{
    return m_index.column();
}

int QAccessibleTable2Cell::rowIndex() const
{
    if (role() == QAccessible::TreeItem) {
       const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
       Q_ASSERT(treeView);
       int row = treeView->d_func()->viewIndex(m_index);
       return row;
    }
    return m_index.row();
}

bool QAccessibleTable2Cell::isSelected() const
{
    return view->selectionModel()->isSelected(m_index);
}

void QAccessibleTable2Cell::rowColumnExtents(int *row, int *column, int *rowExtents, int *columnExtents, bool *selected) const
{
    *row = m_index.row();
    *column = m_index.column();
    *rowExtents = 1;
    *columnExtents = 1;
    *selected = isSelected();
}

QAccessibleTable2Interface* QAccessibleTable2Cell::table() const
{
    return QAccessible::queryAccessibleInterface(view)->table2Interface();
}

QAccessible::Role QAccessibleTable2Cell::role() const
{
    return m_role;
}

QAccessible::State QAccessibleTable2Cell::state() const
{
    State st = Normal;

    QRect globalRect = view->rect();
    globalRect.translate(view->mapToGlobal(QPoint(0,0)));
    if (!globalRect.intersects(rect()))
        st |= Invisible;

    if (view->selectionModel()->isSelected(m_index))
        st |= Selected;
    if (view->selectionModel()->currentIndex() == m_index)
        st |= Focused;
    if (m_index.model()->data(m_index, Qt::CheckStateRole).toInt() == Qt::Checked)
        st |= Checked;

    Qt::ItemFlags flags = m_index.flags();
    if (flags & Qt::ItemIsSelectable) {
        st |= Selectable;
        st |= Focusable;
        if (view->selectionMode() == QAbstractItemView::MultiSelection)
            st |= MultiSelectable;
        if (view->selectionMode() == QAbstractItemView::ExtendedSelection)
            st |= ExtSelectable;
    }
    if (m_role == QAccessible::TreeItem) {
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
        if (treeView->isExpanded(m_index))
            st |= Expanded;
    }
    return st;
}

bool QAccessibleTable2Cell::isExpandable() const
{
    return view->model()->hasChildren(m_index);
}

QRect QAccessibleTable2Cell::rect() const
{
    QRect r;
    r = view->visualRect(m_index);

    if (!r.isNull())
        r.translate(view->viewport()->mapTo(view, QPoint(0,0)));
        r.translate(view->mapToGlobal(QPoint(0, 0)));
    return r;
}

QString QAccessibleTable2Cell::text(Text t) const
{
    QAbstractItemModel *model = view->model();
    QString value;
    switch (t) {
    case QAccessible::Value:
    case QAccessible::Name:
        value = model->data(m_index, Qt::AccessibleTextRole).toString();
        if (value.isEmpty())
            value = model->data(m_index, Qt::DisplayRole).toString();
        break;
    case QAccessible::Description:
        value = model->data(m_index, Qt::AccessibleDescriptionRole).toString();
        break;
    default:
        break;
    }
    return value;
}

void QAccessibleTable2Cell::setText(Text /*t*/, const QString &text)
{
    if (!(m_index.flags() & Qt::ItemIsEditable))
        return;
    view->model()->setData(m_index, text);
}

bool QAccessibleTable2Cell::isValid() const
{
    if (!m_index.isValid()) {
        qDebug() << "Interface is not valid";
    }

    return m_index.isValid();
}

QAccessibleInterface *QAccessibleTable2Cell::parent() const
{
    if (m_role == QAccessible::TreeItem)
        return new QAccessibleTree(view);

    return new QAccessibleTable2(view);
}

QAccessibleInterface *QAccessibleTable2Cell::child(int) const
{
    return 0;
}

int QAccessibleTable2Cell::navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    if (relation == Ancestor && index == 1) {
        *iface = parent();
        return 0;
    }

    *iface = 0;
    if (!view)
        return -1;

    switch (relation) {

    case Child: {
        return -1;
    }
    case Sibling:
        if (index > 0) {
            QAccessibleInterface *parent = queryAccessibleInterface(view);
            *iface = parent->child(index - 1);
            delete parent;
            return *iface ? 0 : -1;
        }
        return -1;

// From table1 implementation:
//    case Up:
//    case Down:
//    case Left:
//    case Right: {
//        // This is in the "not so nice" category. In order to find out which item
//        // is geometrically around, we have to set the current index, navigate
//        // and restore the index as well as the old selection
//        view->setUpdatesEnabled(false);
//        const QModelIndex oldIdx = view->currentIndex();
//        QList<QModelIndex> kids = children();
//        const QModelIndex currentIndex = index ? kids.at(index - 1) : QModelIndex(row);
//        const QItemSelection oldSelection = view->selectionModel()->selection();
//        view->setCurrentIndex(currentIndex);
//        const QModelIndex idx = view->moveCursor(toCursorAction(relation), Qt::NoModifier);
//        view->setCurrentIndex(oldIdx);
//        view->selectionModel()->select(oldSelection, QItemSelectionModel::ClearAndSelect);
//        view->setUpdatesEnabled(true);
//        if (!idx.isValid())
//            return -1;

//        if (idx.parent() != row.parent() || idx.row() != row.row())
//            *iface = cell(idx);
//        return index ? kids.indexOf(idx) + 1 : 0; }
    default:
        break;
    }

    return -1;
}

QAccessible::Relation QAccessibleTable2Cell::relationTo(const QAccessibleInterface *other) const
{
    // we only check for parent-child relationships in trees
    if (m_role == QAccessible::TreeItem && other->role() == QAccessible::TreeItem) {
        QModelIndex otherIndex = static_cast<const QAccessibleTable2Cell*>(other)->m_index;
        // is the other our parent?
        if (otherIndex.parent() == m_index)
            return QAccessible::Ancestor;
        // are we the other's child?
        if (m_index.parent() == otherIndex)
            return QAccessible::Child;
    }
    return QAccessible::Unrelated;
}

QAccessibleTable2HeaderCell::QAccessibleTable2HeaderCell(QAbstractItemView *view_, int index_, Qt::Orientation orientation_)
    : view(view_), index(index_), orientation(orientation_)
{
    Q_ASSERT(index_ >= 0);
}

QAccessible::Role QAccessibleTable2HeaderCell::role() const
{
    if (orientation == Qt::Horizontal)
        return QAccessible::ColumnHeader;
    return QAccessible::RowHeader;
}

QAccessible::State QAccessibleTable2HeaderCell::state() const
{
    return QAccessible::Normal;
}

QRect QAccessibleTable2HeaderCell::rect() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        if (orientation == Qt::Horizontal) {
            header = tv->horizontalHeader();
        } else {
            header = tv->verticalHeader();
        }
#endif
#ifndef QT_NO_TREEVIEW
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
#endif
    }
    QPoint zero = header->mapToGlobal(QPoint(0, 0));
    int sectionSize = header->sectionSize(index);
    int sectionPos = header->sectionPosition(index);
    return orientation == Qt::Horizontal
            ? QRect(zero.x() + sectionPos, zero.y(), sectionSize, header->height())
            : QRect(zero.x(), zero.y() + sectionPos, header->width(), sectionSize);
}

QString QAccessibleTable2HeaderCell::text(Text t) const
{
    QAbstractItemModel *model = view->model();
    QString value;
    switch (t) {
    case QAccessible::Value:
    case QAccessible::Name:
        value = model->headerData(index, orientation, Qt::AccessibleTextRole).toString();
        if (value.isEmpty())
            value = model->headerData(index, orientation, Qt::DisplayRole).toString();
        break;
    case QAccessible::Description:
        value = model->headerData(index, orientation, Qt::AccessibleDescriptionRole).toString();
        break;
    default:
        break;
    }
    return value;
}

void QAccessibleTable2HeaderCell::setText(Text, const QString &)
{
    return;
}

bool QAccessibleTable2HeaderCell::isValid() const
{
    return true;
}

QAccessibleInterface *QAccessibleTable2HeaderCell::parent() const
{
    if (false) {
#ifndef QT_NO_TREEVIEW
    } else if (qobject_cast<const QTreeView*>(view)) {
        return new QAccessibleTree(view);
#endif
    } else {
        return new QAccessibleTable2(view);
    }
}

QAccessibleInterface *QAccessibleTable2HeaderCell::child(int) const
{
    return 0;
}

int QAccessibleTable2HeaderCell::navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    if (relation == QAccessible::Ancestor && index == 1) {
        *iface = parent();
        return *iface ? 0 : -1;
    }
    *iface = 0;
    return -1;
}

QAccessible::Relation QAccessibleTable2HeaderCell::relationTo(int, const QAccessibleInterface *, int) const
{
    return QAccessible::Unrelated;
}

#endif // QT_NO_ITEMVIEWS

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
