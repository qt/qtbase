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
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>

/*
    To add a view to be tested add the header file to the includes
    and impliment what is needed in the functions below.

    You can add more then one view, several Qt views are included as examples.

    In tst_qitemview.cpp a new ViewsToTest object is created for each test.

    When you have errors fix the first ones first.  Later tests depend upon them working
*/

class ViewsToTest
{
public:
    ViewsToTest();

    QAbstractItemView *createView(const QString &viewType);
    void hideIndexes(QAbstractItemView *view);

    enum Display { DisplayNone, DisplayRoot };

    struct test {
        test(QString m, Display d) : viewType(m), display(d){};
        QString viewType;
        Display display;
    };

    QList<test> tests;
};


/*!
    Add new tests, they can be the same view, but in a different state.
 */
ViewsToTest::ViewsToTest()
{
    tests.append(test("QTreeView_ScrollPerItem", DisplayRoot));
    tests.append(test("QTreeView_ScrollPerPixel", DisplayRoot));
    tests.append(test("QListView_ScrollPerItem", DisplayRoot));
    tests.append(test("QListView_ScrollPerPixel", DisplayRoot));
    tests.append(test("QHeaderViewHorizontal", DisplayNone));
    tests.append(test("QHeaderViewVertical", DisplayNone));
    tests.append(test("QTableView_ScrollPerItem", DisplayRoot));
    tests.append(test("QTableView_ScrollPerPixel", DisplayRoot));
    tests.append(test("QTableViewNoGrid", DisplayRoot));
}

/*!
    Return a new viewType.
 */
QAbstractItemView *ViewsToTest::createView(const QString &viewType)
{
    QAbstractItemView *view = 0;
    if (viewType == "QListView_ScrollPerItem") {
        view = new QListView();
        view->setObjectName("QListView");
        view->setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    } else if (viewType == "QListView_ScrollPerPixel") {
        view = new QListView();
        view->setObjectName("QListView");
        view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    } else if (viewType == "QHeaderViewHorizontal") {
        view = new QHeaderView(Qt::Horizontal);
        view->setObjectName("QHeaderView");
    } else if (viewType == "QHeaderViewVertical") {
        view = new QHeaderView(Qt::Vertical);
        view->setObjectName("QHeaderView");
    } else if (viewType == "QTableView_ScrollPerItem") {
        view = new QTableView();
        view->setObjectName("QTableView");
        view->setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    } else if (viewType == "QTableView_ScrollPerPixel") {
        view = new QTableView();
        view->setObjectName("QTableView");
        view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    } else if (viewType == "QTableViewNoGrid") {
        QTableView *table = new QTableView();
        table->setObjectName("QTableView");
        table->setShowGrid(false);
        view = table;
    } else if (viewType == "QTreeView_ScrollPerItem") {
        view = new QTreeView();
        view->setObjectName("QTreeView");
        view->setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
        view->setSelectionBehavior(QAbstractItemView::SelectItems);
    } else if (viewType == "QTreeView_ScrollPerPixel") {
        view = new QTreeView();
        view->setObjectName("QTreeView");
        view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        view->setSelectionBehavior(QAbstractItemView::SelectItems);
    }
    return view;
}

void ViewsToTest::hideIndexes(QAbstractItemView *view)
{
    if (QTableView *tableView = qobject_cast<QTableView *>(view)) {
        tableView->setColumnHidden(1, true);
        tableView->setRowHidden(1, true);
        tableView->setRowHidden(tableView->model()->rowCount()-2, true);
    }
    if (QTreeView *treeView = qobject_cast<QTreeView *>(view)) {
        treeView->setColumnHidden(1, true);
        treeView->setRowHidden(1, QModelIndex(), true);
        treeView->setRowHidden(treeView->model()->rowCount()-2, QModelIndex(), true);
    }
    if (QListView *listView = qobject_cast<QListView *>(view)) {
        listView->setRowHidden(1, true);
        listView->setRowHidden(listView->model()->rowCount()-2, true);
    }
}

