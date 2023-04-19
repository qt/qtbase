// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <qset.h>
#include <qdebug.h>

int toNumber(const QString &str)
{
    int res = 0;
    for (int i = 0; i < str.size(); ++i)
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
    void cpp17ctad();
    void remove();
    void contains();
    void containsSet();
    void begin();
    void end();
    void insert();
    void insertConstructionCounted();
    void setOperations();
    void setOperationsOnEmptySet();
    void stlIterator();
    void stlMutableIterator();
    void javaIterator();
    void javaMutableIterator();
    void makeSureTheComfortFunctionsCompile();
    void initializerList();
    void qhash();
    void intersects();
    void find();
    void values();
};

struct IdentityTracker {
    int value, id;
};

inline size_t qHash(IdentityTracker key) { return qHash(key.value); }
inline bool operator==(IdentityTracker lhs, IdentityTracker rhs) { return lhs.value == rhs.value; }

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

    {
        QSet<int> s1, s2;
        s1.reserve(100);
        s2.reserve(4);
        QVERIFY(s1 == s2);
        s1 << 100 << 200 << 300 << 400;
        s2 << 400 << 300 << 200 << 100;
        QVERIFY(s1 == s2);
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
    QVERIFY(set.size() == set.size());
    QVERIFY(set.isEmpty() == set.empty());
    QVERIFY(!set.isDetached());

    set.insert(1);
    QVERIFY(set.size() == 1);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.size() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.insert(1);
    QVERIFY(set.size() == 1);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.size() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.insert(2);
    QVERIFY(set.size() == 2);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.size() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.remove(1);
    QVERIFY(set.size() == 1);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.size() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.remove(1);
    QVERIFY(set.size() == 1);
    QVERIFY(!set.isEmpty());
    QVERIFY(set.size() == set.size());
    QVERIFY(set.isEmpty() == set.empty());

    set.remove(2);
    QVERIFY(set.size() == 0);
    QVERIFY(set.isEmpty());
    QVERIFY(set.size() == set.size());
    QVERIFY(set.isEmpty() == set.empty());
}

