// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include "../qrangemodel/data.h"

// std::ranges::min() and max() of a range store the result in a temporary.
// libstdc++ before GCC 16 declares that temporary as auto, so with a mutable
// adapter it is a copy of the proxy reference, and assigning to it modifies the
// model (GCC bug 111861). Other implementations use the range's value_type.
#if defined(__cpp_lib_ranges) && (!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE >= 16)
#  define QRMA_RANGES_MINMAX_USES_VALUE_TYPE
#endif

class tst_QRangeModelAdapter : public QRangeModelTest
{
    Q_OBJECT
public:
    using QRangeModelTest::QRangeModelTest;

    // compile tests
    void construct_API();
    void typedefs_API();

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
    void sort_API();

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
    void referenceComparison();

    void frontBack();
    void reverseIterate();
    void iteratorValueType();

    void treeIterate();
    void treeAccess();
    void treeWriteAccess();
    void treeFindRecursively();

    void insertRow();
    void insertRows();
    void insertAtIterator();
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
    void sortRows();

    void buildValueTree();
    void buildPointerTree();

    void insertAutoConnectObjects();

private:
    void expectInvalidIndex(int count);
    static value_tree createValueTree();
    static pointer_tree createPointerTree();

    std::unique_ptr<Data> m_data;
};
