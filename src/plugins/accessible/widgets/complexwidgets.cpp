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

#include "complexwidgets.h"

#include <qaccessible.h>
#include <qapplication.h>
#include <qabstractbutton.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qtabbar.h>
#include <qcombobox.h>
#include <qlistview.h>
#include <qtableview.h>
#include <qlineedit.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qtreeview.h>
#include <private/qtabbar_p.h>
#include <QAbstractScrollArea>
#include <QScrollArea>
#include <QScrollBar>
#include <QDebug>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

QString Q_GUI_EXPORT qt_accStripAmp(const QString &text);

#if 0
#ifndef QT_NO_ITEMVIEWS
/*
The MSDN article "Exposing Data Tables through Microsoft Active Accessibility" explains
how data tables should be exposed. Url: http://msdn2.microsoft.com/en-us/library/ms971325.aspx
Basically, the model is like this:

ROLE_SYSTEM_TABLE
  |- ROLE_SYSTEM_ROW
  |  |- ROLE_SYSTEM_ROWHEADER
  |  |- ROLE_SYSTEM_COLUMNHEADER
  |  |- ROLE_SYSTEM_COLUMNHEADER
  |  |- ROLE_SYSTEM_COLUMNHEADER
  |     '- ..
  |- ROLE_SYSTEM_ROW
  |  |- ROLE_SYSTEM_ROWHEADER
  |  |- ROLE_SYSTEM_CELL
  |  |- ROLE_SYSTEM_CELL
  |  |- ROLE_SYSTEM_CELL
  |   '- ..
  |- ROLE_SYSTEM_ROW
  |  |- ROLE_SYSTEM_ROWHEADER
  |  |- ROLE_SYSTEM_CELL
  |  |- ROLE_SYSTEM_CELL
  |  |- ROLE_SYSTEM_CELL
  |   '- ..
   '- ..

The headers of QTreeView is also represented like this.
*/
QAccessibleItemRow::QAccessibleItemRow(QAbstractItemView *aView, const QModelIndex &index, bool isHeader)
    : row(index), view(aView), m_header(isHeader)
{
}

QHeaderView *QAccessibleItemRow::horizontalHeader() const
{
    QHeaderView *header = 0;
    if (m_header) {
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
    }
    return header;
}

QHeaderView *QAccessibleItemRow::verticalHeader() const
{
    QHeaderView *header = 0;
#ifndef QT_NO_TABLEVIEW
    if (const QTableView *tv = qobject_cast<const QTableView*>(view))
        header = tv->verticalHeader();
#endif
    return header;
}

int QAccessibleItemRow::logicalFromChild(QHeaderView *header, int child) const
{
    int logical = -1;
    if (header->sectionsHidden()) {
        int kid = 0;
        for (int i = 0; i < header->count(); ++i) {
            if (!header->isSectionHidden(i))
                ++kid;
            if (kid == child) {
                logical = i;
                break;
            }
        }
    } else {
        logical = child - 1;
    }
    return logical;
}

QRect QAccessibleItemRow::rect(int child) const
{
    QRect r;
    if (view && view->isVisible()) {
        if (QHeaderView *header = horizontalHeader()) {
            if (!child) {
                r = header->rect();
            } else {
                if (QHeaderView *vheader = verticalHeader()) {
                    if (child == 1) {
                        int w = vheader->width();
                        int h = header->height();
                        r.setRect(0, 0, w, h);
                    }
                    --child;
                }
                if (child) {
                    int logical = logicalFromChild(header, child);
                    int w = header->sectionSize(logical);
                    r.setRect(header->sectionViewportPosition(logical), 0, w, header->height());
                    r.translate(header->mapTo(view, QPoint(0, 0)));
                }
            }
        } else if (row.isValid()) {
            if (!child) {
                QModelIndex parent = row.parent();
                const int colCount = row.model()->columnCount(parent);
                for (int i = 0; i < colCount; ++i)
                    r |= view->visualRect(row.model()->index(row.row(), i, parent));
                r.translate(view->viewport()->mapTo(view, QPoint(0,0)));

                if (const QHeaderView *vheader = verticalHeader()) { // include the section of the vertical header
                    QRect re;
                    int logicalRow = row.row();
                    int h = vheader->sectionSize(logicalRow);
                    re.setRect(0, vheader->sectionViewportPosition(logicalRow), vheader->width(), h);
                    re.translate(vheader->mapTo(view, QPoint(0, 0)));
                    r |= re;
                }
            } else {
                if (QHeaderView *vheader = verticalHeader()) {
                    if (child == 1) {
                        int logicalRow = row.row();
                        int h = vheader->sectionSize(logicalRow);
                        r.setRect(0, vheader->sectionViewportPosition(logicalRow), vheader->width(), h);
                        r.translate(vheader->mapTo(view, QPoint(0, 0)));
                    }
                    --child;
                }
                if (child) {
                    r = view->visualRect(childIndex(child));
                    r.translate(view->viewport()->mapTo(view, QPoint(0,0)));
                }
            }
        }
    }
    if (!r.isNull())
        r.translate(view->mapToGlobal(QPoint(0, 0)));

    return r;
}

int QAccessibleItemRow::treeLevel() const
{
    int level = 0;
    QModelIndex idx = row;
    while (idx.isValid()) {
        idx = idx.parent();
        ++level;
    }
    return level;
}

QString QAccessibleItemRow::text_helper(int child) const
{
    QString value;
    if (m_header) {
        if (!child)
            return QString();
        if (verticalHeader()) {
            if (child == 1)
                return QString();
            --child;
        }
        QHeaderView *header = horizontalHeader();
        int logical = logicalFromChild(header, child);
        value = view->model()->headerData(logical, Qt::Horizontal, Qt::AccessibleTextRole).toString();
        if (value.isEmpty())
            value = view->model()->headerData(logical, Qt::Horizontal).toString();
        return value;
    } else {
        if (!child) {   // for one-column views (i.e. QListView)
            if (children().count() >= 1)
                child = 1;
            else
                return QString();
        }
        if (verticalHeader()) {
            if (child == 1) {
                int logical = row.row();
                value = view->model()->headerData(logical, Qt::Vertical, Qt::AccessibleTextRole).toString();
                if (value.isEmpty())
                    value = view->model()->headerData(logical, Qt::Vertical).toString();
                return value;
            } else {
                --child;
            }
        }
    }
    if (value.isEmpty()) {
        QModelIndex idx = childIndex(child);
        if (idx.isValid()) {
            value = idx.model()->data(idx, Qt::AccessibleTextRole).toString();
            if (value.isEmpty())
                value = idx.model()->data(idx, Qt::DisplayRole).toString();
        }
    }
    return value;
}

