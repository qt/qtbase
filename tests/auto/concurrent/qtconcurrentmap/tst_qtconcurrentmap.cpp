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
#include <qtconcurrentmap.h>
#include <qexception.h>

#include <qdebug.h>
#include <QThread>
#include <QMutex>

#include <QtTest/QtTest>

#include "functions.h"

class tst_QtConcurrentMap : public QObject
{
    Q_OBJECT
private slots:
    void map();
    void blocking_map();
    void mapped();
    void blocking_mapped();
    void mappedReduced();
    void mappedReducedLambda();
    void blocking_mappedReduced();
    void blocking_mappedReducedLambda();
    void assignResult();
    void functionOverloads();
    void noExceptFunctionOverloads();
#ifndef QT_NO_EXCEPTIONS
    void exceptions();
#endif
    void incrementalResults();
    void noDetach();
    void stlContainers();
    void stlContainersLambda();
    void qFutureAssignmentLeak();
    void stressTest();
    void persistentResultTest();
    void mappedReducedInitialValue();
    void blocking_mappedReducedInitialValue();
public slots:
    void throttling();
};

using namespace QtConcurrent;

void multiplyBy2Immutable(int x)
{
    x *= 2;
}

class MultiplyBy2Immutable
{
public:
    void operator()(int x)
    {
        x *= 2;
    }
};

void multiplyBy2InPlace(int &x)
{
    x *= 2;
}

class MultiplyBy2InPlace
{
public:
    void operator()(int &x)
    {
        x *= 2;
    }
};

Q_DECLARE_METATYPE(QList<Number>);

void tst_QtConcurrentMap::map()
{
    // functors take arguments by reference, modifying the sequence in place
    {
        QList<int> list;
        list << 1 << 2 << 3;

        // functor
        QtConcurrent::map(list, MultiplyBy2InPlace()).waitForFinished();
        QCOMPARE(list, QList<int>() << 2 << 4 << 6);
        QtConcurrent::map(list.begin(), list.end(), MultiplyBy2InPlace()).waitForFinished();
        QCOMPARE(list, QList<int>() << 4 << 8 << 12);

        // function
        QtConcurrent::map(list, multiplyBy2InPlace).waitForFinished();
        QCOMPARE(list, QList<int>() << 8 << 16 << 24);
        QtConcurrent::map(list.begin(), list.end(), multiplyBy2InPlace).waitForFinished();
        QCOMPARE(list, QList<int>() << 16 << 32 << 48);

        // bound function
        QtConcurrent::map(list, multiplyBy2InPlace).waitForFinished();
        QCOMPARE(list, QList<int>() << 32 << 64 << 96);
        QtConcurrent::map(list.begin(), list.end(), multiplyBy2InPlace).waitForFinished();
        QCOMPARE(list, QList<int>() << 64 << 128 << 192);

        // member function
        QList<Number> numberList;
        numberList << 1 << 2 << 3;
        QtConcurrent::map(numberList, &Number::multiplyBy2).waitForFinished();
        QCOMPARE(numberList, QList<Number>() << 2 << 4 << 6);
        QtConcurrent::map(numberList.begin(), numberList.end(), &Number::multiplyBy2).waitForFinished();
        QCOMPARE(numberList, QList<Number>() << 4 << 8 << 12);

        // lambda
        QtConcurrent::map(list, [](int &x){x *= 2;}).waitForFinished();
        QCOMPARE(list, QList<int>() << 128 << 256 << 384);
        QtConcurrent::map(list.begin(), list.end(), [](int &x){x *= 2;}).waitForFinished();
        QCOMPARE(list, QList<int>() << 256 << 512 << 768);
    }

    // functors don't take arguments by reference, making these no-ops
    {
        QList<int> list;
        list << 1 << 2 << 3;

        // functor
        QtConcurrent::map(list, MultiplyBy2Immutable()).waitForFinished();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QtConcurrent::map(list.begin(), list.end(), MultiplyBy2Immutable()).waitForFinished();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        // function
        QtConcurrent::map(list, multiplyBy2Immutable).waitForFinished();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QtConcurrent::map(list.begin(), list.end(), multiplyBy2Immutable).waitForFinished();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        // bound function
        QtConcurrent::map(list, multiplyBy2Immutable).waitForFinished();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QtConcurrent::map(list.begin(), list.end(), multiplyBy2Immutable).waitForFinished();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        // lambda
        QtConcurrent::map(list, [](int x){x *= 2;}).waitForFinished();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QtConcurrent::map(list.begin(), list.end(), [](int x){x *= 2;}).waitForFinished();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
    }

#if 0
    // not allowed: map() with immutable sequences makes no sense
    {
        const QList<int> list = QList<int>() << 1 << 2 << 3;

        QtConcurrent::map(list, MultiplyBy2Immutable());
        QtConcurrent::map(list, multiplyBy2Immutable);
        QtConcurrent::map(list, multiplyBy2Immutable);
    }
#endif

#if 0
    // not allowed: in place modification of a temp copy (since temp copy goes out of scope)
    {
        QList<int> list;
        list << 1 << 2 << 3;

        QtConcurrent::map(QList<int>(list), MultiplyBy2InPlace());
        QtConcurrent::map(QList<int>(list), multiplyBy2);
        QtConcurrent::map(QList<int>(list), multiplyBy2InPlace);

        QList<Number> numberList;
        numberList << 1 << 2 << 3;
        QtConcurrent::map(QList<Number>(numberList), &Number::multiplyBy2);
    }
#endif

#if 0
    // not allowed: map() on a const list, where functors try to modify the items in the list
    {
        const QList<int> list = QList<int>() << 1 << 2 << 3;;

        QtConcurrent::map(list, MultiplyBy2InPlace());
        QtConcurrent::map(list, multiplyBy2InPlace);
        QtConcurrent::map(list, multiplyBy2InPlace);

        const QList<Number> numberList = QList<Number>() << 1 << 2 << 3;
        QtConcurrent::map(numberList, &Number::multiplyBy2);
    }
#endif
}

void tst_QtConcurrentMap::blocking_map()
{
    // functors take arguments by reference, modifying the sequence in place
    {
        QList<int> list;
        list << 1 << 2 << 3;

        // functor
        QtConcurrent::blockingMap(list, MultiplyBy2InPlace());
        QCOMPARE(list, QList<int>() << 2 << 4 << 6);
        QtConcurrent::blockingMap(list.begin(), list.end(), MultiplyBy2InPlace());
        QCOMPARE(list, QList<int>() << 4 << 8 << 12);

        // function
        QtConcurrent::blockingMap(list, multiplyBy2InPlace);
        QCOMPARE(list, QList<int>() << 8 << 16 << 24);
        QtConcurrent::blockingMap(list.begin(), list.end(), multiplyBy2InPlace);
        QCOMPARE(list, QList<int>() << 16 << 32 << 48);

        // bound function
        QtConcurrent::blockingMap(list, multiplyBy2InPlace);
        QCOMPARE(list, QList<int>() << 32 << 64 << 96);
        QtConcurrent::blockingMap(list.begin(), list.end(), multiplyBy2InPlace);
        QCOMPARE(list, QList<int>() << 64 << 128 << 192);

        // member function
        QList<Number> numberList;
        numberList << 1 << 2 << 3;
        QtConcurrent::blockingMap(numberList, &Number::multiplyBy2);
        QCOMPARE(numberList, QList<Number>() << 2 << 4 << 6);
        QtConcurrent::blockingMap(numberList.begin(), numberList.end(), &Number::multiplyBy2);
        QCOMPARE(numberList, QList<Number>() << 4 << 8 << 12);

        // lambda
        QtConcurrent::blockingMap(list, [](int &x) { x *= 2; });
        QCOMPARE(list, QList<int>() << 128 << 256 << 384);
        QtConcurrent::blockingMap(list.begin(), list.end(), [](int &x) { x *= 2; });
        QCOMPARE(list, QList<int>() << 256 << 512 << 768);
    }

    // functors don't take arguments by reference, making these no-ops
    {
        QList<int> list;
        list << 1 << 2 << 3;

        // functor
        QtConcurrent::blockingMap(list, MultiplyBy2Immutable());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QtConcurrent::blockingMap(list.begin(), list.end(), MultiplyBy2Immutable());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        // function
        QtConcurrent::blockingMap(list, multiplyBy2Immutable);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QtConcurrent::blockingMap(list.begin(), list.end(), multiplyBy2Immutable);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        // bound function
        QtConcurrent::blockingMap(list, multiplyBy2Immutable);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QtConcurrent::blockingMap(list.begin(), list.end(), multiplyBy2Immutable);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        // lambda
        QtConcurrent::blockingMap(list, [](int x) { x *= 2; });
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QtConcurrent::blockingMap(list.begin(), list.end(), [](int x) { x *= 2; });
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
    }

#if 0
    // not allowed: map() with immutable sequences makes no sense
    {
        const QList<int> list = QList<int>() << 1 << 2 << 3;

        QtConcurrent::blockingMap(list, MultiplyBy2Immutable());
        QtConcurrent::blockkng::map(list, multiplyBy2Immutable);
        QtConcurrent::blockingMap(list, multiplyBy2Immutable);
    }
#endif

#if 0
    // not allowed: in place modification of a temp copy (since temp copy goes out of scope)
    {
        QList<int> list;
        list << 1 << 2 << 3;

        QtConcurrent::blockingMap(QList<int>(list), MultiplyBy2InPlace());
        QtConcurrent::blockingMap(QList<int>(list), multiplyBy2);
        QtConcurrent::blockingMap(QList<int>(list), multiplyBy2InPlace);

        QList<Number> numberList;
        numberList << 1 << 2 << 3;
        QtConcurrent::blockingMap(QList<Number>(numberList), &Number::multiplyBy2);
    }
#endif

#if 0
    // not allowed: map() on a const list, where functors try to modify the items in the list
    {
        const QList<int> list = QList<int>() << 1 << 2 << 3;;

        QtConcurrent::blockingMap(list, MultiplyBy2InPlace());
        QtConcurrent::blockingMap(list, multiplyBy2InPlace);
        QtConcurrent::blockingMap(list, multiplyBy2InPlace);

        const QList<Number> numberList = QList<Number>() << 1 << 2 << 3;
        QtConcurrent::blockingMap(numberList, &Number::multiplyBy2);
    }
#endif
}

