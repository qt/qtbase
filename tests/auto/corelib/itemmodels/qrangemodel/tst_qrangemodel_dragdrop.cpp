// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qrangemodel.h"

void tst_QRangeModel::dragDropActions_data()
{
    QTest::addColumn<Factory>("factory");
    QTest::addColumn<Qt::DropActions>("defaultDragActions");
    QTest::addColumn<Qt::DropActions>("possibleDragActions");
    QTest::addColumn<Qt::DropActions>("defaultDropActions");
    QTest::addColumn<Qt::DropActions>("possibleDropActions");

    m_data.reset(new Data);

    QTest::addRow("Fixed-sized") << Factory([this]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(std::ref(m_data->fixedArrayOfNumbers));
    }) << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::MoveAction|Qt::LinkAction)
       << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::MoveAction|Qt::LinkAction);
    QTest::addRow("Fixed-Columns") << Factory([this]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(std::ref(m_data->vectorOfFixedColumns));
    }) << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::MoveAction|Qt::LinkAction)
       << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::MoveAction|Qt::LinkAction);
    QTest::addRow("Read-Only") << Factory([this]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(std::ref(m_data->constTableOfNumbers));
    }) << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::LinkAction)
       << Qt::DropActions(Qt::IgnoreAction) << Qt::DropActions(Qt::IgnoreAction);
}

void tst_QRangeModel::dragDropActions()
{
    QFETCH(Factory, factory);
    QFETCH(Qt::DropActions, defaultDragActions);
    QFETCH(Qt::DropActions, possibleDragActions);
    QFETCH(Qt::DropActions, defaultDropActions);
    QFETCH(Qt::DropActions, possibleDropActions);
    auto model = factory();
    auto *rangeModel = qobject_cast<QRangeModel *>(model.get());

    QCOMPARE(model->supportedDragActions(), defaultDragActions);
    QCOMPARE(model->supportedDropActions(), defaultDropActions);

    rangeModel->setSupportedDragActions(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
    QCOMPARE(model->supportedDragActions(), possibleDragActions);
    rangeModel->setSupportedDropActions(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
    QCOMPARE(model->supportedDropActions(), possibleDropActions);
}