QString QAccessibleItemRow::text(Text t, int child) const
{
    QString value;
    if (t == Name) {
        value = text_helper(child);
    } else if (t == Value) {
#ifndef QT_NO_TREEVIEW
        if (qobject_cast<const QTreeView*>(view)) {
            if (child == 0)
                value = QString::number(treeLevel());
        } else
#endif
        {
            value = text_helper(child);
        }
    } else if (t == Description) {
#ifndef QT_NO_TREEVIEW
        if (child == 0 && qobject_cast<const QTreeView*>(view)) {
            // We store the tree coordinates of the current item in the description.
            // This enables some screen readers to report where the focus is
            // in a tree view. (works in JAWS). Also, Firefox does the same thing.
            // For instance the description "L2, 4 of 25 with 24" means
            // "L2": Tree Level 2
            // "4 of 25": We are item 4 out of in total 25 other siblings
            // "with 24": We have 24 children. (JAWS does not read this number)

            // level
            int level = treeLevel();

            QAbstractItemModel *m = view->model();
            // totalSiblings and itemIndex
            QModelIndex parent = row.parent();
            int rowCount = m->rowCount(parent);
            int itemIndex = -1;
            int totalSiblings = 0;
            for (int i = 0 ; i < rowCount; ++i) {
                QModelIndex sibling = row.sibling(i, 0);
                if (!view->isIndexHidden(sibling))
                    ++totalSiblings;
                if (row == sibling)
                    itemIndex = totalSiblings;
            }
            int totalChildren = m->rowCount(row);   // JAWS does not report child count, so we do
                                                    // this simple and efficient.
                                                    // (don't check if they are all visible).
            value = QString::fromAscii("L%1, %2 of %3 with %4").arg(level).arg(itemIndex).arg(totalSiblings).arg(totalChildren);
        } else
#endif // QT_NO_TREEVIEW
        {
            if (!m_header) {
                if (child == 0 && children().count() >= 1)
                    child = 1;
                if (verticalHeader()) {
                    if (child == 1) {
                        value = view->model()->headerData(row.row(), Qt::Vertical).toString();
                    }
                    --child;
                }
                if (child) {
                    QModelIndex idx = childIndex(child);
                    value = idx.model()->data(idx, Qt::AccessibleDescriptionRole).toString();
                }

            }
        }
    }
    return value;
}

void QAccessibleItemRow::setText(Text t, int child, const QString &text)
{
    if (m_header) {
        if (child)
            view->model()->setHeaderData(child - 1, Qt::Horizontal, text);
        // child == 0 means the cell to the left of the horizontal header, which is empty!?
    } else {
        if (!child) {
            if (children().count() == 1)
                child = 1;
            else
                return;
        }

        if (verticalHeader()) {
            if (child == 1) {
                view->model()->setHeaderData(row.row(), Qt::Vertical, text);
                return;
            }
            --child;
        }
        QModelIndex idx = childIndex(child);
        if (!idx.isValid())
            return;

        switch (t) {
        case Description:
            const_cast<QAbstractItemModel *>(idx.model())->setData(idx, text,
                                             Qt::AccessibleDescriptionRole);
            break;
        case Value:
            const_cast<QAbstractItemModel *>(idx.model())->setData(idx, text, Qt::EditRole);
            break;
        default:
            break;
        }
    }
}

QModelIndex QAccessibleItemRow::childIndex(int child) const
{
    QList<QModelIndex> kids = children();
    Q_ASSERT(child >= 1 && child <= kids.count());
    return kids.at(child - 1);
}

QList<QModelIndex> QAccessibleItemRow::children() const
{
    QList<QModelIndex> kids;
    for (int i = 0; i < row.model()->columnCount(row.parent()); ++i) {
        QModelIndex idx = row.model()->index(row.row(), i, row.parent());
        if (!view->isIndexHidden(idx)) {
            kids << idx;
        }
    }
    return kids;
}

bool QAccessibleItemRow::isValid() const
{
    return m_header ? true : row.isValid();
}

QObject *QAccessibleItemRow::object() const
{
    return 0;
}

int QAccessibleItemRow::childCount() const
{
    int count = 0;
    if (QHeaderView *header = horizontalHeader()) {
        count = header->count() - header->hiddenSectionCount();
    } else {
        count = children().count();
    }
#ifndef QT_NO_TABLEVIEW
    if (qobject_cast<const QTableView*>(view)) {
        if (verticalHeader())
            ++count;
    }
#endif
    return count;
}

int QAccessibleItemRow::indexOfChild(const QAccessibleInterface *iface) const
{
    if (!iface || iface->role() != Row)
        return -1;

    //### meaningless code?
    QList<QModelIndex> kids = children();
    QModelIndex idx = static_cast<const QAccessibleItemRow *>(iface)->row;
    if (!idx.isValid())
        return -1;
    return kids.indexOf(idx) + 1;
}

QAccessible::Relation QAccessibleItemRow::relationTo(int child, const QAccessibleInterface *other,
        int otherChild) const
{
    if (!child && !otherChild && other->object() == view)
        return Child;
    if (!child && !otherChild && other == this)
        return Self;
    if (!child && otherChild && other == this)
        return Ancestor;
    if (child && otherChild && other == this)
        return Sibling;
    return Unrelated;
}

int QAccessibleItemRow::childAt(int x, int y) const
{
    if (!view || !view->isVisible())
        return -1;

    for (int i = childCount(); i >= 0; --i) {
        if (rect(i).contains(x, y))
            return i;
    }
    return -1;
}

QAbstractItemView::CursorAction QAccessibleItemRow::toCursorAction(
                                           QAccessible::Relation rel)
{
    switch (rel) {
    case QAccessible::Up:
        return QAbstractItemView::MoveUp;
    case QAccessible::Down:
        return QAbstractItemView::MoveDown;
    case QAccessible::Left:
        return QAbstractItemView::MoveLeft;
    case QAccessible::Right:
        return QAbstractItemView::MoveRight;
    default:
        Q_ASSERT(false);
    }
    // should never be reached.
    return QAbstractItemView::MoveRight;
}

QAccessibleInterface *QAccessibleItemRow::parent() const
{
    return new QAccessibleItemView(view->viewport());
}

QAccessibleInterface *QAccessibleItemRow::child(int) const
{
    // FIXME? port to IA2 table2.
    return 0;
}

