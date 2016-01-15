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

#include <QtTest/QtTest>

class tst_QtConcurrentMedian: public QObject
{
    Q_OBJECT
private slots:
    void median_data();
    void median();
};

void tst_QtConcurrentMedian::median_data()
{
    QTest::addColumn<QList<int> >("values");
    QTest::addColumn<int>("expectedMedian");

    QTest::newRow("size=1")
        << (QList<int>() << 1)
        << 1;

    QTest::newRow("size=2")
        << (QList<int>() << 3 << 2)
        << 3;

    QTest::newRow("size=3")
        << (QList<int>() << 3 << 1 << 2)
        << 2;

    QTest::newRow("gcc bug 58800 (nth_element)")
        << (QList<int>() << 207089 << 202585 << 180067 << 157549 << 211592 << 216096 << 207089)
        << 207089;
}

void tst_QtConcurrentMedian::median()
{
    QFETCH(QList<int> , values);
    QFETCH(int, expectedMedian);

    QtConcurrent::Median<int> m(values.size());
    foreach (int value, values)
        m.addValue(value);
    QCOMPARE(m.median(), expectedMedian);
}

QTEST_MAIN(tst_QtConcurrentMedian)
#include "tst_qtconcurrentmedian.moc"
