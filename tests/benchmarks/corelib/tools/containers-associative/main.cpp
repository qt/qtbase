/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QString>

#include <qtest.h>

class tst_associative_containers : public QObject
{
    Q_OBJECT
private slots:
    void insert_data();
    void insert();
    void lookup_data();
    void lookup();
};

template <typename T>
void testInsert(int size)
{
    T container;

    QBENCHMARK {
        for (int i = 0; i < size; ++i)
            container.insert(i, i);
    }
}

void tst_associative_containers::insert_data()
{
    QTest::addColumn<bool>("useHash");
    QTest::addColumn<int>("size");

    for (int size = 10; size < 20000; size += 100) {

        const QByteArray sizeString = QByteArray::number(size);

        QTest::newRow(QByteArray("hash--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("map--" + sizeString).constData()) << false << size;
    }
}

void tst_associative_containers::insert()
{
    QFETCH(bool, useHash);
    QFETCH(int, size);

    QHash<int, int> testHash;
    QMap<int, int> testMap;

    if (useHash) {
        testInsert<QHash<int, int> >(size);
    } else {
        testInsert<QMap<int, int> >(size);
    }
}

void tst_associative_containers::lookup_data()
{
//    setReportType(LineChartReport);
//    setChartTitle("Time to call value(), with an increasing number of items in the container");

    QTest::addColumn<bool>("useHash");
    QTest::addColumn<int>("size");

    for (int size = 10; size < 20000; size += 100) {

        const QByteArray sizeString = QByteArray::number(size);

        QTest::newRow(QByteArray("hash--" + sizeString).constData()) << true << size;
        QTest::newRow(QByteArray("map--" + sizeString).constData()) << false << size;
    }
}

template <typename T>
void testLookup(int size)
{
    T container;

    for (int i = 0; i < size; ++i)
        container.insert(i, i);

    int val;

    QBENCHMARK {
        for (int i = 0; i < size; ++i)
            val = container.value(i);

    }
}

void tst_associative_containers::lookup()
{
    QFETCH(bool, useHash);
    QFETCH(int, size);

    if (useHash) {
        testLookup<QHash<int, int> >(size);
    } else {
        testLookup<QMap<int, int> >(size);
    }
}

QTEST_MAIN(tst_associative_containers)
#include "main.moc"