int multiplyBy2(int x)
{
    int y = x * 2;
    return y;
}

class MultiplyBy2
{
public:
    int operator()(int x) const
    {
        int y = x * 2;
        return y;
    }
};

double intToDouble(int x)
{
    return double(x);
}

class IntToDouble
{
public:
    double operator()(int x) const
    {
        return double(x);
    }
};

int stringToInt(const QString &string)
{
    return string.toInt();
}

class StringToInt
{
public:
    int operator()(const QString &string) const
    {
        return string.toInt();
    }
};

void tst_QtConcurrentMap::mapped()
{
    QList<int> list;
    list << 1 << 2 << 3;
    QList<Number> numberList;
    numberList << 1 << 2 << 3;

    // functor
    {
        QList<int> list2 = QtConcurrent::mapped(list, MultiplyBy2()).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 2 << 4 << 6);

        QList<int> list3 = QtConcurrent::mapped(list.constBegin(),
                                                list.constEnd(),
                                                MultiplyBy2()).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 2 << 4 << 6);

        QList<int> list4 = QtConcurrent::mapped(QList<int>(list), MultiplyBy2()).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 2 << 4 << 6);
    }

    // function
    {
        QList<int> list2 = QtConcurrent::mapped(list, multiplyBy2).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 2 << 4 << 6);

        QList<int> list3 = QtConcurrent::mapped(list.constBegin(),
                                                list.constEnd(),
                                                multiplyBy2).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 2 << 4 << 6);

        QList<int> list4 = QtConcurrent::mapped(QList<int>(list), multiplyBy2).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 2 << 4 << 6);
    }

    // bound function
    {
        QList<int> list2 = QtConcurrent::mapped(list, multiplyBy2).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 2 << 4 << 6);

        QList<int> list3 = QtConcurrent::mapped(list.constBegin(),
                                                list.constEnd(),
                                                multiplyBy2).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 2 << 4 << 6);

        QList<int> list4 = QtConcurrent::mapped(QList<int>(list), multiplyBy2).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 2 << 4 << 6);
    }

    // const member function
    {
        QList<Number> numberList2 = QtConcurrent::mapped(numberList, &Number::multipliedBy2)
                                    .results();
        QCOMPARE(numberList, QList<Number>() << 1 << 2 << 3);
        QCOMPARE(numberList2, QList<Number>() << 2 << 4 << 6);

        QList<Number> numberList3 = QtConcurrent::mapped(numberList.constBegin(),
                                                         numberList.constEnd(),
                                                         &Number::multipliedBy2)
                                    .results();
        QCOMPARE(numberList, QList<Number>() << 1 << 2 << 3);
        QCOMPARE(numberList3, QList<Number>() << 2 << 4 << 6);

        QList<Number> numberList4 = QtConcurrent::mapped(QList<Number>(numberList),
                                                         &Number::multipliedBy2)
                                    .results();
        QCOMPARE(numberList, QList<Number>() << 1 << 2 << 3);
        QCOMPARE(numberList4, QList<Number>() << 2 << 4 << 6);
    }

    {
        QList<Number> numberList2 = QtConcurrent::mapped(numberList,
            [](const Number &num) {
                return num.multipliedBy2();
            }
        ).results();
        QCOMPARE(numberList, QList<Number>() << 1 << 2 << 3);
        QCOMPARE(numberList2, QList<Number>() << 2 << 4 << 6);

        QList<Number> numberList3 = QtConcurrent::mapped(numberList.constBegin(), numberList.constEnd(),
            [](const Number &num) {
                return num.multipliedBy2();
            }
        ).results();
        QCOMPARE(numberList, QList<Number>() << 1 << 2 << 3);
        QCOMPARE(numberList3, QList<Number>() << 2 << 4 << 6);

        QList<Number> numberList4 = QtConcurrent::mapped(QList<Number>(numberList),
            [](const Number &num) {
                return num.multipliedBy2();
            }
        ).results();
        QCOMPARE(numberList, QList<Number>() << 1 << 2 << 3);
        QCOMPARE(numberList4, QList<Number>() << 2 << 4 << 6);
    }

    // change the value_type, same container

    // functor
    {
        QList<double> list2 = QtConcurrent::mapped(list, IntToDouble()).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list3 = QtConcurrent::mapped(list.constBegin(),
                                                   list.constEnd(),
                                                   IntToDouble())
                              .results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list4 = QtConcurrent::mapped(QList<int>(list),
                                                   IntToDouble())
                              .results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<double>() << 1.0 << 2.0 << 3.0);
    }

    // function
    {
        QList<double> list2 = QtConcurrent::mapped(list, intToDouble).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list3 = QtConcurrent::mapped(list.constBegin(),
                                                   list.constEnd(),
                                                   intToDouble)
                              .results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list4 = QtConcurrent::mapped(QList<int>(list), intToDouble).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<double>() << 1.0 << 2.0 << 3.0);
    }

    // bound function
    {
        QList<double> list2 = QtConcurrent::mapped(list, intToDouble).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list3 = QtConcurrent::mapped(list.constBegin(),
                                                   list.constEnd(),
                                                   intToDouble)
                              .results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<double>() << 1.0 << 2.0 << 3.0);


        QList<double> list4 = QtConcurrent::mapped(QList<int>(list),
                                                   intToDouble)
                              .results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<double>() << 1.0 << 2.0 << 3.0);
    }

    // const member function
    {
        QList<QString> list2 = QtConcurrent::mapped(numberList, &Number::toString).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<QString>() << "1" << "2" << "3");

        QList<QString> list3 = QtConcurrent::mapped(numberList.constBegin(),
                                                    numberList.constEnd(),
                                                    &Number::toString)
                               .results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<QString>() << "1" << "2" << "3");

        QList<QString> list4 = QtConcurrent::mapped(QList<Number>(numberList), &Number::toString)
                               .results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<QString>() << "1" << "2" << "3");
    }

    // lambda
    {
        QList<double> list2 = QtConcurrent::mapped(list,
            [](int x) {
                return double(x);
            }
        ).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list3 = QtConcurrent::mapped(list.constBegin(), list.constEnd(),
            [](int x) {
                return double(x);
            }
        ).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list4 = QtConcurrent::mapped(QList<int>(list),
            [](int x) {
                return double(x);
            }
        ).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<double>() << 1.0 << 2.0 << 3.0);
    }

    // change the value_type
    {
        QList<QString> strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::mapped(strings, StringToInt()).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::mapped(strings.constBegin(),
                                                strings.constEnd(),
                                                StringToInt())
                           .results();
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }
    {
        QList<QString> strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::mapped(strings, stringToInt).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::mapped(strings.constBegin(),
                                                strings.constEnd(),
                                                stringToInt).results();
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }

    {
        QList<int> numberList2 = QtConcurrent::mapped(numberList, &Number::toInt).results();
        QCOMPARE(numberList2, QList<int>() << 1 << 2 << 3);

        QList<int> numberList3 = QtConcurrent::mapped(numberList.constBegin(),
                                                      numberList.constEnd(),
                                                      &Number::toInt)
                                 .results();
        QCOMPARE(numberList3, QList<int>() << 1 << 2 << 3);
    }

    {
        QList<int> numberList2 = QtConcurrent::mapped(numberList,
            [] (const Number number) {
                return number.toInt();
            }
        ).results();
        QCOMPARE(numberList2, QList<int>() << 1 << 2 << 3);

        QList<int> numberList3 = QtConcurrent::mapped(numberList.constBegin(), numberList.constEnd(),
            [](const Number number) {
                return number.toInt();
            }
        ).results();
        QCOMPARE(numberList3, QList<int>() << 1 << 2 << 3);
    }

    // change the value_type from QStringList
    {
        QStringList strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::mapped(strings, StringToInt()).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::mapped(strings.constBegin(),
                                                strings.constEnd(),
                                                StringToInt())
                           .results();
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }
    {
        QStringList strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::mapped(strings, stringToInt).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::mapped(strings.constBegin(),
                                                strings.constEnd(),
                                                stringToInt)
                           .results();
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }
    {
        QStringList strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::mapped(strings,
            [](const QString &string) {
                return string.toInt();
            }
        ).results();
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::mapped(strings.constBegin(), strings.constEnd(),
            [](const QString &string) {
                return string.toInt();
            }
        ).results();
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }
}

