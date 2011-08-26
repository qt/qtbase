/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QList>

//TESTED_CLASS=QList
//TESTED_FILES=corelib/tools/qlist.h corelib/tools/qlist.cpp

/*!
  \class tst_QVector
  \internal
  \since 4.5
  \brief Test Qt's class QList.
 */
class tst_QList : public QObject
{
    Q_OBJECT

private slots:
    void length() const;
    void lengthSignature() const;
    void append() const;
    void prepend() const;
    void mid() const;
    void at() const;
    void first() const;
    void last() const;
    void begin() const;
    void end() const;
    void contains() const;
    void count() const;
    void empty() const;
    void endsWith() const;
    void lastIndexOf() const;
    void move() const;
    void removeAll() const;
    void removeAt() const;
    void removeOne() const;
    void replace() const;
    void startsWith() const;
    void swap() const;
    void takeAt() const;
    void takeFirst() const;
    void takeLast() const;
    void toSet() const;
    void toStdList() const;
    void toVector() const;
    void value() const;

    void testSTLIterators() const;
    void testOperators() const;

    void initializeList() const;
};

void tst_QList::length() const
{
    /* Empty list. */
    {
        const QList<int> list;
        QCOMPARE(list.length(), 0);
    }

    /* One entry. */
    {
        QList<int> list;
        list.append(0);
        QCOMPARE(list.length(), 1);
    }

    /* Two entries. */
    {
        QList<int> list;
        list.append(0);
        list.append(1);
        QCOMPARE(list.length(), 2);
    }

    /* Three entries. */
    {
        QList<int> list;
        list.append(0);
        list.append(0);
        list.append(0);
        QCOMPARE(list.length(), 3);
    }
}

void tst_QList::lengthSignature() const
{
    /* Constness. */
    {
        const QList<int> list;
        /* The function should be const. */
        list.length();
    }
}

void tst_QList::append() const
{
    /* test append(const QList<T> &) function */
    QString one("one");
    QString two("two");
    QString three("three");
    QString four("four");
    QList<QString> list1;
    QList<QString> list2;
    QList<QString> listTotal;
    list1.append(one);
    list1.append(two);
    list2.append(three);
    list2.append(four);
    list1.append(list2);
    qDebug() << list1;
    listTotal.append(one);
    listTotal.append(two);
    listTotal.append(three);
    listTotal.append(four);
    QCOMPARE(list1, listTotal);

}

void tst_QList::prepend() const
{
    QList<QString *> list;
    QString *str1 = new QString;
    list.prepend(str1);
    QVERIFY(list.size() == 1);
    QVERIFY(list.at(0) == str1);
    QString *str2 = new QString;
    list.prepend(str2);
    QVERIFY(list.size() == 2);
    QVERIFY(list.at(0) == str2);
    QVERIFY(list.at(1) == str1);
    QString *str3 = new QString;
    list.prepend(str3);
    QVERIFY(list.size() == 3);
    QVERIFY(list.at(0) == str3);
    QVERIFY(list.at(1) == str2);
    QVERIFY(list.at(2) == str1);
    list.removeAll(str2);
    delete str2;
    QVERIFY(list.size() == 2);
    QVERIFY(list.at(0) == str3);
    QVERIFY(list.at(1) == str1);
    QString *str4 = new QString;
    list.prepend(str4);
    QVERIFY(list.size() == 3);
    QVERIFY(list.at(0) == str4);
    QVERIFY(list.at(1) == str3);
    QVERIFY(list.at(2) == str1);
    qDeleteAll(list);
    list.clear();
}

void tst_QList::mid() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz" << "bak" << "buck" << "hello" << "kitty";

    QCOMPARE(list.mid(3, 3),
             QList<QString>() << "bak" << "buck" << "hello");

    QList<int> list1;
    QCOMPARE(list1.mid(1, 1).length(), 0);
}

