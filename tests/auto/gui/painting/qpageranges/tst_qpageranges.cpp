// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <qpageranges.h>

typedef QList<QPageRanges::Range> PageRangeList;

class tst_QPageRanges : public QObject
{
    Q_OBJECT

private slots:
    void basics();

    void addPage_data();
    void addPage();

    void addRange_data();
    void addRange();

    void fromToString_data();
    void fromToString();
};

void tst_QPageRanges::basics()
{
    QPageRanges empty;
    QVERIFY(empty.isEmpty());
    QVERIFY(!empty.contains(0));
    QCOMPARE(empty.firstPage(), 0);
    QCOMPARE(empty.lastPage(), 0);

    QPageRanges single;
    single.addPage(1);

    QVERIFY(!single.isEmpty());
    QVERIFY(single.contains(1));
    QCOMPARE(single.firstPage(), 1);
    QCOMPARE(single.firstPage(), 1);

    QPageRanges copy(single);
    QCOMPARE(copy, single);

    QPageRanges moved(std::move(copy));
    QCOMPARE(moved, single);
    QVERIFY(copy.isEmpty());

    single.clear();
    QVERIFY(single.isEmpty());
    QVERIFY(!moved.isEmpty());
}

void tst_QPageRanges::addPage_data()
{
    QTest::addColumn<QList<int>>("pageNumbers");
    QTest::addColumn<PageRangeList>("expected");

    QTest::newRow("single") << QList<int>{5} << PageRangeList{{5, 5}};
    QTest::newRow("duplicate") << QList<int>{5, 5} << PageRangeList{{5, 5}};
    QTest::newRow("adjacent") << QList<int>{5, 6, 7} << PageRangeList{{5, 7}};
    QTest::newRow("separate") << QList<int>{1, 3, 5} << PageRangeList{{1, 1}, {3, 3}, {5, 5}};
    QTest::newRow("unsorted") << QList<int>{5, 3, 4, 1} << PageRangeList{{1, 1}, {3, 5}};
    QTest::newRow("invalid") << QList<int>{-1} << PageRangeList{};
}

void tst_QPageRanges::addPage()
{
    QFETCH(QList<int>, pageNumbers);
    QFETCH(PageRangeList, expected);

    QPageRanges result;
    for (int pageNumber : std::as_const(pageNumbers)) {
        if (QByteArrayView(QTest::currentDataTag()) == "invalid")
            QTest::ignoreMessage(QtWarningMsg, "QPageRanges::addPage: 'pageNumber' must be greater than 0");
        result.addPage(pageNumber);
    }

    QCOMPARE(result.toRangeList(), expected);
}

void tst_QPageRanges::addRange_data()
{
    QTest::addColumn<PageRangeList>("ranges");
    QTest::addColumn<PageRangeList>("expected");

    QTest::newRow("single")     << PageRangeList{{5, 5}}
                                << PageRangeList{{5, 5}};
    QTest::newRow("duplicate")  << PageRangeList{{5, 5}, {5, 5}}
                                << PageRangeList{{5, 5}};
    QTest::newRow("adjacent")   << PageRangeList{{1, 3}, {4, 6}, {7, 10}}
                                << PageRangeList{{1, 10}};
    QTest::newRow("separate")   << PageRangeList{{1, 2}, {4, 5}, {7, 8}}
                                << PageRangeList{{1, 2}, {4, 5}, {7, 8}};
    QTest::newRow("overlap")    << PageRangeList{{1, 5}, {4, 8}, {8, 10}}
                                << PageRangeList{{1, 10}};
    QTest::newRow("included")   << PageRangeList{{1, 5}, {2, 3}, {8, 9}, {7, 10}}
                                << PageRangeList{{1, 5}, {7, 10}};
    QTest::newRow("unsorted")   << PageRangeList{{7, 8}, {1, 4}, {9, 10}}
                                << PageRangeList{{1, 4}, {7, 10}};
    QTest::newRow("flipped")    << PageRangeList{{5, 1}}
                                << PageRangeList{{1, 5}};
    QTest::newRow("invalid1")   << PageRangeList{{-1, 2}}
                                << PageRangeList{};
    QTest::newRow("invalid2")   << PageRangeList{{0, -1}}
                                << PageRangeList{};
}

void tst_QPageRanges::addRange()
{
    QFETCH(PageRangeList, ranges);
    QFETCH(PageRangeList, expected);

    QPageRanges result;
    for (const auto &range : std::as_const(ranges)) {
        const QByteArrayView testdata(QTest::currentDataTag());
        if (testdata.startsWith("invalid"))
            QTest::ignoreMessage(QtWarningMsg, "QPageRanges::addRange: 'from' and 'to' must be greater than 0");

        result.addRange(range.from, range.to);
    }

    QCOMPARE(result.toRangeList(), expected);
}

void tst_QPageRanges::fromToString_data()
{
    QTest::addColumn<QString>("fromString");
    QTest::addColumn<PageRangeList>("rangeList");
    QTest::addColumn<QString>("toString");

    QTest::newRow("invalid") << QString(",-8")
                             << PageRangeList{}
                             << QString();

    QTest::newRow("overlapping") << QString("1-3,5-9,6-7,8-11")
                                 << PageRangeList{{1, 3}, {5, 11}}
                                 << QString("1-3,5-11");

    QTest::newRow("spacy") << QString("1 -2, 3- 4 ,\t5-6\t,7 - 8 ,\n9 - 10")
                                 << PageRangeList{{1, 10}}
                                 << QString("1-10");
}

void tst_QPageRanges::fromToString()
{
    QFETCH(QString, fromString);
    QFETCH(PageRangeList, rangeList);
    QFETCH(QString, toString);

    QPageRanges ranges = QPageRanges::fromString(fromString);
    QCOMPARE(ranges.toRangeList(), rangeList);
    QCOMPARE(ranges.toString(), toString);
}

QTEST_APPLESS_MAIN(tst_QPageRanges)

#include "tst_qpageranges.moc"