void tst_QtConcurrentMap::blocking_mapped()
{
    QList<int> list;
    list << 1 << 2 << 3;
    QList<Number> numberList;
    numberList << 1 << 2 << 3;

    // functor
    {
        QList<int> list2 = QtConcurrent::blockingMapped(list, MultiplyBy2());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 2 << 4 << 6);

        QList<int> list3 = QtConcurrent::blockingMapped<QList<int> >(list.constBegin(),
                                                                       list.constEnd(),
                                                                       MultiplyBy2());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 2 << 4 << 6);

        QList<int> list4 = QtConcurrent::blockingMapped(QList<int>(list), MultiplyBy2());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 2 << 4 << 6);
    }

    // function
    {
        QList<int> list2 = QtConcurrent::blockingMapped(list, multiplyBy2);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 2 << 4 << 6);

        QList<int> list3 = QtConcurrent::blockingMapped<QList<int> >(list.constBegin(),
                                                             list.constEnd(),
                                                             multiplyBy2);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 2 << 4 << 6);

        QList<int> list4 = QtConcurrent::blockingMapped(QList<int>(list), multiplyBy2);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 2 << 4 << 6);
    }

    // bound function
    {
        QList<int> list2 = QtConcurrent::blockingMapped(list, multiplyBy2);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 2 << 4 << 6);

        QList<int> list3 = QtConcurrent::blockingMapped<QList<int> >(list.constBegin(),
                                                             list.constEnd(),
                                                             multiplyBy2);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 2 << 4 << 6);

        QList<int> list4 = QtConcurrent::blockingMapped(QList<int>(list), multiplyBy2);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 2 << 4 << 6);
    }

    // const member function
    {
        QList<Number> numberList2 = QtConcurrent::blockingMapped(numberList, &Number::multipliedBy2);
        QCOMPARE(numberList, QList<Number>() << 1 << 2 << 3);
        QCOMPARE(numberList2, QList<Number>() << 2 << 4 << 6);

        QList<Number> numberList3 = QtConcurrent::blockingMapped<QList<Number> >(numberList.constBegin(),
                                                                         numberList.constEnd(),
                                                                         &Number::multipliedBy2);
        QCOMPARE(numberList, QList<Number>() << 1 << 2 << 3);
        QCOMPARE(numberList3, QList<Number>() << 2 << 4 << 6);

        QList<Number> numberList4 = QtConcurrent::blockingMapped(QList<Number>(numberList),
                                                         &Number::multipliedBy2);
        QCOMPARE(numberList, QList<Number>() << 1 << 2 << 3);
        QCOMPARE(numberList4, QList<Number>() << 2 << 4 << 6);
    }

    // lambda
    {
        QList<int> list2 = QtConcurrent::blockingMapped(list,
            [](int x) {
                return x * 2;
            }
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 2 << 4 << 6);

        QList<int> list3 = QtConcurrent::blockingMapped<QList<int> >(list.constBegin(), list.constEnd(),
            [](int x) {
                return x * 2;
            }
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 2 << 4 << 6);

        QList<int> list4 = QtConcurrent::blockingMapped(QList<int>(list),
            [](int x) {
                return x * 2;
            }
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 2 << 4 << 6);
    }

    // change the value_type, same container

    // functor
    {
        QList<double> list2 = QtConcurrent::blockingMapped<QList<double> >(list, IntToDouble());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list3 = QtConcurrent::blockingMapped<QList<double> >(list.constBegin(),
                                                                   list.constEnd(),
                                                                   IntToDouble());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list4 = QtConcurrent::blockingMapped<QList<double> >(QList<int>(list),
                                                                   IntToDouble());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<double>() << 1.0 << 2.0 << 3.0);
    }

    // function
    {
        QList<double> list2 = QtConcurrent::blockingMapped<QList<double> >(list, intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list3 = QtConcurrent::blockingMapped<QList<double> >(list.constBegin(),
                                                                   list.constEnd(),
                                                                   intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list4 = QtConcurrent::blockingMapped<QList<double> >(QList<int>(list), intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<double>() << 1.0 << 2.0 << 3.0);
    }

    // bound function
    {
        QList<double> list2 = QtConcurrent::blockingMapped<QList<double> >(list, intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<double>() << 1.0 << 2.0 << 3.0);

        QList<double> list3 = QtConcurrent::blockingMapped<QList<double> >(list.constBegin(),
                                                                   list.constEnd(),
                                                                   intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<double>() << 1.0 << 2.0 << 3.0);


        QList<double> list4 = QtConcurrent::blockingMapped<QList<double> >(QList<int>(list),
                                                                   intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<double>() << 1.0 << 2.0 << 3.0);
    }

    // const member function
    {
        QList<QString> list2 =
            QtConcurrent::blockingMapped<QList<QString> >(numberList, &Number::toString);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<QString>() << "1" << "2" << "3");

        QList<QString> list3 = QtConcurrent::blockingMapped<QList<QString> >(numberList.constBegin(),
                                                                     numberList.constEnd()
                                                                     , &Number::toString);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<QString>() << "1" << "2" << "3");

        QList<QString> list4 =
            QtConcurrent::blockingMapped<QList<QString> >(QList<Number>(numberList), &Number::toString);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<QString>() << "1" << "2" << "3");
    }

    // lambda
    {
        QList<QString> list2 = QtConcurrent::blockingMapped<QList<QString> >(numberList,
            [] (const Number &number) {
                return number.toString();
            }
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<QString>() << "1" << "2" << "3");

        QList<QString> list3 = QtConcurrent::blockingMapped<QList<QString> >(numberList.constBegin(),
            numberList.constEnd(),
            [](const Number &number) {
                return number.toString();
            }
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<QString>() << "1" << "2" << "3");

        QList<QString> list4 = QtConcurrent::blockingMapped<QList<QString> >(QList<Number>(numberList),
            [](const Number &number) {
                return number.toString();
            }
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<QString>() << "1" << "2" << "3");
    }

    // change the value_type
    {
        QList<QString> strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::blockingMapped(strings, StringToInt());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::blockingMapped<QList<int> >(strings.constBegin(),
                                                             strings.constEnd(),
                                                             StringToInt());
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }
    {
        QList<QString> strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::blockingMapped(strings, stringToInt);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::blockingMapped<QList<int> >(strings.constBegin(),
                                                             strings.constEnd(),
                                                             stringToInt);
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }

    {
        QList<int> numberList2 = QtConcurrent::blockingMapped(numberList, &Number::toInt);
        QCOMPARE(numberList2, QList<int>() << 1 << 2 << 3);

        QList<int> numberList3 = QtConcurrent::blockingMapped<QList<int> >(numberList.constBegin(),
                                                                   numberList.constEnd(),
                                                                   &Number::toInt);
        QCOMPARE(numberList3, QList<int>() << 1 << 2 << 3);
    }

    {
        QList<int> numberList2 = QtConcurrent::blockingMapped(numberList,
            [] (const Number &number) {
                return number.toInt();
            }
        );
        QCOMPARE(numberList2, QList<int>() << 1 << 2 << 3);

        QList<int> numberList3 = QtConcurrent::blockingMapped<QList<int> >(numberList.constBegin(),
            numberList.constEnd(),
            [](const Number &number) {
                return number.toInt();
            }
        );
        QCOMPARE(numberList3, QList<int>() << 1 << 2 << 3);
    }

    // change the value_type from QStringList
    {
        QStringList strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::blockingMapped(strings, StringToInt());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::blockingMapped<QList<int> >(strings.constBegin(),
                                                             strings.constEnd(),
                                                             StringToInt());
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }
    {
        QStringList strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::blockingMapped(strings, stringToInt);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::blockingMapped<QList<int> >(strings.constBegin(),
                                                             strings.constEnd(),
                                                             stringToInt);
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }
    {
        QStringList strings = QStringList() << "1" << "2" << "3";
        QList<int> list = QtConcurrent::blockingMapped(strings,
            [](const QString &string) {
                return string.toInt();
            }
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);

        QList<int> list2 = QtConcurrent::blockingMapped<QList<int> >(strings.constBegin(),
            strings.constEnd(),
            [](const QString &string) {
                return string.toInt();
            }
        );
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);
    }

    // functor
    {
        QVector<double> list2 = QtConcurrent::blockingMapped<QVector<double> >(list, IntToDouble());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QVector<double>() << 1.0 << 2.0 << 3.0);

        QVector<double> list3 = QtConcurrent::blockingMapped<QVector<double> >(list.constBegin(),
                                                                       list.constEnd(),
                                                                       IntToDouble());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QVector<double>() << 1.0 << 2.0 << 3.0);

        QVector<double> list4 = QtConcurrent::blockingMapped<QVector<double> >(QList<int>(list),
                                                                       IntToDouble());
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QVector<double>() << 1.0 << 2.0 << 3.0);

        QStringList strings = QStringList() << "1" << "2" << "3";
        QVector<int> list5 = QtConcurrent::blockingMapped<QVector<int> >(strings, StringToInt());
        QCOMPARE(list5, QVector<int>() << 1 << 2 << 3);

        QVector<int> list6 = QtConcurrent::blockingMapped<QVector<int> >(strings.constBegin(),
                                                                 strings.constEnd(),
                                                                 StringToInt());
        QCOMPARE(list6, QVector<int>() << 1 << 2 << 3);

        QVector<int> list7 = QtConcurrent::blockingMapped<QVector<int> >(QStringList(strings),
                                                                 StringToInt());
        QCOMPARE(list7, QVector<int>() << 1 << 2 << 3);
    }

    // function
    {
        QVector<double> list2 = QtConcurrent::blockingMapped<QVector<double> >(list, intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QVector<double>() << 1.0 << 2.0 << 3.0);

        QVector<double> list3 = QtConcurrent::blockingMapped<QVector<double> >(list.constBegin(),
                                                                       list.constEnd(),
                                                                       intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QVector<double>() << 1.0 << 2.0 << 3.0);

        QVector<double> list4 = QtConcurrent::blockingMapped<QVector<double> >(QList<int>(list),
                                                                       intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QVector<double>() << 1.0 << 2.0 << 3.0);

        QStringList strings = QStringList() << "1" << "2" << "3";
        QVector<int> list5 = QtConcurrent::blockingMapped<QVector<int> >(strings, stringToInt);
        QCOMPARE(list5, QVector<int>() << 1 << 2 << 3);

        QVector<int> list6 = QtConcurrent::blockingMapped<QVector<int> >(strings.constBegin(),
                                                                 strings.constEnd(),
                                                                 stringToInt);
        QCOMPARE(list6, QVector<int>() << 1 << 2 << 3);

        QVector<int> list7 = QtConcurrent::blockingMapped<QVector<int> >(QStringList(strings),
                                                                 stringToInt);
        QCOMPARE(list7, QVector<int>() << 1 << 2 << 3);
    }

    // bound function
    {
        QVector<double> list2 = QtConcurrent::blockingMapped<QVector<double> >(list, intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QVector<double>() << 1.0 << 2.0 << 3.0);

        QVector<double> list3 = QtConcurrent::blockingMapped<QVector<double> >(QList<int>(list), intToDouble);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QVector<double>() << 1.0 << 2.0 << 3.0);

        QStringList strings = QStringList() << "1" << "2" << "3";
        QVector<int> list4 = QtConcurrent::blockingMapped<QVector<int> >(strings, stringToInt);
        QCOMPARE(list4, QVector<int>() << 1 << 2 << 3);

        QVector<int> list5 = QtConcurrent::blockingMapped<QVector<int> >(QStringList(strings), stringToInt);
        QCOMPARE(list5, QVector<int>() << 1 << 2 << 3);
    }

    // const member function
    {
        QVector<QString> list2 = QtConcurrent::blockingMapped<QVector<QString> >(numberList, &Number::toString);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QVector<QString>() << "1" << "2" << "3");

        QVector<QString> list3 =
            QtConcurrent::blockingMapped<QVector<QString> >(QList<Number>(numberList), &Number::toString);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QVector<QString>() << "1" << "2" << "3");

        // not allowed: const member function where all arguments have default values
