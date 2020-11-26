/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    QFETCH(QList<double> , values);
    QFETCH(double, expectedMedian);

    QtConcurrent::Median m;
    foreach (double value, values)
        m.addValue(value);
    QCOMPARE(m.median(), expectedMedian);
}

QTEST_MAIN(tst_QtConcurrentMedian)
#include "tst_qtconcurrentmedian.moc"