void tst_QList::at() const
{
    // test at() and make sure it functions correctly with some simple list manipulation.
    QList<QString> list;

    // create a list
    list << "foo" << "bar" << "baz";
    QVERIFY(list.size() == 3);
    QCOMPARE(list.at(0), QLatin1String("foo"));
    QCOMPARE(list.at(1), QLatin1String("bar"));
    QCOMPARE(list.at(2), QLatin1String("baz"));

    // append an item
    list << "hello";
    QVERIFY(list.size() == 4);
    QCOMPARE(list.at(0), QLatin1String("foo"));
    QCOMPARE(list.at(1), QLatin1String("bar"));
    QCOMPARE(list.at(2), QLatin1String("baz"));
    QCOMPARE(list.at(3), QLatin1String("hello"));

    // remove an item
    list.removeAt(1);
    QVERIFY(list.size() == 3);
    QCOMPARE(list.at(0), QLatin1String("foo"));
    QCOMPARE(list.at(1), QLatin1String("baz"));
    QCOMPARE(list.at(2), QLatin1String("hello"));
}

void tst_QList::first() const
{
    QList<QString> list;
    list << "foo" << "bar";

    QCOMPARE(list.first(), QLatin1String("foo"));

    // remove an item, make sure it still works
    list.pop_front();
    QVERIFY(list.size() == 1);
    QCOMPARE(list.first(), QLatin1String("bar"));
}

void tst_QList::last() const
{
    QList<QString> list;
    list << "foo" << "bar";

    QCOMPARE(list.last(), QLatin1String("bar"));

    // remove an item, make sure it still works
    list.pop_back();
    QVERIFY(list.size() == 1);
    QCOMPARE(list.last(), QLatin1String("foo"));
}

void tst_QList::begin() const
{
    QList<QString> list;
    list << "foo" << "bar";

    QCOMPARE(*list.begin(), QLatin1String("foo"));

    // remove an item, make sure it still works
    list.pop_front();
    QVERIFY(list.size() == 1);
    QCOMPARE(*list.begin(), QLatin1String("bar"));
}

void tst_QList::end() const
{
    QList<QString> list;
    list << "foo" << "bar";

    QCOMPARE(*--list.end(), QLatin1String("bar"));

    // remove an item, make sure it still works
    list.pop_back();
    QVERIFY(list.size() == 1);
    QCOMPARE(*--list.end(), QLatin1String("foo"));
}

void tst_QList::contains() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    QVERIFY(list.contains(QLatin1String("foo")) == true);
    QVERIFY(list.contains(QLatin1String("pirates")) != true);

    // add it and make sure it matches
    list.append(QLatin1String("ninjas"));
    QVERIFY(list.contains(QLatin1String("ninjas")) == true);
}

void tst_QList::count() const
{
    QList<QString> list;

    // starts empty
    QVERIFY(list.count() == 0);

    // goes up
    list.append(QLatin1String("foo"));
    QVERIFY(list.count() == 1);

    // and up
    list.append(QLatin1String("bar"));
    QVERIFY(list.count() == 2);

    // and down
    list.pop_back();
    QVERIFY(list.count() == 1);

    // and empty. :)
    list.pop_back();
    QVERIFY(list.count() == 0);
}

void tst_QList::empty() const
{
    QList<QString> list;

    // make sure it starts empty
    QVERIFY(list.empty());

    // and doesn't stay empty
    list.append(QLatin1String("foo"));
    QVERIFY(!list.empty());

    // and goes back to being empty
    list.pop_back();
    QVERIFY(list.empty());
}

void tst_QList::endsWith() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // test it returns correctly in both cases
    QVERIFY(list.endsWith(QLatin1String("baz")));
    QVERIFY(!list.endsWith(QLatin1String("bar")));

    // remove an item and make sure the end item changes
    list.pop_back();
    QVERIFY(list.endsWith(QLatin1String("bar")));
}

void tst_QList::lastIndexOf() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // one instance of the target item
    QVERIFY(list.lastIndexOf(QLatin1String("baz")) == 2);

    // shouldn't find this
    QVERIFY(list.lastIndexOf(QLatin1String("shouldntfindme")) == -1);

    // multiple instances
    list.append("baz");
    list.append("baz");
    QVERIFY(list.lastIndexOf(QLatin1String("baz")) == 4);

    // search from the middle to find the last one
    QVERIFY(list.lastIndexOf(QLatin1String("baz"), 3) == 3);

    // try find none
    QVERIFY(list.lastIndexOf(QLatin1String("baz"), 1) == -1);
}

void tst_QList::move() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // move an item
    list.move(0, list.count() - 1);
    QCOMPARE(list, QList<QString>() << "bar" << "baz" << "foo");

    // move it back
    list.move(list.count() - 1, 0);
    QCOMPARE(list, QList<QString>() << "foo" << "bar" << "baz");

    // move an item in the middle
    list.move(1, 0);
    QCOMPARE(list, QList<QString>() << "bar" << "foo" << "baz");
}

