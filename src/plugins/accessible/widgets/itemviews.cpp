/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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


QAbstractItemView *QAccessibleTable::view() const
{
    return qobject_cast<QAbstractItemView*>(object());
}

int QAccessibleTable::logicalIndex(const QModelIndex &index) const
{
    if (!view()->model() || !index.isValid())
        return -1;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    return (index.row() + hHeader)*(index.model()->columnCount() + vHeader) + (index.column() + vHeader);
}

QAccessibleInterface *QAccessibleTable::childFromLogical(int logicalIndex) const
{
    if (!view()->model())
        return 0;

    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;

    int columns = view()->model()->columnCount() + vHeader;

    int row = logicalIndex / columns;
    int column = logicalIndex % columns;

    if (vHeader) {
        if (column == 0) {
            if (row == 0) {
                return new QAccessibleTableCornerButton(view());
            }
            return new QAccessibleTableHeaderCell(view(), row-1, Qt::Vertical);
        }
        --column;
    }
    if (hHeader) {
        if (row == 0) {
            return new QAccessibleTableHeaderCell(view(), column, Qt::Horizontal);
        }
        --row;
    }

    QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
    if (!index.isValid()) {
        qWarning() << "QAccessibleTable::childFromLogical: Invalid index at: " << row << column;
        return 0;
    }
    return new QAccessibleTableCell(view(), index, cellRole());
}

QAccessibleTable::QAccessibleTable(QWidget *w)
    : QAccessibleObject(w)
{
    Q_ASSERT(view());

    if (qobject_cast<const QTableView*>(view())) {
        m_role = QAccessible::Table;
    } else if (qobject_cast<const QTreeView*>(view())) {
        m_role = QAccessible::Tree;
    } else if (qobject_cast<const QListView*>(view())) {
        m_role = QAccessible::List;
    } else {
        // is this our best guess?
        m_role = QAccessible::Table;
    }
}

QAccessibleTable::~QAccessibleTable()
{
}

QHeaderView *QAccessibleTable::horizontalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view())) {
        header = tv->horizontalHeader();
#endif
#ifndef QT_NO_TREEVIEW
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view())) {
        header = tv->header();
#endif
    }
    return header;
}

QHeaderView *QAccessibleTable::verticalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view())) {
        header = tv->verticalHeader();
#endif
    }
    return header;
}

QAccessibleTableCell *QAccessibleTable::cell(const QModelIndex &index) const
{
    if (index.isValid())
        return new QAccessibleTableCell(view(), index, cellRole());
    return 0;
}

QAccessibleInterface *QAccessibleTable::cellAt(int row, int column) const
{
    if (!view()->model())
        return 0;
    Q_ASSERT(role() != QAccessible::Tree);
    QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
    if (!index.isValid()) {
        qWarning() << "QAccessibleTable::cellAt: invalid index: " << index << " for " << view();
        return 0;
    }
    return cell(index);
}

QAccessibleInterface *QAccessibleTable::caption() const
{
    return 0;
}

QString QAccessibleTable::columnDescription(int column) const
{
    if (!view()->model())
        return QString();
    return view()->model()->headerData(column, Qt::Horizontal).toString();
}

int QAccessibleTable::columnCount() const
{
    if (!view()->model())
        return 0;
    return view()->model()->columnCount();
}

int QAccessibleTable::rowCount() const
{
    if (!view()->model())
        return 0;
    return view()->model()->rowCount();
}

int QAccessibleTable::selectedCellCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedIndexes().count();
}

int QAccessibleTable::selectedColumnCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedColumns().count();
}

int QAccessibleTable::selectedRowCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedRows().count();
}

QString QAccessibleTable::rowDescription(int row) const
{
    if (!view()->model())
        return QString();
    return view()->model()->headerData(row, Qt::Vertical).toString();
}

