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

#ifndef TST_QSORTFILTERPROXYMODEL_H
#define TST_QSORTFILTERPROXYMODEL_H

#include "dynamictreemodel.h"
#include <QLoggingCategory>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

enum class FilterType {
    RegExp,
    RegularExpression
};

Q_DECLARE_METATYPE(QList<QPersistentModelIndex>)

class tst_QSortFilterProxyModel : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

private slots:
    void getSetCheck();
    void sort_data();
    void sort();
    void sortHierarchy_data();
    void sortHierarchy();

    void insertRows_data();
    void insertRows();
    void prependRow();
    void appendRowFromCombobox_data();
    void appendRowFromCombobox();
    void removeRows_data();
    void removeRows();
    void removeColumns_data();
    void removeColumns();
    void insertAfterSelect();
    void removeAfterSelect();
    void filter_data();
    void filter();
    void filterHierarchy_data();
    void filterHierarchy();
    void filterColumns_data();
    void filterColumns();

    void filterTable();
    void filterCurrent();
    void filter_qtbug30662();

    void changeSourceLayout();
    void changeSourceLayoutFilteredOut();
    void removeSourceRows_data();
    void removeSourceRows();
    void insertSourceRows_data();
    void insertSourceRows();
    void changeFilter_data();
    void changeFilter();
    void changeSourceData_data();
    void changeSourceData();
    void changeSourceDataKeepsStableSorting_qtbug1548();
    void changeSourceDataForwardsRoles_qtbug35440();
    void resortingDoesNotBreakTreeModels();
    void dynamicFilterWithoutSort();
    void sortFilterRole();
    void selectionFilteredOut();
    void match_data();
    void match();
    void matchTree();
    void insertIntoChildrenlessItem();
    void invalidateMappedChildren();
    void insertRowIntoFilteredParent();
    void filterOutParentAndFilterInChild();

    void sourceInsertRows();
    void sourceModelDeletion();

    void sortColumnTracking1();
    void sortColumnTracking2();

    void sortStable();

    void hiddenColumns();
    void insertRowsSort();
    void staticSorting();
    void dynamicSorting();
    void fetchMore();
    void hiddenChildren();
    void mapFromToSource();
    void removeRowsRecursive();
    void doubleProxySelectionSetSourceModel();
    void appearsAndSort();
    void unnecessaryDynamicSorting();
    void unnecessaryMapCreation();
    void resetInvalidate_data();
    void resetInvalidate();

    void testMultipleProxiesWithSelection();
    void mapSelectionFromSource();
    void testResetInternalData();
    void filteredColumns();
    void headerDataChanged();

    void testParentLayoutChanged();
    void moveSourceRows();

    void hierarchyFilterInvalidation();
    void simpleFilterInvalidation();

    void chainedProxyModelRoleNames();

    void noMapAfterSourceDelete();
    void forwardDropApi();
    void canDropMimeData();
    void filterHint();

    void sourceLayoutChangeLeavesValidPersistentIndexes();
    void rowMoveLeavesValidPersistentIndexes();

    void emitLayoutChangedOnlyIfSortingChanged_data();
    void emitLayoutChangedOnlyIfSortingChanged();

    void checkSetNewModel();
    void filterAndInsertRow_data();
    void filterAndInsertRow();
    void filterAndInsertColumn_data();
    void filterAndInsertColumn();

    void removeIntervals_data();
    void removeIntervals();

protected:
    void buildHierarchy(const QStringList &data, QAbstractItemModel *model);
    void checkHierarchy(const QStringList &data, const QAbstractItemModel *model);
    void setupFilter(QSortFilterProxyModel *model, const QString& pattern);

protected:
    FilterType m_filterType;

private:
    QStandardItemModel *m_model = nullptr;
    QSortFilterProxyModel *m_proxy = nullptr;
};

Q_DECLARE_METATYPE(QAbstractItemModel::LayoutChangeHint)

Q_DECLARE_LOGGING_CATEGORY(lcItemModels)

#endif // TST_QSORTFILTERPROXYMODEL_H
