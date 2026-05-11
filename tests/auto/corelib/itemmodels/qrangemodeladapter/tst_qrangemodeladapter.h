// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include "../qrangemodel/data.h"

class tst_QRangeModelAdapter : public QRangeModelTest
{
    Q_OBJECT
public:
    using QRangeModelTest::QRangeModelTest;

    // compile tests
    void construct_API();

    void assign_API();

    void indexOfRow_API();
    void indexOfCell_API();
    void indexOfPath_API();

    void dimension_API();

    void iterator_API();
    void access_API();

    void insertRow_API();
    void insertRows_API();
    void removeRow_API();
    void removeRows_API();
    void moveRow_API();
    void moveRows_API();

    void insertColumn_API();
    void insertColumns_API();
    void removeColumn_API();
    void removeColumns_API();
    void moveColumn_API();
    void moveColumns_API();

private slots:
    void init();
    void cleanup();

    void construct();

    void modelLifetime();
    void valueBehavior();
    void modelReset();

    void listIterate();
    void listAccess();
    void listWriteAccess();

    void tableIterate();
    void tableAccess();
    void tableWriteAccess();

    void treeIterate();
    void treeAccess();
    void treeWriteAccess();
    void treeFindRecursively();

    void insertRow();
    void insertRows();
    void removeRow();
    void removeRows();
    void moveRow();
    void moveRows();

    void insertColumn();
    void insertColumns();
    void removeColumn();
    void removeColumns();
    void moveColumn();
    void moveColumns();

    void buildValueTree();
    void buildPointerTree();

    void insertAutoConnectObjects();

private:
    void expectInvalidIndex(int count);
    static value_tree createValueTree();
    static pointer_tree createPointerTree();

    std::unique_ptr<Data> m_data;
};
