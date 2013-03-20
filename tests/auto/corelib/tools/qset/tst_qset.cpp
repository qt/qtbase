/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//#define QT_STRICT_ITERATORS

#include <QtTest/QtTest>
#include <qset.h>
#include <qdebug.h>

int toNumber(const QString &str)
{
    int res = 0;
    for (int i = 0; i < str.length(); ++i)
        res = (res * 10) + str[i].digitValue();
    return res;
}

class tst_QSet : public QObject
{
    Q_OBJECT

private slots:
    void operator_eq();
    void swap();
    void size();
    void capacity();
    void reserve();
    void squeeze();
    void detach();
    void isDetached();
    void clear();
    void remove();
    void contains();
    void containsSet();
    void begin();
    void end();
    void insert();
    void setOperations();
    void stlIterator();
    void stlMutableIterator();
    void javaIterator();
    void javaMutableIterator();
    void makeSureTheComfortFunctionsCompile();
    void initializerList();
};

void tst_QSet::operator_eq()
{
    {
        QSet<int> set1, set2;
        QVERIFY(set1 == set2);
        QVERIFY(!(set1 != set2));

        set1.insert(1);
        QVERIFY(set1 != set2);
        QVERIFY(!(set1 == set2));

        set2.insert(1);
        QVERIFY(set1 == set2);
        QVERIFY(!(set1 != set2));

        set2.insert(1);
        QVERIFY(set1 == set2);
        QVERIFY(!(set1 != set2));

        set1.insert(2);
        QVERIFY(set1 != set2);
        QVERIFY(!(set1 == set2));
    }

    {
        QSet<QString> set1, set2;
        QVERIFY(set1 == set2);
        QVERIFY(!(set1 != set2));

        set1.insert("one");
        QVERIFY(set1 != set2);
        QVERIFY(!(set1 == set2));

        set2.insert("one");
        QVERIFY(set1 == set2);
        QVERIFY(!(set1 != set2));

        set2.insert("one");
        QVERIFY(set1 == set2);
        QVERIFY(!(set1 != set2));

        set1.insert("two");
        QVERIFY(set1 != set2);
        QVERIFY(!(set1 == set2));
    }

    {
        QSet<QString> a;
        QSet<QString> b;

        a += "otto";
        b += "willy";

        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }
}

void tst_QSet::swap()
{
    QSet<int> s1, s2;
    s1.insert(1);
    s2.insert(2);
    s1.swap(s2);
    QCOMPARE(*s1.begin(),2);
    QCOMPARE(*s2.begin(),1);
}

