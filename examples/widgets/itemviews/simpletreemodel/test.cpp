// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "treemodel.h"

#include <QObject>
#include <QAbstractItemModelTester>
#include <QTest>

using namespace Qt::StringLiterals;

//! [1]
class TestSimpleTreeModel : public QObject
{
    Q_OBJECT

private slots:
    void testTreeModel();
};

void TestSimpleTreeModel::testTreeModel()
{
    constexpr auto fileName = ":/default.txt"_L1;
    QFile file(fileName);
    QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text),
             qPrintable(fileName + " cannot be opened: "_L1 + file.errorString()));
    TreeModel model(QString::fromUtf8(file.readAll()));

    QAbstractItemModelTester tester(&model);
}

QTEST_APPLESS_MAIN(TestSimpleTreeModel)

#include "test.moc"
//! [1]