void tst_QList::removeAll() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // remove one instance
    list.removeAll(QLatin1String("bar"));
    QCOMPARE(list, QList<QString>() << "foo" << "baz");

    // many instances
    list << "foo" << "bar" << "baz";
    list << "foo" << "bar" << "baz";
    list << "foo" << "bar" << "baz";
    list.removeAll(QLatin1String("bar"));
    QCOMPARE(list, QList<QString>() << "foo" << "baz" << "foo" << "baz" << "foo" << "baz" << "foo" << "baz");

    // try remove something that doesn't exist
    list.removeAll(QLatin1String("you won't remove anything I hope"));
    QCOMPARE(list, QList<QString>() << "foo" << "baz" << "foo" << "baz" << "foo" << "baz" << "foo" << "baz");
}

void tst_QList::removeAt() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // middle
    list.removeAt(1);
    QCOMPARE(list, QList<QString>() << "foo" << "baz");

    // start
    list.removeAt(0);
    QCOMPARE(list, QList<QString>() << "baz");

    // final
    list.removeAt(0);
    QCOMPARE(list, QList<QString>());
}

void tst_QList::removeOne() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // middle
    list.removeOne(QLatin1String("bar"));
    QCOMPARE(list, QList<QString>() << "foo" << "baz");

    // start
    list.removeOne(QLatin1String("foo"));
    QCOMPARE(list, QList<QString>() << "baz");

    // last
    list.removeOne(QLatin1String("baz"));
    QCOMPARE(list, QList<QString>());

    // make sure it really only removes one :)
    list << "foo" << "foo";
    list.removeOne("foo");
    QCOMPARE(list, QList<QString>() << "foo");

    // try remove something that doesn't exist
    list.removeOne(QLatin1String("you won't remove anything I hope"));
    QCOMPARE(list, QList<QString>() << "foo");
}

void tst_QList::replace() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // start
    list.replace(0, "moo");
    QCOMPARE(list, QList<QString>() << "moo" << "bar" << "baz");

    // middle
    list.replace(1, "cow");
    QCOMPARE(list, QList<QString>() << "moo" << "cow" << "baz");

    // end
    list.replace(2, "milk");
    QCOMPARE(list, QList<QString>() << "moo" << "cow" << "milk");
}

void tst_QList::startsWith() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // make sure it starts ok
    QVERIFY(list.startsWith(QLatin1String("foo")));

    // remove an item
    list.removeFirst();
    QVERIFY(list.startsWith(QLatin1String("bar")));
}

void tst_QList::swap() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // swap
    list.swap(0, 2);
    QCOMPARE(list, QList<QString>() << "baz" << "bar" << "foo");

    // swap again
    list.swap(1, 2);
    QCOMPARE(list, QList<QString>() << "baz" << "foo" << "bar");

    QList<QString> list2;
    list2 << "alpha" << "beta";

    list.swap(list2);
    QCOMPARE(list,  QList<QString>() << "alpha" << "beta");
    QCOMPARE(list2, QList<QString>() << "baz" << "foo" << "bar");
}

void tst_QList::takeAt() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    QCOMPARE(list.takeAt(0), QLatin1String("foo"));
    QVERIFY(list.size() == 2);
    QCOMPARE(list.takeAt(1), QLatin1String("baz"));
    QVERIFY(list.size() == 1);
    QCOMPARE(list.takeAt(0), QLatin1String("bar"));
    QVERIFY(list.size() == 0);
}

void tst_QList::takeFirst() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    QCOMPARE(list.takeFirst(), QLatin1String("foo"));
    QVERIFY(list.size() == 2);
    QCOMPARE(list.takeFirst(), QLatin1String("bar"));
    QVERIFY(list.size() == 1);
    QCOMPARE(list.takeFirst(), QLatin1String("baz"));
    QVERIFY(list.size() == 0);
}

void tst_QList::takeLast() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    QCOMPARE(list.takeLast(), QLatin1String("baz"));
    QCOMPARE(list.takeLast(), QLatin1String("bar"));
    QCOMPARE(list.takeLast(), QLatin1String("foo"));
}