void tst_QSet::capacity()
{
    QSet<int> set;
    int n = set.capacity();
    QVERIFY(n == 0);
    QVERIFY(!set.isDetached());

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
    QCOMPARE(set.capacity(), 0);

    set.squeeze();
    QCOMPARE(set.capacity(), 0);

    QVERIFY(!set.isDetached());

    set.reserve(1000);
    QVERIFY(set.capacity() >= 1000);

    set.squeeze();
    QVERIFY(set.capacity() < 100);

    for (int i = 0; i < 500; ++i)
        set.insert(i);
    QCOMPARE(set.size(), 500);

    // squeezed capacity for 500 elements
    qsizetype capacity = set.capacity();    // current implementation: 512
    QCOMPARE_GE(capacity, set.size());

    set.reserve(50000);
    QVERIFY(set.capacity() >= 50000);       // current implementation: 65536

    set.squeeze();
    QCOMPARE(set.capacity(), capacity);

    // removing elements does not shed capacity
    set.remove(499);
    QCOMPARE(set.capacity(), capacity);

    set.insert(499);
    QCOMPARE(set.capacity(), capacity);

    // grow it beyond the current capacity
    for (int i = set.size(); i <= capacity; ++i)
        set.insert(i);
    QCOMPARE(set.size(), capacity + 1);
    QCOMPARE_GT(set.capacity(), capacity + 1);// current implementation: 2 * capacity (1024)

    for (int i = 0; i < 500; ++i)
        set.remove(i);

    // removing elements does not shed capacity
    QCOMPARE_GT(set.capacity(), capacity + 1);

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
    QVERIFY(!set1.isDetached());

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

void tst_QSet::cpp17ctad()
{
#define QVERIFY_IS_SET_OF(obj, Type) \
    QVERIFY2((std::is_same<decltype(obj), QSet<Type>>::value), \
             QMetaType::fromType<decltype(obj)::value_type>().name())
#define CHECK(Type, One, Two, Three) \
    do { \
        const Type v[] = {One, Two, Three}; \
        QSet v1 = {One, Two, Three}; \
        QVERIFY_IS_SET_OF(v1, Type); \
        QSet v2(v1.begin(), v1.end()); \
        QVERIFY_IS_SET_OF(v2, Type); \
        QSet v3(std::begin(v), std::end(v)); \
        QVERIFY_IS_SET_OF(v3, Type); \
    } while (false) \
    /*end*/
    CHECK(int, 1, 2, 3);
    CHECK(double, 1.0, 2.0, 3.0);
    CHECK(QString, QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three"));
#undef QVERIFY_IS_SET_OF
#undef CHECK
}

void tst_QSet::remove()
{
    QSet<QString> set;
    QCOMPARE(set.remove("test"), false);
    QVERIFY(!set.isDetached());

    const auto cnt = set.removeIf([](auto it) {
        Q_UNUSED(it);
        return true;
    });
    QCOMPARE(cnt, 0);

    for (int i = 0; i < 500; ++i)
        set.insert(QString::number(i));

    QCOMPARE(set.size(), 500);

    for (int j = 0; j < 500; ++j) {
        set.remove(QString::number((j * 17) % 500));
        QCOMPARE(set.size(), 500 - j - 1);
    }
}

void tst_QSet::contains()
{
    QSet<QString> set1;
    QVERIFY(!set1.contains("test"));
    QVERIFY(!set1.isDetached());

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
    QVERIFY(!set1.isDetached());

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
    QVERIFY(!set3.isDetached());

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
        QVERIFY(!set1.isDetached());
        QVERIFY(!set2.isDetached());
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

    const QSet<int> set3;
    QSet<int> set4 = set3;

    {
        QSet<int>::const_iterator i = set3.begin();
        QSet<int>::const_iterator j = set3.cbegin();
        QSet<int>::const_iterator k = set4.begin();
        QVERIFY(i == j);
        QVERIFY(k == j);
        QVERIFY(!set3.isDetached());
        QVERIFY(set4.isDetached());
    }

    set4.insert(1);

    {
        QSet<int>::const_iterator i = set3.begin();
        QSet<int>::const_iterator j = set3.cbegin();
        QSet<int>::const_iterator k = set4.begin();
        QVERIFY(i == j);
        QVERIFY(k != j);
        QVERIFY(!set3.isDetached());
        QVERIFY(set4.isDetached());
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

        QVERIFY(!set1.isDetached());
        QVERIFY(!set2.isDetached());
    }

    set1.insert(44);

    {
        QSet<int>::const_iterator i = set1.constEnd();
        QSet<int>::const_iterator j = set1.cend();
        QSet<int>::const_iterator k = set2.constEnd();
        QSet<int>::const_iterator ell = set2.cend();

        QVERIFY(i == j);
        QVERIFY(k == ell);

        QVERIFY(set1.constBegin() != set1.constEnd());
        QVERIFY(set2.constBegin() == set2.constEnd());
        QVERIFY(set1.constBegin() != set2.constBegin());
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

    const QSet<int> set3;
    QSet<int> set4 = set3;

    {
        QSet<int>::const_iterator i = set3.end();
        QSet<int>::const_iterator j = set3.cend();
        QSet<int>::const_iterator k = set4.end();
        QVERIFY(i == j);
        QVERIFY(k == j);
        QVERIFY(!set3.isDetached());
        QVERIFY(!set4.isDetached());

        QVERIFY(set3.constBegin() == set3.constEnd());
        QVERIFY(set4.constBegin() == set4.constEnd());
    }

    set4.insert(1);

    {
        QSet<int>::const_iterator i = set3.end();
        QSet<int>::const_iterator j = set3.cend();
        QSet<int>::const_iterator k = set4.end();
        QVERIFY(i == j);
        QVERIFY(k == j);
        QVERIFY(!set3.isDetached());
        QVERIFY(set4.isDetached());

        QVERIFY(set3.constBegin() == set3.constEnd());
        QVERIFY(set4.constBegin() != set4.constEnd());
    }
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

    {
        QSet<IdentityTracker> set;
        QCOMPARE(set.size(), 0);
        const int dummy = -1;
        IdentityTracker id00 = {0, 0}, id01 = {0, 1}, searchKey = {0, dummy};
        QCOMPARE(set.insert(id00)->id, id00.id);
        QCOMPARE(set.size(), 1);
        QCOMPARE(set.insert(id01)->id, id00.id); // first inserted is kept
        QCOMPARE(set.size(), 1);
        QCOMPARE(set.find(searchKey)->id, id00.id);
    }
}

struct ConstructionCounted
{
    ConstructionCounted(int i) : i(i) { }
    ConstructionCounted(ConstructionCounted &&other) noexcept
        : i(other.i), copies(other.copies), moves(other.moves + 1)
    {
        // set to some easily noticeable values
        other.i = -64;
        other.copies = -64;
        other.moves = -64;
    }
    ConstructionCounted &operator=(ConstructionCounted &&other) noexcept
    {
        ConstructionCounted moved = std::move(other);
        std::swap(*this, moved);
        return *this;
    }
    ConstructionCounted(const ConstructionCounted &other) noexcept
        : i(other.i), copies(other.copies + 1), moves(other.moves)
    {
    }
    ConstructionCounted &operator=(const ConstructionCounted &other) noexcept
    {
        ConstructionCounted copy = other;
        std::swap(*this, copy);
        return *this;
    }
    ~ConstructionCounted() = default;

    friend bool operator==(const ConstructionCounted &lhs, const ConstructionCounted &rhs)
    {
        return lhs.i == rhs.i;
    }

    QString toString() { return QString::number(i); }

    int i;
    int copies = 0;
    int moves = 0;
};

size_t qHash(const ConstructionCounted &c, std::size_t seed = 0)
{
    return qHash(c.i, seed);
}

void tst_QSet::insertConstructionCounted()
{
    QSet<ConstructionCounted> set;

    // copy-insert
    ConstructionCounted toCopy(7);
    auto inserted = set.insert(toCopy);
    QCOMPARE(set.size(), 1);
    auto element = set.begin();
    QCOMPARE(inserted, element);
    QCOMPARE(inserted->copies, 1);
    QCOMPARE(inserted->moves, 1);
    QCOMPARE(inserted->i, 7);

    // move-insert
    ConstructionCounted toMove(8);
    inserted = set.insert(std::move(toMove));
    element = set.find(8);
    QCOMPARE(set.size(), 2);
    QVERIFY(element != set.end());
    QCOMPARE(inserted, element);
    QCOMPARE(inserted->copies, 0);
    QCOMPARE(inserted->moves, 1);
    QCOMPARE(inserted->i, 8);

    inserted = set.insert(std::move(toCopy)); // move-insert an existing value
    QCOMPARE(set.size(), 2);
    // The previously existing key is used as they compare equal:
    QCOMPARE(inserted->copies, 1);
    QCOMPARE(inserted->moves, 1);
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

void tst_QSet::setOperationsOnEmptySet()
{
    {
        // Both sets are empty
        QSet<int> set1;
        QSet<int> set2;

        set1.unite(set2);
        QVERIFY(set1.isEmpty());
        QVERIFY(!set1.isDetached());

        set1.intersect(set2);
        QVERIFY(set1.isEmpty());
        QVERIFY(!set1.isDetached());

        set1.subtract(set2);
        QVERIFY(set1.isEmpty());
        QVERIFY(!set1.isDetached());
    }
    {
        // Second set is not empty
        QSet<int> empty;
        QSet<int> nonEmpty { 1, 2, 3 };

        empty.intersect(nonEmpty);
        QVERIFY(empty.isEmpty());
        QVERIFY(!empty.isDetached());

        empty.subtract(nonEmpty);
        QVERIFY(empty.isEmpty());
        QVERIFY(!empty.isDetached());

        empty.unite(nonEmpty);
        QCOMPARE(empty, nonEmpty);
        QVERIFY(empty.isDetached());
    }
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
        QSet<QString> set2 = set1;
        QSet<QString> set3 = set2;

        QSet<QString>::iterator i = set2.begin();

        while (i != set2.end()) {
            i = set2.erase(i);
        }
        QVERIFY(set2.isEmpty());
        QVERIFY(!set3.isEmpty());

        i = set2.insert("foo");
        QCOMPARE(*i, QLatin1String("foo"));
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

    int sum1 = 0;
    int sum2 = 0;
    QSetIterator<QString> i(set1);
    QSetIterator<QString> j(set1);

    while (i.hasNext()) {
        QVERIFY(j.hasNext());
        set1.remove(i.peekNext());
        sum1 += toNumber(i.next());
        sum2 += toNumber(j.next());
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
        QSet<QString> set2 = set1;
        QSet<QString> set3 = set2;

        QMutableSetIterator<QString> i(set2);

        while (i.hasNext()) {
            i.next();
            i.remove();
        }
        QVERIFY(set2.isEmpty());
        QVERIFY(!set3.isEmpty());
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
    QSet<int> set = {1, 1, 2, 3, 4, 5};
    QCOMPARE(set.size(), 5);
    QVERIFY(set.contains(1));
    QVERIFY(set.contains(2));
    QVERIFY(set.contains(3));
    QVERIFY(set.contains(4));
    QVERIFY(set.contains(5));

    // check _which_ of the equal elements gets inserted (in the QHash/QMap case, it's the last):
    const QSet<IdentityTracker> set2 = {{1, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}};
    QCOMPARE(set2.size(), 5);
    const int dummy = -1;
    const IdentityTracker searchKey = {1, dummy};
    QCOMPARE(set2.find(searchKey)->id, 0);

    QSet<int> emptySet{};
    QVERIFY(emptySet.isEmpty());

    QSet<int> set3{{}, {}, {}};
    QVERIFY(!set3.isEmpty());
}

void tst_QSet::qhash()
{
    //
    // check that sets containing the same elements hash to the same value
    //
    {
        // create some deterministic initial state:
        QHashSeed::setDeterministicGlobalSeed();

        QSet<int> s1;
        s1.reserve(4);
        s1 << 400 << 300 << 200 << 100;

        int retries = 128;
        while (--retries >= 0) {
            // reset the global seed to something different
            QHashSeed::resetRandomGlobalSeed();

            QSet<int> s2;
            s2.reserve(100); // provoke different bucket counts
            s2 << 100 << 200 << 300 << 400; // and insert elements in different order, too
            QVERIFY(s1.capacity() != s2.capacity());

            // see if we got a _different_ order
            if (std::equal(s1.cbegin(), s1.cend(), s2.cbegin()))
                continue;

            // check if the two QHashes still compare equal and produce the
            // same hash, despite containing elements in different orders
            QCOMPARE(s1, s2);
            QCOMPARE(qHash(s1), qHash(s2));
        }
        QVERIFY2(retries != 0, "Could not find a QSet with a different order of elements even "
                               "after a lot of retries. This is unlikely, but possible.");
    }

    //
    // check that sets of sets work:
    //
    {
        QSet<QSet<int> > intSetSet = { { 0, 1, 2 }, { 0, 1 }, { 1, 2 } };
        QCOMPARE(intSetSet.size(), 3);
    }
}

void tst_QSet::intersects()
{
    QSet<int> s1;
    QSet<int> s2;

    QVERIFY(!s1.intersects(s1));
    QVERIFY(!s1.intersects(s2));
    QVERIFY(!s1.isDetached());
    QVERIFY(!s2.isDetached());

    s1 << 100;
    QVERIFY(s1.intersects(s1));
    QVERIFY(!s1.intersects(s2));

    s2 << 200;
    QVERIFY(!s1.intersects(s2));

    s1 << 200;
    QVERIFY(s1.intersects(s2));

    QHashSeed::resetRandomGlobalSeed();
    QSet<int> s3;
    s3 << 500;
    QVERIFY(!s1.intersects(s3));
    s3 << 200;
    QVERIFY(s1.intersects(s3));
}

void tst_QSet::find()
{
    QSet<int> set;
    QCOMPARE(set.find(1), set.end());
    QCOMPARE(set.constFind(1), set.constEnd());
    QVERIFY(!set.isDetached());

    set.insert(1);
    set.insert(2);

    QVERIFY(set.find(1) != set.end());
    QVERIFY(set.constFind(2) != set.constEnd());
    QVERIFY(set.find(3) == set.end());
    QVERIFY(set.constFind(4) == set.constEnd());
}

template<typename T>
QList<T> sorted(const QList<T> &list)
{
    QList<T> res = list;
    std::sort(res.begin(), res.end());
    return res;
}

void tst_QSet::values()
{
    QSet<int> set;
    QVERIFY(set.values().isEmpty());
    QVERIFY(!set.isDetached());

    set.insert(1);
    QCOMPARE(set.values(), QList<int> { 1 });

    set.insert(10);
    set.insert(5);
    set.insert(2);

    QCOMPARE(sorted(set.values()), QList<int>({ 1, 2, 5, 10 }));

    set.remove(5);

    QCOMPARE(sorted(set.values()), QList<int>({ 1, 2, 10 }));
}

QTEST_APPLESS_MAIN(tst_QSet)

#include "tst_qset.moc"