QList<QAccessibleInterface *> QAccessibleTable::selectedCells() const
{
    QList<QAccessibleInterface*> cells;
    if (!view()->selectionModel())
        return cells;
    Q_FOREACH (const QModelIndex &index, view()->selectionModel()->selectedIndexes()) {
        cells.append(cell(index));
    }
    return cells;
}

QList<int> QAccessibleTable::selectedColumns() const
{
    if (!view()->selectionModel())
        return QList<int>();
    QList<int> columns;
    Q_FOREACH (const QModelIndex &index, view()->selectionModel()->selectedColumns()) {
        columns.append(index.column());
    }
    return columns;
}

QList<int> QAccessibleTable::selectedRows() const
{
    if (!view()->selectionModel())
        return QList<int>();
    QList<int> rows;
    Q_FOREACH (const QModelIndex &index, view()->selectionModel()->selectedRows()) {
        rows.append(index.row());
    }
    return rows;
}

QAccessibleInterface *QAccessibleTable::summary() const
{
    return 0;
}

bool QAccessibleTable::isColumnSelected(int column) const
{
    if (!view()->selectionModel())
        return false;
    return view()->selectionModel()->isColumnSelected(column, QModelIndex());
}

bool QAccessibleTable::isRowSelected(int row) const
{
    if (!view()->selectionModel())
        return false;
    return view()->selectionModel()->isRowSelected(row, QModelIndex());
}

bool QAccessibleTable::selectRow(int row)
{
    if (!view()->model() || !view()->selectionModel())
        return false;
    QModelIndex index = view()->model()->index(row, 0, view()->rootIndex());

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectColumns)
        return false;

    switch (view()->selectionMode()) {
    case QAbstractItemView::NoSelection:
        return false;
    case QAbstractItemView::SingleSelection:
        if (view()->selectionBehavior() != QAbstractItemView::SelectRows && columnCount() > 1 )
            return false;
        view()->clearSelection();
        break;
    case QAbstractItemView::ContiguousSelection:
        if ((!row || !view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex()))
            && !view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex()))
            view()->clearSelection();
        break;
    default:
        break;
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return true;
}

bool QAccessibleTable::selectColumn(int column)
{
    if (!view()->model() || !view()->selectionModel())
        return false;
    QModelIndex index = view()->model()->index(0, column, view()->rootIndex());

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectRows)
        return false;

    switch (view()->selectionMode()) {
    case QAbstractItemView::NoSelection:
        return false;
    case QAbstractItemView::SingleSelection:
        if (view()->selectionBehavior() != QAbstractItemView::SelectColumns && rowCount() > 1)
            return false;
    case QAbstractItemView::ContiguousSelection:
        if ((!column || !view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex()))
            && !view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex()))
            view()->clearSelection();
        break;
    default:
        break;
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Columns);
    return true;
}

bool QAccessibleTable::unselectRow(int row)
{
    if (!view()->model() || !view()->selectionModel())
        return false;

    QModelIndex index = view()->model()->index(row, 0, view()->rootIndex());
    if (!index.isValid())
        return false;

    QItemSelection selection(index, index);

    switch (view()->selectionMode()) {
    case QAbstractItemView::SingleSelection:
        //In SingleSelection and ContiguousSelection once an item
        //is selected, there's no way for the user to unselect all items
        if (selectedRowCount() == 1)
            return false;
        break;
    case QAbstractItemView::ContiguousSelection:
        if (selectedRowCount() == 1)
            return false;

        if ((!row || view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex()))
            && view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex())) {
            //If there are rows selected both up the current row and down the current rown,
            //the ones which are down the current row will be deselected
            selection = QItemSelection(index, view()->model()->index(rowCount() - 1, 0, view()->rootIndex()));
        }
    default:
        break;
    }

    view()->selectionModel()->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
    return true;
}

