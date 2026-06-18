// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include "data.h"

#include <QTest>
#include <QtCore/qrangemodel.h>

class tst_QRangeModel : public QRangeModelTest
{
    Q_OBJECT

private slots:
    void basics_data() { createTestData(); }
    void basics();
    void modifies_data();
    void modifies();
    void minimalIterator();
    void ranges();
    void json();
    void ownership();
    void overrideRoleNames();
    void setRoleNames();
    void defaultRoleNames();
    void autoConnectPolicy_data();
    void autoConnectPolicy();

    void dimensions_data() { createTestData(); }
    void dimensions();
    void sibling_data() { createTestData(); }
    void sibling();
    void flags_data() { createTestData(); }
    void flags();
    void headerData_data() { createTestData(); }
    void headerData();
    void data_data() { createTestData(); }
    void data();
    void multiData_data() { createTestData(); }
    void multiData();
    void setData_data() { createTestData(); }
    void setData();
    void itemData_data() { createTestData(); }
    void itemData();
    void setItemData_data() { createTestData(); }
    void setItemData();
    void clearItemData_data() { createTestData(); }
    void clearItemData();
    void modelData_data() { createTestData(); }
    void modelData();
    void rangeModelDataInTable();

    void insertRows_data() { createTestData(); }
    void insertRows();
    void removeRows_data() { createTestData(); }
    void removeRows();
    void moveRows_data() { createTestData(); }
    void moveRows();
    void insertColumns_data() { createTestData(); }
    void insertColumns();
    void removeColumns_data() { createTestData(); }
    void removeColumns();
    void moveColumns_data() { createTestData(); }
    void moveColumns();

    void inconsistentColumnCount();
    void largeArrays();
    void mapsAsRange();
    void spanAsRange();
    void filterAsRange();
    void multiRoleContainer();
    void multiRoleTuple();

    void tree_data();
    void tree();
    void gadgetTree();
    void treeModifyBranch_data() { tree_data(); }
    void treeModifyBranch();
    void treeCreateBranch_data() { tree_data(); }
    void treeCreateBranch();
    void treeRemoveBranch_data() { tree_data(); }
    void treeRemoveBranch();
    void treeMoveRows_data() { tree_data(); }
    void treeMoveRows();
    void treeMoveRowBranches_data() { tree_data(); }
    void treeMoveRowBranches();

    void matchRecursive_data() { tree_data(); }
    void matchRecursive();

    void adlTest();

    void itemAccess_data();
    void itemAccess();

    void sortBasic();
    void sort_data() { createTestData(); }
    void sort();
    void sortRole();
    void sortCollator_data();
    void sortCollator();
    void sortTree_data(){ tree_data(); }
    void sortTree();

    void matchBasic();
    void match_data() { createTestData(); }
    void match();

    void dragDropActions_data();
    void dragDropActions();

    void mimeTypes_data();
    void mimeTypes();

    void mimeData_data();
    void mimeData();

    void dropMimeData_data();
    void dropMimeData();

    void dragDropFlagsNullPointer();
    void dropMimeDataNullRow();

private:
    void createTestData();
    void createTree();

    QList<QPersistentModelIndex> allIndexes(QAbstractItemModel *model, const QModelIndex &parent = {});
    void verifyPmiList(const QList<QPersistentModelIndex> &pmiList);
    bool treeIntegrityCheck();

    std::unique_ptr<QAbstractItemModel> makeTreeModel();
    std::unique_ptr<Data> m_data;
};

using Factory = std::function<std::unique_ptr<QAbstractItemModel>()>;