int QAccessibleItemRow::navigate(RelationFlag relation, int index,
                                 QAccessibleInterface **iface) const
{
    *iface = 0;
    if (!view)
        return -1;

    switch (relation) {
    case Ancestor:
        *iface = parent();
        return *iface ? 0 : -1;
    case Child: {
        if (!index)
            return -1;
        if (index < 1 && index > childCount())
            return -1;

        return index;}
    case Sibling:
        if (index) {
            QAccessibleInterface *ifaceParent = parent();
            if (ifaceParent) {
                *iface = ifaceParent->child(index - 1);
                delete ifaceParent;
                return *iface ? 0 : -1;
            }
        }
        return -1;
    case Up:
    case Down:
    case Left:
    case Right: {
        // This is in the "not so nice" category. In order to find out which item
        // is geometrically around, we have to set the current index, navigate
        // and restore the index as well as the old selection
        view->setUpdatesEnabled(false);
        const QModelIndex oldIdx = view->currentIndex();
        QList<QModelIndex> kids = children();
        const QModelIndex currentIndex = index ? kids.at(index - 1) : QModelIndex(row);
        const QItemSelection oldSelection = view->selectionModel()->selection();
        view->setCurrentIndex(currentIndex);
        const QModelIndex idx = view->moveCursor(toCursorAction(relation), Qt::NoModifier);
        view->setCurrentIndex(oldIdx);
        view->selectionModel()->select(oldSelection, QItemSelectionModel::ClearAndSelect);
        view->setUpdatesEnabled(true);
        if (!idx.isValid())
            return -1;

        if (idx.parent() != row.parent() || idx.row() != row.row())
            *iface = new QAccessibleItemRow(view, idx);
        return index ? kids.indexOf(idx) + 1 : 0; }
    default:
        break;
    }

    return -1;
}

QAccessible::Role QAccessibleItemRow::role(int child) const
{
    if (false) {
#ifndef QT_NO_TREEVIEW
    } else if (qobject_cast<const QTreeView*>(view)) {
        if (horizontalHeader()) {
            if (!child)
                return Row;
            return ColumnHeader;
        }
        return TreeItem;
#endif
#ifndef QT_NO_LISTVIEW
    } else if (qobject_cast<const QListView*>(view)) {
        return ListItem;
#endif
#ifndef QT_NO_TABLEVIEW
    } else if (qobject_cast<const QTableView *>(view)) {
        if (!child)
            return Row;
        if (child == 1) {
            if (verticalHeader())
                return RowHeader;
        }
        if (m_header)
            return ColumnHeader;
#endif
    }
    return Cell;
}

QAccessible::State QAccessibleItemRow::state(int child) const
{
    State st = Normal;

    if (!view)
        return st;

    QAccessibleInterface *parentIface = parent();
    QRect globalRect;
    if (parentIface) {
        globalRect = parentIface->rect(0);
        delete parentIface;
    }
    if (!globalRect.intersects(rect(child)))
        st |= Invisible;

    if (!horizontalHeader()) {
        if (!(st & Invisible)) {
            if (child) {
                if (QHeaderView *vheader = verticalHeader() ) {
                    if (child == 1) {
                        if (!vheader->isVisible())
                            st |= Invisible;
                    }
                    --child;
                }
                if (child) {
                    QModelIndex idx = childIndex(child);
                    if (!idx.isValid())
                        return st;

                    if (view->selectionModel()->isSelected(idx))
                        st |= Selected;
                    if (view->selectionModel()->currentIndex() == idx)
                        st |= Focused;
                    if (idx.model()->data(idx, Qt::CheckStateRole).toInt() == Qt::Checked)
                        st |= Checked;

                    Qt::ItemFlags flags = idx.flags();
                    if (flags & Qt::ItemIsSelectable) {
                        st |= Selectable;
                        if (view->selectionMode() == QAbstractItemView::MultiSelection)
                            st |= MultiSelectable;
                        if (view->selectionMode() == QAbstractItemView::ExtendedSelection)
                            st |= ExtSelectable;
                    }
                }
            } else {
                Qt::ItemFlags flags = row.flags();
                if (flags & Qt::ItemIsSelectable) {
                    st |= Selectable;
                    st |= Focusable;
                }
                if (view->selectionModel()->isRowSelected(row.row(), row.parent()))
                    st |= Selected;
                if (view->selectionModel()->currentIndex().row() == row.row())
                    st |= Focused;
            }
        }
    }

    return st;
}

int QAccessibleItemRow::userActionCount(int) const
{
    return 0;
}

QString QAccessibleItemRow::actionText(int, Text, int) const
{
    return QString();
}

static QItemSelection rowAt(const QModelIndex &idx)
{
    return QItemSelection(idx.sibling(idx.row(), 0),
                idx.sibling(idx.row(), idx.model()->columnCount(idx.parent())));
}

bool QAccessibleItemRow::doAction(int action, int child, const QVariantList & /*params*/)
{
    if (!view)
        return false;

    if (verticalHeader())
        --child;

    QModelIndex idx = child ? childIndex(child) : QModelIndex(row);
    if (!idx.isValid())
        return false;

    QItemSelectionModel::SelectionFlags command = QItemSelectionModel::NoUpdate;

    switch  (action) {
    case SetFocus:
        view->setCurrentIndex(idx);
        return true;
    case ExtendSelection:
        if (!child)
            return false;
        view->selectionModel()->select(QItemSelection(view->currentIndex(), idx),
                    QItemSelectionModel::SelectCurrent);
        return true;
    case Select:
        command = QItemSelectionModel::ClearAndSelect;
        break;
    case ClearSelection:
        command = QItemSelectionModel::Clear;
        break;
    case RemoveSelection:
        command = QItemSelectionModel::Deselect;
        break;
    case AddToSelection:
        command = QItemSelectionModel::SelectCurrent;
        break;
    }
    if (command == QItemSelectionModel::NoUpdate)
        return false;

    if (child)
        view->selectionModel()->select(idx, command);
    else
        view->selectionModel()->select(rowAt(row), command);
    return true;
}

class ModelIndexIterator
{
public:
    ModelIndexIterator(QAbstractItemView *view, const QModelIndex &start = QModelIndex()) : m_view(view)
    {
#ifndef QT_NO_LISTVIEW
        list = qobject_cast<QListView*>(m_view);
#endif
#ifndef QT_NO_TREEVIEW
        tree = qobject_cast<QTreeView*>(m_view);
#endif
#ifndef QT_NO_TABLEVIEW
        table = qobject_cast<QTableView*>(m_view);
#endif
        if (start.isValid()) {
            m_current = start;
        } else if (m_view && m_view->model()) {
            m_current = view->rootIndex().isValid() ? 
                        view->rootIndex().child(0,0) : view->model()->index(0, 0);
        }
    }

    bool next(int count = 1) {
        for (int i = 0; i < count; ++i) {
            do {
                if (m_current.isValid()) {
                    const QAbstractItemModel *m = m_current.model();
#ifndef QT_NO_TREEVIEW
                    if (tree && m_current.model()->hasChildren(m_current) && tree->isExpanded(m_current)) {
                        m_current = m_current.child(0, 0);
                    } else
#endif
                    {
                        int row = m_current.row();
                        QModelIndex par = m_current.parent();
                        
                        // Go up to the parent if we reach the end of the rows
                        // If m_curent becomses invalid, stop going up.
                        while (row + 1 >= m->rowCount(par)) {
                            m_current = par;
                            if (m_current.isValid()) {
                                row = m_current.row();
                                par = m_current.parent();
                            } else {
                                row = 0;
                                par = QModelIndex();
                                break;
                            }
                        }

                        if (m_current.isValid())
                            m_current = m_current.sibling(row + 1, 0);
                    }
                }
            } while (isHidden());
        }
        return m_current.isValid();
    }