bool QAccessibleTable::unselectColumn(int column)
{
    if (!view()->model() || !view()->selectionModel())
        return false;

    QModelIndex index = view()->model()->index(0, column, view()->rootIndex());
    if (!index.isValid())
        return false;

    QItemSelection selection(index, index);

    switch (view()->selectionMode()) {
    case QAbstractItemView::SingleSelection:
        //In SingleSelection and ContiguousSelection once an item
        //is selected, there's no way for the user to unselect all items
        if (selectedColumnCount() == 1)
            return false;
        break;
    case QAbstractItemView::ContiguousSelection:
        if (selectedColumnCount() == 1)
            return false;

        if ((!column || view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex()))
            && view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex())) {
            //If there are columns selected both at the left of the current row and at the right
            //of the current rown, the ones which are at the right will be deselected
            selection = QItemSelection(index, view()->model()->index(0, columnCount() - 1, view()->rootIndex()));
        }
    default:
        break;
    }

    view()->selectionModel()->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Columns);
    return true;
}

QAccessible::Role QAccessibleTable::role() const
{
    return m_role;
}

QAccessible::State QAccessibleTable::state() const
{
    return QAccessible::State();
}

QAccessibleInterface *QAccessibleTable::childAt(int x, int y) const
{
    QPoint viewportOffset = view()->viewport()->mapTo(view(), QPoint(0,0));
    QPoint indexPosition = view()->mapFromGlobal(QPoint(x, y) - viewportOffset);
    // FIXME: if indexPosition < 0 in one coordinate, return header

    QModelIndex index = view()->indexAt(indexPosition);
    if (index.isValid()) {
        return childFromLogical(logicalIndex(index));
    }
    return 0;
}

int QAccessibleTable::childCount() const
{
    if (!view()->model())
        return 0;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    return (view()->model()->rowCount()+hHeader) * (view()->model()->columnCount()+vHeader);
}

int QAccessibleTable::indexOfChild(const QAccessibleInterface *iface) const
{
    if (!view()->model())
        return -1;
    QSharedPointer<QAccessibleInterface> parent(iface->parent());
    if (parent->object() != view())
        return -1;

    Q_ASSERT(iface->role() != QAccessible::TreeItem); // should be handled by tree class
    if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
        const QAccessibleTableCell* cell = static_cast<const QAccessibleTableCell*>(iface);
        return logicalIndex(cell->m_index);
    } else if (iface->role() == QAccessible::ColumnHeader){
        const QAccessibleTableHeaderCell* cell = static_cast<const QAccessibleTableHeaderCell*>(iface);
        return cell->index + (verticalHeader() ? 1 : 0);
    } else if (iface->role() == QAccessible::RowHeader){
        const QAccessibleTableHeaderCell* cell = static_cast<const QAccessibleTableHeaderCell*>(iface);
        return (cell->index + 1) * (view()->model()->rowCount() + 1);
    } else if (iface->role() == QAccessible::Pane) {
        return 0; // corner button
    } else {
        qWarning() << "WARNING QAccessibleTable::indexOfChild Fix my children..."
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: we are in denial of our children. this should stop.
    return -1;
}

QString QAccessibleTable::text(QAccessible::Text t) const
{
    if (t == QAccessible::Description)
        return view()->accessibleDescription();
    return view()->accessibleName();
}

QRect QAccessibleTable::rect() const
{
    if (!view()->isVisible())
        return QRect();
    QPoint pos = view()->mapToGlobal(QPoint(0, 0));
    return QRect(pos.x(), pos.y(), view()->width(), view()->height());
}

QAccessibleInterface *QAccessibleTable::parent() const
{
    if (view()->parent()) {
        if (qstrcmp("QComboBoxPrivateContainer", view()->parent()->metaObject()->className()) == 0) {
            return QAccessible::queryAccessibleInterface(view()->parent()->parent());
        }
        return QAccessible::queryAccessibleInterface(view()->parent());
    }
    return 0;
}

QAccessibleInterface *QAccessibleTable::child(int index) const
{
    return childFromLogical(index);
}

