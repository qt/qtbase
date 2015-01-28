/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2014 by Southwest Research Institute (R)
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
#include <qbytearraylist.h>

#include <qmetatype.h>

Q_DECLARE_METATYPE(QByteArrayList)

class tst_QByteArrayList : public QObject
{
    Q_OBJECT
private slots:
    void join() const;
    void join_data() const;
    void joinByteArray() const;
    void joinByteArray_data() const;
    void joinChar() const;
    void joinChar_data() const;
    void joinEmptiness() const;

    void operator_plus() const;
    void operator_plus_data() const;

    void initializerList() const;
};

void tst_QByteArrayList::join() const
{
    QFETCH(QByteArrayList, input);
    QFETCH(QByteArray, expectedResult);

    QCOMPARE(input.join(), expectedResult);
}

void tst_QByteArrayList::join_data() const
{
    QTest::addColumn<QByteArrayList>("input");
    QTest::addColumn<QByteArray>("expectedResult");

    QTest::newRow("data1") << QByteArrayList()
                           << QByteArray();

    QTest::newRow("data2") << (QByteArrayList() << "one")
                           << QByteArray("one");

    QTest::newRow("data3") << (QByteArrayList() << "a" << "b")
                           << QByteArray("ab");

    QTest::newRow("data4") << (QByteArrayList() << "a" << "b" << "c")
                           << QByteArray("abc");
}

void tst_QByteArrayList::joinByteArray() const
{
    QFETCH(QByteArrayList, input);
    QFETCH(QByteArray, separator);
    QFETCH(QByteArray, expectedResult);

    QCOMPARE(input.join(separator), expectedResult);
}

void tst_QByteArrayList::joinByteArray_data() const
{
    QTest::addColumn<QByteArrayList>("input");
    QTest::addColumn<QByteArray>("separator");
    QTest::addColumn<QByteArray>("expectedResult");

    QTest::newRow("data1") << QByteArrayList()
                           << QByteArray()
                           << QByteArray();

    QTest::newRow("data2") << QByteArrayList()
                           << QByteArray("separator")
                           << QByteArray();

    QTest::newRow("data3") << (QByteArrayList() << "one")
                           << QByteArray("separator")
                           << QByteArray("one");

    QTest::newRow("data4") << (QByteArrayList() << "a" << "b")
                           << QByteArray(" ")
                           << QByteArray("a b");

    QTest::newRow("data5") << (QByteArrayList() << "a" << "b" << "c")
                           << QByteArray(" ")
                           << QByteArray("a b c");

    QTest::newRow("data6") << (QByteArrayList()  << "a" << "b" << "c")
                           << QByteArray()
                           << QByteArray("abc");

    QTest::newRow("data7") << (QByteArrayList()  << "a" << "b" << "c")
                           << QByteArray("") //empty
                           << QByteArray("abc");
}

void tst_QByteArrayList::joinChar() const
{
    QFETCH(QByteArrayList, input);
    QFETCH(char, separator);
    QFETCH(QByteArray, expectedResult);

    QCOMPARE(input.join(separator), expectedResult);
}

void tst_QByteArrayList::joinChar_data() const
{
    QTest::addColumn<QByteArrayList>("input");
    QTest::addColumn<char>("separator");
    QTest::addColumn<QByteArray>("expectedResult");

    QTest::newRow("data1") << QByteArrayList()
                           << ' '
                           << QByteArray();

    QTest::newRow("data2") << (QByteArrayList() << "a a" << "b")
                           << ' '
                           << QByteArray("a a b");

    QTest::newRow("data3") << (QByteArrayList() << "a" << "b" << "c c")
                           << ' '
                           << QByteArray("a b c c");
}

void tst_QByteArrayList::joinEmptiness() const
{
    QByteArrayList list;
    QByteArray string = list.join(QByteArray());

    QVERIFY(string.isEmpty());
    QVERIFY(string.isNull());
}

void tst_QByteArrayList::operator_plus() const
{
    QFETCH(QByteArrayList, lhs);
    QFETCH(QByteArrayList, rhs);
    QFETCH(QByteArrayList, expectedResult);

    // operator+ for const lvalues
    {
        const QByteArrayList bal1 = lhs;
        const QByteArrayList bal2 = rhs;
        QCOMPARE(bal1 + bal2, expectedResult);
    }
    {
        const QList<QByteArray> lba1 = lhs;
        const QByteArrayList bal2 = rhs;
        QCOMPARE(lba1 + bal2, expectedResult);
    }
    {
        const QByteArrayList bal1 = lhs;
        const QList<QByteArray> lba2 = rhs;
        QCOMPARE(bal1 + lba2, expectedResult);
    }
    {
        const QList<QByteArray> lba1 = lhs;
        const QList<QByteArray> lba2 = rhs;
        QCOMPARE(lba1 + lba2, QList<QByteArray>(expectedResult)); // check we don't mess with old code
    }

    // operator+ for rvalues (only lhs)
    {
        QByteArrayList bal1 = lhs;
        const QByteArrayList bal2 = rhs;
        QCOMPARE(qMove(bal1) + bal2, expectedResult);
    }
    {
        QList<QByteArray> lba1 = lhs;
        const QByteArrayList bal2 = rhs;
        QCOMPARE(qMove(lba1) + bal2, expectedResult);
    }
    {
        QByteArrayList bal1 = lhs;
        const QList<QByteArray> lba2 = rhs;
        QCOMPARE(qMove(bal1) + lba2, expectedResult);
    }
    {
        QList<QByteArray> lba1 = lhs;
        const QList<QByteArray> lba2 = rhs;
        QCOMPARE(qMove(lba1) + lba2, QList<QByteArray>(expectedResult)); // check we don't mess with old code
    }

    // operator += for const lvalues
    {
        QByteArrayList bal1 = lhs;
        const QByteArrayList bal2 = rhs;
        QCOMPARE(bal1 += bal2, expectedResult);
    }
    {
        QByteArrayList bal1 = lhs;
        const QList<QByteArray> lba2 = rhs;
        QCOMPARE(bal1 += lba2, expectedResult);
    }
    {
        QList<QByteArray> lba1 = lhs;
        const QByteArrayList bal2 = rhs;
        QCOMPARE(lba1 += bal2, QList<QByteArray>(expectedResult));
    }

    QByteArrayList t1 = lhs;
    QByteArrayList t2 = rhs;

    QCOMPARE(qMove(t1) + t2, expectedResult);
}

void tst_QByteArrayList::operator_plus_data() const
{
    QTest::addColumn<QByteArrayList>("lhs");
    QTest::addColumn<QByteArrayList>("rhs");
    QTest::addColumn<QByteArrayList>("expectedResult");

    QTest::newRow("simpl") << ( QByteArrayList() << "a" )
                           << ( QByteArrayList() << "b" << "c" )
                           << ( QByteArrayList() << "a" << "b" << "c" );

    QTest::newRow("blank1") << QByteArrayList()
                            << QByteArrayList()
                            << QByteArrayList();

    QTest::newRow("blank2") << ( QByteArrayList() )
                            << ( QByteArrayList() << "b" << "c" )
                            << ( QByteArrayList() << "b" << "c" );

    QTest::newRow("empty1") << ( QByteArrayList() << "" )
                            << ( QByteArrayList() << "b" << "c" )
                            << ( QByteArrayList() << "" << "b" << "c" );

    QTest::newRow("empty2") << ( QByteArrayList() << "a" )
                            << ( QByteArrayList() << "" << "c" )
                            << ( QByteArrayList() << "a" << "" << "c" );
}

void tst_QByteArrayList::initializerList() const
{
#ifdef Q_COMPILER_INITIALIZER_LISTS
    // constructor
    QByteArrayList v1 = {QByteArray("hello"),"world",QByteArray("plop")};
    QCOMPARE(v1, (QByteArrayList() << "hello" << "world" << "plop"));
    QCOMPARE(v1, (QByteArrayList{"hello","world","plop"}));
    // assignment operator (through implicit temporary)
    QByteArrayList v2;
    v2 = {QByteArray("hello"),"world",QByteArray("plop")};
    QCOMPARE(v2, v1);
#else
    QSKIP("This test requires C++11 initializer_list support in the compiler.");
#endif
}

QTEST_APPLESS_MAIN(tst_QByteArrayList)
#include "tst_qbytearraylist.moc"
