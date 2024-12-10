// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QCOMPAREHELPERS_H
#define TST_QCOMPAREHELPERS_H

#include <QtCore/qcompare.h>

#include <QtTest/qtest.h>
#include <QtTest/private/qcomparisontesthelper_p.h>

#ifndef QTEST_THROW_ON_FAIL
# error This test requires QTEST_THROW_ON_FAIL being active.
#endif

class tst_QCompareHelpers : public QObject
{
    Q_OBJECT

private:
    template <typename LeftType, typename RightType, typename OrderingType>
    void compareImpl();

    template <typename LeftType, typename RightType>
    void compareIntData();

    template <typename LeftType, typename RightType>
    void compareFloatData();

    template <typename LeftType, typename RightType>
    void compareStringData();

private Q_SLOTS:
    // tst_qcomparehelpers.cpp
    void comparisonCompiles();

    void compare_IntWrapper_data();
    void compare_IntWrapper();

    void compare_IntWrapper_int_data();
    void compare_IntWrapper_int();

    void compare_DoubleWrapper_data();
    void compare_DoubleWrapper();

    void compare_DoubleWrapper_double_data();
    void compare_DoubleWrapper_double();

    void compare_IntWrapper_DoubleWrapper_data();
    void compare_IntWrapper_DoubleWrapper();

    void compare_StringWrapper_data();
    void compare_StringWrapper();

    void compare_StringWrapper_AnyStringView_data();
    void compare_StringWrapper_AnyStringView();

    void generatedClasses();

    void builtinOrder();

    // Add new test cases to tst_qcomparehelpers1.cpp, because minGW already
    // complains about a too large tst_qcomparehelpers.cpp.obj object file
    void compareWithAttributes();

    void totallyOrderedWrapperBasics();

    void compareAutoReturnType();

private:
    template <typename LeftType, typename RightType, typename OrderingType>
    void lexicographicalCompareThreeWayDataImpl();

    template <typename LeftType, typename RightType, typename OrderingType>
    void lexicographicalCompareThreeWayImpl();

    template <typename LeftType, typename RightType, typename OrderingType>
    void lexicographicalCompareThreeWayComparatorImpl();

private slots:
    void lexicographicalCompareThreeWay_ThreeWayCmp_Int_data();
    void lexicographicalCompareThreeWay_ThreeWayCmp_Int();
    void lexicographicalCompareThreeWay_ThreeWayCmp_Float_data();
    void lexicographicalCompareThreeWay_ThreeWayCmp_Float();
    void lexicographicalCompareThreeWay_ThreeWayCmp_Weak_data();
    void lexicographicalCompareThreeWay_ThreeWayCmp_Weak();

    void lexicographicalCompareThreeWay_Mixed_Int_data();
    void lexicographicalCompareThreeWay_Mixed_Int();
    void lexicographicalCompareThreeWay_Mixed_Float_data();
    void lexicographicalCompareThreeWay_Mixed_Float();
    void lexicographicalCompareThreeWay_Mixed_Weak_data();
    void lexicographicalCompareThreeWay_Mixed_Weak();

    void lexicographicalCompareThreeWay_Comparator_Int_data();
    void lexicographicalCompareThreeWay_Comparator_Int();
    void lexicographicalCompareThreeWay_Comparator_Float_data();
    void lexicographicalCompareThreeWay_Comparator_Float();
    void lexicographicalCompareThreeWay_Comparator_Weak_data();
    void lexicographicalCompareThreeWay_Comparator_Weak();

    void lexicographicalCompareThreeWay_Pointers();
    void lexicographicalCompareThreeWay_NonCopyMove();
    void lexicographicalCompareThreeWay_CustomPointerHelper();
};

#endif // TST_QCOMPAREHELPERS_H