void *QAccessibleTable::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableInterface)
       return static_cast<QAccessibleTableInterface*>(this);
   return 0;
}

// TREE VIEW

QModelIndex QAccessibleTree::indexFromLogical(int row, int column) const
{
    if (!isValid() || !view()->model())
        return QModelIndex();

    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    if ((row < 0) || (column < 0) || (treeView->d_func()->viewItems.count() <= row)) {
        qWarning() << "QAccessibleTree::indexFromLogical: invalid index: " << row << column << " for " << treeView;
        return QModelIndex();
    }
    QModelIndex modelIndex = treeView->d_func()->viewItems.at(row).index;

    if (modelIndex.isValid() && column > 0) {
        modelIndex = view()->model()->index(modelIndex.row(), column, modelIndex.parent());
    }
    return modelIndex;
}

QAccessibleInterface *QAccessibleTree::childAt(int x, int y) const
{
    if (!view()->model())
        return 0;
    QPoint viewportOffset = view()->viewport()->mapTo(view(), QPoint(0,0));
    QPoint indexPosition = view()->mapFromGlobal(QPoint(x, y) - viewportOffset);

    QModelIndex index = view()->indexAt(indexPosition);
    if (!index.isValid())
        return 0;

    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    int row = treeView->d_func()->viewIndex(index) + (horizontalHeader() ? 1 : 0);
    int column = index.column();

    int i = row * view()->model()->columnCount() + column;
    return child(i);
}

int QAccessibleTree::childCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    Q_ASSERT(treeView);
    if (!view()->model())
        return 0;

    int hHeader = horizontalHeader() ? 1 : 0;
    return (treeView->d_func()->viewItems.count() + hHeader)* view()->model()->columnCount();
}


QAccessibleInterface *QAccessibleTree::child(int index) const
{
    if ((index < 0) || (!view()->model()))
        return 0;
    int hHeader = horizontalHeader() ? 1 : 0;

    if (hHeader) {
        if (index < view()->model()->columnCount()) {
            return new QAccessibleTableHeaderCell(view(), index, Qt::Horizontal);
        } else {
            index -= view()->model()->columnCount();
        }
    }

    int row = index / view()->model()->columnCount();
    int column = index % view()->model()->columnCount();
    QModelIndex modelIndex = indexFromLogical(row, column);
    if (modelIndex.isValid()) {
        return cell(modelIndex);
    }
    return 0;
}

int QAccessibleTree::rowCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    Q_ASSERT(treeView);
    return treeView->d_func()->viewItems.count();
}

int QAccessibleTree::indexOfChild(const QAccessibleInterface *iface) const
{
    if (!view()->model())
        return -1;
    QSharedPointer<QAccessibleInterface> parent(iface->parent());
    if (parent->object() != view())
        return -1;

    if (iface->role() == QAccessible::TreeItem) {
        const QAccessibleTableCell* cell = static_cast<const QAccessibleTableCell*>(iface);
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
        Q_ASSERT(treeView);
        int row = treeView->d_func()->viewIndex(cell->m_index) + (horizontalHeader() ? 1 : 0);
        int column = cell->m_index.column();

        int index = row * view()->model()->columnCount() + column;
        //qDebug() << "QAccessibleTree::indexOfChild r " << row << " c " << column << "index " << index;
        Q_ASSERT(index >= treeView->model()->columnCount());
        return index;
    } else if (iface->role() == QAccessible::ColumnHeader){
        const QAccessibleTableHeaderCell* cell = static_cast<const QAccessibleTableHeaderCell*>(iface);
        //qDebug() << "QAccessibleTree::indexOfChild header " << cell->index;
        return cell->index;
    } else {
        qWarning() << "WARNING QAccessibleTable::indexOfChild invalid child"
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: add scrollbars and don't just ignore them
    return -1;
}

QAccessibleInterface *QAccessibleTree::cellAt(int row, int column) const
{
    QModelIndex index = indexFromLogical(row, column);
    if (!index.isValid()) {
        qWarning() << "Requested invalid tree cell: " << row << column;
        return 0;
    }
    return new QAccessibleTableCell(view(), index, cellRole());
}

QString QAccessibleTree::rowDescription(int) const
{
    return QString(); // no headers for rows in trees
}

bool QAccessibleTree::isRowSelected(int row) const
{
    if (!view()->selectionModel())
        return false;
    QModelIndex index = indexFromLogical(row);
    return view()->selectionModel()->isRowSelected(index.row(), index.parent());
}

bool QAccessibleTree::selectRow(int row)
{
    if (!view()->selectionModel())
        return false;
    QModelIndex index = indexFromLogical(row);

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectColumns)
        return false;

    switch (view()->selectionMode()) {
    case QAbstractItemView::NoSelection:
        return false;
    case QAbstractItemView::SingleSelection:
        if ((view()->selectionBehavior() != QAbstractItemView::SelectRows) && (columnCount() > 1))
            return false;
        view()->clearSelection();
        break;
    case QAbstractItemView::ContiguousSelection:
        if ((!row || !view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex()))
            && !view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex()))
            view()->clearSelection();
        break;
    default:
        break;
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return true;
}

