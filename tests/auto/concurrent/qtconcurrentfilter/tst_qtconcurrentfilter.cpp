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
#include <qtconcurrentfilter.h>
#include <QCoreApplication>
#include <QList>
#include <QLinkedList>
#include <QtTest/QtTest>

#include "../qtconcurrentmap/functions.h"

class tst_QtConcurrentFilter : public QObject
{
    Q_OBJECT

private slots:
    void filter();
    void filtered();
    void filteredReduced();
    void resultAt();
    void incrementalResults();
    void noDetach();
    void stlContainers();
};

void tst_QtConcurrentFilter::filter()
{
    // functor
    {
        QList<int> list;
        list << 1 << 2 << 3 << 4;
        QtConcurrent::filter(list, KeepEvenIntegers()).waitForFinished();
        QCOMPARE(list, QList<int>() << 2 << 4);
    }
    {
        QList<int> list;
        list << 1 << 2 << 3 << 4;
        QtConcurrent::blockingFilter(list, KeepEvenIntegers());
        QCOMPARE(list, QList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QtConcurrent::filter(linkedList, KeepEvenIntegers()).waitForFinished();
        QCOMPARE(linkedList, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QtConcurrent::blockingFilter(linkedList, KeepEvenIntegers());
        QCOMPARE(linkedList, QLinkedList<int>() << 2 << 4);
    }
    {
        QVector<int> vector;
        vector << 1 << 2 << 3 << 4;
        QtConcurrent::filter(vector, KeepEvenIntegers()).waitForFinished();
        QCOMPARE(vector, QVector<int>() << 2 << 4);
    }
    {
        QVector<int> vector;
        vector << 1 << 2 << 3 << 4;
        QtConcurrent::blockingFilter(vector, KeepEvenIntegers());
        QCOMPARE(vector, QVector<int>() << 2 << 4);
    }

    // function
    {
        QList<int> list;
        list << 1 << 2 << 3 << 4;
        QtConcurrent::filter(list, keepEvenIntegers).waitForFinished();
        QCOMPARE(list, QList<int>() << 2 << 4);
    }
    {
        QList<int> list;
        list << 1 << 2 << 3 << 4;
        QtConcurrent::blockingFilter(list, keepEvenIntegers);
        QCOMPARE(list, QList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QtConcurrent::filter(linkedList, keepEvenIntegers).waitForFinished();
        QCOMPARE(linkedList, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QtConcurrent::blockingFilter(linkedList, keepEvenIntegers);
        QCOMPARE(linkedList, QLinkedList<int>() << 2 << 4);
    }

    // bound function
    {
        QList<int> list;
        list << 1 << 2 << 3 << 4;
        QtConcurrent::filter(list, keepEvenIntegers).waitForFinished();
        QCOMPARE(list, QList<int>() << 2 << 4);
    }
    {
        QList<int> list;
        list << 1 << 2 << 3 << 4;
        QtConcurrent::blockingFilter(list, keepEvenIntegers);
        QCOMPARE(list, QList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QtConcurrent::filter(linkedList, keepEvenIntegers).waitForFinished();
        QCOMPARE(linkedList, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QtConcurrent::blockingFilter(linkedList, keepEvenIntegers);
        QCOMPARE(linkedList, QLinkedList<int>() << 2 << 4);
    }

    // member
    {
        QList<Number> list;
        list << 1 << 2 << 3 << 4;
        QtConcurrent::filter(list, &Number::isEven).waitForFinished();
        QCOMPARE(list, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> list;
        list << 1 << 2 << 3 << 4;
        QtConcurrent::blockingFilter(list, &Number::isEven);
        QCOMPARE(list, QList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QtConcurrent::filter(linkedList, &Number::isEven).waitForFinished();
        QCOMPARE(linkedList, QLinkedList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QtConcurrent::blockingFilter(linkedList, &Number::isEven);
        QCOMPARE(linkedList, QLinkedList<Number>() << 2 << 4);
    }
}

void tst_QtConcurrentFilter::filtered()
{
    QList<int> list;
    list << 1 << 2 << 3 << 4;

    // functor
    {
        QFuture<int> f = QtConcurrent::filtered(list, KeepEvenIntegers());
        QList<int> list2 = f.results();
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(list.begin(), list.end(), KeepEvenIntegers());
        QList<int> list2 = f.results();
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(list.constBegin(),
                                                list.constEnd(),
                                                KeepEvenIntegers());
        QList<int> list2 = f.results();
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFiltered(list, KeepEvenIntegers());
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFiltered<QList<int> >(list.begin(),
                                                                       list.end(),
                                                                       KeepEvenIntegers());
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFiltered<QList<int> >(list.constBegin(),
                                                                       list.constEnd(),
                                                                       KeepEvenIntegers());
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }

    {
        QVector<int> vector;
        vector << 1 << 2 << 3 << 4;
        QVector<int> vector2 = QtConcurrent::blockingFiltered(vector, KeepEvenIntegers());
        QCOMPARE(vector2, QVector<int>() << 2 << 4);
    }
    {
        QVector<int> vector;
        vector << 1 << 2 << 3 << 4;
        QFuture<int> f = QtConcurrent::filtered(vector, KeepEvenIntegers());
        QCOMPARE(f.results(), QList<int>() << 2 << 4);
    }

    {
        QLinkedList<int> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered(linkedList, KeepEvenIntegers());
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList;
        linkedList << 1 << 2 << 3 << 4;
        QFuture<int> f = QtConcurrent::filtered(linkedList, KeepEvenIntegers());
        QCOMPARE(f.results(), QList<int>() << 2 << 4);
    }

    // function
    {
        QFuture<int> f = QtConcurrent::filtered(list, keepEvenIntegers);
        QList<int> list2 = f.results();
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(list.begin(), list.end(), keepEvenIntegers);
        QList<int> list2 = f.results();
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(list.constBegin(),
                                                list.constEnd(),
                                                keepEvenIntegers);
        QList<int> list2 = f.results();
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFiltered(list, keepEvenIntegers);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFiltered<QList<int> >(list.begin(),
                                                                       list.end(),
                                                                       keepEvenIntegers);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFiltered<QList<int> >(list.constBegin(),
                                                                       list.constEnd(),
                                                                       keepEvenIntegers);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }

    // bound function
    {
        QFuture<int> f = QtConcurrent::filtered(list, keepEvenIntegers);
        QList<int> list2 = f.results();
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(list.begin(), list.end(), keepEvenIntegers);
        QList<int> list2 = f.results();
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(list.constBegin(),
                                                list.constEnd(),
                                                keepEvenIntegers);
        QList<int> list2 = f.results();
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFiltered(list, keepEvenIntegers);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFiltered<QList<int> >(list.begin(),
                                                                       list.end(),
                                                                       keepEvenIntegers);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFiltered<QList<int> >(list.constBegin(),
                                                                       list.constEnd(),
                                                                       keepEvenIntegers);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }

    // const member function
    {
        QList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QFuture<Number> f = QtConcurrent::filtered(integers, &Number::isEven);
        QList<Number> list2 = f.results();
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QFuture<Number> f = QtConcurrent::filtered(integers.begin(),
                                                   integers.end(),
                                                   &Number::isEven);
        QList<Number> list2 = f.results();
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QFuture<Number> f = QtConcurrent::filtered(integers.constBegin(),
                                                   integers.constEnd(),
                                                   &Number::isEven);
        QList<Number> list2 = f.results();
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QList<Number> list2 = QtConcurrent::blockingFiltered(integers, &Number::isEven);
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QList<Number> list2 = QtConcurrent::blockingFiltered<QList<Number> >(integers.begin(),
                                                                             integers.end(),
                                                                             &Number::isEven);
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QList<Number> list2 =
            QtConcurrent::blockingFiltered<QList<Number> >(integers.constBegin(),
                                                           integers.constEnd(),
                                                           &Number::isEven);
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }

    // same thing on linked lists

    QLinkedList<int> linkedList;
    linkedList << 1 << 2 << 3 << 4;

    // functor
    {
        QFuture<int> f = QtConcurrent::filtered(linkedList, KeepEvenIntegers());
        QList<int> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(linkedList.begin(),
                                                linkedList.end(),
                                                KeepEvenIntegers());
        QList<int> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(linkedList.constBegin(),
                                                linkedList.constEnd(),
                                                KeepEvenIntegers());
        QList<int> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered(linkedList, KeepEvenIntegers());
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered<QLinkedList<int> >(linkedList.begin(),
                                                                                         linkedList.end(),
                                                                                         KeepEvenIntegers());
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered<QLinkedList<int> >(linkedList.constBegin(),
                                                                                         linkedList.constEnd(),
                                                                                         KeepEvenIntegers());
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }

    // function
    {
        QFuture<int> f = QtConcurrent::filtered(linkedList, keepEvenIntegers);
        QList<int> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(linkedList.begin(),
                                                linkedList.end(),
                                                keepEvenIntegers);
        QList<int> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(linkedList.constBegin(),
                                                linkedList.constEnd(),
                                                keepEvenIntegers);
        QList<int> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered(linkedList, keepEvenIntegers);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered<QLinkedList<int> >(linkedList.begin(),
                                                                                         linkedList.end(),
                                                                                         keepEvenIntegers);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered<QLinkedList<int> >(linkedList.constBegin(),
                                                                                         linkedList.constEnd(),
                                                                                         keepEvenIntegers);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }

    // bound function
    {
        QFuture<int> f = QtConcurrent::filtered(linkedList, keepEvenIntegers);
        QList<int> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(linkedList.begin(),
                                                linkedList.end(),
                                                keepEvenIntegers);
        QList<int> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<int>() << 2 << 4);
    }
    {
        QFuture<int> f = QtConcurrent::filtered(linkedList.constBegin(),
                                                linkedList.constEnd(),
                                                keepEvenIntegers);
        QList<int> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered(linkedList, keepEvenIntegers);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered<QLinkedList<int> >(linkedList.begin(),
                                                                                         linkedList.end(),
                                                                                         keepEvenIntegers);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFiltered<QLinkedList<int> >(linkedList.constBegin(),
                                                                                         linkedList.constEnd(),
                                                                                         keepEvenIntegers);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }

    // const member function
    {
        QLinkedList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QFuture<Number> f = QtConcurrent::filtered(integers, &Number::isEven);
        QList<Number> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QFuture<Number> f = QtConcurrent::filtered(integers.begin(),
                                                   integers.end(),
                                                   &Number::isEven);
        QList<Number> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QFuture<Number> f = QtConcurrent::filtered(integers.constBegin(),
                                                   integers.constEnd(),
                                                   &Number::isEven);
        QList<Number> linkedList2 = f.results();
        QCOMPARE(linkedList2, QList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QLinkedList<Number> linkedList2 = QtConcurrent::blockingFiltered(integers, &Number::isEven);
        QCOMPARE(linkedList2, QLinkedList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QLinkedList<Number> linkedList2 = QtConcurrent::blockingFiltered<QLinkedList<Number> >(integers.begin(),
                                                                                               integers.end(),
                                                                                               &Number::isEven);
        QCOMPARE(linkedList2, QLinkedList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> integers;
        integers << 1 << 2 << 3 << 4;
        QLinkedList<Number> linkedList2 =
            QtConcurrent::blockingFiltered<QLinkedList<Number> >(integers.constBegin(),
                                                                 integers.constEnd(),
                                                                 &Number::isEven);
        QCOMPARE(linkedList2, QLinkedList<Number>() << 2 << 4);
    }
}

void tst_QtConcurrentFilter::filteredReduced()
{
    QList<int> list;
    list << 1 << 2 << 3 << 4;
    QList<Number> numberList;
    numberList << 1 << 2 << 3 << 4;

    // functor-functor
    {
        int sum = QtConcurrent::filteredReduced<int>(list, KeepEvenIntegers(), IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced<int>(list, keepEvenIntegers, intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        QVector<int> vector;
        vector << 1 << 2 << 3 << 4;
        int sum = QtConcurrent::filteredReduced<int>(vector, KeepEvenIntegers(), IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(list.begin(),
                                                     list.end(),
                                                     KeepEvenIntegers(),
                                                     IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced<int>(list.begin(),
                                                      list.end(),
                                                      keepEvenIntegers,
                                                      intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(list.constBegin(),
                                                     list.constEnd(),
                                                     KeepEvenIntegers(),
                                                     IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced<int>(list.constBegin(),
                                                      list.constEnd(),
                                                      keepEvenIntegers,
                                                      intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(list, KeepEvenIntegers(), IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced<int>(list, keepEvenIntegers, intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(list.begin(),
                                                             list.end(),
                                                             KeepEvenIntegers(),
                                                             IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced<int>(list.begin(),
                                                              list.end(),
                                                              keepEvenIntegers,
                                                              intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(list.constBegin(),
                                                             list.constEnd(),
                                                             KeepEvenIntegers(),
                                                             IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced<int>(list.constBegin(),
                                                              list.constEnd(),
                                                              keepEvenIntegers,
                                                              intSumReduce);
        QCOMPARE(sum2, 6);
    }

    // function-functor
    {
        int sum = QtConcurrent::filteredReduced<int>(list, keepEvenIntegers, IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(list.begin(),
                                                     list.end(),
                                                     keepEvenIntegers,
                                                     IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(list.constBegin(),
                                                     list.constEnd(),
                                                     keepEvenIntegers,
                                                     IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(list, keepEvenIntegers, IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(list.begin(),
                                                             list.end(),
                                                             keepEvenIntegers,
                                                             IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(list.constBegin(),
                                                             list.constEnd(),
                                                             keepEvenIntegers,
                                                             IntSumReduce());
        QCOMPARE(sum, 6);
    }

    // functor-function
    {
        int sum = QtConcurrent::filteredReduced(list, KeepEvenIntegers(), intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(list.begin(),
                                                list.end(),
                                                KeepEvenIntegers(),
                                                intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(list.constBegin(),
                                                list.constEnd(),
                                                KeepEvenIntegers(),
                                                intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(list, KeepEvenIntegers(), intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(list.begin(),
                                                        list.end(),
                                                        KeepEvenIntegers(),
                                                        intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(list.constBegin(),
                                                        list.constEnd(),
                                                        KeepEvenIntegers(),
                                                        intSumReduce);
        QCOMPARE(sum, 6);
    }

    // function-function
    {
        int sum = QtConcurrent::filteredReduced(list, keepEvenIntegers, intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(list.begin(),
                                                list.end(),
                                                keepEvenIntegers,
                                                intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(list.constBegin(),
                                                list.constEnd(),
                                                keepEvenIntegers,
                                                intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(list, keepEvenIntegers, intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(list.begin(),
                                                        list.end(),
                                                        keepEvenIntegers,
                                                        intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(list.constBegin(),
                                                        list.constEnd(),
                                                        keepEvenIntegers,
                                                        intSumReduce);
        QCOMPARE(sum, 6);
    }

    // functor-member
    {
        QList<int> list2 = QtConcurrent::filteredReduced(list, KeepEvenIntegers(), &QList<int>::push_back, QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::filteredReduced(list.begin(),
                                                         list.end(),
                                                         KeepEvenIntegers(),
                                                         &QList<int>::push_back,
                                                         QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::filteredReduced(list.constBegin(),
                                                         list.constEnd(),
                                                         KeepEvenIntegers(),
                                                         &QList<int>::push_back,
                                                         QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFilteredReduced(list, KeepEvenIntegers(), &QList<int>::push_back, QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFilteredReduced(list.begin(),
                                                                 list.end(),
                                                                 KeepEvenIntegers(),
                                                                 &QList<int>::push_back,
                                                                 QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFilteredReduced(list.constBegin(),
                                                                 list.constEnd(),
                                                                 KeepEvenIntegers(),
                                                                 &QList<int>::push_back,
                                                                 QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }

    // member-functor
    {
        int sum = QtConcurrent::filteredReduced<int>(numberList, &Number::isEven, NumberSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced<int>(QList<Number>(numberList),
                                                      &Number::isEven,
                                                      NumberSumReduce());
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(numberList.begin(),
                                                     numberList.end(),
                                                     &Number::isEven,
                                                     NumberSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(numberList.constBegin(),
                                                     numberList.constEnd(),
                                                     &Number::isEven,
                                                     NumberSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(numberList, &Number::isEven, NumberSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced<int>(QList<Number>(numberList),
                                                              &Number::isEven,
                                                              NumberSumReduce());
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(numberList.begin(),
                                                             numberList.end(),
                                                             &Number::isEven,
                                                             NumberSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(numberList.constBegin(),
                                                             numberList.constEnd(),
                                                             &Number::isEven,
                                                             NumberSumReduce());
        QCOMPARE(sum, 6);
    }

    // member-member
    {
        QList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QList<Number> list2 = QtConcurrent::filteredReduced(numbers,
                                                            &Number::isEven,
                                                            &QList<Number>::push_back, QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QList<Number> list2 = QtConcurrent::filteredReduced(numbers.begin(),
                                                            numbers.end(),
                                                            &Number::isEven,
                                                            &QList<Number>::push_back,
                                                            QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QList<Number> list2 = QtConcurrent::filteredReduced(numbers.constBegin(),
                                                            numbers.constEnd(),
                                                            &Number::isEven,
                                                            &QList<Number>::push_back,
                                                            QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QList<Number> list2 = QtConcurrent::blockingFilteredReduced(numbers,
                                                                    &Number::isEven,
                                                                    &QList<Number>::push_back, QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QList<Number> list2 = QtConcurrent::blockingFilteredReduced(numbers.begin(),
                                                                    numbers.end(),
                                                                    &Number::isEven,
                                                                    &QList<Number>::push_back,
                                                                    QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }
    {
        QList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QList<Number> list2 = QtConcurrent::blockingFilteredReduced(numbers.constBegin(),
                                                                    numbers.constEnd(),
                                                                    &Number::isEven,
                                                                    &QList<Number>::push_back,
                                                                    QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<Number>() << 2 << 4);
    }

    // function-member
    {
        QList<int> list2 = QtConcurrent::filteredReduced(list, keepEvenIntegers, &QList<int>::push_back, QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::filteredReduced(list.begin(),
                                                         list.end(),
                                                         keepEvenIntegers,
                                                         &QList<int>::push_back,
                                                         QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::filteredReduced(list.constBegin(),
                                                         list.constEnd(),
                                                         keepEvenIntegers,
                                                         &QList<int>::push_back,
                                                         QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFilteredReduced(list, keepEvenIntegers, &QList<int>::push_back, QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFilteredReduced(list.begin(),
                                                                 list.end(),
                                                                 keepEvenIntegers,
                                                                 &QList<int>::push_back,
                                                                 QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }
    {
        QList<int> list2 = QtConcurrent::blockingFilteredReduced(list.constBegin(),
                                                                 list.constEnd(),
                                                                 keepEvenIntegers,
                                                                 &QList<int>::push_back,
                                                                 QtConcurrent::OrderedReduce);
        QCOMPARE(list2, QList<int>() << 2 << 4);
    }

    // member-function
    {
        int sum = QtConcurrent::filteredReduced(numberList, &Number::isEven, numberSumReduce);
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced(QList<Number>(numberList),
                                                 &Number::isEven,
                                                 numberSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(numberList.begin(),
                                                numberList.end(),
                                                &Number::isEven,
                                                numberSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(numberList.constBegin(),
                                                numberList.constEnd(),
                                                &Number::isEven,
                                                numberSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(numberList, &Number::isEven, numberSumReduce);
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced(QList<Number>(numberList),
                                                         &Number::isEven,
                                                         numberSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(numberList.begin(),
                                                        numberList.end(),
                                                        &Number::isEven,
                                                        numberSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(numberList.constBegin(),
                                                        numberList.constEnd(),
                                                        &Number::isEven,
                                                        numberSumReduce);
        QCOMPARE(sum, 6);
    }

    // same as above on linked lists
    QLinkedList<int> linkedList;
    linkedList << 1 << 2 << 3 << 4;
    QLinkedList<Number> numberLinkedList;
    numberLinkedList << 1 << 2 << 3 << 4;

    // functor-functor
    {
        int sum = QtConcurrent::filteredReduced<int>(linkedList, KeepEvenIntegers(), IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced<int>(linkedList, keepEvenIntegers, intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(linkedList.begin(),
                                                     linkedList.end(),
                                                     KeepEvenIntegers(),
                                                     IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced<int>(linkedList.begin(),
                                                      linkedList.end(),
                                                      keepEvenIntegers,
                                                      intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(linkedList.constBegin(),
                                                     linkedList.constEnd(),
                                                     KeepEvenIntegers(),
                                                     IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced<int>(linkedList.constBegin(),
                                                      linkedList.constEnd(),
                                                      keepEvenIntegers,
                                                      intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(linkedList, KeepEvenIntegers(), IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced<int>(linkedList, keepEvenIntegers, intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(linkedList.begin(),
                                                             linkedList.end(),
                                                             KeepEvenIntegers(),
                                                             IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced<int>(linkedList.begin(),
                                                              linkedList.end(),
                                                              keepEvenIntegers,
                                                              intSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(linkedList.constBegin(),
                                                             linkedList.constEnd(),
                                                             KeepEvenIntegers(),
                                                             IntSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced<int>(linkedList.constBegin(),
                                                              linkedList.constEnd(),
                                                              keepEvenIntegers,
                                                              intSumReduce);
        QCOMPARE(sum2, 6);
    }

    // function-functor
    {
        int sum = QtConcurrent::filteredReduced<int>(linkedList, keepEvenIntegers, IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(linkedList.begin(),
                                                     linkedList.end(),
                                                     keepEvenIntegers,
                                                     IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(linkedList.constBegin(),
                                                     linkedList.constEnd(),
                                                     keepEvenIntegers,
                                                     IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(linkedList, keepEvenIntegers, IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(linkedList.begin(),
                                                             linkedList.end(),
                                                             keepEvenIntegers,
                                                             IntSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(linkedList.constBegin(),
                                                             linkedList.constEnd(),
                                                             keepEvenIntegers,
                                                             IntSumReduce());
        QCOMPARE(sum, 6);
    }

    // functor-function
    {
        int sum = QtConcurrent::filteredReduced(linkedList, KeepEvenIntegers(), intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(linkedList.begin(),
                                                linkedList.end(),
                                                KeepEvenIntegers(),
                                                intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(linkedList.constBegin(),
                                                linkedList.constEnd(),
                                                KeepEvenIntegers(),
                                                intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(linkedList, KeepEvenIntegers(), intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(linkedList.begin(),
                                                        linkedList.end(),
                                                        KeepEvenIntegers(),
                                                        intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(linkedList.constBegin(),
                                                        linkedList.constEnd(),
                                                        KeepEvenIntegers(),
                                                        intSumReduce);
        QCOMPARE(sum, 6);
    }

    // function-function
    {
        int sum = QtConcurrent::filteredReduced(linkedList, keepEvenIntegers, intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(linkedList.begin(),
                                                linkedList.end(),
                                                keepEvenIntegers,
                                                intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(linkedList.constBegin(),
                                                linkedList.constEnd(),
                                                keepEvenIntegers,
                                                intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(linkedList, keepEvenIntegers, intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(linkedList.begin(),
                                                        linkedList.end(),
                                                        keepEvenIntegers,
                                                        intSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(linkedList.constBegin(),
                                                        linkedList.constEnd(),
                                                        keepEvenIntegers,
                                                        intSumReduce);
        QCOMPARE(sum, 6);
    }

    // functor-member
    {
        QLinkedList<int> linkedList2 = QtConcurrent::filteredReduced(linkedList, KeepEvenIntegers(), &QLinkedList<int>::append, QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::filteredReduced(linkedList.begin(),
                                                                     linkedList.end(),
                                                                     KeepEvenIntegers(),
                                                                     &QLinkedList<int>::append,
                                                                     QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::filteredReduced(linkedList.constBegin(),
                                                                     linkedList.constEnd(),
                                                                     KeepEvenIntegers(),
                                                                     &QLinkedList<int>::append,
                                                                     QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFilteredReduced(linkedList, KeepEvenIntegers(), &QLinkedList<int>::append, QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFilteredReduced(linkedList.begin(),
                                                                             linkedList.end(),
                                                                             KeepEvenIntegers(),
                                                                             &QLinkedList<int>::append,
                                                                             QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFilteredReduced(linkedList.constBegin(),
                                                                             linkedList.constEnd(),
                                                                             KeepEvenIntegers(),
                                                                             &QLinkedList<int>::append,
                                                                             QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }

    // member-functor
    {
        int sum = QtConcurrent::filteredReduced<int>(numberLinkedList, &Number::isEven, NumberSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced<int>(QLinkedList<Number>(numberLinkedList),
                                                      &Number::isEven,
                                                      NumberSumReduce());
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(numberLinkedList.begin(),
                                                     numberLinkedList.end(),
                                                     &Number::isEven,
                                                     NumberSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced<int>(numberLinkedList.constBegin(),
                                                     numberLinkedList.constEnd(),
                                                     &Number::isEven,
                                                     NumberSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(numberLinkedList, &Number::isEven, NumberSumReduce());
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced<int>(QLinkedList<Number>(numberLinkedList),
                                                              &Number::isEven,
                                                              NumberSumReduce());
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(numberLinkedList.begin(),
                                                             numberLinkedList.end(),
                                                             &Number::isEven,
                                                             NumberSumReduce());
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced<int>(numberLinkedList.constBegin(),
                                                             numberLinkedList.constEnd(),
                                                             &Number::isEven,
                                                             NumberSumReduce());
        QCOMPARE(sum, 6);
    }

    // member-member
    {
        QLinkedList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QLinkedList<Number> linkedList2 = QtConcurrent::filteredReduced(numbers,
                                                                        &Number::isEven,
                                                                        &QLinkedList<Number>::append, QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QLinkedList<Number> linkedList2 = QtConcurrent::filteredReduced(numbers.begin(),
                                                                        numbers.end(),
                                                                        &Number::isEven,
                                                                        &QLinkedList<Number>::append,
                                                                        QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QLinkedList<Number> linkedList2 = QtConcurrent::filteredReduced(numbers.constBegin(),
                                                                        numbers.constEnd(),
                                                                        &Number::isEven,
                                                                        &QLinkedList<Number>::append,
                                                                        QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QLinkedList<Number> linkedList2 = QtConcurrent::blockingFilteredReduced(numbers,
                                                                                &Number::isEven,
                                                                                &QLinkedList<Number>::append, QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QLinkedList<Number> linkedList2 = QtConcurrent::blockingFilteredReduced(numbers.begin(),
                                                                                numbers.end(),
                                                                                &Number::isEven,
                                                                                &QLinkedList<Number>::append,
                                                                                QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<Number>() << 2 << 4);
    }
    {
        QLinkedList<Number> numbers;
        numbers << 1 << 2 << 3 << 4;
        QLinkedList<Number> linkedList2 = QtConcurrent::blockingFilteredReduced(numbers.constBegin(),
                                                                                numbers.constEnd(),
                                                                                &Number::isEven,
                                                                                &QLinkedList<Number>::append,
                                                                                QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<Number>() << 2 << 4);
    }

    // function-member
    {
        QLinkedList<int> linkedList2 = QtConcurrent::filteredReduced(linkedList, keepEvenIntegers, &QLinkedList<int>::append, QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::filteredReduced(linkedList.begin(),
                                                                     linkedList.end(),
                                                                     keepEvenIntegers,
                                                                     &QLinkedList<int>::append,
                                                                     QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::filteredReduced(linkedList.constBegin(),
                                                                     linkedList.constEnd(),
                                                                     keepEvenIntegers,
                                                                     &QLinkedList<int>::append,
                                                                     QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFilteredReduced(linkedList, keepEvenIntegers, &QLinkedList<int>::append, QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFilteredReduced(linkedList.begin(),
                                                                             linkedList.end(),
                                                                             keepEvenIntegers,
                                                                             &QLinkedList<int>::append,
                                                                             QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }
    {
        QLinkedList<int> linkedList2 = QtConcurrent::blockingFilteredReduced(linkedList.constBegin(),
                                                                             linkedList.constEnd(),
                                                                             keepEvenIntegers,
                                                                             &QLinkedList<int>::append,
                                                                             QtConcurrent::OrderedReduce);
        QCOMPARE(linkedList2, QLinkedList<int>() << 2 << 4);
    }

    // member-function
    {
        int sum = QtConcurrent::filteredReduced(numberLinkedList, &Number::isEven, numberSumReduce);
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::filteredReduced(QLinkedList<Number>(numberLinkedList),
                                                 &Number::isEven,
                                                 numberSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(numberLinkedList.begin(),
                                                numberLinkedList.end(),
                                                &Number::isEven,
                                                numberSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::filteredReduced(numberLinkedList.constBegin(),
                                                numberLinkedList.constEnd(),
                                                &Number::isEven,
                                                numberSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(numberLinkedList, &Number::isEven, numberSumReduce);
        QCOMPARE(sum, 6);

        int sum2 = QtConcurrent::blockingFilteredReduced(QLinkedList<Number>(numberLinkedList),
                                                         &Number::isEven,
                                                         numberSumReduce);
        QCOMPARE(sum2, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(numberLinkedList.begin(),
                                                        numberLinkedList.end(),
                                                        &Number::isEven,
                                                        numberSumReduce);
        QCOMPARE(sum, 6);
    }
    {
        int sum = QtConcurrent::blockingFilteredReduced(numberLinkedList.constBegin(),
                                                        numberLinkedList.constEnd(),
                                                        &Number::isEven,
                                                        numberSumReduce);
        QCOMPARE(sum, 6);
    }

    // ### the same as above, with an initial result value
}

bool filterfn(int i)
{
    return (i % 2);
}

void tst_QtConcurrentFilter::resultAt()
{

    QList<int> ints;
    for (int i=0; i < 1000; ++i)
        ints << i;

    QFuture<int> future = QtConcurrent::filtered(ints, filterfn);
    future.waitForFinished();

    for (int i = 0; i < future.resultCount(); ++i) {
        QCOMPARE(future.resultAt(i), ints.at(i * 2 + 1));
    }

}

bool waitFilterfn(const int &i)
{
    QTest::qWait(1);
    return (i % 2);
}

void tst_QtConcurrentFilter::incrementalResults()
{
    const int count = 200;
    QList<int> ints;
    for (int i=0; i < count; ++i)
        ints << i;

    QFuture<int> future = QtConcurrent::filtered(ints, waitFilterfn);

    QList<int> results;

    while (future.isFinished() == false) {
        for (int i = 0; i < future.resultCount(); ++i) {
            results += future.resultAt(i);
        }
        QTest::qWait(1);
    }

    QCOMPARE(future.isFinished(), true);
    QCOMPARE(future.resultCount(), count / 2);
    QCOMPARE(future.results().count(), count / 2);
}

void tst_QtConcurrentFilter::noDetach()
{
    {
        QList<int> l = QList<int>() << 1;
        QVERIFY(l.isDetached());

        QList<int> ll = l;
        QVERIFY(!l.isDetached());

        QtConcurrent::filtered(l, waitFilterfn).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::blockingFiltered(l, waitFilterfn);

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::filteredReduced(l, waitFilterfn, intSumReduce).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::filter(l, waitFilterfn).waitForFinished();
        if (!l.isDetached())
            QEXPECT_FAIL("", "QTBUG-20688: Known unstable failure", Abort);
        QVERIFY(l.isDetached());
        QVERIFY(ll.isDetached());
    }
    {
        const QList<int> l = QList<int>() << 1;
        QVERIFY(l.isDetached());

        const QList<int> ll = l;
        QVERIFY(!l.isDetached());

        QtConcurrent::filtered(l, waitFilterfn).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::filteredReduced(l, waitFilterfn, intSumReduce).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());
    }
}

void tst_QtConcurrentFilter::stlContainers()
{
    std::vector<int> vector;
    vector.push_back(1);
    vector.push_back(2);

    std::vector<int> vector2 =  QtConcurrent::blockingFiltered(vector, waitFilterfn);
    QCOMPARE(vector2.size(), (std::vector<int>::size_type)(1));
    QCOMPARE(vector2[0], 1);

    std::list<int> list;
    list.push_back(1);
    list.push_back(2);

    std::list<int> list2 =  QtConcurrent::blockingFiltered(list, waitFilterfn);
    QCOMPARE(list2.size(), (std::list<int>::size_type)(1));
    QCOMPARE(*list2.begin(), 1);

    QtConcurrent::filtered(list, waitFilterfn).waitForFinished();
    QtConcurrent::filtered(vector, waitFilterfn).waitForFinished();
    QtConcurrent::filtered(vector.begin(), vector.end(), waitFilterfn).waitForFinished();

    QtConcurrent::blockingFilter(list, waitFilterfn);
    QCOMPARE(list2.size(), (std::list<int>::size_type)(1));
    QCOMPARE(*list2.begin(), 1);
}

QTEST_MAIN(tst_QtConcurrentFilter)
#include "tst_qtconcurrentfilter.moc"