    bool isHidden() const {
        if (false) {
#ifndef QT_NO_LISTVIEW
        } else if (list) {
            return list->isRowHidden(m_current.row());
#endif
#ifndef QT_NO_TREEVIEW
        } else if (tree) {
            return tree->isRowHidden(m_current.row(), m_current.parent());
#endif
#ifndef QT_NO_TABLEVIEW
        } else if (table) {
            return table->isRowHidden(m_current.row());
#endif
        }
        return false;
    }

    QModelIndex current() const {
        return m_current;
    }

private:
    QModelIndex m_current;
    QAbstractItemView *m_view;

#ifndef QT_NO_TREEVIEW
    QTreeView *tree;
#endif
#ifndef QT_NO_LISTVIEW
    QListView *list;
#endif
#ifndef QT_NO_TABLEVIEW
    QTableView *table;
#endif
};

QAccessibleItemView::QAccessibleItemView(QWidget *w)
    : QAccessibleAbstractScrollArea(w->objectName() == QLatin1String("qt_scrollarea_viewport") ? w->parentWidget() : w)
{
    atVP = w->objectName() == QLatin1String("qt_scrollarea_viewport");

}


QHeaderView *QAccessibleItemView::horizontalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(itemView())) {
        header = tv->horizontalHeader();
#endif
#ifndef QT_NO_TREEVIEW
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(itemView())) {
        header = tv->header();
#endif
    }
    return header;
}

QHeaderView *QAccessibleItemView::verticalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(itemView())) {
        header = tv->verticalHeader();
#endif
    }
    return header;
}


bool QAccessibleItemView::isValidChildRole(QAccessible::Role role) const
{
    if (atViewport()) {
        if (false) {
#ifndef QT_NO_TREEVIEW
        } else if (qobject_cast<const QTreeView*>(itemView())) {
            return (role == TreeItem || role == Row);
#endif
#ifndef QT_NO_LISTVIEW
        } else if (qobject_cast<const QListView*>(itemView())) {
            return (role == ListItem);
#endif
        }
        // TableView
        return role == Row;
    } else {
        if (false) {
#ifndef QT_NO_TREEVIEW
        } else if (qobject_cast<const QTreeView*>(itemView())) {
            return (role == Tree);
#endif
#ifndef QT_NO_LISTVIEW
        } else if (qobject_cast<const QListView*>(itemView())) {
            return (role == List);
#endif
        }
        // TableView
        return (role == Table);
    }
}

QObject *QAccessibleItemView::object() const
{
    QObject *view = QAccessibleAbstractScrollArea::object();
    Q_ASSERT(qobject_cast<const QAbstractItemView *>(view));
    if (atViewport())
        view = qobject_cast<const QAbstractItemView *>(view)->viewport();
    return view;
}

QAbstractItemView *QAccessibleItemView::itemView() const
{
    return qobject_cast<QAbstractItemView *>(QAccessibleAbstractScrollArea::object());
}

int QAccessibleItemView::indexOfChild(const QAccessibleInterface *iface) const
{
    if (atViewport()) {
        if (!iface || !isValidChildRole(iface->role(0)))
            return -1;

        int entry = -1;
        // ### This will fail if a row is hidden.
        const QAccessibleItemRow *ifRow = static_cast<const QAccessibleItemRow *>(iface);
        if (ifRow->horizontalHeader())
            return 1;

        QModelIndex idx = ifRow->row;
        if (!idx.isValid())
            return -1;

        entry = entryFromIndex(idx);
        if (horizontalHeader())
            ++entry;

        return entry;

    } else {
        return QAccessibleAbstractScrollArea::indexOfChild(iface);
    }
}

QModelIndex QAccessibleItemView::childIndex(int child) const
{
    if (!atViewport())
        return QModelIndex();
    ModelIndexIterator it(itemView());
    it.next(child - 1);
    return it.current();
}

int QAccessibleItemView::entryFromIndex(const QModelIndex &index) const
{
    int entry = -1;
    if (false) {
#ifndef QT_NO_TREEVIEW
    } else if (QTreeView *tree = qobject_cast<QTreeView*>(itemView())) {
        entry = tree->visualIndex(index) + 1;
#endif
#ifndef QT_NO_LISTVIEW
    } else if (QListView *list = qobject_cast<QListView*>(itemView())) {
        entry = list->visualIndex(index) + 1;
#endif
#ifndef QT_NO_TABLEVIEW
    } else if (QTableView *table = qobject_cast<QTableView*>(itemView())) {
        entry = table->visualIndex(index) + 1;
#endif
    }
    return entry;
}

int QAccessibleItemView::childCount() const
{
    if (atViewport()) {
        if (itemView()->model() == 0)
            return 0;
        QAbstractItemModel *m = itemView()->model();
        QModelIndex idx = m->index(0,0);
        if (!idx.isValid())
            return 0;
        ModelIndexIterator it(itemView());
        int count = 1;
        while (it.next()) {
            ++count;
        }
        if (horizontalHeader())
            ++count;

        return count;
    } else {
        return QAccessibleAbstractScrollArea::childCount();
    }
}

QString QAccessibleItemView::text(Text t, int child) const
{
    if (atViewport()) {
        if (!child)
            return QAccessibleAbstractScrollArea::text(t, child);

        QAccessibleItemRow item(itemView(), childIndex(child));
        if (item.isValid()) {
            return item.text(t, 1);
        } else {
            return QString();
        }
    } else {
        return QAccessibleAbstractScrollArea::text(t, child);
    }
}

void QAccessibleItemView::setText(Text t, int child, const QString &text)
{
    if (atViewport()) {
        if (!child) {
            QAccessibleAbstractScrollArea::setText(t, child, text);
            return;
        }

        QAccessibleItemRow item(itemView(), childIndex(child));
        item.setText(t, 1, text);
    } else {
        QAccessibleAbstractScrollArea::setText(t, child, text);
    }
}

QRect QAccessibleItemView::rect(int childIndex) const
{
    if (atViewport()) {
        QRect r;
        if (!childIndex) {
            // Make sure that the rect *include* the vertical and horizontal headers, while
            // not including the potential vertical and horizontal scrollbars.
            QAbstractItemView *w = itemView();

            int vscrollWidth = 0;
            const QScrollBar *sb = w->verticalScrollBar();
            if (sb && sb->isVisible())
                vscrollWidth = sb->width();

            int hscrollHeight = 0;
            sb = w->horizontalScrollBar();
            if (sb && sb->isVisible())
                hscrollHeight = sb->height();

            QPoint globalPos = w->mapToGlobal(QPoint(0,0));
            r = w->rect().translated(globalPos);
            if (w->isRightToLeft()) {
                r.adjust(vscrollWidth, 0, 0, -hscrollHeight);
            } else {
                r.adjust(0, 0, -vscrollWidth, -hscrollHeight);
            }
        } else {
            QAccessibleInterface *iface = child(childIndex - 1);
            if (iface) {
                r = iface->rect(0);
                delete iface;
            }
        }
        return r;
    } else {
        QRect r = QAccessibleAbstractScrollArea::rect(childIndex);
        if (childIndex == 1) {
            // include the potential vertical and horizontal headers

            const QHeaderView *header = verticalHeader();
            int headerWidth = (header && header->isVisible()) ? header->width() : 0;
            header = horizontalHeader();
            int headerHeight= (header && header->isVisible()) ? header->height() : 0;
            if (itemView()->isRightToLeft()) {
                r.adjust(0, -headerHeight, headerWidth, 0);
            } else {
                r.adjust(-headerWidth, -headerHeight, 0, 0);
            }
        }
        return r;
    }
}