#if 0
        QStringList strings = QStringList() << "1" << "2" << "3";
        QVector<int> list4 = QtConcurrent::blockingMapped<QVector<int> >(strings, &QString::toInt);
        QCOMPARE(list4, QVector<int>() << 1 << 2 << 3);

        QVector<int> list5 = QtConcurrent::blockingMapped<QVector<int> >(QStringList(strings), &QString::toInt);
        QCOMPARE(list5, QVector<int>() << 1 << 2 << 3);
#endif
    }

    // lambda
    {
        QVector<double> list2 = QtConcurrent::blockingMapped<QVector<double> >(list,
            [](int x) {
                return double(x);
            }
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QVector<double>() << 1.0 << 2.0 << 3.0);

        QVector<double> list3 = QtConcurrent::blockingMapped<QVector<double> >(QList<int>(list),
            [](int x) {
                return double(x);
            }
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QVector<double>() << 1.0 << 2.0 << 3.0);

        QStringList strings = QStringList() << "1" << "2" << "3";
        QVector<int> list4 = QtConcurrent::blockingMapped<QVector<int> >(strings,
            [](const QString &string) {
                return string.toInt();
            }
        );
        QCOMPARE(list4, QVector<int>() << 1 << 2 << 3);

        QVector<int> list5 = QtConcurrent::blockingMapped<QVector<int> >(QStringList(strings),
            [](const QString &string) {
                return string.toInt();
            }
        );
        QCOMPARE(list5, QVector<int>() << 1 << 2 << 3);
    }
}

int intSquare(int x)
{
    return x * x;
}

class IntSquare
{
public:
    int operator()(int x)
    {
        return x * x;
    }
};