void tst_QList::toSet() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // no duplicates
    QCOMPARE(list.toSet(), QSet<QString>() << "foo" << "bar" << "baz");
    QCOMPARE(list, QList<QString>() << "foo" << "bar" << "baz");

    // duplicates (is this more of a QSet test?)
    list << "foo" << "bar" << "baz";
    QCOMPARE(list.toSet(), QSet<QString>() << "foo" << "bar" << "baz");
    QCOMPARE(list, QList<QString>() << "foo" << "bar" << "baz" << "foo" << "bar" << "baz");
}

void tst_QList::toStdList() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // yuck.
    std::list<QString> slist;
    slist.push_back(QLatin1String("foo"));
    slist.push_back(QLatin1String("bar"));
    slist.push_back(QLatin1String("baz"));

    QCOMPARE(list.toStdList(), slist);
    QCOMPARE(list, QList<QString>() << "foo" << "bar" << "baz");
}

void tst_QList::toVector() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    QCOMPARE(list.toVector(), QVector<QString>() << "foo" << "bar" << "baz");
}

void tst_QList::value() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    // test real values
    QCOMPARE(list.value(0), QLatin1String("foo"));
    QCOMPARE(list.value(2), QLatin1String("baz"));

    // test empty default
    QCOMPARE(list.value(3), QString());
    QCOMPARE(list.value(-1), QString());

    // test defaults
    QLatin1String defaultstr("default");
    QCOMPARE(list.value(-1, defaultstr), defaultstr);
    QCOMPARE(list.value(3, defaultstr), defaultstr);
}

void tst_QList::testOperators() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz";

    QList<QString> listtwo;
    listtwo << "foo" << "bar" << "baz";

    // test equal
    QVERIFY(list == listtwo);

    // not equal
    listtwo.append("not equal");
    QVERIFY(list != listtwo);

    // +=
    list += listtwo;
    QVERIFY(list.size() == 7);
    QVERIFY(listtwo.size() == 4);
    QCOMPARE(list, QList<QString>() << "foo" << "bar" << "baz" << "foo" << "bar" << "baz" << "not equal");

    // =
    list = listtwo;
    QCOMPARE(list, listtwo);
    QCOMPARE(list, QList<QString>() << "foo" << "bar" << "baz" << "not equal");

    // []
    QCOMPARE(list[0], QLatin1String("foo"));
    QCOMPARE(list[list.size() - 1], QLatin1String("not equal"));
}

void tst_QList::testSTLIterators() const
{
    QList<QString> list;

    // create a list
    list << "foo" << "bar" << "baz";
    QList<QString>::iterator it = list.begin();
    QCOMPARE(*it, QLatin1String("foo")); it++;
    QCOMPARE(*it, QLatin1String("bar")); it++;
    QCOMPARE(*it, QLatin1String("baz")); it++;
    QCOMPARE(it, list.end()); it--;

    // walk backwards
    QCOMPARE(*it, QLatin1String("baz")); it--;
    QCOMPARE(*it, QLatin1String("bar")); it--;
    QCOMPARE(*it, QLatin1String("foo"));

    // test erase
    it = list.erase(it);
    QVERIFY(list.size() == 2);
    QCOMPARE(*it, QLatin1String("bar"));

    // test multiple erase
    it = list.erase(it, it + 2);
    QVERIFY(list.size() == 0);
    QCOMPARE(it, list.end());

    // insert again
    it = list.insert(it, QLatin1String("foo"));
    QVERIFY(list.size() == 1);
    QCOMPARE(*it, QLatin1String("foo"));

    // insert again
    it = list.insert(it, QLatin1String("bar"));
    QVERIFY(list.size() == 2);
    QCOMPARE(*it++, QLatin1String("bar"));
    QCOMPARE(*it, QLatin1String("foo"));
}

void tst_QList::initializeList() const
{
#ifdef Q_COMPILER_INITIALIZER_LISTS
    QList<int> v1{2,3,4};
    QCOMPARE(v1, QList<int>() << 2 << 3 << 4);
    QCOMPARE(v1, (QList<int>{2,3,4}));

    QList<QList<int>> v2{ v1, {1}, QList<int>(), {2,3,4}  };
    QList<QList<int>> v3;
    v3 << v1 << (QList<int>() << 1) << QList<int>() << v1;
    QCOMPARE(v3, v2);
#endif
}

QTEST_APPLESS_MAIN(tst_QList)
#include "tst_qlist.moc"
