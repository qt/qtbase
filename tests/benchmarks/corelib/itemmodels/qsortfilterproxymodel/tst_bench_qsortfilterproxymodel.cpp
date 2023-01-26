// Copyright (C) 2021 Igor Kushnir <igorkuo@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QTest>

static void resizeNumberList(QStringList &numberList, int size)
{
    if (!numberList.empty())
        QCOMPARE(numberList.constLast(), QString::number(numberList.size()));

    if (numberList.size() < size) {
        numberList.reserve(size);
        for (int i = numberList.size() + 1; i <= size; ++i)
            numberList.push_back(QString::number(i));
    } else if (numberList.size() > size) {
        numberList.erase(numberList.begin() + size, numberList.end());
    }

    QCOMPARE(numberList.size(), size);
    if (!numberList.empty())
        QCOMPARE(numberList.constLast(), QString::number(numberList.size()));
}

class tst_QSortFilterProxyModel : public QObject
{
    Q_OBJECT
private slots:
    void clearFilter_data();
    void clearFilter();
    void setSourceModel();

private:
    QStringList m_numberList; ///< Cache the strings for efficiency.
};

void tst_QSortFilterProxyModel::clearFilter_data()
{
    QTest::addColumn<int>("itemCount");
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<int>("filteredRowCount");

    const auto matchSingleItem = [](int item) { return QStringLiteral("^%1$").arg(item); };

    for (int thousandItemCount : { 10, 25, 50, 100, 250, 500, 1000, 2000 }) {
        const auto itemCount = thousandItemCount * 1000;

        QTest::addRow("no match in %dK", thousandItemCount) << itemCount << "-" << 0;
        QTest::addRow("match all in %dK", thousandItemCount) << itemCount << "\\d+" << itemCount;

        QTest::addRow("match first in %dK", thousandItemCount)
                << itemCount << matchSingleItem(1) << 1;
        QTest::addRow("match 1000th in %dK", thousandItemCount)
                << itemCount << matchSingleItem(1000) << 1;
        QTest::addRow("match middle in %dK", thousandItemCount)
                << itemCount << matchSingleItem(itemCount / 2) << 1;
        QTest::addRow("match 1000th from end in %dK", thousandItemCount)
                << itemCount << matchSingleItem(itemCount - 999) << 1;
        QTest::addRow("match last in %dK", thousandItemCount)
                << itemCount << matchSingleItem(itemCount) << 1;

        QTest::addRow("match each 10'000th in %dK", thousandItemCount)
                << itemCount << "0000$" << thousandItemCount / 10;
        QTest::addRow("match each 100'000th in %dK", thousandItemCount)
                << itemCount << "00000$" << thousandItemCount / 100;
    }
}

void tst_QSortFilterProxyModel::clearFilter()
{
    QFETCH(const int, itemCount);
    resizeNumberList(m_numberList, itemCount);
    QStringListModel model(std::as_const(m_numberList));
    QCOMPARE(model.rowCount(), itemCount);

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    QCOMPARE(model.rowCount(), itemCount);
    QCOMPARE(proxy.rowCount(), itemCount);

    QFETCH(const QString, pattern);
    QFETCH(const int, filteredRowCount);
    proxy.setFilterRegularExpression(pattern);
    QCOMPARE(model.rowCount(), itemCount);
    QCOMPARE(proxy.rowCount(), filteredRowCount);

    QBENCHMARK_ONCE {
        proxy.setFilterRegularExpression(QString());
    }
    QCOMPARE(model.rowCount(), itemCount);
    QCOMPARE(proxy.rowCount(), itemCount);
}

void tst_QSortFilterProxyModel::setSourceModel()
{
    QStringListModel model1;
    QStringListModel model2;

    QSortFilterProxyModel proxy;

    QBENCHMARK {
        proxy.setSourceModel(&model1);
        proxy.setSourceModel(&model2);
    }
}

QTEST_MAIN(tst_QSortFilterProxyModel)

#include "tst_bench_qsortfilterproxymodel.moc"
