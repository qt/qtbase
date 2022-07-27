// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QTest>

//dummy class
class tst_Skip
{
 public:
     void test_data();
};

void tst_Skip::test_data()
{
//! [1]
    QTest::addColumn<bool>("bool");
    QTest::newRow("local.1") << false;
    QTest::newRow("local.2") << true;

    QSKIP("skipping all");
//! [1]
}