int QAccessibleItemView::childAt(int x, int y) const
{
    if (atViewport()) {
        QPoint p(x, y);
        for (int i = childCount(); i >= 0; --i) {
            if (rect(i).contains(p))
                return i;
        }
        return -1;
    } else {
        return QAccessibleAbstractScrollArea::childAt(x, y);
    }
}

QAccessible::Role QAccessibleItemView::role(int child) const
{
    if ((!atViewport() && child) || (atViewport() && child == 0)) {
        QAbstractItemView *view = itemView();
#ifndef QT_NO_TABLEVIEW
        if (qobject_cast<QTableView *>(view))
            return Table;
#endif
#ifndef QT_NO_LISTVIEW
        if (qobject_cast<QListView *>(view))
            return List;
#endif
        return Tree;
    }
    if (atViewport()) {
        if (child)
            return Row;
    }

    return QAccessibleAbstractScrollArea::role(child);
}

QAccessible::State QAccessibleItemView::state(int child) const
{
    State st = Normal;

    if (itemView() == 0)
        return State(Unavailable);

    bool queryViewPort = (atViewport() && child == 0) || (!atViewport() && child == 1);
    if (queryViewPort) {
        if (itemView()->selectionMode() != QAbstractItemView::NoSelection) {
            st |= Selectable;
            st |= Focusable;
        }
    } else if (atViewport()) {    // children of viewport
        if (horizontalHeader())
            --child;
        if (child) {
            QAccessibleItemRow item(itemView(), childIndex(child));
            st |= item.state(0);
        }
    } else if (!atViewport() && child != 1) {
        st = QAccessibleAbstractScrollArea::state(child);
    }
    return st;
}

bool QAccessibleItemView::isValid() const
{
    if (atViewport())
        return QAccessibleWidget::isValid();
    else
        return QAccessibleAbstractScrollArea::isValid();
}

int QAccessibleItemView::navigate(RelationFlag relation, int index,
                                  QAccessibleInterface **iface) const
{
    if (atViewport()) {
        if (relation == Ancestor && index == 1) {
            *iface = new QAccessibleItemView(itemView());
            return 0;
        } else if (relation == Child && index >= 1) {
            if (horizontalHeader()) {
                if (index == 1) {
                    *iface = new QAccessibleItemRow(itemView(), QModelIndex(), true);
                    return 0;
                }
                --index;
            }

            //###JAS hidden rows..
            QModelIndex idx = childIndex(index);
            if (idx.isValid()) {
                *iface = new QAccessibleItemRow(itemView(), idx);
                return 0;
            }
        } else if (relation == Sibling && index >= 1) {
            QAccessibleInterface *parent = new QAccessibleItemView(itemView());
            return parent->navigate(Child, index, iface);
        }
        *iface = 0;
        return -1;
    } else {
        return QAccessibleAbstractScrollArea::navigate(relation, index, iface);
    }
}

/* returns the model index for a given row and column */
QModelIndex QAccessibleItemView::index(int row, int column) const
{
    return itemView()->model()->index(row, column);
}

QAccessibleInterface *QAccessibleItemView::accessibleAt(int row, int column)
{
    QWidget *indexWidget = itemView()->indexWidget(index(row, column));
    return QAccessible::queryAccessibleInterface(indexWidget);
}

/* We don't have a concept of a "caption" in Qt's standard widgets */
QAccessibleInterface *QAccessibleItemView::caption()
{
    return 0;
}

/* childIndex is row * columnCount + columnIndex */
int QAccessibleItemView::childIndex(int rowIndex, int columnIndex)
{
    return rowIndex * itemView()->model()->columnCount() + columnIndex;
}

/* Return the header data as column description */
QString QAccessibleItemView::columnDescription(int column)
{
    return itemView()->model()->headerData(column, Qt::Horizontal).toString();
}

/* We don't support column spanning atm */
int QAccessibleItemView::columnSpan(int /* row */, int /* column */)
{
    return 1;
}

/* Return the horizontal header view */
QAccessibleInterface *QAccessibleItemView::columnHeader()
{
#ifndef QT_NO_TREEVIEW
    if (QTreeView *tree = qobject_cast<QTreeView *>(itemView()))
        return QAccessible::queryAccessibleInterface(tree->header());
#endif
#ifndef QT_NO_TABLEVIEW
    if (QTableView *table = qobject_cast<QTableView *>(itemView()))
        return QAccessible::queryAccessibleInterface(table->horizontalHeader());
#endif
    return 0;
}

int QAccessibleItemView::columnIndex(int childIndex)
{
    int columnCount = itemView()->model()->columnCount();
    if (!columnCount)
        return 0;

    return childIndex % columnCount;
}

int QAccessibleItemView::columnCount()
{
    return itemView()->model()->columnCount();
}

int QAccessibleItemView::rowCount()
{
    return itemView()->model()->rowCount();
}

int QAccessibleItemView::selectedColumnCount()
{
    return itemView()->selectionModel()->selectedColumns().count();
}

int QAccessibleItemView::selectedRowCount()
{
    return itemView()->selectionModel()->selectedRows().count();
}

QString QAccessibleItemView::rowDescription(int row)
{
    return itemView()->model()->headerData(row, Qt::Vertical).toString();
}

/* We don't support row spanning */
int QAccessibleItemView::rowSpan(int /*row*/, int /*column*/)
{
    return 1;
}

QAccessibleInterface *QAccessibleItemView::rowHeader()
{
#ifndef QT_NO_TABLEVIEW
    if (QTableView *table = qobject_cast<QTableView *>(itemView()))
        return QAccessible::queryAccessibleInterface(table->verticalHeader());
#endif
    return 0;
}

int QAccessibleItemView::rowIndex(int childIndex)
{
    int columnCount = itemView()->model()->columnCount();
    if (!columnCount)
        return 0;

    return int(childIndex / columnCount);
}

int QAccessibleItemView::selectedRows(int maxRows, QList<int> *rows)
{
    Q_ASSERT(rows);

    const QModelIndexList selRows = itemView()->selectionModel()->selectedRows();
    int maxCount = qMin(selRows.count(), maxRows);

    for (int i = 0; i < maxCount; ++i)
        rows->append(selRows.at(i).row());

    return maxCount;
}

