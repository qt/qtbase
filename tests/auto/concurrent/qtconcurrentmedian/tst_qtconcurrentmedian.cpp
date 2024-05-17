// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtconcurrentmedian.h>

#include <QTest>

class tst_QtConcurrentMedian: public QObject
{
    Q_OBJECT
private slots:
    void median_data();
    void median();
};

void tst_QtConcurrentMedian::median_data()
{
    QTest::addColumn<QList<double> >("values");
    QTest::addColumn<double>("expectedMedian");

    QTest::newRow("size=1")
        << (QList<double>() << 1.0)
        << 0.0; // six 0.0 in front of the actual value

    QTest::newRow("size=2")
        << (QList<double>() << 3.0 << 2.0)
        << 0.0; // five 0.0 in front of the actual value

    QTest::newRow("size=3")
        << (QList<double>() << 3.0 << 1.0 << 2.0)
        << 0.0; // four 0.0 in front of the actual value

    QTest::newRow("size=4")
        << (QList<double>() << 3.0 << 1.0 << 2.0 << 4.0)
        << 1.0; // three 0.0 in front of the first actual value, pick 1.0

    QTest::newRow("size=5")
        << (QList<double>() << 3.0 << 1.0 << 2.0 << 3.0 << 1.0)
        << 1.0; // two 0.0 in front of the first actual value, pick 1.0

    QTest::newRow("size=6")
        << (QList<double>() << 3.0 << 1.0 << 2.0 << 3.0 << 1.0 << 2.0)
        << 2.0; // one 0.0 in front of the first actual value, pick 2.0

    QTest::newRow("size=7")
        << QList<double> { 207089.0, 202585.0, 180067.0, 157549.0, 211592.0, 216096.0, 207089.0 }
        << 207089.0;
}

void tst_QtConcurrentMedian::median()
{
    QFETCH(const QList<double> , values);
    QFETCH(double, expectedMedian);

    QtConcurrent::Median m;
    for (double value : values)
        m.addValue(value);
    QCOMPARE(m.median(), expectedMedian);
}

QTEST_MAIN(tst_QtConcurrentMedian)
#include "tst_qtconcurrentmedian.moc"
