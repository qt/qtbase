// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QCOMPAREHELPERS_H
#define TST_QCOMPAREHELPERS_H

#include <QtCore/qcompare.h>

#include <QtTest/qtest.h>
#include <QtTest/private/qcomparisontesthelper_p.h>

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

    // Add new test cases to another cpp file, because minGW already complains
    // about a too large tst_qcomparehelpers.cpp.obj object file
};

#endif // TST_QCOMPAREHELPERS_H
