// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qrangemodel.h>
#include <QtCore/qstringlistmodel.h>

class tst_bench_QRangeModel: public QObject
{
    Q_OBJECT
public:
    tst_bench_QRangeModel(QObject *parent = nullptr);

    enum Type {
        StringListModel,
        VectorStrings,
        ArrayStrings,
    };

private Q_SLOTS:
    void stringList_data();
    void stringList();
};

tst_bench_QRangeModel::tst_bench_QRangeModel(QObject *parent)
    : QObject(parent)
{
}

void tst_bench_QRangeModel::stringList_data()
{
    QTest::addColumn<Type>("type");
    QTest::addRow("StringListModel") << StringListModel;
    QTest::addRow("VectorStrings") << VectorStrings;
    QTest::addRow("ArrayStrings") << ArrayStrings;
}

void tst_bench_QRangeModel::stringList()
{
    QFETCH(Type, type);

    std::array<QString, 100000> strings;
    for (size_t i = 0; i < std::size(strings); ++i)
        strings[i] = QString::number(i);

    std::unique_ptr<QAbstractItemModel> model;

    switch (type) {
    case StringListModel: {
        model.reset(new QStringListModel(QStringList(std::begin(strings), std::end(strings))));
        break;
    }
    case VectorStrings:
        model.reset(new QRangeModel(std::vector<QString>(std::begin(strings),
                                                         std::end(strings))));
        break;
    case ArrayStrings:
        model.reset(new QRangeModel(strings));
        break;
    }

    QBENCHMARK {
        for (size_t i = 0; i < std::size(strings); ++i) {
            model->data(model->index(i, 0));
        }
    }
}

QTEST_MAIN(tst_bench_QRangeModel)
#include "tst_bench_qrangemodel.moc"
