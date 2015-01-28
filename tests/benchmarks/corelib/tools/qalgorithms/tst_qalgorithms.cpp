/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2012 Robin Burchell <robin+qt@viroteck.net>
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

#include <QtTest/QtTest>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <qalgorithms.h>
#include <QStringList>
#include <QString>
#include <QVector>

using namespace std;

class tst_QAlgorithms : public QObject
{
    Q_OBJECT
private slots:
    void stableSort_data();
    void stableSort();

    void sort_data();
    void sort();
};

template <typename DataType>
QVector<DataType> generateData(QString dataSetType, const int length)
{
    QVector<DataType> container;
    if (dataSetType == "Random") {
        for (int i = 0; i < length; ++i)
            container.append(rand());
    } else if (dataSetType == "Ascending") {
        for (int i = 0; i < length; ++i)
            container.append(i);
    } else if (dataSetType == "Descending") {
        for (int i = 0; i < length; ++i)
            container.append(length - i);
    } else if (dataSetType == "Equal") {
        for (int i = 0; i < length; ++i)
            container.append(43);
    } else if (dataSetType == "Duplicates") {
        for (int i = 0; i < length; ++i)
            container.append(i % 10);
    } else if (dataSetType == "Almost Sorted") {
        for (int i = 0; i < length; ++i)
            container.append(i);
        for (int i = 0; i<= length / 10; ++i) {
            const int iswap = i * 9;
            DataType tmp = container.at(iswap);
            container[iswap] = container.at(iswap + 1);
            container[iswap + 1] = tmp;
        }
    }

    return container;
}


void tst_QAlgorithms::stableSort_data()
{
    const int dataSize = 5000;
    QTest::addColumn<QVector<int> >("unsorted");
    QTest::newRow("Equal") << (generateData<int>("Equal", dataSize));
    QTest::newRow("Ascending") << (generateData<int>("Ascending", dataSize));
    QTest::newRow("Descending") << (generateData<int>("Descending", dataSize));
    QTest::newRow("Duplicates") << (generateData<int>("Duplicates", dataSize));
    QTest::newRow("Almost Sorted") << (generateData<int>("Almost Sorted", dataSize));
}

void tst_QAlgorithms::stableSort()
{
    QFETCH(QVector<int>, unsorted);

    QBENCHMARK {
        QVector<int> sorted = unsorted;
        qStableSort(sorted.begin(), sorted.end());
    }
}

void tst_QAlgorithms::sort_data()
{
    stableSort_data();
}

void tst_QAlgorithms::sort()
{
    QFETCH(QVector<int>, unsorted);

    QBENCHMARK {
        QVector<int> sorted = unsorted;
        qSort(sorted.begin(), sorted.end());
    }
}


QTEST_MAIN(tst_QAlgorithms)
#include "tst_qalgorithms.moc"

