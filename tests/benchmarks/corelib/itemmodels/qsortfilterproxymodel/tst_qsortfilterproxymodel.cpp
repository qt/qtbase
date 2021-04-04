/****************************************************************************
**
** Copyright (C) 2021 Igor Kushnir <igorkuo@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    QStringListModel model(qAsConst(m_numberList));
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

QTEST_MAIN(tst_QSortFilterProxyModel)

#include "tst_qsortfilterproxymodel.moc"