// TABLE CELL

QAccessibleTableCell::QAccessibleTableCell(QAbstractItemView *view_, const QModelIndex &index_, QAccessible::Role role_)
    : /* QAccessibleSimpleEditableTextInterface(this), */ view(view_), m_index(index_), m_role(role_)
{
    if (!index_.isValid())
        qWarning() << "QAccessibleTableCell::QAccessibleTableCell with invalid index: " << index_;
}

void *QAccessibleTableCell::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableCellInterface)
        return static_cast<QAccessibleTableCellInterface*>(this);
    return 0;
}

int QAccessibleTableCell::columnExtent() const { return 1; }
int QAccessibleTableCell::rowExtent() const { return 1; }

QList<QAccessibleInterface*> QAccessibleTableCell::rowHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (verticalHeader()) {
        headerCell.append(new QAccessibleTableHeaderCell(view, m_index.row(), Qt::Vertical));
    }
    return headerCell;
}

QList<QAccessibleInterface*> QAccessibleTableCell::columnHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (horizontalHeader()) {
        headerCell.append(new QAccessibleTableHeaderCell(view, m_index.column(), Qt::Horizontal));
    }
    return headerCell;
}

QHeaderView *QAccessibleTableCell::horizontalHeader() const
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

QHeaderView *QAccessibleTableCell::verticalHeader() const
{
    QHeaderView *header = 0;
#ifndef QT_NO_TABLEVIEW
    if (const QTableView *tv = qobject_cast<const QTableView*>(view))
        header = tv->verticalHeader();
#endif
    return header;
}

int QAccessibleTableCell::columnIndex() const
{
    return m_index.column();
}

int QAccessibleTableCell::rowIndex() const
{
    if (role() == QAccessible::TreeItem) {
       const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
       Q_ASSERT(treeView);
       int row = treeView->d_func()->viewIndex(m_index);
       return row;
    }
    return m_index.row();
}

bool QAccessibleTableCell::isSelected() const
{
    return view->selectionModel()->isSelected(m_index);
}

void QAccessibleTableCell::rowColumnExtents(int *row, int *column, int *rowExtents, int *columnExtents, bool *selected) const
{
    *row = m_index.row();
    *column = m_index.column();
    *rowExtents = 1;
    *columnExtents = 1;
    *selected = isSelected();
}

QAccessibleInterface *QAccessibleTableCell::table() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessible::Role QAccessibleTableCell::role() const
{
    return m_role;
}