int QAccessibleItemView::selectedColumns(int maxColumns, QList<int> *columns)
{
    Q_ASSERT(columns);

    const QModelIndexList selColumns = itemView()->selectionModel()->selectedColumns();
    int maxCount = qMin(selColumns.count(), maxColumns);

    for (int i = 0; i < maxCount; ++i)
        columns->append(selColumns.at(i).row());

    return maxCount;
}

/* Qt widgets don't have a concept of a summary */
QAccessibleInterface *QAccessibleItemView::summary()
{
    return 0;
}

bool QAccessibleItemView::isColumnSelected(int column)
{
    return itemView()->selectionModel()->isColumnSelected(column, QModelIndex());
}

bool QAccessibleItemView::isRowSelected(int row)
{
    return itemView()->selectionModel()->isRowSelected(row, QModelIndex());
}

bool QAccessibleItemView::isSelected(int row, int column)
{
    return itemView()->selectionModel()->isSelected(index(row, column));
}

void QAccessibleItemView::selectRow(int row)
{
    QItemSelectionModel *s = itemView()->selectionModel();
    s->select(index(row, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void QAccessibleItemView::selectColumn(int column)
{
    QItemSelectionModel *s = itemView()->selectionModel();
    s->select(index(0, column), QItemSelectionModel::Select | QItemSelectionModel::Columns);
}

void QAccessibleItemView::unselectRow(int row)
{
    QItemSelectionModel *s = itemView()->selectionModel();
    s->select(index(row, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
}

void QAccessibleItemView::unselectColumn(int column)
{
    QItemSelectionModel *s = itemView()->selectionModel();
    s->select(index(0, column), QItemSelectionModel::Deselect | QItemSelectionModel::Columns);
}

void QAccessibleItemView::cellAtIndex(int index, int *row, int *column, int *rSpan,
                                      int *cSpan, bool *isSelect)
{
    *row = rowIndex(index);
    *column = columnIndex(index);
    *rSpan = rowSpan(*row, *column);
    *cSpan = columnSpan(*row, *column);
    *isSelect = isSelected(*row, *column);
}

/*!
  \class QAccessibleHeader
  \brief The QAccessibleHeader class implements the QAccessibleInterface for header widgets.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleHeader object for \a w.
*/
QAccessibleHeader::QAccessibleHeader(QWidget *w)
: QAccessibleWidget(w)
{
    Q_ASSERT(header());
    addControllingSignal(QLatin1String("sectionClicked(int)"));
}

/*! Returns the QHeaderView. */
QHeaderView *QAccessibleHeader::header() const
{
    return qobject_cast<QHeaderView*>(object());
}

QRect QAccessibleHeader::rect(int child) const
{
    if (!child)
        return QAccessibleWidget::rect(0);

    QHeaderView *h = header();
    QPoint zero = h->mapToGlobal(QPoint(0, 0));
    int sectionSize = h->sectionSize(child - 1);
    int sectionPos = h->sectionPosition(child - 1);
    return h->orientation() == Qt::Horizontal
        ? QRect(zero.x() + sectionPos, zero.y(), sectionSize, h->height())
        : QRect(zero.x(), zero.y() + sectionPos, h->width(), sectionSize);
}

int QAccessibleHeader::childCount() const
{
    return header()->count();
}

QString QAccessibleHeader::text(Text t, int child) const
{
    QString str;

    if (child > 0 && child <= childCount()) {
        switch (t) {
        case Name:
            str = header()->model()->headerData(child - 1, header()->orientation()).toString();
            break;
        case Description: {
            QAccessibleEvent event(QEvent::AccessibilityDescription, child);
            if (QApplication::sendEvent(widget(), &event))
                str = event.value();
            break; }
        case Help: {
            QAccessibleEvent event(QEvent::AccessibilityHelp, child);
            if (QApplication::sendEvent(widget(), &event))
                str = event.value();
            break; }
        default:
            break;
        }
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t, child);
    return str;
}

QAccessible::Role QAccessibleHeader::role(int) const
{
    return (header()->orientation() == Qt::Horizontal) ? ColumnHeader : RowHeader;
}

QAccessible::State QAccessibleHeader::state(int child) const
{
    State state = QAccessibleWidget::state(child);

    if (child) {
        int section = child - 1;
        if (header()->isSectionHidden(section))
            state |= Invisible;
        if (header()->resizeMode(section) != QHeaderView::Custom)
            state |= Sizeable;
    } else {
        if (header()->isMovable())
            state |= Movable;
    }
    if (!header()->isClickable())
        state |= Unavailable;
    return state;
}
#endif // QT_NO_ITEMVIEWS
#endif // 0

#ifndef QT_NO_TABBAR
/*!
  \class QAccessibleTabBar
  \brief The QAccessibleTabBar class implements the QAccessibleInterface for tab bars.
  \internal

  \ingroup accessibility
*/

class QAccessibleTabButton: public QAccessibleInterface, public QAccessibleActionInterface
{
    Q_ACCESSIBLE_OBJECT
public:
    QAccessibleTabButton(QTabBar *parent, int index)
        : m_parent(parent), m_index(index)
    {}

    QObject *object() const { return 0; }
    Role role() const { return QAccessible::PageTab; }
    State state() const {
        QAccessibleInterface *parentInterface = parent();
        State state = parentInterface->state();
        delete parentInterface;
        return state;
    }
    QRect rect() const {
        if (!isValid())
            return QRect();

        QPoint tp = m_parent->mapToGlobal(QPoint(0,0));
        QRect rec = m_parent->tabRect(m_index);
        rec = QRect(tp.x() + rec.x(), tp.y() + rec.y(), rec.width(), rec.height());
        return rec;
    }

    bool isValid() const { return true; }// (!m_parent.isNull()) && m_parent->count() > m_index; }

    int childAt(int, int) const { return 0; }
    int childCount() const { return 0; }
    int indexOfChild(const QAccessibleInterface *) const  { return -1; }

    QString text(Text) const { return qt_accStripAmp(m_parent->tabText(m_index)); }
    void setText(Text, const QString &) {}

    QAccessibleInterface *parent() const {
        return QAccessible::queryAccessibleInterface(m_parent);
    }
    QAccessibleInterface *child(int) const { return 0; }
    int navigate(RelationFlag relation, int index, QAccessibleInterface **iface) const
    {
        if (relation == QAccessible::Ancestor && index == 1) {
            *iface = parent();
            return 0;
        }
        return -1;
    }
    Relation relationTo(const QAccessibleInterface *) const
    {
        return QAccessible::Unrelated;
    }

    // action interface
    QStringList actionNames() const
    {
        return QStringList(pressAction());
    }

    void doAction(const QString &actionName)
    {
        if (actionName == pressAction())
            m_parent->setCurrentIndex(m_index);
    }

    QStringList keyBindingsForAction(const QString &) const
    {
        return QStringList();
    }

private:
    QPointer<QTabBar> m_parent;
    int m_index;
};

/*!
  Constructs a QAccessibleTabBar object for \a w.
*/
QAccessibleTabBar::QAccessibleTabBar(QWidget *w)
: QAccessibleWidget(w, PageTabList)
{
    Q_ASSERT(tabBar());
}

/*! Returns the QTabBar. */
QTabBar *QAccessibleTabBar::tabBar() const
{
    return qobject_cast<QTabBar*>(object());
}

int QAccessibleTabBar::navigate(RelationFlag rel, int entry, QAccessibleInterface **target) const
{
    if (rel == QAccessible::Child) {
        *target = child(entry - 1);
        if (*target) {
            return 0;
        }
        return -1;
    }
    return QAccessibleWidget::navigate(rel, entry, target);
}

QAccessibleInterface* QAccessibleTabBar::child(int index) const
{
    // first the tabs, then 2 buttons
    if (index < tabBar()->count()) {
        QAccessibleTabButton *button = new QAccessibleTabButton(tabBar(), index);
        return button;
    } else if (index >= tabBar()->count()) {
        // left button
        if (index - tabBar()->count() == 0) {
            return QAccessible::queryAccessibleInterface(tabBar()->d_func()->leftB);
        }
        // right button
        if (index - tabBar()->count() == 1) {
            return QAccessible::queryAccessibleInterface(tabBar()->d_func()->rightB);
        }
    }
    return 0;
}

int QAccessibleTabBar::indexOfChild(const QAccessibleInterface *child) const
{
    if (child->object() && child->object() == tabBar()->d_func()->leftB)
        return tabBar()->count() + 1; // fixme - one based
    if (child->object() && child->object() == tabBar()->d_func()->rightB)
        return tabBar()->count() + 2; // fixme - one based
    return 0;
}

int QAccessibleTabBar::childCount() const
{
    // tabs + scroll buttons
    return tabBar()->count() + 2;
}

QString QAccessibleTabBar::text(Text t) const
{
    if (t == QAccessible::Name) {
        return qt_accStripAmp(tabBar()->tabText(tabBar()->currentIndex()));
    }
    return QString();
}

/*!
    Selects the item with index \a child if \a on is true; otherwise
    unselects it. If \a extend is true and the selection mode is not
    \c Single and there is an existing selection, the selection is
    extended to include all the items from the existing selection up
    to and including the item with index \a child. Returns true if a
    selection was made or extended; otherwise returns false.

    \sa selection() clearSelection()
*/
bool QAccessibleTabBar::setSelected(int child, bool on, bool extend)
{
    if (!child || !on || extend || child > tabBar()->count())
        return false;

    if (!tabBar()->isTabEnabled(child - 1))
        return false;
    tabBar()->setCurrentIndex(child - 1);
    return true;
}

/*!
    Returns a (possibly empty) list of indexes of the items selected
    in the list box.

    \sa setSelected() clearSelection()
*/
QVector<int> QAccessibleTabBar::selection() const
{
    QVector<int> array;
    if (tabBar()->currentIndex() != -1)
        array +=tabBar()->currentIndex() + 1;
    return array;
}

#endif // QT_NO_TABBAR

#ifndef QT_NO_COMBOBOX
/*!
  \class QAccessibleComboBox
  \brief The QAccessibleComboBox class implements the QAccessibleInterface for editable and read-only combo boxes.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleComboBox object for \a w.
*/
QAccessibleComboBox::QAccessibleComboBox(QWidget *w)
: QAccessibleWidget(w, ComboBox)
{
    Q_ASSERT(comboBox());
}

/*!
  Returns the combobox.
*/
QComboBox *QAccessibleComboBox::comboBox() const
{
    return qobject_cast<QComboBox*>(object());
}

QAccessibleInterface* QAccessibleComboBox::child(int index) const
{
    if (index == 0) {
        QAbstractItemView *view = comboBox()->view();
        //QWidget *parent = view ? view->parentWidget() : 0;
        return QAccessible::queryAccessibleInterface(view);
    } else if (index == 1 && comboBox()->isEditable()) {
        return QAccessible::queryAccessibleInterface(comboBox()->lineEdit());
    }
    return 0;
}

int QAccessibleComboBox::childCount() const
{
    // list and text edit
    return comboBox()->isEditable() ? 2 : 1;
}

int QAccessibleComboBox::childAt(int x, int y) const
{
    if (comboBox()->isEditable() && comboBox()->lineEdit()->rect().contains(x, y))
        return 1;
    return 0;
}

int QAccessibleComboBox::indexOfChild(const QAccessibleInterface *child) const
{
    if (comboBox()->view() == child->object())
        return 0;
    if (comboBox()->isEditable() && comboBox()->lineEdit() == child->object())
        return 1;
    return -1;
}

/*! \reimp */
QString QAccessibleComboBox::text(Text t) const
{
    QString str;

    switch (t) {
    case Name:
#ifndef Q_OS_UNIX // on Linux we use relations for this, name is text (fall through to Value)
        str = QAccessibleWidget::text(t);
        break;
#endif
    case Value:
        if (comboBox()->isEditable())
            str = comboBox()->lineEdit()->text();
        else
            str = comboBox()->currentText();
        break;
#ifndef QT_NO_SHORTCUT
    case Accelerator:
        str = (QString)QKeySequence(Qt::Key_Down);
        break;
#endif
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t);
    return str;
}

QStringList QAccessibleComboBox::actionNames() const
{
    return QStringList(showMenuAction());
}

QString QAccessibleComboBox::localizedActionDescription(const QString &actionName) const
{
    if (actionName == showMenuAction())
        return QComboBox::tr("Open the combo box selection popup");
    return QString();
}

void QAccessibleComboBox::doAction(const QString &actionName)
{
    if (actionName == showMenuAction()) {
        if (comboBox()->view()->isVisible()) {
            comboBox()->hidePopup();
        } else {
            comboBox()->showPopup();
        }
    }
}

QStringList QAccessibleComboBox::keyBindingsForAction(const QString &/*actionName*/) const
{
    return QStringList();
}

#endif // QT_NO_COMBOBOX

static inline void removeInvisibleWidgetsFromList(QWidgetList *list)
{
    if (!list || list->isEmpty())
        return;

    for (int i = 0; i < list->count(); ++i) {
        QWidget *widget = list->at(i);
        if (!widget->isVisible())
            list->removeAt(i);
    }
}

#ifndef QT_NO_SCROLLAREA
// ======================= QAccessibleAbstractScrollArea =======================
QAccessibleAbstractScrollArea::QAccessibleAbstractScrollArea(QWidget *widget)
    : QAccessibleWidget(widget, Client)
{
    Q_ASSERT(qobject_cast<QAbstractScrollArea *>(widget));
}

QVariant QAccessibleAbstractScrollArea::invokeMethod(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}

QAccessibleInterface *QAccessibleAbstractScrollArea::child(int index) const
{
    return QAccessible::queryAccessibleInterface(accessibleChildren().at(index));
}

int QAccessibleAbstractScrollArea::childCount() const
{
    return accessibleChildren().count();
}

int QAccessibleAbstractScrollArea::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child || !child->object())
        return -1;
    int index = accessibleChildren().indexOf(qobject_cast<QWidget *>(child->object()));
    if (index >= 0)
        return ++index;
    return -1;
}

bool QAccessibleAbstractScrollArea::isValid() const
{
    return (QAccessibleWidget::isValid() && abstractScrollArea() && abstractScrollArea()->viewport());
}

int QAccessibleAbstractScrollArea::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    if (!target)
        return -1;

    *target = 0;

    QWidget *targetWidget = 0;
    QWidget *entryWidget = 0;

    if (relation == Child ||
        relation == Left || relation == Up || relation == Right || relation == Down) {
        QWidgetList children = accessibleChildren();
        if (entry < 0 || entry > children.count())
            return -1;

        if (entry == Self)
            entryWidget = abstractScrollArea();
        else
            entryWidget = children.at(entry - 1);
        AbstractScrollAreaElement entryElement = elementType(entryWidget);

        // Not one of the most beautiful switches I've ever seen, but I believe it has
        // to be like this since each case need special handling.
        // It might be possible to make it more general, but I'll leave that as an exercise
        // to the reader. :-)
        switch (relation) {
        case Child:
            if (entry > 0) {
                *target = child(entry - 1);
                return *target ? 0 : -1;
            }
        case Left:
            if (entry < 1)
                break;
            switch (entryElement) {
            case Viewport:
                if (!isLeftToRight())
                    targetWidget = abstractScrollArea()->verticalScrollBar();
                break;
            case HorizontalContainer:
                if (!isLeftToRight())
                    targetWidget = abstractScrollArea()->cornerWidget();
                break;
            case VerticalContainer:
                if (isLeftToRight())
                    targetWidget = abstractScrollArea()->viewport();
                break;
            case CornerWidget:
                if (isLeftToRight())
                    targetWidget = abstractScrollArea()->horizontalScrollBar();
                break;
            default:
                break;
            }
            break;
        case Right:
            if (entry < 1)
                break;
            switch (entryElement) {
            case Viewport:
                if (isLeftToRight())
                    targetWidget = abstractScrollArea()->verticalScrollBar();
                break;
            case HorizontalContainer:
                targetWidget = abstractScrollArea()->cornerWidget();
                break;
            case VerticalContainer:
                if (!isLeftToRight())
                    targetWidget = abstractScrollArea()->viewport();
                break;
            case CornerWidget:
                if (!isLeftToRight())
                    targetWidget = abstractScrollArea()->horizontalScrollBar();
                break;
            default:
                break;
            }
            break;
        case Up:
            if (entry < 1)
                break;
            switch (entryElement) {
            case HorizontalContainer:
                targetWidget = abstractScrollArea()->viewport();
                break;
            case CornerWidget:
                targetWidget = abstractScrollArea()->verticalScrollBar();
                break;
            default:
                break;
            }
            break;
        case Down:
            if (entry < 1)
                break;
            switch (entryElement) {
            case Viewport:
                targetWidget = abstractScrollArea()->horizontalScrollBar();
                break;
            case VerticalContainer:
                targetWidget = abstractScrollArea()->cornerWidget();
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    } else {
        return QAccessibleWidget::navigate(relation, entry, target);
    }

    if (qobject_cast<const QScrollBar *>(targetWidget))
        targetWidget = targetWidget->parentWidget();
    *target = QAccessible::queryAccessibleInterface(targetWidget);
    return *target ? 0: -1;
}

//int QAccessibleAbstractScrollArea::childAt(int x, int y) const
//{
//    if (!abstractScrollArea()->isVisible())
//        return -1;
//#if 0
//    const QRect globalSelfGeometry = rect(Self);
//    if (!globalSelfGeometry.isValid() || !globalSelfGeometry.contains(QPoint(x, y)))
//        return -1;
//    const QWidgetList children = accessibleChildren();
//    for (int i = 0; i < children.count(); ++i) {
//        const QWidget *child = children.at(i);
//        const QRect globalChildGeometry = QRect(child->mapToGlobal(QPoint(0, 0)), child->size());
//        if (globalChildGeometry.contains(QPoint(x, y))) {
//            return ++i;
//        }
//    }
//    return 0;
//#else
//    for (int i = childCount(); i >= 0; --i) {
//        if (rect().contains(x, y))
//            return i;
//    }
//    return -1;
//#endif
//}

QAbstractScrollArea *QAccessibleAbstractScrollArea::abstractScrollArea() const
{
    return static_cast<QAbstractScrollArea *>(object());
}

QWidgetList QAccessibleAbstractScrollArea::accessibleChildren() const
{
    QWidgetList children;

    // Viewport.
    QWidget * viewport = abstractScrollArea()->viewport();
    if (viewport)
        children.append(viewport);

    // Horizontal scrollBar container.
    QScrollBar *horizontalScrollBar = abstractScrollArea()->horizontalScrollBar();
    if (horizontalScrollBar && horizontalScrollBar->isVisible()) {
        children.append(horizontalScrollBar->parentWidget());
    }

    // Vertical scrollBar container.
    QScrollBar *verticalScrollBar = abstractScrollArea()->verticalScrollBar();
    if (verticalScrollBar && verticalScrollBar->isVisible()) {
        children.append(verticalScrollBar->parentWidget());
    }

    // CornerWidget.
    QWidget *cornerWidget = abstractScrollArea()->cornerWidget();
    if (cornerWidget && cornerWidget->isVisible())
        children.append(cornerWidget);

    return children;
}

QAccessibleAbstractScrollArea::AbstractScrollAreaElement
QAccessibleAbstractScrollArea::elementType(QWidget *widget) const
{
    if (!widget)
        return Undefined;

    if (widget == abstractScrollArea())
        return Self;
    if (widget == abstractScrollArea()->viewport())
        return Viewport;
    if (widget->objectName() == QLatin1String("qt_scrollarea_hcontainer"))
        return HorizontalContainer;
    if (widget->objectName() == QLatin1String("qt_scrollarea_vcontainer"))
        return VerticalContainer;
    if (widget == abstractScrollArea()->cornerWidget())
        return CornerWidget;

    return Undefined;
}

bool QAccessibleAbstractScrollArea::isLeftToRight() const
{
    return abstractScrollArea()->isLeftToRight();
}

// ======================= QAccessibleScrollArea ===========================
QAccessibleScrollArea::QAccessibleScrollArea(QWidget *widget)
    : QAccessibleAbstractScrollArea(widget)
{
    Q_ASSERT(qobject_cast<QScrollArea *>(widget));
}
#endif // QT_NO_SCROLLAREA

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