void tst_QSet::size()
{
    QSet<int> set;
    QVERIFY(set.size() == 0);
    QVERIFY(set.isEmpty());
    QVERIFY(set.count() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.insert(1);
    QVERIFY(set.size() == 1);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.count() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.insert(1);
    QVERIFY(set.size() == 1);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.count() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.insert(2);
    QVERIFY(set.size() == 2);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.count() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.remove(1);
    QVERIFY(set.size() == 1);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.count() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.remove(1);
    QVERIFY(set.size() == 1);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.count() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.remove(2);
    QVERIFY(set.size() == 0);
    QVERIFY(set.isEmpty());
    QVERIFY(set.count() == set.size());
    QVERIFY(set.isEmpty() == set.empty());
}

void tst_QSet::capacity()
{
    QSet<int> set;
    int n = set.capacity();
    QVERIFY(n == 0);

    for (int i = 0; i < 1000; ++i) {
        set.insert(i);
        QVERIFY(set.capacity() >= set.size());
    }
}

void tst_QSet::reserve()
{
    QSet<int> set;
    int n = set.capacity();
    QVERIFY(n == 0);

    set.reserve(1000);
    QVERIFY(set.capacity() >= 1000);

    for (int i = 0; i < 500; ++i)
        set.insert(i);

    QVERIFY(set.capacity() >= 1000);

    for (int j = 0; j < 500; ++j)
        set.remove(j);

    QVERIFY(set.capacity() >= 1000);

    set.clear();
    QVERIFY(set.capacity() == 0);
}

void tst_QSet::squeeze()
{
    QSet<int> set;
    int n = set.capacity();
    QVERIFY(n == 0);

    set.reserve(1000);
    QVERIFY(set.capacity() >= 1000);

    set.squeeze();
    QVERIFY(set.capacity() < 100);

    for (int i = 0; i < 500; ++i)
        set.insert(i);
    QVERIFY(set.capacity() >= 500 && set.capacity() < 10000);

    set.reserve(50000);
    QVERIFY(set.capacity() >= 50000);

    set.squeeze();
    QVERIFY(set.capacity() < 500);

    set.remove(499);
    QVERIFY(set.capacity() < 500);

    set.insert(499);
    QVERIFY(set.capacity() >= 500);

    for (int i = 0; i < 500; ++i)
        set.remove(i);
    set.squeeze();
    QVERIFY(set.capacity() < 100);
}

void tst_QSet::detach()
{
    QSet<int> set;
    set.detach();

    set.insert(1);
    set.insert(2);
    set.detach();

    QSet<int> copy = set;
    set.detach();
}

void tst_QSet::isDetached()
{
    QSet<int> set1, set2;
    QVERIFY(!set1.isDetached()); // shared_null
    QVERIFY(!set2.isDetached()); // shared_null

    set1.insert(1);
    QVERIFY(set1.isDetached());
    QVERIFY(!set2.isDetached()); // shared_null

    set2 = set1;
    QVERIFY(!set1.isDetached());
    QVERIFY(!set2.isDetached());

    set1.detach();
    QVERIFY(set1.isDetached());
    QVERIFY(set2.isDetached());
}

void tst_QSet::clear()
{
    QSet<QString> set1, set2;
    QVERIFY(set1.size() == 0);

    set1.clear();
    QVERIFY(set1.size() == 0);

    set1.insert("foo");
    QVERIFY(set1.size() != 0);

    set2 = set1;

    set1.clear();
    QVERIFY(set1.size() == 0);
    QVERIFY(set2.size() != 0);

    set2.clear();
    QVERIFY(set1.size() == 0);
    QVERIFY(set2.size() == 0);
}

void tst_QSet::remove()
{
    QSet<QString> set1;

    for (int i = 0; i < 500; ++i)
        set1.insert(QString::number(i));

    QCOMPARE(set1.size(), 500);

    for (int j = 0; j < 500; ++j) {
        set1.remove(QString::number((j * 17) % 500));
        QCOMPARE(set1.size(), 500 - j - 1);
    }
}

void tst_QSet::contains()
{
    QSet<QString> set1;

    for (int i = 0; i < 500; ++i) {
        QVERIFY(!set1.contains(QString::number(i)));
        set1.insert(QString::number(i));
        QVERIFY(set1.contains(QString::number(i)));
    }

    QCOMPARE(set1.size(), 500);

    for (int j = 0; j < 500; ++j) {
        int i = (j * 17) % 500;
        QVERIFY(set1.contains(QString::number(i)));
        set1.remove(QString::number(i));
        QVERIFY(!set1.contains(QString::number(i)));
    }
}

void tst_QSet::containsSet()
{
    QSet<QString> set1;
    QSet<QString> set2;

    // empty set contains the empty set
    QVERIFY(set1.contains(set2));

    for (int i = 0; i < 500; ++i) {
        set1.insert(QString::number(i));
        set2.insert(QString::number(i));
    }
    QVERIFY(set1.contains(set2));

    set2.remove(QString::number(19));
    set2.remove(QString::number(82));
    set2.remove(QString::number(7));
    QVERIFY(set1.contains(set2));

    set1.remove(QString::number(23));
    QVERIFY(!set1.contains(set2));

    // filled set contains the empty set as well
    QSet<QString> set3;
    QVERIFY(set1.contains(set3));

    // the empty set doesn't contain a filled set
    QVERIFY(!set3.contains(set1));

    // verify const signature
    const QSet<QString> set4;
    QVERIFY(set3.contains(set4));
}

void tst_QSet::begin()
{
    QSet<int> set1;
    QSet<int> set2 = set1;

    {
        QSet<int>::const_iterator i = set1.constBegin();
        QSet<int>::const_iterator j = set1.cbegin();
        QSet<int>::const_iterator k = set2.constBegin();
        QSet<int>::const_iterator ell = set2.cbegin();

        QVERIFY(i == j);
        QVERIFY(k == ell);
        QVERIFY(i == k);
        QVERIFY(j == ell);
    }

    set1.insert(44);

    {
        QSet<int>::const_iterator i = set1.constBegin();
        QSet<int>::const_iterator j = set1.cbegin();
        QSet<int>::const_iterator k = set2.constBegin();
        QSet<int>::const_iterator ell = set2.cbegin();

        QVERIFY(i == j);
        QVERIFY(k == ell);
        QVERIFY(i != k);
        QVERIFY(j != ell);
    }

    set2 = set1;

    {
        QSet<int>::const_iterator i = set1.constBegin();
        QSet<int>::const_iterator j = set1.cbegin();
        QSet<int>::const_iterator k = set2.constBegin();
        QSet<int>::const_iterator ell = set2.cbegin();

        QVERIFY(i == j);
        QVERIFY(k == ell);
        QVERIFY(i == k);
        QVERIFY(j == ell);
    }
}

void tst_QSet::end()
{
    QSet<int> set1;
    QSet<int> set2 = set1;

    {
        QSet<int>::const_iterator i = set1.constEnd();
        QSet<int>::const_iterator j = set1.cend();
        QSet<int>::const_iterator k = set2.constEnd();
        QSet<int>::const_iterator ell = set2.cend();

        QVERIFY(i == j);
        QVERIFY(k == ell);
        QVERIFY(i == k);
        QVERIFY(j == ell);

        QVERIFY(set1.constBegin() == set1.constEnd());
        QVERIFY(set2.constBegin() == set2.constEnd());
    }

    set1.insert(44);

    {
        QSet<int>::const_iterator i = set1.constEnd();
        QSet<int>::const_iterator j = set1.cend();
        QSet<int>::const_iterator k = set2.constEnd();
        QSet<int>::const_iterator ell = set2.cend();

        QVERIFY(i == j);
        QVERIFY(k == ell);
        QVERIFY(i != k);
        QVERIFY(j != ell);

        QVERIFY(set1.constBegin() != set1.constEnd());
        QVERIFY(set2.constBegin() == set2.constEnd());
    }

    set2 = set1;

    {
        QSet<int>::const_iterator i = set1.constEnd();
        QSet<int>::const_iterator j = set1.cend();
        QSet<int>::const_iterator k = set2.constEnd();
        QSet<int>::const_iterator ell = set2.cend();

        QVERIFY(i == j);
        QVERIFY(k == ell);
        QVERIFY(i == k);
        QVERIFY(j == ell);

        QVERIFY(set1.constBegin() != set1.constEnd());
        QVERIFY(set2.constBegin() != set2.constEnd());
    }

    set1.clear();
    set2.clear();
    QVERIFY(set1.constBegin() == set1.constEnd());
    QVERIFY(set2.constBegin() == set2.constEnd());
}

void tst_QSet::insert()
{
    {
        QSet<int> set1;
        QVERIFY(set1.size() == 0);
        set1.insert(1);
        QVERIFY(set1.size() == 1);
        set1.insert(2);
        QVERIFY(set1.size() == 2);
        set1.insert(2);
        QVERIFY(set1.size() == 2);
        QVERIFY(set1.contains(2));
        set1.remove(2);
        QVERIFY(set1.size() == 1);
        QVERIFY(!set1.contains(2));
        set1.insert(2);
        QVERIFY(set1.size() == 2);
        QVERIFY(set1.contains(2));
    }

    {
        QSet<int> set1;
        QVERIFY(set1.size() == 0);
        set1 << 1;
        QVERIFY(set1.size() == 1);
        set1 << 2;
        QVERIFY(set1.size() == 2);
        set1 << 2;
        QVERIFY(set1.size() == 2);
        QVERIFY(set1.contains(2));
        set1.remove(2);
        QVERIFY(set1.size() == 1);
        QVERIFY(!set1.contains(2));
        set1 << 2;
        QVERIFY(set1.size() == 2);
        QVERIFY(set1.contains(2));
    }
}

void tst_QSet::setOperations()
{
    QSet<QString> set1, set2;
    set1 << "alpha" << "beta" << "gamma" << "delta"              << "zeta"           << "omega";
    set2            << "beta" << "gamma" << "delta" << "epsilon"           << "iota" << "omega";

    QSet<QString> set3 = set1;
    set3.unite(set2);
    QVERIFY(set3.size() == 8);
    QVERIFY(set3.contains("alpha"));
    QVERIFY(set3.contains("beta"));
    QVERIFY(set3.contains("gamma"));
    QVERIFY(set3.contains("delta"));
    QVERIFY(set3.contains("epsilon"));
    QVERIFY(set3.contains("zeta"));
    QVERIFY(set3.contains("iota"));
    QVERIFY(set3.contains("omega"));

    QSet<QString> set4 = set2;
    set4.unite(set1);
    QVERIFY(set4.size() == 8);
    QVERIFY(set4.contains("alpha"));
    QVERIFY(set4.contains("beta"));
    QVERIFY(set4.contains("gamma"));
    QVERIFY(set4.contains("delta"));
    QVERIFY(set4.contains("epsilon"));
    QVERIFY(set4.contains("zeta"));
    QVERIFY(set4.contains("iota"));
    QVERIFY(set4.contains("omega"));

    QVERIFY(set3 == set4);

    QSet<QString> set5 = set1;
    set5.intersect(set2);
    QVERIFY(set5.size() == 4);
    QVERIFY(set5.contains("beta"));
    QVERIFY(set5.contains("gamma"));
    QVERIFY(set5.contains("delta"));
    QVERIFY(set5.contains("omega"));

    QSet<QString> set6 = set2;
    set6.intersect(set1);
    QVERIFY(set6.size() == 4);
    QVERIFY(set6.contains("beta"));
    QVERIFY(set6.contains("gamma"));
    QVERIFY(set6.contains("delta"));
    QVERIFY(set6.contains("omega"));

    QVERIFY(set5 == set6);

    QSet<QString> set7 = set1;
    set7.subtract(set2);
    QVERIFY(set7.size() == 2);
    QVERIFY(set7.contains("alpha"));
    QVERIFY(set7.contains("zeta"));

    QSet<QString> set8 = set2;
    set8.subtract(set1);
    QVERIFY(set8.size() == 2);
    QVERIFY(set8.contains("epsilon"));
    QVERIFY(set8.contains("iota"));

    QSet<QString> set9 = set1 | set2;
    QVERIFY(set9 == set3);

    QSet<QString> set10 = set1 & set2;
    QVERIFY(set10 == set5);

    QSet<QString> set11 = set1 + set2;
    QVERIFY(set11 == set3);

    QSet<QString> set12 = set1 - set2;
    QVERIFY(set12 == set7);

    QSet<QString> set13 = set2 - set1;
    QVERIFY(set13 == set8);

    QSet<QString> set14 = set1;
    set14 |= set2;
    QVERIFY(set14 == set3);

    QSet<QString> set15 = set1;
    set15 &= set2;
    QVERIFY(set15 == set5);

    QSet<QString> set16 = set1;
    set16 += set2;
    QVERIFY(set16 == set3);

    QSet<QString> set17 = set1;
    set17 -= set2;
    QVERIFY(set17 == set7);

    QSet<QString> set18 = set2;
    set18 -= set1;
    QVERIFY(set18 == set8);
}

void tst_QSet::stlIterator()
{
    QSet<QString> set1;
    for (int i = 0; i < 25000; ++i)
        set1.insert(QString::number(i));

    {
        int sum = 0;
        QSet<QString>::const_iterator i = set1.begin();
        while (i != set1.end()) {
            sum += toNumber(*i);
            ++i;
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QSet<QString>::const_iterator i = set1.end();
        while (i != set1.begin()) {
            --i;
            sum += toNumber(*i);
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }
}

void tst_QSet::stlMutableIterator()
{
    QSet<QString> set1;
    for (int i = 0; i < 25000; ++i)
        set1.insert(QString::number(i));

    {
        int sum = 0;
        QSet<QString>::iterator i = set1.begin();
        while (i != set1.end()) {
            sum += toNumber(*i);
            ++i;
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QSet<QString>::iterator i = set1.end();
        while (i != set1.begin()) {
            --i;
            sum += toNumber(*i);
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        QSet<QString> set2 = set1;
        QSet<QString> set3 = set2;

        QSet<QString>::iterator i = set2.begin();
        QSet<QString>::iterator j = set3.begin();

        while (i != set2.end()) {
            i = set2.erase(i);
        }
        QVERIFY(set2.isEmpty());
        QVERIFY(!set3.isEmpty());

        j = set3.end();
        while (j != set3.begin()) {
            j--;
            if (j + 1 != set3.end())
                set3.erase(j + 1);
        }
        if (set3.begin() != set3.end())
            set3.erase(set3.begin());

        QVERIFY(set2.isEmpty());
        QVERIFY(set3.isEmpty());

// #if QT_VERSION >= 0x050000
//         i = set2.insert("foo");
// #else
        QSet<QString>::const_iterator k = set2.insert("foo");
        i = reinterpret_cast<QSet<QString>::iterator &>(k);
// #endif
        QVERIFY(*i == "foo");
    }
}

void tst_QSet::javaIterator()
{
    QSet<QString> set1;
    for (int k = 0; k < 25000; ++k)
        set1.insert(QString::number(k));

    {
        int sum = 0;
        QSetIterator<QString> i(set1);
        while (i.hasNext())
            sum += toNumber(i.next());
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QSetIterator<QString> i(set1);
        while (i.hasNext()) {
            sum += toNumber(i.peekNext());
            i.next();
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QSetIterator<QString> i(set1);
        while (i.hasNext()) {
            i.next();
            sum += toNumber(i.peekPrevious());
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QSetIterator<QString> i(set1);
        i.toBack();
        while (i.hasPrevious())
            sum += toNumber(i.previous());
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QSetIterator<QString> i(set1);
        i.toBack();
        while (i.hasPrevious()) {
            sum += toNumber(i.peekPrevious());
            i.previous();
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QSetIterator<QString> i(set1);
        i.toBack();
        while (i.hasPrevious()) {
            i.previous();
            sum += toNumber(i.peekNext());
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    int sum1 = 0;
    int sum2 = 0;
    QSetIterator<QString> i(set1);
    QSetIterator<QString> j(set1);

    int n = 0;
    while (i.hasNext()) {
        QVERIFY(j.hasNext());
        set1.remove(i.peekNext());
        sum1 += toNumber(i.next());
        sum2 += toNumber(j.next());
        ++n;
    }
    QVERIFY(!j.hasNext());
    QVERIFY(sum1 == 24999 * 25000 / 2);
    QVERIFY(sum2 == sum1);
    QVERIFY(set1.isEmpty());
}

void tst_QSet::javaMutableIterator()
{
    QSet<QString> set1;
    for (int k = 0; k < 25000; ++k)
        set1.insert(QString::number(k));

    {
        int sum = 0;
        QMutableSetIterator<QString> i(set1);
        while (i.hasNext())
            sum += toNumber(i.next());
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QMutableSetIterator<QString> i(set1);
        while (i.hasNext()) {
            i.next();
            sum += toNumber(i.value());
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QMutableSetIterator<QString> i(set1);
        while (i.hasNext()) {
            sum += toNumber(i.peekNext());
            i.next();
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QMutableSetIterator<QString> i(set1);
        while (i.hasNext()) {
            i.next();
            sum += toNumber(i.peekPrevious());
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QMutableSetIterator<QString> i(set1);
        i.toBack();
        while (i.hasPrevious())
            sum += toNumber(i.previous());
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QMutableSetIterator<QString> i(set1);
        i.toBack();
        while (i.hasPrevious()) {
            sum += toNumber(i.peekPrevious());
            i.previous();
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        int sum = 0;
        QMutableSetIterator<QString> i(set1);
        i.toBack();
        while (i.hasPrevious()) {
            i.previous();
            sum += toNumber(i.peekNext());
        }
        QVERIFY(sum == 24999 * 25000 / 2);
    }

    {
        QSet<QString> set2 = set1;
        QSet<QString> set3 = set2;

        QMutableSetIterator<QString> i(set2);
        QMutableSetIterator<QString> j(set3);

        while (i.hasNext()) {
            i.next();
            i.remove();
        }
        QVERIFY(set2.isEmpty());
        QVERIFY(!set3.isEmpty());

        j.toBack();
        while (j.hasPrevious()) {
            j.previous();
            j.remove();
        }
        QVERIFY(set2.isEmpty());
        QVERIFY(set3.isEmpty());
    }
}

void tst_QSet::makeSureTheComfortFunctionsCompile()
{
    QSet<int> set1, set2, set3;
    set1 << 5;
    set1 |= set2;
    set1 |= 5;
    set1 &= set2;
    set1 &= 5;
    set1 += set2;
    set1 += 5;
    set1 -= set2;
    set1 -= 5;
    set1 = set2 | set3;
    set1 = set2 & set3;
    set1 = set2 + set3;
    set1 = set2 - set3;
}

void tst_QSet::initializerList()
{
#ifdef Q_COMPILER_INITIALIZER_LISTS
    QSet<int> set{1, 2, 3, 4, 5};
    QCOMPARE(set.count(), 5);
    QVERIFY(set.contains(1));
    QVERIFY(set.contains(2));
    QVERIFY(set.contains(3));
    QVERIFY(set.contains(4));
    QVERIFY(set.contains(5));

    QSet<int> emptySet{};
    QVERIFY(emptySet.isEmpty());

    QSet<int> set3{{}, {}, {}};
    QVERIFY(!set3.isEmpty());
#else
    QSKIP("Compiler doesn't support initializer lists");
#endif
}

QTEST_APPLESS_MAIN(tst_QSet)

#include "tst_qset.moc"