QAccessible::State QAccessibleTableCell::state() const
{
    QAccessible::State st;
    QRect globalRect = view->rect();
    globalRect.translate(view->mapToGlobal(QPoint(0,0)));
    if (!globalRect.intersects(rect()))
        st.invisible = true;

    if (view->selectionModel()->isSelected(m_index))
        st.selected = true;
    if (view->selectionModel()->currentIndex() == m_index)
        st.focused = true;
    if (m_index.model()->data(m_index, Qt::CheckStateRole).toInt() == Qt::Checked)
        st.checked = true;

    Qt::ItemFlags flags = m_index.flags();
    if (flags & Qt::ItemIsSelectable) {
        st.selectable = true;
        st.focusable = true;
        if (view->selectionMode() == QAbstractItemView::MultiSelection)
            st.multiSelectable = true;
        if (view->selectionMode() == QAbstractItemView::ExtendedSelection)
            st.extSelectable = true;
    }
    if (m_role == QAccessible::TreeItem) {
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
        if (treeView->model()->hasChildren(m_index))
            st.expandable = true;
        if (treeView->isExpanded(m_index))
            st.expanded = true;
    }
    return st;
}


QRect QAccessibleTableCell::rect() const
{
    QRect r;
    r = view->visualRect(m_index);

    if (!r.isNull())
        r.translate(view->viewport()->mapTo(view, QPoint(0,0)));
        r.translate(view->mapToGlobal(QPoint(0, 0)));
    return r;
}

QString QAccessibleTableCell::text(QAccessible::Text t) const
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

void QAccessibleTableCell::setText(QAccessible::Text /*t*/, const QString &text)
{
    if (!(m_index.flags() & Qt::ItemIsEditable))
        return;
    view->model()->setData(m_index, text);
}

bool QAccessibleTableCell::isValid() const
{
    return view && view->model() && m_index.isValid();
}

QAccessibleInterface *QAccessibleTableCell::parent() const
{
    if (m_role == QAccessible::TreeItem)
        return new QAccessibleTree(view);

    return new QAccessibleTable(view);
}

QAccessibleInterface *QAccessibleTableCell::child(int) const
{
    return 0;
}

QAccessibleTableHeaderCell::QAccessibleTableHeaderCell(QAbstractItemView *view_, int index_, Qt::Orientation orientation_)
    : view(view_), index(index_), orientation(orientation_)
{
    Q_ASSERT(index_ >= 0);
}

QAccessible::Role QAccessibleTableHeaderCell::role() const
{
    if (orientation == Qt::Horizontal)
        return QAccessible::ColumnHeader;
    return QAccessible::RowHeader;
}

QAccessible::State QAccessibleTableHeaderCell::state() const
{
    return QAccessible::State();
}

QRect QAccessibleTableHeaderCell::rect() const
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
    if (!header)
        return QRect();
    QPoint zero = header->mapToGlobal(QPoint(0, 0));
    int sectionSize = header->sectionSize(index);
    int sectionPos = header->sectionPosition(index);
    return orientation == Qt::Horizontal
            ? QRect(zero.x() + sectionPos, zero.y(), sectionSize, header->height())
            : QRect(zero.x(), zero.y() + sectionPos, header->width(), sectionSize);
}

QString QAccessibleTableHeaderCell::text(QAccessible::Text t) const
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

void QAccessibleTableHeaderCell::setText(QAccessible::Text, const QString &)
{
    return;
}

bool QAccessibleTableHeaderCell::isValid() const
{
    return view && view->model() && (index >= 0)
            && ((orientation == Qt::Horizontal) ? (index < view->model()->columnCount()) : (index < view->model()->rowCount()));
}

QAccessibleInterface *QAccessibleTableHeaderCell::parent() const
{
    if (false) {
#ifndef QT_NO_TREEVIEW
    } else if (qobject_cast<const QTreeView*>(view)) {
        return new QAccessibleTree(view);
#endif
    } else {
        return new QAccessibleTable(view);
    }
}

QAccessibleInterface *QAccessibleTableHeaderCell::child(int) const
{
    return 0;
}

#endif // QT_NO_ITEMVIEWS

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
