/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QACCESSIBLE2_H
#define QACCESSIBLE2_H

#include <QtGui/qaccessible.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_ACCESSIBILITY

class Q_GUI_EXPORT QAccessibleTextInterface
{
public:
    virtual ~QAccessibleTextInterface() {}
    // selection
    virtual void selection(int selectionIndex, int *startOffset, int *endOffset) const = 0;
    virtual int selectionCount() const = 0;
    virtual void addSelection(int startOffset, int endOffset) = 0;
    virtual void removeSelection(int selectionIndex) = 0;
    virtual void setSelection(int selectionIndex, int startOffset, int endOffset) = 0;

    // cursor
    virtual int cursorPosition() const = 0;
    virtual void setCursorPosition(int position) = 0;

    // text
    virtual QString text(int startOffset, int endOffset) const = 0;
    virtual QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                     int *startOffset, int *endOffset) const;
    virtual QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                    int *startOffset, int *endOffset) const;
    virtual QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                 int *startOffset, int *endOffset) const;
    virtual int characterCount() const = 0;

    // character <-> geometry
    virtual QRect characterRect(int offset) const = 0;
    virtual int offsetAtPoint(const QPoint &point) const = 0;

    virtual void scrollToSubstring(int startIndex, int endIndex) = 0;
    virtual QString attributes(int offset, int *startOffset, int *endOffset) const = 0;
};

class Q_GUI_EXPORT QAccessibleEditableTextInterface
{
public:
    virtual ~QAccessibleEditableTextInterface() {}

    virtual void deleteText(int startOffset, int endOffset) = 0;
    virtual void insertText(int offset, const QString &text) = 0;
    virtual void replaceText(int startOffset, int endOffset, const QString &text) = 0;
};

class Q_GUI_EXPORT QAccessibleValueInterface
{
public:

    virtual ~QAccessibleValueInterface() {}

    virtual QVariant currentValue() const = 0;
    virtual void setCurrentValue(const QVariant &value) = 0;
    virtual QVariant maximumValue() const = 0;
    virtual QVariant minimumValue() const = 0;
    virtual QVariant minimumStepSize() const = 0;
};

class Q_GUI_EXPORT QAccessibleTableCellInterface
{
public:
    virtual ~QAccessibleTableCellInterface() {}

    //            Returns the number of columns occupied by this cell accessible.
    virtual int columnExtent() const = 0;

    //            Returns the column headers as an array of cell accessibles.
    virtual QList<QAccessibleInterface*> columnHeaderCells() const = 0;

    //            Translates this cell accessible into the corresponding column index.
    virtual int columnIndex() const = 0;
    //            Returns the number of rows occupied by this cell accessible.
    virtual int rowExtent() const = 0;
    //            Returns the row headers as an array of cell accessibles.
    virtual QList<QAccessibleInterface*> rowHeaderCells() const = 0;
    //            Translates this cell accessible into the corresponding row index.
    virtual int rowIndex() const = 0;
    //            Returns a boolean value indicating whether this cell is selected.
    virtual bool isSelected() const = 0;

    //            Gets the row and column indexes and extents of this cell accessible and whether or not it is selected.
    //          ### Is this really needed??
    //
    //          ### Maybe change to QSize cellSize(), we already have accessors for the row, column and selected
    virtual void rowColumnExtents(int *row, int *column, int *rowExtents, int *columnExtents, bool *selected) const = 0;
    //            Returns a reference to the accessbile of the containing table.
    virtual QAccessibleInterface* table() const = 0;
};

class Q_GUI_EXPORT QAccessibleTableInterface
{
public:
    virtual ~QAccessibleTableInterface() {}

    // Returns the cell at the specified row and column in the table.
    virtual QAccessibleInterface *cellAt (int row, int column) const = 0;
    // Returns the caption for the table.
    virtual QAccessibleInterface *caption() const = 0;
    // Returns the description text of the specified column in the table.
    virtual QString columnDescription(int column) const = 0;
    // Returns the total number of columns in table.
    virtual int columnCount() const = 0;
    // Returns the total number of rows in table.
    virtual int rowCount() const = 0;
    // Returns the total number of selected cells.
    virtual int selectedCellCount() const = 0;
    // Returns the total number of selected columns.
    virtual int selectedColumnCount() const = 0;
    // Returns the total number of selected rows.
    virtual int selectedRowCount() const = 0;
    // Returns the description text of the specified row in the table.
    virtual QString rowDescription(int row) const = 0;
    // Returns a list of accessibles currently selected.
    virtual QList<QAccessibleInterface*> selectedCells() const = 0;
    // Returns a list of column indexes currently selected (0 based).
    virtual QList<int> selectedColumns() const = 0;
    // Returns a list of row indexes currently selected (0 based).
    virtual QList<int> selectedRows() const = 0;
    // Returns the summary description of the table.
    virtual QAccessibleInterface *summary() const = 0;
    // Returns a boolean value indicating whether the specified column is completely selected.
    virtual bool isColumnSelected(int column) const = 0;
    // Returns a boolean value indicating whether the specified row is completely selected.
    virtual bool isRowSelected(int row) const = 0;
    // Selects a row and it might unselect all previously selected rows.
    virtual bool selectRow(int row) = 0;
    // Selects a column it might unselect all previously selected columns.
    virtual bool selectColumn(int column) = 0;
    // Unselects one row, leaving other selected rows selected (if any).
    virtual bool unselectRow(int row) = 0;
    // Unselects one column, leaving other selected columns selected (if any).
    virtual bool unselectColumn(int column) = 0;

    virtual void modelChange(QAccessibleTableModelChangeEvent *event) = 0;

protected:
friend class QAbstractItemView;
friend class QAbstractItemViewPrivate;
};

class Q_GUI_EXPORT QAccessibleActionInterface
{
    Q_DECLARE_TR_FUNCTIONS(QAccessibleActionInterface)
public:
    virtual ~QAccessibleActionInterface() {}

    virtual QStringList actionNames() const = 0;
    virtual QString localizedActionName(const QString &name) const;
    virtual QString localizedActionDescription(const QString &name) const;
    virtual void doAction(const QString &actionName) = 0;
    virtual QStringList keyBindingsForAction(const QString &actionName) const = 0;

    static const QString &pressAction();
    static const QString &increaseAction();
    static const QString &decreaseAction();
    static const QString &showMenuAction();
    static const QString &setFocusAction();
    static const QString &toggleAction();
};

class Q_GUI_EXPORT QAccessibleImageInterface
{
public:
    virtual ~QAccessibleImageInterface() {}

    virtual QString imageDescription() const = 0;
    virtual QSize imageSize() const = 0;
    virtual QRect imagePosition() const = 0;
};

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif
