// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 by Southwest Research Institute (R)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define QT_USE_QSTRINGBUILDER

#include <QTest>
#include <qbytearraylist.h>

#include <qmetatype.h>
#include <qproperty.h>

Q_DECLARE_METATYPE(QByteArrayList)

class tst_QByteArrayList : public QObject
{
    Q_OBJECT
private slots:
    void join_overloads() const;
    void join() const;
    void join_data() const;
    void joinByteArray() const;
    void joinByteArray_data() const;
    void joinChar() const;
    void joinChar_data() const;
    void joinEmptiness() const;

    void operator_plus() const;
    void operator_plus_data() const;

    void indexOf_data() const;
    void indexOf() const;

    void initializerList() const;
};

void tst_QByteArrayList::join_overloads() const
{
    // Checks that there are no ambiguities between the different join() overloads:

    const QByteArrayList list = {"a", "b", "c"};
    const QByteArray expected = "aXbXc";

    QCOMPARE(list.join('X'), expected);
    QCOMPARE(list.join("X"), expected);
    QCOMPARE(list.join(QByteArrayLiteral("X")), expected);
    QCOMPARE(list.join(QByteArray("X")), expected);
    QCOMPARE(list.join(QByteArrayView("X")), expected);
    const char *sep = "X";
    QCOMPARE(list.join(sep), expected);
    QCOMPARE(list.join(QByteArray() % "X"), expected); // QStringBuilder expression
    QProperty<QByteArray> prop("X"); // implicitly convertible to QByteArray
    QCOMPARE(list.join(prop), expected);
    QCOMPARE(list.join(std::as_const(prop)), expected);
}

void tst_QByteArrayList::join() const
{
    QFETCH(QByteArrayList, input);
    QFETCH(QByteArray, expectedResult);

    QCOMPARE(input.join(), expectedResult);
    QCOMPARE(input.join(QByteArrayView{}), expectedResult);
    QCOMPARE(input.join(QByteArray{}), expectedResult);
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
    QCOMPARE(input.join(QByteArrayView{separator}), expectedResult);
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
    QCOMPARE(input.join(QByteArrayView{&separator, 1}), expectedResult);
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
        QCOMPARE(std::move(bal1) + bal2, expectedResult);
    }
    {
        QList<QByteArray> lba1 = lhs;
        const QByteArrayList bal2 = rhs;
        QCOMPARE(std::move(lba1) + bal2, expectedResult);
    }
    {
        QByteArrayList bal1 = lhs;
        const QList<QByteArray> lba2 = rhs;
        QCOMPARE(std::move(bal1) + lba2, expectedResult);
    }
    {
        QList<QByteArray> lba1 = lhs;
        const QList<QByteArray> lba2 = rhs;
        QCOMPARE(std::move(lba1) + lba2, QList<QByteArray>(expectedResult)); // check we don't mess with old code
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

    QCOMPARE(std::move(t1) + t2, expectedResult);
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

void tst_QByteArrayList::indexOf_data() const
{
    QTest::addColumn<QByteArrayList>("list");
    QTest::addColumn<QByteArray>("item");
    QTest::addColumn<int>("expectedResult");

    QTest::newRow("empty") << QByteArrayList() << QByteArray("a") << -1;
    QTest::newRow("found_1") << ( QByteArrayList() << "a" ) << QByteArray("a") << 0;
    QTest::newRow("not_found_1") << ( QByteArrayList() << "a" ) << QByteArray("b") << -1;
    QTest::newRow("found_2") << ( QByteArrayList() << "hello" << "world" ) << QByteArray("world") << 1;
    QTest::newRow("returns_first") << ( QByteArrayList() << "hello" << "world" << "hello" << "again" ) << QByteArray("hello") << 0;
}

void tst_QByteArrayList::indexOf() const
{
    QFETCH(QByteArrayList, list);
    QFETCH(QByteArray, item);
    QFETCH(int, expectedResult);

    QCOMPARE(list.indexOf(item), expectedResult);
    QCOMPARE(list.indexOf(item.constData()), expectedResult);
}

void tst_QByteArrayList::initializerList() const
{
    // constructor
    QByteArrayList v1 = {QByteArray("hello"),"world",QByteArray("plop")};
    QCOMPARE(v1, (QByteArrayList() << "hello" << "world" << "plop"));
    QCOMPARE(v1, (QByteArrayList{"hello","world","plop"}));
    // assignment operator (through implicit temporary)
    QByteArrayList v2;
    v2 = {QByteArray("hello"),"world",QByteArray("plop")};
    QCOMPARE(v2, v1);
}

QTEST_APPLESS_MAIN(tst_QByteArrayList)
#include "tst_qbytearraylist.moc"