void tst_QtConcurrentMap::mappedReduced()
{
    QList<int> list;
    list << 1 << 2 << 3;
    QList<Number> numberList;
    numberList << 1 << 2 << 3;

    // test Q_DECLARE_OPERATORS_FOR_FLAGS
    QtConcurrent::ReduceOptions opt = (QtConcurrent::UnorderedReduce | QtConcurrent::SequentialReduce);
    QVERIFY(opt);

    // functor-functor
    {
        int sum = QtConcurrent::mappedReduced<int>(list, IntSquare(), IntSumReduce());
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::mappedReduced<int>(list.constBegin(),
                                                    list.constEnd(),
                                                    IntSquare(),
                                                    IntSumReduce());
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::mappedReduced<int>(QList<int>(list), IntSquare(), IntSumReduce());
        QCOMPARE(sum3, 14);

        int sum4 = QtConcurrent::mappedReduced<int>(list, intSquare, intSumReduce);
        QCOMPARE(sum4, 14);
        int sum5 = QtConcurrent::mappedReduced<int>(list.constBegin(),
                                                    list.constEnd(),
                                                    intSquare,
                                                    intSumReduce);
        QCOMPARE(sum5, 14);

        int sum6 = QtConcurrent::mappedReduced<int>(QList<int>(list),
                                                    intSquare,
                                                    intSumReduce);
        QCOMPARE(sum6, 14);
    }

    // function-functor
    {
        int sum = QtConcurrent::mappedReduced<int>(list, intSquare, IntSumReduce());
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::mappedReduced<int>(list.constBegin(),
                                                    list.constEnd(),
                                                    intSquare,
                                                    IntSumReduce());
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::mappedReduced<int>(QList<int>(list), intSquare, IntSumReduce());
        QCOMPARE(sum3, 14);
    }

    // functor-function
    {
        int sum = QtConcurrent::mappedReduced(list, IntSquare(), intSumReduce);
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::mappedReduced(list.constBegin(),
                                               list.constEnd(),
                                               IntSquare(),
                                               intSumReduce);
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::mappedReduced(QList<int>(list), IntSquare(), intSumReduce);
        QCOMPARE(sum3, 14);
    }

    // function-function
    {
        int sum = QtConcurrent::mappedReduced(list, intSquare, intSumReduce);
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::mappedReduced(list.constBegin(),
                                               list.constEnd(),
                                               intSquare,
                                               intSumReduce);
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::mappedReduced(QList<int>(list), intSquare, intSumReduce);
        QCOMPARE(sum3, 14);
    }

    auto push_back = static_cast<void (QVector<int>::*)(const int &)>(&QVector<int>::push_back);

    // functor-member
    {
        QList<int> list2 = QtConcurrent::mappedReduced(list,
                                                       IntSquare(),
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::mappedReduced(list.constBegin(),
                                                       list.constEnd(),
                                                       IntSquare(),
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::mappedReduced(QList<int>(list),
                                                       IntSquare(),
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 1 << 4 << 9);
    }

    // member-functor
    {
        int sum = QtConcurrent::mappedReduced<int>(numberList, &Number::toInt, IntSumReduce());
        QCOMPARE(sum, 6);
        int sum2 = QtConcurrent::mappedReduced<int>(numberList.constBegin(),
                                                    numberList.constEnd(),
                                                    &Number::toInt,
                                                    IntSumReduce());
        QCOMPARE(sum2, 6);

        int sum3 = QtConcurrent::mappedReduced<int>(QList<Number>(numberList),
                                                    &Number::toInt,
                                                    IntSumReduce());
        QCOMPARE(sum3, 6);
    }

    // member-member
    {
        QList<int> list2 = QtConcurrent::mappedReduced(numberList,
                                                       &Number::toInt,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);

        QList<int> list3 = QtConcurrent::mappedReduced(numberList.constBegin(),
                                                       numberList.constEnd(),
                                                       &Number::toInt,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list3, QList<int>() << 1 << 2 << 3);

        QList<int> list4 = QtConcurrent::mappedReduced(QList<Number>(numberList),
                                                       &Number::toInt,
                                                       push_back, OrderedReduce);
        QCOMPARE(list4, QList<int>() << 1 << 2 << 3);
    }

    // function-member
    {
        QList<int> list2 = QtConcurrent::mappedReduced(list,
                                                       intSquare,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::mappedReduced(list.constBegin(),
                                                       list.constEnd(),
                                                       intSquare,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::mappedReduced(QList<int>(list),
                                                       intSquare,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 1 << 4 << 9);
    }

    // member-function
    {
        int sum = QtConcurrent::mappedReduced(numberList,
                                              &Number::toInt,
                                              intSumReduce);
        QCOMPARE(sum, 6);
        int sum2 = QtConcurrent::mappedReduced(numberList.constBegin(),
                                               numberList.constEnd(),
                                              &Number::toInt,
                                              intSumReduce);
        QCOMPARE(sum2, 6);

        int sum3 = QtConcurrent::mappedReduced(QList<Number>(numberList),
                                               &Number::toInt,
                                               intSumReduce);
        QCOMPARE(sum3, 6);
    }
}

void tst_QtConcurrentMap::mappedReducedLambda()
{
    QList<int> list;
    list << 1 << 2 << 3;
    QList<Number> numberList;
    numberList << 1 << 2 << 3;

    // lambda-lambda
    {
        int sum = QtConcurrent::mappedReduced<int>(list,
            [](int x) {
                return x * x;
            },
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::mappedReduced<int>(list.constBegin(), list.constEnd(),
            [](int x) {
                return x * x;
            },
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::mappedReduced<int>(QList<int>(list),
            [](int x) {
                return x * x;
            },
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum3, 14);
    }

    // lambda-functor
    {
        int sum = QtConcurrent::mappedReduced<int>(list,
            [](int x) {
                return x * x;
            },
            IntSumReduce()
        );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::mappedReduced<int>(list.constBegin(), list.constEnd(),
            [](int x) {
                return x * x;
            },
            IntSumReduce()
        );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::mappedReduced<int>(QList<int>(list),
            [](int x) {
                return x * x;
            },
            IntSumReduce()
        );
        QCOMPARE(sum3, 14);
    }

    // functor-lambda
    {
        int sum = QtConcurrent::mappedReduced<int>(list,
            IntSquare(),
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::mappedReduced<int>(list.constBegin(), list.constEnd(),
            IntSquare(),
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::mappedReduced<int>(QList<int>(list),
            IntSquare(),
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum3, 14);
    }

    // lambda-function
    {
        int sum = QtConcurrent::mappedReduced<int>(list,
            [](int x) {
                return x * x;
            },
            intSumReduce
            );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::mappedReduced<int>(list.constBegin(), list.constEnd(),
            [](int x) {
                return x * x;
            },
            intSumReduce
            );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::mappedReduced<int>(QList<int>(list),
            [](int x) {
                return x * x;
            },
            intSumReduce
            );
        QCOMPARE(sum3, 14);
    }

    // function-lambda
    {
        int sum = QtConcurrent::mappedReduced<int>(list,
            intSquare,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::mappedReduced<int>(list.constBegin(), list.constEnd(),
            intSquare,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::mappedReduced<int>(QList<int>(list),
            intSquare,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum3, 14);
    }

    // lambda-member
    {
        auto push_back = static_cast<void (QVector<int>::*)(const int &)>(&QVector<int>::push_back);

        QList<int> list2 = QtConcurrent::mappedReduced(list,
            [](int x) {
                return x * x;
            },
            push_back,
            OrderedReduce
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::mappedReduced(list.constBegin(), list.constEnd(),
            [](int x) {
                return x * x;
            },
            push_back,
            OrderedReduce
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::mappedReduced(QList<int>(list),
            [](int x) {
                return x * x;
            },
            push_back,
            OrderedReduce
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 1 << 4 << 9);
    }

    // member-lambda
    {
        int sum = QtConcurrent::mappedReduced<int>(numberList,
            &Number::toInt,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum, 6);
        int sum2 = QtConcurrent::mappedReduced<int>(numberList.constBegin(), numberList.constEnd(),
            &Number::toInt,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum2, 6);

        int sum3 = QtConcurrent::mappedReduced<int>(QList<Number>(numberList),
            &Number::toInt,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum3, 6);
    }
}

void tst_QtConcurrentMap::blocking_mappedReduced()
{
    QList<int> list;
    list << 1 << 2 << 3;
    QList<Number> numberList;
    numberList << 1 << 2 << 3;

    // functor-functor
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(list, IntSquare(), IntSumReduce());
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(list.constBegin(),
                                                    list.constEnd(),
                                                    IntSquare(),
                                                    IntSumReduce());
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(QList<int>(list), IntSquare(), IntSumReduce());
        QCOMPARE(sum3, 14);

        int sum4 = QtConcurrent::blockingMappedReduced<int>(list, intSquare, intSumReduce);
        QCOMPARE(sum4, 14);
        int sum5 = QtConcurrent::blockingMappedReduced<int>(list.constBegin(),
                                                    list.constEnd(),
                                                    intSquare,
                                                    intSumReduce);
        QCOMPARE(sum5, 14);

        int sum6 = QtConcurrent::blockingMappedReduced<int>(QList<int>(list),
                                                    intSquare,
                                                    intSumReduce);
        QCOMPARE(sum6, 14);
    }

    // function-functor
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(list, intSquare, IntSumReduce());
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(list.constBegin(),
                                                    list.constEnd(),
                                                    intSquare,
                                                    IntSumReduce());
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(QList<int>(list), intSquare, IntSumReduce());
        QCOMPARE(sum3, 14);
    }

    // functor-function
    {
        int sum = QtConcurrent::blockingMappedReduced(list, IntSquare(), intSumReduce);
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::blockingMappedReduced(list.constBegin(),
                                               list.constEnd(),
                                               IntSquare(),
                                               intSumReduce);
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::blockingMappedReduced(QList<int>(list), IntSquare(), intSumReduce);
        QCOMPARE(sum3, 14);
    }

    // function-function
    {
        int sum = QtConcurrent::blockingMappedReduced(list, intSquare, intSumReduce);
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::blockingMappedReduced(list.constBegin(),
                                                         list.constEnd(),
                                                         intSquare,
                                                         intSumReduce);
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::blockingMappedReduced(QList<int>(list), intSquare, intSumReduce);
        QCOMPARE(sum3, 14);
    }

    auto push_back = static_cast<void (QVector<int>::*)(const int &)>(&QVector<int>::push_back);

    // functor-member
    {
        QList<int> list2 = QtConcurrent::blockingMappedReduced(list,
                                                       IntSquare(),
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::blockingMappedReduced(list.constBegin(),
                                                       list.constEnd(),
                                                       IntSquare(),
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::blockingMappedReduced(QList<int>(list),
                                                       IntSquare(),
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 1 << 4 << 9);
    }

    // member-functor
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(numberList, &Number::toInt,
                                                             IntSumReduce());
        QCOMPARE(sum, 6);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(numberList.constBegin(),
                                                              numberList.constEnd(),
                                                              &Number::toInt,
                                                              IntSumReduce());
        QCOMPARE(sum2, 6);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(QList<Number>(numberList),
                                                              &Number::toInt,
                                                              IntSumReduce());
        QCOMPARE(sum3, 6);
    }

    // member-member
    {
        QList<int> list2 = QtConcurrent::blockingMappedReduced(numberList,
                                                       &Number::toInt,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list2, QList<int>() << 1 << 2 << 3);

        QList<int> list3 = QtConcurrent::blockingMappedReduced(numberList.constBegin(),
                                                       numberList.constEnd(),
                                                       &Number::toInt,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list3, QList<int>() << 1 << 2 << 3);

        QList<int> list4 = QtConcurrent::blockingMappedReduced(QList<Number>(numberList),
                                                       &Number::toInt,
                                                       push_back, OrderedReduce);
        QCOMPARE(list4, QList<int>() << 1 << 2 << 3);
    }

    // function-member
    {
        QList<int> list2 = QtConcurrent::blockingMappedReduced(list,
                                                       intSquare,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::blockingMappedReduced(list.constBegin(),
                                                       list.constEnd(),
                                                       intSquare,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::blockingMappedReduced(QList<int>(list),
                                                       intSquare,
                                                       push_back,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 1 << 4 << 9);
    }

    // member-function
    {
        int sum = QtConcurrent::blockingMappedReduced(numberList,
                                              &Number::toInt,
                                              intSumReduce);
        QCOMPARE(sum, 6);
        int sum2 = QtConcurrent::blockingMappedReduced(numberList.constBegin(),
                                               numberList.constEnd(),
                                              &Number::toInt,
                                              intSumReduce);
        QCOMPARE(sum2, 6);

        int sum3 = QtConcurrent::blockingMappedReduced(QList<Number>(numberList),
                                               &Number::toInt,
                                               intSumReduce);
        QCOMPARE(sum3, 6);
    }
}

void tst_QtConcurrentMap::blocking_mappedReducedLambda()
{
    QList<int> list;
    list << 1 << 2 << 3;
    QList<Number> numberList;
    numberList << 1 << 2 << 3;

    // lambda-lambda
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(list,
            [](int x) {
                return x * x;
            },
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(list.constBegin(), list.constEnd(),
            [](int x) {
                return x * x;
            },
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(QList<int>(list),
            [](int x) {
                return x * x;
            },
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum3, 14);
    }

    // lambda-functor
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(list,
            [](int x) {
                return x * x;
            },
            IntSumReduce()
        );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(list.constBegin(), list.constEnd(),
            [](int x) {
                return x * x;
            },
            IntSumReduce()
        );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(QList<int>(list),
            [](int x) {
                return x * x;
            },
            IntSumReduce()
        );
        QCOMPARE(sum3, 14);
    }

    // functor-lambda
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(list,
            IntSquare(),
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(list.constBegin(), list.constEnd(),
            IntSquare(),
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(QList<int>(list),
            IntSquare(),
                [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum3, 14);
    }

    // lambda-function
    {
        int sum = QtConcurrent::blockingMappedReduced(list,
            [](int x) {
                return x * x;
            },
            intSumReduce
        );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::blockingMappedReduced(list.constBegin(), list.constEnd(),
            [](int x) {
                return x * x;
            },
            intSumReduce
        );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::blockingMappedReduced(QList<int>(list),
            [](int x) {
                return x * x;
            },
            intSumReduce
        );
        QCOMPARE(sum3, 14);
    }

    // function-lambda
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(list,
            intSquare,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum, 14);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(list.constBegin(), list.constEnd(),
            intSquare,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum2, 14);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(QList<int>(list),
            intSquare,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum3, 14);
    }

    // lambda-member
    {
        auto push_back = static_cast<void (QVector<int>::*)(const int &)>(&QVector<int>::push_back);

        QList<int> list2 = QtConcurrent::blockingMappedReduced(list,
            [](int x) {
                return x * x;
            },
            push_back,
            OrderedReduce
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::blockingMappedReduced(list.constBegin(), list.constEnd(),
            [](int x) {
                return x * x;
            },
            push_back,
            OrderedReduce
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::blockingMappedReduced(QList<int>(list),
            [](int x) {
                return x * x;
            },
            push_back,
            OrderedReduce
        );
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 1 << 4 << 9);
    }

    // member-lambda
    {
        std::function<void(int&, int)> sumRecuce = [](int &sum, int x) {
            sum += x;
        };

        int sum = QtConcurrent::blockingMappedReduced(numberList,
            &Number::toInt,
            sumRecuce
        );
        QCOMPARE(sum, 6);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(numberList.constBegin(), numberList.constEnd(),
            &Number::toInt,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum2, 6);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(QList<Number>(numberList),
            &Number::toInt,
            [](int &sum, int x) {
                sum += x;
            }
        );
        QCOMPARE(sum3, 6);
    }
}

int sleeper(int val)
{
    QTest::qSleep(100);
    return val;
}

void tst_QtConcurrentMap::assignResult()
{
    const QList<int> startList = QList<int>() << 0 << 1 << 2;
    QList<int> list = QtConcurrent::blockingMapped(startList, sleeper);
    QCOMPARE(list.at(0), 0);
    QCOMPARE(list.at(1), 1);
}

int fnConst(const int &i)
{
    return i;
}

int fn(int &i)
{
    return i;
}

int fnConstNoExcept(const int &i) noexcept
{
    return i;
}

int fnNoExcept(int &i) noexcept
{
    return i;
}

QString changeTypeConst(const int &)
{
    return QString();
}

QString changeType(int &)
{
    return QString();
}

QString changeTypeConstNoExcept(const int &) noexcept
{
    return QString();
}

QString changeTypeNoExcept(int &) noexcept
{
    return QString();
}

int changeTypeQStringListConst(const QStringList &)
{
    return 0;
}

int changeTypeQStringList(QStringList &)
{
    return 0;
}

int changeTypeQStringListConstNoExcept(const QStringList &) noexcept
{
    return 0;
}

int changeTypeQStringListNoExcept(QStringList &) noexcept
{
    return 0;
}

class MemFnTester
{
public:
    MemFnTester() {}

    MemFnTester fn()
    {
        return MemFnTester();
    }

    MemFnTester fnConst() const
    {
        return MemFnTester();
    }

    QString changeType()
    {
        return QString();
    }

    QString changeTypeConst() const
    {
        return QString();
    }

    MemFnTester fnNoExcept() noexcept
    {
        return MemFnTester();
    }

    MemFnTester fnConstNoExcept() const noexcept
    {
        return MemFnTester();
    }

    QString changeTypeNoExcept() noexcept
    {
        return QString();
    }

    QString changeTypeConstNoExcept() const noexcept
    {
        return QString();
    }
};

Q_DECLARE_METATYPE(QVector<MemFnTester>);

void tst_QtConcurrentMap::functionOverloads()
{
    QList<int> intList;
    const QList<int> constIntList;
    QList<MemFnTester> classList;
    const QList<MemFnTester> constMemFnTesterList;

    QtConcurrent::mapped(intList, fnConst);
    QtConcurrent::mapped(constIntList, fnConst);
    QtConcurrent::mapped(classList, &MemFnTester::fnConst);
    QtConcurrent::mapped(constMemFnTesterList, &MemFnTester::fnConst);

    QtConcurrent::blockingMapped<QVector<int> >(intList, fnConst);
    QtConcurrent::blockingMapped<QVector<int> >(constIntList, fnConst);
    QtConcurrent::blockingMapped<QVector<MemFnTester> >(classList, &MemFnTester::fnConst);
    QtConcurrent::blockingMapped<QVector<MemFnTester> >(constMemFnTesterList, &MemFnTester::fnConst);

    QtConcurrent::blockingMapped<QList<QString> >(intList, changeTypeConst);
    QtConcurrent::blockingMapped<QList<QString> >(constIntList, changeTypeConst);
    QtConcurrent::blockingMapped<QList<QString> >(classList, &MemFnTester::changeTypeConst);
    QtConcurrent::blockingMapped<QList<QString> >(constMemFnTesterList, &MemFnTester::changeTypeConst);
}

void tst_QtConcurrentMap::noExceptFunctionOverloads()
{
    QList<int> intList;
    const QList<int> constIntList;
    QList<MemFnTester> classList;
    const QList<MemFnTester> constMemFnTesterList;

    QtConcurrent::mapped(intList, fnConstNoExcept);
    QtConcurrent::mapped(constIntList, fnConstNoExcept);
    QtConcurrent::mapped(classList, &MemFnTester::fnConstNoExcept);
    QtConcurrent::mapped(constMemFnTesterList, &MemFnTester::fnConstNoExcept);

    QtConcurrent::blockingMapped<QVector<int> >(intList, fnConstNoExcept);
    QtConcurrent::blockingMapped<QVector<int> >(constIntList, fnConstNoExcept);
    QtConcurrent::blockingMapped<QVector<MemFnTester> >(classList, &MemFnTester::fnConstNoExcept);
    QtConcurrent::blockingMapped<QVector<MemFnTester> >(constMemFnTesterList, &MemFnTester::fnConstNoExcept);

    QtConcurrent::blockingMapped<QList<QString> >(intList, changeTypeConstNoExcept);
    QtConcurrent::blockingMapped<QList<QString> >(constIntList, changeTypeConstNoExcept);
    QtConcurrent::blockingMapped<QList<QString> >(classList, &MemFnTester::changeTypeConstNoExcept);
    QtConcurrent::blockingMapped<QList<QString> >(constMemFnTesterList, &MemFnTester::changeTypeConstNoExcept);
}

QAtomicInt currentInstanceCount;
QAtomicInt peakInstanceCount;
class InstanceCounter
{
public:
    inline InstanceCounter()
    { currentInstanceCount.fetchAndAddRelaxed(1); updatePeak(); }
    inline ~InstanceCounter()
    { currentInstanceCount.fetchAndAddRelaxed(-1);}
    inline InstanceCounter(const InstanceCounter &)
    { currentInstanceCount.fetchAndAddRelaxed(1); updatePeak(); }

    void updatePeak()
    {
        forever {
            const int localPeak = peakInstanceCount.loadRelaxed();
            const int localCurrent = currentInstanceCount.loadRelaxed();
            if (localCurrent <= localPeak)
                break;
            if (peakInstanceCount.testAndSetOrdered(localPeak, localCurrent))
                break;
        }
    }
};

InstanceCounter slowMap(const InstanceCounter &in)
{
    QTest::qSleep(2);
    return in;
}

InstanceCounter fastMap(const InstanceCounter &in)
{
    QTest::qSleep(QRandomGenerator::global()->bounded(2) + 1);
    return in;
}

void slowReduce(int &result, const InstanceCounter&)
{
    QTest::qSleep(QRandomGenerator::global()->bounded(4) + 1);
    ++result;
}

void fastReduce(int &result, const InstanceCounter&)
{
    ++result;
}

void tst_QtConcurrentMap::throttling()
{
    const int itemcount = 100;
    const int allowedTemporaries = QThread::idealThreadCount() * 40;

    {
        currentInstanceCount.storeRelaxed(0);
        peakInstanceCount.storeRelaxed(0);

        QList<InstanceCounter> instances;
        for (int i = 0; i < itemcount; ++i)
            instances.append(InstanceCounter());

        QCOMPARE(currentInstanceCount.loadRelaxed(), itemcount);

        int results = QtConcurrent::blockingMappedReduced(instances, slowMap, fastReduce);
        QCOMPARE(results, itemcount);
        QCOMPARE(currentInstanceCount.loadRelaxed(), itemcount);
        QVERIFY(peakInstanceCount.loadRelaxed() < itemcount + allowedTemporaries);
    }

    {
        QCOMPARE(currentInstanceCount.loadRelaxed(), 0);
        peakInstanceCount.storeRelaxed(0);

        QList<InstanceCounter> instances;
        for (int i = 0; i < itemcount; ++i)
            instances.append(InstanceCounter());

        QCOMPARE(currentInstanceCount.loadRelaxed(), itemcount);
        int results = QtConcurrent::blockingMappedReduced(instances, fastMap, slowReduce);

        QCOMPARE(results, itemcount);
        QCOMPARE(currentInstanceCount.loadRelaxed(), itemcount);
        QVERIFY(peakInstanceCount.loadRelaxed() < itemcount + allowedTemporaries);
    }
}

#ifndef QT_NO_EXCEPTIONS
void throwMapper(int &e)
{
    Q_UNUSED(e);
    throw QException();
}

void tst_QtConcurrentMap::exceptions()
{
    bool caught = false;
    try  {
        QList<int> list = QList<int>() << 1 << 2 << 3;
        QtConcurrent::map(list, throwMapper).waitForFinished();
    } catch (const QException &) {
        caught = true;
    }
    if (!caught)
        QFAIL("did not get exception");
}
#endif

int mapper(const int &i)
{
    QTest::qWait(1);
    return i;
}

void tst_QtConcurrentMap::incrementalResults()
{
    const int count = 200;
    QList<int> ints;
    for (int i=0; i < count; ++i)
        ints << i;

    QFuture<int> future = QtConcurrent::mapped(ints, mapper);

    QList<int> results;

    while (future.isFinished() == false) {
        for (int i = 0; i < future.resultCount(); ++i) {
            results += future.resultAt(i);
        }

        QTest::qWait(1);
    }

    QCOMPARE(future.isFinished(), true);
    QCOMPARE(future.resultCount(), count);
    QCOMPARE(future.results().count(), count);
}

/*
    Test that mapped does not cause deep copies when holding
    references to Qt containers.
*/
void tst_QtConcurrentMap::noDetach()
{
    {
        QList<int> l = QList<int>() << 1;
        QVERIFY(l.isDetached());

        QList<int> ll = l;
        QVERIFY(!l.isDetached());

        QtConcurrent::mapped(l, mapper).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::mappedReduced(l, mapper, intSumReduce).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::map(l, multiplyBy2Immutable).waitForFinished();
        QVERIFY(l.isDetached());
        QVERIFY(ll.isDetached());
    }
    {
        const QList<int> l = QList<int>() << 1;
        QVERIFY(l.isDetached());

        const QList<int> ll = l;
        QVERIFY(!l.isDetached());

        QtConcurrent::mapped(l, mapper).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::mappedReduced(l, mapper, intSumReduce).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());
    }

}

void tst_QtConcurrentMap::stlContainers()
{
    std::vector<int> vector;
    vector.push_back(1);
    vector.push_back(2);

    std::vector<int> vector2 =  QtConcurrent::blockingMapped<std::vector<int> >(vector, mapper);
    QCOMPARE(vector2.size(), (std::vector<int>::size_type)(2));

    std::list<int> list;
    list.push_back(1);
    list.push_back(2);

    std::list<int> list2 =  QtConcurrent::blockingMapped<std::list<int> >(list, mapper);
    QCOMPARE(list2.size(), (std::vector<int>::size_type)(2));

    QtConcurrent::mapped(list, mapper).waitForFinished();

    QtConcurrent::blockingMap(list, multiplyBy2Immutable);
}

void tst_QtConcurrentMap::stlContainersLambda()
{
    std::vector<int> vector;
    vector.push_back(1);
    vector.push_back(2);

    std::vector<int> vector2 = QtConcurrent::blockingMapped<std::vector<int> >(vector,
        [](const int &i) {
            return mapper(i);
        }
    );
    QCOMPARE(vector2.size(), (std::vector<int>::size_type)(2));

    std::list<int> list;
    list.push_back(1);
    list.push_back(2);

    std::list<int> list2 = QtConcurrent::blockingMapped<std::list<int> >(list,
        [](const int &i) {
            return mapper(i);
        }
    );
    QCOMPARE(list2.size(), (std::vector<int>::size_type)(2));

    QtConcurrent::mapped(list, [](const int &i) { return mapper(i); }).waitForFinished();

    QtConcurrent::blockingMap(list, [](int x) { x *= 2; });
}

InstanceCounter ic_fn(const InstanceCounter & ic)
{
    return InstanceCounter(ic);
}

// Verify that held results are deleted when a future is
// assigned over with operator ==
void tst_QtConcurrentMap::qFutureAssignmentLeak()
{
    currentInstanceCount.storeRelaxed(0);
    peakInstanceCount.storeRelaxed(0);
    QFuture<InstanceCounter> future;
    {
        QList<InstanceCounter> list;
        for (int i=0;i<1000;++i)
            list += InstanceCounter();
        future = QtConcurrent::mapped(list, ic_fn);
        future.waitForFinished();

        future = QtConcurrent::mapped(list, ic_fn);
        future.waitForFinished();

        future = QtConcurrent::mapped(list, ic_fn);
        future.waitForFinished();
    }

    // Use QTRY_COMPARE because QtConcurrent::ThreadEngine::asynchronousFinish()
    // deletes its internals after signaling finished, so it might still be holding
    // on to copies of InstanceCounter for a short while.
    QTRY_COMPARE(currentInstanceCount.loadRelaxed(), 1000);
    future = QFuture<InstanceCounter>();
    QTRY_COMPARE(currentInstanceCount.loadRelaxed(), 0);
}

inline void increment(int &num)
{
    ++num;
}

inline int echo(const int &num)
{
    return num;
}

void add(int &result, const int &sum)
{
    result += sum;
}

void tst_QtConcurrentMap::stressTest()
{
    const int listSize = 1000;
    const int sum = (listSize - 1) * (listSize / 2);
    QList<int> list;


    for (int i = 0; i < listSize; ++i) {
        list.append(i);
    }

    for (int i =0 ; i < 100; ++i) {
        QList<int> result = QtConcurrent::blockingMapped(list, echo);
        for (int j = 0; j < listSize; ++j)
            QCOMPARE(result.at(j), j);
    }

    for (int i = 0 ; i < 100; ++i) {
        int result = QtConcurrent::blockingMappedReduced(list, echo, add);
        QCOMPARE(result, sum);
    }

    for (int i = 0 ; i < 100; ++i) {
        QtConcurrent::map(list, increment).waitForFinished();
        for (int j = 0; j < listSize; ++j)
            QCOMPARE(list.at(j), i + j + 1);
    }
}

struct LockedCounter
{
    LockedCounter(QMutex *mutex, QAtomicInt *ai)
        : mtx(mutex),
          ref(ai) {}

    int operator()(int x)
    {
        QMutexLocker locker(mtx);
        ref->ref();
        return ++x;
    }

    QMutex *mtx;
    QAtomicInt *ref;
};

// The Thread engine holds the last reference
// to the QFuture, so this should not leak
// or fail.
void tst_QtConcurrentMap::persistentResultTest()
{
    QFuture<void> voidFuture;
    QMutex mtx;
    QAtomicInt ref;
    LockedCounter lc(&mtx, &ref);
    QList<int> list;
    {
        list << 1 << 2 << 3;
        mtx.lock();
        QFuture<int> future = QtConcurrent::mapped(list
                                                   ,lc);
        voidFuture = future;
    }
    QCOMPARE(ref.loadAcquire(), 0);
    mtx.unlock(); // Unblock
    voidFuture.waitForFinished();
    QCOMPARE(ref.loadAcquire(), 3);
}

void tst_QtConcurrentMap::mappedReducedInitialValue()
{
    // This is a copy of tst_QtConcurrentMap::mappedReduced with the initial value parameter added

    QList<int> list;
    list << 1 << 2 << 3;
    QList<Number> numberList;
    numberList << 1 << 2 << 3;

    // test Q_DECLARE_OPERATORS_FOR_FLAGS
    QtConcurrent::ReduceOptions opt =
            (QtConcurrent::UnorderedReduce | QtConcurrent::SequentialReduce);
    QVERIFY(opt);

    int initialValue = 10;
    // functor-functor
    {
        int sum = QtConcurrent::mappedReduced<int>(list, IntSquare(), IntSumReduce(), initialValue);
        QCOMPARE(sum, 24);
        int sum2 = QtConcurrent::mappedReduced<int>(
                list.constBegin(), list.constEnd(), IntSquare(), IntSumReduce(), initialValue);
        QCOMPARE(sum2, 24);

        int sum3 = QtConcurrent::mappedReduced<int>(
                QList<int>(list), IntSquare(), IntSumReduce(), initialValue);
        QCOMPARE(sum3, 24);

        int sum4 = QtConcurrent::mappedReduced<int>(list, intSquare, intSumReduce, initialValue);
        QCOMPARE(sum4, 24);
        int sum5 = QtConcurrent::mappedReduced<int>(
                list.constBegin(), list.constEnd(), intSquare, intSumReduce, initialValue);
        QCOMPARE(sum5, 24);

        int sum6 = QtConcurrent::mappedReduced<int>(
                QList<int>(list), intSquare, intSumReduce, initialValue);
        QCOMPARE(sum6, 24);
    }

    // function-functor
    {
        int sum = QtConcurrent::mappedReduced<int>(list, intSquare, IntSumReduce(), initialValue);
        QCOMPARE(sum, 24);
        int sum2 = QtConcurrent::mappedReduced<int>(
                list.constBegin(), list.constEnd(), intSquare, IntSumReduce(), initialValue);
        QCOMPARE(sum2, 24);

        int sum3 = QtConcurrent::mappedReduced<int>(
                QList<int>(list), intSquare, IntSumReduce(), initialValue);
        QCOMPARE(sum3, 24);
    }

    // functor-function
    {
        int sum = QtConcurrent::mappedReduced(list, IntSquare(), intSumReduce, initialValue);
        QCOMPARE(sum, 24);
        int sum2 = QtConcurrent::mappedReduced(
                list.constBegin(), list.constEnd(), IntSquare(), intSumReduce, initialValue);
        QCOMPARE(sum2, 24);

        int sum3 = QtConcurrent::mappedReduced(
                QList<int>(list), IntSquare(), intSumReduce, initialValue);
        QCOMPARE(sum3, 24);
    }

    // function-function
    {
        int sum = QtConcurrent::mappedReduced(list, intSquare, intSumReduce, initialValue);
        QCOMPARE(sum, 24);
        int sum2 = QtConcurrent::mappedReduced(
                list.constBegin(), list.constEnd(), intSquare, intSumReduce, initialValue);
        QCOMPARE(sum2, 24);

        int sum3 = QtConcurrent::mappedReduced(
                QList<int>(list), intSquare, intSumReduce, initialValue);
        QCOMPARE(sum3, 24);
    }

    auto push_back = static_cast<void (QVector<int>::*)(const int &)>(&QVector<int>::push_back);

    QVector<int> initialIntVector;
    initialIntVector.push_back(10);
    // functor-member
    {
        QList<int> list2 = QtConcurrent::mappedReduced(
                list, IntSquare(), push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 10 << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::mappedReduced(list.constBegin(),
                                                       list.constEnd(),
                                                       IntSquare(),
                                                       push_back,
                                                       initialIntVector,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 10 << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::mappedReduced(
                QList<int>(list), IntSquare(), push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 10 << 1 << 4 << 9);
    }

    // member-functor
    {
        int sum = QtConcurrent::mappedReduced<int>(
                numberList, &Number::toInt, IntSumReduce(), initialValue);
        QCOMPARE(sum, 16);
        int sum2 = QtConcurrent::mappedReduced<int>(numberList.constBegin(),
                                                    numberList.constEnd(),
                                                    &Number::toInt,
                                                    IntSumReduce(),
                                                    initialValue);
        QCOMPARE(sum2, 16);

        int sum3 = QtConcurrent::mappedReduced<int>(
                QList<Number>(numberList), &Number::toInt, IntSumReduce(), initialValue);
        QCOMPARE(sum3, 16);
    }

    // member-member
    {
        QList<int> list2 = QtConcurrent::mappedReduced(
                numberList, &Number::toInt, push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list2, QList<int>() << 10 << 1 << 2 << 3);

        QList<int> list3 = QtConcurrent::mappedReduced(numberList.constBegin(),
                                                       numberList.constEnd(),
                                                       &Number::toInt,
                                                       push_back,
                                                       initialIntVector,
                                                       OrderedReduce);
        QCOMPARE(list3, QList<int>() << 10 << 1 << 2 << 3);

        QList<int> list4 = QtConcurrent::mappedReduced(QList<Number>(numberList),
                                                       &Number::toInt,
                                                       push_back,
                                                       initialIntVector,
                                                       OrderedReduce);
        QCOMPARE(list4, QList<int>() << 10 << 1 << 2 << 3);
    }

    // function-member
    {
        QList<int> list2 = QtConcurrent::mappedReduced(
                list, intSquare, push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 10 << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::mappedReduced(list.constBegin(),
                                                       list.constEnd(),
                                                       intSquare,
                                                       push_back,
                                                       initialIntVector,
                                                       OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 10 << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::mappedReduced(
                QList<int>(list), intSquare, push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 10 << 1 << 4 << 9);
    }

    // member-function
    {
        int sum =
                QtConcurrent::mappedReduced(numberList, &Number::toInt, intSumReduce, initialValue);
        QCOMPARE(sum, 16);
        int sum2 = QtConcurrent::mappedReduced(numberList.constBegin(),
                                               numberList.constEnd(),
                                               &Number::toInt,
                                               intSumReduce,
                                               initialValue);
        QCOMPARE(sum2, 16);

        int sum3 = QtConcurrent::mappedReduced(
                QList<Number>(numberList), &Number::toInt, intSumReduce, initialValue);
        QCOMPARE(sum3, 16);
    }
}

void tst_QtConcurrentMap::blocking_mappedReducedInitialValue()
{
    // This is a copy of tst_QtConcurrentMap::blocking_mappedReduced with the initial value
    // parameter added

    QList<int> list;
    list << 1 << 2 << 3;
    QList<Number> numberList;
    numberList << 1 << 2 << 3;

    int initialValue = 10;
    // functor-functor
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(
                list, IntSquare(), IntSumReduce(), initialValue);
        QCOMPARE(sum, 24);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(
                list.constBegin(), list.constEnd(), IntSquare(), IntSumReduce(), initialValue);
        QCOMPARE(sum2, 24);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(
                QList<int>(list), IntSquare(), IntSumReduce(), initialValue);
        QCOMPARE(sum3, 24);

        int sum4 = QtConcurrent::blockingMappedReduced<int>(
                list, intSquare, intSumReduce, initialValue);
        QCOMPARE(sum4, 24);
        int sum5 = QtConcurrent::blockingMappedReduced<int>(
                list.constBegin(), list.constEnd(), intSquare, intSumReduce, initialValue);
        QCOMPARE(sum5, 24);

        int sum6 = QtConcurrent::blockingMappedReduced<int>(
                QList<int>(list), intSquare, intSumReduce, initialValue);
        QCOMPARE(sum6, 24);
    }

    // function-functor
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(
                list, intSquare, IntSumReduce(), initialValue);
        QCOMPARE(sum, 24);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(
                list.constBegin(), list.constEnd(), intSquare, IntSumReduce(), initialValue);
        QCOMPARE(sum2, 24);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(
                QList<int>(list), intSquare, IntSumReduce(), initialValue);
        QCOMPARE(sum3, 24);
    }

    // functor-function
    {
        int sum =
                QtConcurrent::blockingMappedReduced(list, IntSquare(), intSumReduce, initialValue);
        QCOMPARE(sum, 24);
        int sum2 = QtConcurrent::blockingMappedReduced(
                list.constBegin(), list.constEnd(), IntSquare(), intSumReduce, initialValue);
        QCOMPARE(sum2, 24);

        int sum3 = QtConcurrent::blockingMappedReduced(
                QList<int>(list), IntSquare(), intSumReduce, initialValue);
        QCOMPARE(sum3, 24);
    }

    // function-function
    {
        int sum = QtConcurrent::blockingMappedReduced(list, intSquare, intSumReduce, initialValue);
        QCOMPARE(sum, 24);
        int sum2 = QtConcurrent::blockingMappedReduced(
                list.constBegin(), list.constEnd(), intSquare, intSumReduce, initialValue);
        QCOMPARE(sum2, 24);

        int sum3 = QtConcurrent::blockingMappedReduced(
                QList<int>(list), intSquare, intSumReduce, initialValue);
        QCOMPARE(sum3, 24);
    }

    auto push_back = static_cast<void (QVector<int>::*)(const int &)>(&QVector<int>::push_back);

    QVector<int> initialIntVector;
    initialIntVector.push_back(10);
    // functor-member
    {
        QList<int> list2 = QtConcurrent::blockingMappedReduced(
                list, IntSquare(), push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 10 << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::blockingMappedReduced(list.constBegin(),
                                                               list.constEnd(),
                                                               IntSquare(),
                                                               push_back,
                                                               initialIntVector,
                                                               OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 10 << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::blockingMappedReduced(
                QList<int>(list), IntSquare(), push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 10 << 1 << 4 << 9);
    }

    // member-functor
    {
        int sum = QtConcurrent::blockingMappedReduced<int>(
                numberList, &Number::toInt, IntSumReduce(), initialValue);
        QCOMPARE(sum, 16);
        int sum2 = QtConcurrent::blockingMappedReduced<int>(numberList.constBegin(),
                                                            numberList.constEnd(),
                                                            &Number::toInt,
                                                            IntSumReduce(),
                                                            initialValue);
        QCOMPARE(sum2, 16);

        int sum3 = QtConcurrent::blockingMappedReduced<int>(
                QList<Number>(numberList), &Number::toInt, IntSumReduce(), initialValue);
        QCOMPARE(sum3, 16);
    }

    // member-member
    {
        QList<int> list2 = QtConcurrent::blockingMappedReduced(
                numberList, &Number::toInt, push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list2, QList<int>() << 10 << 1 << 2 << 3);

        QList<int> list3 = QtConcurrent::blockingMappedReduced(numberList.constBegin(),
                                                               numberList.constEnd(),
                                                               &Number::toInt,
                                                               push_back,
                                                               initialIntVector,
                                                               OrderedReduce);
        QCOMPARE(list3, QList<int>() << 10 << 1 << 2 << 3);

        QList<int> list4 = QtConcurrent::blockingMappedReduced(QList<Number>(numberList),
                                                               &Number::toInt,
                                                               push_back,
                                                               initialIntVector,
                                                               OrderedReduce);
        QCOMPARE(list4, QList<int>() << 10 << 1 << 2 << 3);
    }

    // function-member
    {
        QList<int> list2 = QtConcurrent::blockingMappedReduced(
                list, intSquare, push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list2, QList<int>() << 10 << 1 << 4 << 9);

        QList<int> list3 = QtConcurrent::blockingMappedReduced(list.constBegin(),
                                                               list.constEnd(),
                                                               intSquare,
                                                               push_back,
                                                               initialIntVector,
                                                               OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list3, QList<int>() << 10 << 1 << 4 << 9);

        QList<int> list4 = QtConcurrent::blockingMappedReduced(
                QList<int>(list), intSquare, push_back, initialIntVector, OrderedReduce);
        QCOMPARE(list, QList<int>() << 1 << 2 << 3);
        QCOMPARE(list4, QList<int>() << 10 << 1 << 4 << 9);
    }

    // member-function
    {
        int sum = QtConcurrent::blockingMappedReduced(
                numberList, &Number::toInt, intSumReduce, initialValue);
        QCOMPARE(sum, 16);
        int sum2 = QtConcurrent::blockingMappedReduced(numberList.constBegin(),
                                                       numberList.constEnd(),
                                                       &Number::toInt,
                                                       intSumReduce,
                                                       initialValue);
        QCOMPARE(sum2, 16);

        int sum3 = QtConcurrent::blockingMappedReduced(
                QList<Number>(numberList), &Number::toInt, intSumReduce, initialValue);
        QCOMPARE(sum3, 16);
    }
}

QTEST_MAIN(tst_QtConcurrentMap)
#include "tst_qtconcurrentmap.moc"
